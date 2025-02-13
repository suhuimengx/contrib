#ifndef SAG_RTP_DUMMY_CONTROLLER_H
#define SAG_RTP_DUMMY_CONTROLLER_H

#include "sag_rtp_controller.h"
#include "ns3/type-id.h"

namespace ns3 {

/**
 * Simplistic implementation of a sender-based congestion controller. The
 * algorithm simply returns a constant, hard-coded bandwidth when queried.
 */
class SagDummyController: public SagRtpController
{
public:
	static TypeId GetTypeId (void);
	/** Class constructor */
    SagDummyController();

    /** Class destructor */
    virtual ~SagDummyController();

    /**
     * Set the current bandwidth estimation. This can be useful in test environments
     * to temporarily disrupt the current bandwidth estimation
     *
     * @param [in] newBw Bandwidth estimation to overwrite the current estimation
     */
    virtual void setCurrentBw(float newBw);

    /**
     * Reset the internal state of the congestion controller
     */
    virtual void reset();

    /**
     * Simplistic implementation of feedback packet processing. It simply
     * prints calculated metrics at regular intervals
     */
    virtual bool processFeedback(uint64_t now_ns,
                                 uint16_t sequence,
                                 uint64_t rxTimestampNs,
                                 uint8_t ecn=0);
    /**
     * Simplistic implementation of bandwidth getter. It returns a hard-coded
     * bandwidth value in bits per second
     */
    virtual float getBandwidth(uint64_t now_ns) const;

    virtual void setDir(std::string baseLogsDir);

private:
    void updateMetrics();
    void logStats(uint64_t now_ns);

    uint64_t m_lastTimeCalcNs;
    bool m_lastTimeCalcValid;

    uint64_t m_QdelayNs; /**< estimated queuing delay in microseconds */
    uint32_t m_ploss;  /**< packet loss count within configured window */
    float m_plr;       /**< packet loss ratio within packet history window */
    float m_RecvR;     /**< updated receiving rate in bps */
    bool m_enabled_dummy; /**< Whether record*/
    std::string m_baseLogsDir; //!< Where the burst logs will be written to:
                                 //!<   logs_dir/sag_nada_feedback_[id].csv
};

}

#endif /* SAG_RTP_DUMMY_CONTROLLER_H */
