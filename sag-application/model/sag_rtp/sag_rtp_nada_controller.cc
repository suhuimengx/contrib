#include "sag_rtp_nada_controller.h"
#include "ns3/exp-util.h"
#include "ns3/log.h"
#include <cinttypes>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cassert>
#include <cmath>
#include <cstring>
#include <string>
#include "ns3/string.h"
#include <fstream>

/*
 * Default parameter values of the NADA algorithm,
 * corresponding to Figure 3 in the rmcat-nada draft
 */
/* default parameters for core algorithm (gradual rate update) */

const float NADA_PARAM_PRIO = 1.0;  /**< Weight of priority of the flow */
const float  NADA_PARAM_XREF = 10.0; /**< Reference congestion level (in ms) */

const float NADA_PARAM_KAPPA = 0.5; /**< Scaling parameter for gradual rate update calculation (dimensionless) */
const float NADA_PARAM_ETA = 2.0; /**< Scaling parameter for gradual rate update calculation (dimensionless) */
const float NADA_PARAM_TAU = 500.; /**< Upper bound of RTT (in ms) in gradual rate update calculation */

/**
 * Target interval for receiving feedback from receiver
 * or update rate calculation (in microseconds)
 */
const uint64_t NADA_PARAM_DELTA_US = 1000000000;

/* default parameters for accelerated ramp-up */

/**  Threshold (microseconds) for allowed queuing delay build up at receiver during accelerated ramp-up mode */
const uint64_t NADA_PARAM_QEPS_US = 10 * 1000 * 1000;
const uint64_t NADA_PARAM_DFILT_US = 120 * 1000 * 1000; /**< Bound on filtering delay (in microseconds) */
/** Upper bound on rate increase ratio in accelerated ramp-up mode (dimensionless) */
const float NADA_PARAM_GAMMA_MAX = 0.5;
/** Upper bound on self-inflicted queuing delay during ramp up (in ms) */
const float NADA_PARAM_QBOUND = 50.;

/* default parameters for non-linear warping of queuing delay */

/** multiplier of observed average loss intervals, as a measure
 * of tolerance of missing observed loss events (dimensionless)
 */
const float NADA_PARAM_MULTILOSS = 7.;

const float NADA_PARAM_QTH = 50.; /**< Queuing delay threshold for invoking non-linear warping (in ms) */
const float NADA_PARAM_LAMBDA = 0.5; /**< Exponent of the non-linear warping (dimensionless) */

/* default parameters for calculating aggregated congestion signal */

/**
 * Reference delay penalty (in ms) in terms of value
 * of congestion price when packet loss ratio is at PLRREF
 */
const float NADA_PARAM_DLOSS = 10.;
const float NADA_PARAM_PLRREF = 0.01; /**> Reference packet loss ratio (dimensionless) */
const float NADA_PARAM_XMAX = 500. *1000; /**> Maximum value of aggregate congestion signal (in ms) */

/** Smoothing factor in exponential smoothing of packet loss and marking ratios */
const float NADA_PARAM_ALPHA = 0.1;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SagNadaController");

NS_OBJECT_ENSURE_REGISTERED (SagNadaController);

TypeId
SagNadaController::GetTypeId(void)
{
	static TypeId tid = TypeId ("ns3::SagNadaController")
			.SetParent<Object> ()
            .SetGroupName("BasicSim")
            .AddAttribute ("BaseLogsDir",
                            "Base logging directory (logging will be placed here, i.e. logs_dir/burst_[burst id]_{incoming, outgoing}.csv",
                            StringValue (""),
                            MakeStringAccessor (&SagNadaController::m_baseLogsDir),
                            MakeStringChecker ());
	return tid;
}

SagNadaController::SagNadaController() :
    SagRtpController {},
    m_ploss{0},
    m_plr{0.f},
    m_warpMode{false},
    m_lastTimeCalcUs{0},
    m_lastTimeCalcValid{false},
    m_currBw{m_initBw},
    m_QdelayUs{0},
    m_RttUs{0},
    m_Xcurr{0.f},
    m_Xprev{0.f},
    m_RecvR{0.f},
    m_avgInt{0.f},
    m_currInt{0},
    m_lossesSeen{false},
	m_enabled_nada{false}{}

SagNadaController::~SagNadaController() {}

void SagNadaController::setCurrentBw(float newBw) {
    m_currBw = newBw;
}

/**
 * Implementation of the #reset API: reset all state variables
 * to default values
 */
