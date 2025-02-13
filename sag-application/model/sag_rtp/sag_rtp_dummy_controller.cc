#include "sag_rtp_dummy_controller.h"
#include "ns3/log.h"
#include "ns3/exp-util.h"
#include <cstring>
#include <string>
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/simulator.h"
#include "ns3/abort.h"
#include <sstream>
#include <cassert>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SagDummyController");

NS_OBJECT_ENSURE_REGISTERED (SagDummyController);

TypeId
SagDummyController::GetTypeId(void)
{
	static TypeId tid = TypeId ("ns3::SagDummyController")
			.SetParent<Object> ()
            .SetGroupName("BasicSim");
	return tid;
}

SagDummyController::SagDummyController() :
    SagRtpController{},
    m_lastTimeCalcNs{0},
    m_lastTimeCalcValid{false},
    m_QdelayNs{0},
    m_ploss{0},
    m_plr{0.f},
    m_RecvR{0.} {}

SagDummyController::~SagDummyController() {}

void
SagDummyController::setCurrentBw(float newBw) {
    m_initBw = newBw;
}

void
SagDummyController::reset() {
    m_lastTimeCalcNs = 0;
    m_lastTimeCalcValid = false;

    m_QdelayNs = 0;
    m_ploss = 0;
    m_plr = 0.f;
    m_RecvR = 0.;

    SagRtpController::reset();
}

bool
SagDummyController::processFeedback(uint64_t now_ns,
                                      uint16_t sequence,
                                      uint64_t rxTimestampNs,
                                      uint8_t ecn) {

    // First of all, call the superclass
    const bool res = SagRtpController::processFeedback(now_ns, sequence,
                                                            rxTimestampNs, ecn);
    const uint64_t calcIntervalNs = 200 * 1000 * 1000;
    if (m_lastTimeCalcValid) {
        assert(lessThan(m_lastTimeCalcNs, now_ns + 1));
        if (now_ns - m_lastTimeCalcNs >= calcIntervalNs) {
            updateMetrics();
            logStats(now_ns);
            m_lastTimeCalcNs = now_ns;
        }
    } else {
        m_lastTimeCalcNs = now_ns;
        m_lastTimeCalcValid = true;
    }
    return res;
}

float
SagDummyController::getBandwidth(uint64_t now_ns) const {

    return m_initBw;
}

void
SagDummyController::setDir(std::string baseLogsDir){
	m_baseLogsDir = baseLogsDir;
}

void
SagDummyController::updateMetrics() {
    uint64_t qdelayNs;
    bool qdelayOK = getCurrentQdelay(qdelayNs);
    if (qdelayOK) m_QdelayNs = qdelayNs;

    float rrate;
    bool rrateOK = getCurrentRecvRate(rrate);
    if (rrateOK) m_RecvR = rrate;

    uint32_t nLoss;
    float plr;
    bool plrOK = getPktLossInfo(nLoss, plr);
    if (plrOK) {
        m_ploss = nLoss;
        m_plr = plr;
    }
}

void
SagDummyController::logStats(uint64_t now_ns) {

    std::ostringstream os;
    os << std::fixed;
    os.precision(RTP_LOG_PRINT_PRECISION);

    os  << " algo:dummy " << m_id_control
        << " ts: "     << (now_ns / 1000 / 1000)
        << " loglen: " << m_packetHistory.size()
        << " qdel: "   << (m_QdelayNs / 1000.f / 1000.f)
        << " ploss: "  << m_ploss
        << " plr: "    << m_plr
        << " rrate: "  << m_RecvR
        << " srate: "  << m_initBw;
    //logMessage(os.str());


    // Write Result
    if(m_enabled_dummy==false){
    FILE* file_dummy_control_csv = fopen((m_baseLogsDir + "/" + format_string("sag_dummy_feedback_%" PRIu64 ".csv",m_id_control)).c_str(), "w+");
        fprintf(
                file_dummy_control_csv,
                "%-18s%-18s%-18s%-18s%-18s%-18s%-18s%-18s\n",
                "control_id",
                "timestamp",
                "packet history",
                "queue delay",
                //"rtt delay",
                "packet loss",
                "packet loss rate",
                //"xcurr",
                "receive rate",
                "current bandwidth"
                //"avgInt",
                //"currInt"
        );

        m_enabled_dummy=true;
        fprintf(
                file_dummy_control_csv,
                " %-16s, %-16" PRId64 ", %-16" PRId64 ", %-16f, %-16u, %-16f, %-16f, %-16f \n",
                m_id_control.c_str(),
                (now_ns / 1000 /1000),
                m_packetHistory.size(),
                (m_QdelayNs / 1000.f / 1000.f),
                //(m_RttUs / 1000.f / 1000.f),
                m_ploss,
                m_plr,
                //m_Xcurr,
                m_RecvR,
                m_initBw
                //m_avgInt,
                //m_currInt
        );
        fclose(file_dummy_control_csv);
    }
    else{
    	//FILE* file_dummy_control_csv = fopen(m_sag_dummy_control_csv_filename_rtp.c_str(), "a+");
    	FILE* file_dummy_control_csv = fopen((m_baseLogsDir + "/" + format_string("sag_dummy_feedback_%" PRIu64 ".csv",m_id_control)).c_str(), "a+");
        fprintf(
                file_dummy_control_csv,
				" %-16s, %-16" PRId64 ", %-16" PRId64 ", %-16f, %-16u, %-16f, %-16f, %-16f \n",
                m_id_control.c_str(),
                (now_ns / 1000 /1000),
                m_packetHistory.size(),
                (m_QdelayNs / 1000.f / 1000.f),
                //(m_RttUs / 1000.f / 1000.f),
                m_ploss,
                m_plr,
                //m_Xcurr,
                m_RecvR,
                m_initBw
                //m_avgInt,
                //m_currInt
        );
        fclose(file_dummy_control_csv);
    }


}

}