void SagNadaController::reset() {
    m_ploss = 0;
    m_plr = 0.f;
    m_warpMode = false;
    m_lastTimeCalcUs = 0;
    m_lastTimeCalcValid = false;
    m_currBw = m_initBw;
    m_QdelayUs = 0;
    m_RttUs = 0;
    m_Xcurr = 0.f;
    m_Xprev = 0.f;
    m_RecvR = 0.f;
    m_avgInt = 0.f;
    m_currInt = 0;
    m_lossesSeen = false;
    m_enabled_nada = false;
    SagRtpController ::reset();
}

bool
SagNadaController::processSendPacket(uint64_t txTimestampUs,
                                       uint16_t sequence,
                                       uint32_t size) { // in Bytes
    //NS_LOG_FUNCTION(this << txTimestampUs << sequence << size);
	//std::cout << "SagNadaController::processSendPacket: txTimestampUs: " << txTimestampUs << ", sequence: " << sequence << ", size: " << size << std::endl;
    /* First of all, call the superclass */
    if (!SagRtpController ::processSendPacket(txTimestampUs, sequence, size)) {
        return false;
    }

    /* Optimization: to avoid skipping the rate update upon the first received feedback
     * batch, we initialize the last time the rate was updated to the first media packet sent
     */
    if (!m_lastTimeCalcValid) {
        m_lastTimeCalcUs = txTimestampUs;
        m_lastTimeCalcValid = true;
    }
    return true;
}

/**
 * Implementation of the #processFeedback API
 * in the SagRtpController  class
 *
 * TODO (deferred): Add support for ECN marking
 */
bool SagNadaController::processFeedback(uint64_t nowUs,
                                     uint16_t sequence,
                                     uint64_t rxTimestampUs,
                                     uint8_t ecn) {
    NS_LOG_FUNCTION(this);
    /* First of all, call the superclass */
    if (!SagRtpController ::processFeedback(nowUs,
                                                sequence,
                                                rxTimestampUs,
                                                ecn)) {
        return false;
    }

    /* Update calculation of reference rate (r_ref)
     * if last calculation occurred more than NADA_PARAM_DELTA_US
     * (target update interval in microseconds) ago
     */

    /* First time receiving a feedback message */
    if (!m_lastTimeCalcValid) {
        m_lastTimeCalcUs = nowUs;
        m_lastTimeCalcValid = true;
        return true;
    }

    assert(lessThan(m_lastTimeCalcUs, nowUs + 1));
    /* calculate time since last update */
    const uint64_t deltaUs = nowUs - m_lastTimeCalcUs; // subtraction will wrap correctly
    if (deltaUs >= NADA_PARAM_DELTA_US) {
        /* log & update rate calculation */
        updateMetrics();
        updateBw(deltaUs);
        logStats(nowUs, deltaUs);

        m_lastTimeCalcUs = nowUs;
    }
    return true;
}

bool SagNadaController::processFeedbackBatch(uint64_t nowUs,
                                          const std::vector<FeedbackItem>& feedbackBatch) {
    NS_LOG_FUNCTION(this);
    /* First of all, call the superclass */
    if (!SagRtpController ::processFeedbackBatch(nowUs, feedbackBatch)) {
        return false;
    }

    /* Update calculation of reference rate (r_ref)
     * Make sure that last calculation occurred more than NADA_PARAM_DELTA_US
     * (target update interval in microseconds) ago. Apply with some leniency
     * so that calculation time coincides with aggregate feedback processing
     * most of the time.
     */

    /* First time receiving a feedback message */
    if (!m_lastTimeCalcValid) {
        m_lastTimeCalcUs = nowUs;
        m_lastTimeCalcValid = true;
        return true;
    }

    assert(lessThan(m_lastTimeCalcUs, nowUs + 1));
    /* calculate time since last update */
    const uint64_t deltaUs = nowUs - m_lastTimeCalcUs; // subtraction will wrap correctly
    /* 50% leniency */
    if (deltaUs < NADA_PARAM_DELTA_US * .5) {
        return true;
    }
    /* log & update rate calculation */
    updateMetrics();
    updateBw(deltaUs);
    logStats(nowUs, deltaUs);

    m_lastTimeCalcUs = nowUs;
    return true;
}

/**
 * Implementation of the #getBandwidth API
 * in the SagRtpController  class: simply
 * returns the calculated reference rate
 * (r_ref in rmcat-nada)
 */
float SagNadaController::getBandwidth(uint64_t nowUs) const {

    NS_LOG_FUNCTION(this);
    return m_currBw;
}


void
SagNadaController::setDir(std::string baseLogsDir){
	m_baseLogsDir = baseLogsDir;
}


/**
 * The following implements the core congestion
 * control algorithm as specified in the rmcat-nada
 * draft (see Section 4)
 */
void SagNadaController::updateBw(uint64_t deltaUs) {

    NS_LOG_FUNCTION(this);
    int rmode = getRampUpMode();
    if (rmode == 0) {
        calcAcceleratedRampUp();
    } else {
        calcGradualRateUpdate(deltaUs);
    }

    /* clip final rate within range */
    m_currBw = std::min(m_currBw, m_maxBw);
    m_currBw = std::max(m_currBw, m_minBw);
}

/**
 * The following function retrieves updated
 * estimates of delay, loss, and receiving
 * rate from the base class SagRtpController
 * and saves them to local member variables.
 */
void SagNadaController::updateMetrics() {

    NS_LOG_FUNCTION(this);
    /* Obtain packet stats in terms of loss and delay */
    uint64_t qdelayUs = 0;
    bool qdelayOK = getCurrentQdelay(qdelayUs);
    if (qdelayOK) m_QdelayUs = qdelayUs;

    uint64_t rttUs = 0;
    bool rttOK = getCurrentRTT(rttUs);
    if (rttOK) m_RttUs = rttUs;

    float rrate = 0.f;
    bool rrateOK = getCurrentRecvRate(rrate);
    if (rrateOK) m_RecvR = rrate;

    float plr = 0.f;
    uint32_t nLoss = 0;
    bool plrOK = getPktLossInfo(nLoss, plr);
    if (plrOK) {
        m_ploss = nLoss;
        // Exponential filtering of loss stats
        m_plr += NADA_PARAM_ALPHA * (plr - m_plr);
    }

    float avgInt;
    uint32_t currentInt;
    bool avgIntOK = getLossIntervalInfo(avgInt, currentInt);
    m_lossesSeen = avgIntOK;
    if (avgIntOK) {
        m_avgInt = avgInt;
        m_currInt = currentInt;
    }

    /* update aggregate congestion signal */
    m_Xprev = m_Xcurr;
    if (qdelayOK) updateXcurr();

}

void SagNadaController::logStats(uint64_t nowUs, uint64_t deltaUs)  { //const

	NS_LOG_FUNCTION(this);
    std::ostringstream os;
    os << std::fixed;
    os.precision(RTP_LOG_PRINT_PRECISION);

    /* log packet stats: including common stats
     * (e.g., receiving rate, loss, delay) needed
     * by all controllers and algorithm-specific
     * ones (e.g., xcurr for NADA) */
    os << "algo:nada " << m_id_control
       << " ts: "     << (nowUs / 1000 /1000) // << " ms"  / 1000
       << " loglen: " << m_packetHistory.size()
       << " qdel: "   << (m_QdelayUs / 1000.f / 1000.f) // << " ms" // / 1000
       << " rtt: "    << (m_RttUs / 1000.f / 1000.f) // << " ms" // / 1000
       << " ploss: "  << m_ploss
       << " plr: "    << m_plr
       << " xcurr: "  << m_Xcurr
       << " rrate: "  << m_RecvR // << " bps"
       << " srate: "  << m_currBw // << " bps"
       << " avgint: " << m_avgInt
       << " curint: " << m_currInt
       << " delta: "  << (deltaUs / 1000.f / 1000.f); // << " ms."; //  1000
    //logMessage(os.str());

    // Write Result
    if(m_enabled_nada==false){
    	//m_baseLogsDir + "/" + format_string("sag_nada_feedback_%" PRIu64 ".csv",m_id_control)
    	//FILE* file_nada_control_csv = fopen(m_sag_nada_control_csv_filename_rtp.c_str(), "w+");
        //std::cout << "    >> Opened: " << (m_baseLogsDir + "/" + format_string("sag_nada_feedback_%" PRIu64 ".csv",m_id_control)).c_str() << std::endl;
    	//m_baseLogsDir="/home/zanxin99/eclipse-workspace/SAG_Platform/simulator/scratch/main_satnet/test_data/end_to_end/run/logs_ns3";
    	FILE* file_nada_control_csv = fopen((m_baseLogsDir + "/" + format_string("sag_nada_feedback_%" PRIu64 ".csv",m_id_control.c_str())).c_str(), "w+");
        fprintf(
                file_nada_control_csv,
                "%-18s%-18s%-18s%-18s%-18s%-18s%-18s%-18s%-18s%-18s%-18s%-18s\n",
                "control_id",
                "timestamp",
                "packet history",
                "queue delay",
                "rtt delay",
                "packet loss",
                "packet loss rate",
                "xcurr",
                "receive rate",
                "current bandwidth",
                "avgInt",
                "currInt"
        );

        m_enabled_nada=true;
        fprintf(
                file_nada_control_csv,
                " %-16s, %-16" PRId64 ", %-16" PRId64 ", %-16f, %-16f, %-16u, %-16f, %-16f, %-16f, %-16f, %-16f, %-16u \n",
                m_id_control.c_str(),
                (nowUs / 1000 /1000),
                m_packetHistory.size(),
                (m_QdelayUs / 1000.f / 1000.f),
                (m_RttUs / 1000.f / 1000.f),
                m_ploss,
                m_plr,
                m_Xcurr,
                m_RecvR,
                m_currBw,
                m_avgInt,
                m_currInt
        );
        fclose(file_nada_control_csv);
    }
    else{
    	//FILE* file_nada_control_csv = fopen(m_sag_nada_control_csv_filename_rtp.c_str(), "a+");
    	FILE* file_nada_control_csv = fopen((m_baseLogsDir + "/" + format_string("sag_nada_feedback_%" PRIu64 ".csv",m_id_control.c_str())).c_str(), "a+");
        fprintf(
                file_nada_control_csv,
				" %-16s, %-16" PRId64 ", %-16" PRId64 ", %-16f, %-16f, %-16u, %-16f, %-16f, %-16f, %-16f, %-16f, %-16u \n",
                m_id_control.c_str(),
                (nowUs / 1000 /1000),
                m_packetHistory.size(),
                (m_QdelayUs / 1000.f / 1000.f),
                (m_RttUs / 1000.f / 1000.f),
                m_ploss,
                m_plr,
                m_Xcurr,
                m_RecvR,
                m_currBw,
                m_avgInt,
                m_currInt
        );
        fclose(file_nada_control_csv);
    }
}

/**
 * Function implementing the calculation of the
 * non-linear warping of queuing delay to d_tilde
 * as specified in rmcat-nada. See Section 4.2, Eq. (1).
 *
 *            / d_queue,           if d_queue<QTH;
 *            |
 * d_tilde = <                                           (1)
 *            |                  (d_queue-QTH)
 *            \ QTH exp(-LAMBDA ---------------), otherwise.
 *                                    QTH
 */
float SagNadaController::calcDtilde() const {

    NS_LOG_FUNCTION(this);
    const float qDelay = float(m_QdelayUs) / 1000.f / 1000.f ;
    float xval = qDelay;

    if (m_QdelayUs / 1000.f / 1000.f  > NADA_PARAM_QTH) {
        float ratio = (qDelay - NADA_PARAM_QTH) / NADA_PARAM_QTH;
        ratio = NADA_PARAM_LAMBDA * ratio;
        xval = float(NADA_PARAM_QTH * exp(-ratio));
    }

    return xval;
}

/**
 * Function implementing calculation of the
 * aggregate congestion signal x_curr in
 * rmcat-nada draft. The criteria for
 * invoking the non-linear warping of queuing
 * delay is described in Sec. 4.2 of the draft.
 */
void SagNadaController::updateXcurr() {

    NS_LOG_FUNCTION(this);
    float xdel = float(m_QdelayUs) / 1000.f / 1000.f; // pure delay-based
    float xtilde = calcDtilde();             // warped version
    const float currInt = float(m_currInt);

    /* Criteria for determining whether to perform
     * non-linear warping of queuing delay. The
     * time window for last observed loss self-adapts
     * with previously observed loss intervals
     * */
    if (m_lossesSeen && currInt < NADA_PARAM_MULTILOSS * m_avgInt) {
        /* last loss observed within the time window
         * NADA_PARAM_MULTILOSS * m_avgInt; allowing us to
         * miss up to NADA_PARAM_MULTILOSS-1 loss events
         */
        m_Xcurr = xtilde;
        m_warpMode = true;
     } else if (m_lossesSeen) {
         /* loss recently expired: slowly transition back
          * to non-warped queuing delay over the course
          * of one average packet loss interval (m_avgInt)
          */
        if (currInt < (NADA_PARAM_MULTILOSS + 1.f) * m_avgInt) {
            /* transition period: linearly blending
             * warped and non-warped values for congestion
             * price */
            const float alpha = (currInt - NADA_PARAM_MULTILOSS * m_avgInt) / m_avgInt;
            m_Xcurr = alpha * xdel + (1.f - alpha) * xtilde;
        } else {
            /* after transition period: switch completely
             * to non-warped queuing delay */
            m_Xcurr = xdel;
            m_warpMode = false;
        }
    } else {
        /* no loss recently observed, stick with
         * non-warped queuing delay */
        m_Xcurr = xdel;
        m_warpMode = false;
    }

    /* Add additional loss penalty for the aggregate
     * congestion signal, following Eq.(2) in Sec.4.2 of
     * rmcat-nada draft */
    float plr0 = m_plr / NADA_PARAM_PLRREF;
    m_Xcurr += NADA_PARAM_DLOSS * plr0 * plr0;

    /* Clip final congestion signal within range */
    if (m_Xcurr > NADA_PARAM_XMAX) {
        m_Xcurr = NADA_PARAM_XMAX;
    }

}

/**
 * This function implements the calculation of reference
 * rate (r_ref) in the gradual update mode, following
 * Eq. (5)-(7) in Section 4.3 of the rmcat-nada draft:
 *
 *
 * x_offset = x_curr - PRIO*XREF*RMAX/r_ref          (5)
 *
 * x_diff   = x_curr - x_prev                        (6)
 *
 *                         delta    x_offset
 * r_ref = r_ref - KAPPA*-------*------------*r_ref
 *                         TAU       TAU
 *
 *                            x_diff
 *               - KAPPA*ETA*---------*r_ref         (7)
 *                              TAU
 */
void SagNadaController::calcGradualRateUpdate(uint64_t deltaUs) {

    NS_LOG_FUNCTION(this);
    float x_curr = m_Xcurr;
    float x_prev = m_Xprev;

    float x_offset = x_curr;
    float x_diff = x_curr - x_prev;
    float r_offset = m_currBw;
    float r_diff = m_currBw;

    x_offset -= NADA_PARAM_PRIO * NADA_PARAM_XREF * m_maxBw / m_currBw;

    r_offset *= NADA_PARAM_KAPPA;
    const float delta = float(deltaUs) / 1000. / 1000.;
    r_offset *= delta / NADA_PARAM_TAU;
    r_offset *= x_offset / NADA_PARAM_TAU;

    r_diff *= NADA_PARAM_KAPPA;
    r_diff *= NADA_PARAM_ETA;
    r_diff *= x_diff / NADA_PARAM_TAU;

    m_currBw = m_currBw - r_offset - r_diff;
}

/**
 * This function calculates the reference rate r_ref in the
 * accelarated ramp-up mode, following Eq. (3)-(4) in Section 4.3
 * of the rmcat-nada draft:
 *
 *                            QBOUND
 * gamma = min(GAMMA_MAX, ------------------)     (3)
 *                         rtt+DELTA+DFILT
 *
 * r_ref = max(r_ref, (1+gamma) r_recv)           (4)
 */
void SagNadaController::calcAcceleratedRampUp( ) {

    NS_LOG_FUNCTION(this);
    float gamma = 1.0;

    uint64_t denom = m_RttUs;
    denom += NADA_PARAM_DELTA_US;
    denom += NADA_PARAM_DFILT_US;
    denom /= 1000.f / 1000.f; // Us --> ms

    gamma = NADA_PARAM_QBOUND / float(denom);

    if (gamma > NADA_PARAM_GAMMA_MAX) {
        gamma = NADA_PARAM_GAMMA_MAX;
    }

    float rnew = (1.f + gamma) * m_RecvR;
    if (m_currBw < rnew) m_currBw = rnew;
}

/**
 * This function determines whether the congestion control
 * algorithm should operate in the accelerated ramp up mode
 *
 * The criteria for operating in accelerated ramp-up mode are
 * discussed at the end of Section 4.2 of the rmcat-nada draft,
 * as follows:
 *
 * o No recent packet losses within the observation window LOGWIN; and
 *
 * o No build-up of queuing delay: d_fwd-d_base < QEPS for all previous
 *   delay samples within the observation window LOGWIN.
 */
int SagNadaController::getRampUpMode() {

    NS_LOG_FUNCTION(this);
    int rmode = 0;

    /* If losses are observed, stay with gradual update */
    if (m_ploss > 0) rmode = 1;

    /* check all raw queuing delay samples in
     * packet history log */
    for (auto rit = m_packetHistory.rbegin();
              rit != m_packetHistory.rend() && rmode == 0;
              ++rit) {

        const uint64_t qDelayCurrentUs = rit->owdUs - m_baseDelayUs;
        if (qDelayCurrentUs > NADA_PARAM_QEPS_US ) {
            rmode = 1;  /* Gradual update if queuing delay exceeds threshold*/
        }
    }
    return rmode;
}

}
