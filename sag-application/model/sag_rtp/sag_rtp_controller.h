#ifndef SAG_RTP_CONTROLLER_H
#define SAG_RTP_CONTROLLER_H
#include "ns3/type-id.h"
#include "ns3/object.h"
#include <cstdint>
#include <string>
#include <deque>
#include <vector>
#include <tuple>
#include <utility>


namespace ns3 {

const uint32_t RTP_LOG_PRINT_PRECISION = 2;  /* default precision for logs */

/**
 * This class keeps track of the length of intervals between two packet
 * loss events, in the way TCP-friendly Rate Control (TFRC) calculates it
 */
class InterLossState {
public:
    InterLossState();
    std::deque<uint32_t> intervals;
    uint16_t expectedSeq;
    bool initialized; // did the first loss happen?
};

/**
 * This is the base class to all congestion controllers. Any congestion
 * controller that is to use this NS3 component has to inherit from this
 * class. Additionally a congestion controller needs to implement virtual
 * member functions #processSendPacket , #processFeedback , #getBandwidth ,
 * and (optionally) #reset . The controller's algorithm implementation is
 * indeed the code behind these three member functions.
 *
 * As the class's name implies, the congestion control algorithm has to be
 * sender-based; this means that the congestion control algorithm will be
 * running exclusively at the sender endpoint. The receiver endpoint timestamps
 * the arrival of media packets and sends per-packet feedback back to the
 * sender endpoint.
 *
 * Even if this class has pure virtual methods and cannot therefore be directly
 * instantiated, it contains the common basic behavior we consider all
 * congestion control algorithms need:
 *
 *  - Match send and receive timestamps of a packet and calculate its one way
 *    delay (OWD)
 *  - Calculate other packet metrics the can be useful for congestion control
 *    algorithms (e.g, packet loss ratio, average loss interval)
 *
 * We intend congestion controllers derived from this class to be independent
 * from NS3 (as far as possible). The reason for this requirement is to render
 * these congestion controller implementations easier to reuse in an emulated
 * environment, or on a real testbed, rather than NS3. For this reason:
 *  - We don't follow NS3's code guidelines in these files
 *  - We define a logging callback, rather than directly using NS3 logging
 *    within the controller classes
 */
class SagRtpController : public Object{
public:
	static TypeId GetTypeId (void);
    /**
     * This typedef is used to define a logging callback. For the moment,
     * a simplistic logging callback will do (e.g., no logging levels)
     */
    typedef void (*logCallback) (const std::string&);

    /**
     * This class represents an item of aggregated feedback, where the first item is the
     * sequence number, the second is the receive timestamp (in Nanoseconds), and the
     * third is the ECN marking value read at the receiver
     */
    struct FeedbackItem {
        uint16_t sequence;
        uint64_t rxTimestampUs;
        uint8_t ecn;
    };

    /** To avoid future complexity and defects, we make the following
     *  assumptions regarding wrapping of unsigned integers:
     *    - sequences, uint16_t, can wrap (just like TCP)
     *    - timestamps (Nanoseconds), uint64_t, can wrap (despite being 64 bits long)
     *    - delays (Nanoseconds), uint64_t, can wrap (easily), as they are
     *      obtained from subtraction of timestamps obtained at different endpoints,
     *      which may have non-synchronized clocks.
     */
    struct PacketRecord {
        uint16_t sequence;
        uint64_t txTimestampUs;
        uint32_t size;
        uint64_t owdUs;
        uint64_t rttUs;
    };

    /** Class constructor */
    SagRtpController();

    /** Class destructor */
    virtual ~SagRtpController();

    /**
     * Set id of the controller; it can be used to prepend the log lines
     *
     * @param [in] id A string denoting the flow's id
     */
    void setId(const std::string& id);

    /**
     * Set the initial bandwidth estimation
     *
     * @param [in] initBw Initial bandwidth estimation
     */
    void setInitBw(float initBw);

    /**
     * Set the minimal bandwidth. Controllers should never output
     * a bandwidth smaller than this one
     *
     * @param [in] initBw Minimal bandwidth
     */
    void setMinBw(float minBw);

    /**
     * Set the maximal bandwidth. Controllers should never output
     * a bandwidth greater than this one
     *
     * @param [in] initBw Maximal bandwidth
     */
    void setMaxBw(float maxBw);

    /**
     * Set the current bandwidth estimation. This can be useful in test environments
     * to temporarily disrupt the current bandwidth estimation
     *
     * @param [in] newBw Bandwidth estimation to overwrite the current estimation
     */
    virtual void setCurrentBw(float newBw) =0;

    /**
     * Set the logging callback. Congestion controllers can use this callback
     * to log events occurring within the algorithm's logic
     *
     * @param [in] f Logging function to be called from the congestion
     *               controller implementation
     */
    void setLogCallback(logCallback f);

    /**
     * This API call will reset the internal state of the congestion
     * controller. The new state will be the same as that of a freshly
     * instantiated controller object
     */
    virtual void reset();

    /**
     * This function is called every time a media packet is ready to be sent
     * off to the receiver endpoint. By calling this function, the sender
     * application provides the congestion controller with information on the
     * media packet being sent
     *
     * This member function is not pure virtual. All subclasses should call
     * the superclass's method to ensure the proper operation of the common
     * logic implemented by the superclass
     *
     * @param [in] txTimestampUs The time (in Nanoseconds) at which the packet
     *                           is sent
     * @param [in] sequence The sequence number in the packet, which will be
     *                      used to identify the corresponding feedback from
     *                      the receiver endpoint
     * @param [in] size Size of the packet in bytes. In principle, the sender
     *                  application denotes the size of the payload (i.e.,
     *                  without accounting for any RTP/UDP/IP overheads).
     *                  This can be changed, though
     * @retval true if all went well, false if there was an error
     *
     * @note There are two ways this function can fail:
     *       - returning false: this means that a problem was detected in
     *                          the input parameters (e.g., bad sequence)
     *       - asserting (crashing): this means there is a bug in the
     *                               function's logic
     */
    virtual bool processSendPacket(uint64_t txTimestampUs,
                                   uint16_t sequence,
                                   uint32_t size); // in Bytes

    /**
     * Upon arrival of a feedback packet from the receiver endpoint, the send
     * application calls this function to deliver the data contained in the
     * feedback packet to the congestion controller
     *
     * This member function is not pure virtual. All subclasses should call
     * the superclass's method to ensure the proper operation of the common
     * logic implemented by the superclass
     *
     * @param [in] nowUs The time (in Nanoseconds) at which this function is called
     * @param [in] sequence The sequence number of the media packet that this
     *                      feedback refers to
     * @param [in] rxTimestampUs The time (in Nanoseconds) at which this the media
     *                           packet was received at the receiver endpoint
     * @param [in] ecn The Explicit Congestion Notification (ECN) marking value
     *                 (specified in rfc3168), as seen by the receiver endpoint
     * @retval true if all went well, false if there was an error
     *
     * @note There are two ways this function can fail:
     *       - returning false: this means that a problem was detected in
     *                          the input parameters (e.g., sequence from
     *                          the future, decreasing timestamps)
     *       - asserting (crashing): this means there is a bug in the
     *                               function's logic
     */
    virtual bool processFeedback(uint64_t nowUs,
                                 uint16_t sequence,
                                 uint64_t rxTimestampUs,
                                 uint8_t ecn=0);

    /**
     * If aggregated feedback is received from the receiver endpoint, this function
     * offers the send application a way to process the aggregated feedback as a batch
     *
     * This member function is not pure virtual, as contains a base implementation that
     * calls function #processSendPacket in a loop
     *
     * @param [in] nowUs The time (in Nanoseconds) at which this function is called
     * @param [in] feedbackBatch A vector of items containing sequence numbers, receive
     *             timestamps (in Nanoseconds), and ECN marking values of
     *             the aggregated feedback
     */
    virtual bool processFeedbackBatch(uint64_t nowUs,
                                      const std::vector<FeedbackItem>& feedbackBatch);

    /**
     * The sender application will call this function every time it needs to
     * know what is the current bandwidth as estimated by the congestion
     * controller. The bandwidth information is typically used to have the
     * media codecs adapt to the current (estimated) available bandwidth
     *
     * @param [in] nowUs The time at which this function is called, in Nanoseconds
     * @retval the congestion controller's bandwidth estimation, in bps
     */
    virtual float getBandwidth(uint64_t nowUs) const =0;


    virtual void setDir(std::string baseLogsDir);

protected:
    /** A "less than" operator for unsigned integers that supports wrapping */
    template <typename UINT>
    bool lessThan(UINT lhs, UINT rhs) const {
        UINT noWrapSubtract = rhs - lhs;
        UINT wrapSubtract = lhs - rhs;
        return noWrapSubtract < wrapSubtract;
    }

    /**
     * Set the history length, in Nanoseconds, for calculating metrics.
     * Information from packets sent more than lenUs Nanoseconds ago will be
     * garbage collected and thus not used for metric calculation
     *
     * @param [in] lenUs New history length (in Nanoseconds)
     */
    void setHistoryLength(uint64_t lenUs);

    /**
     * Get the current history length, in Nanoseconds. Any packet that was
     * sent more than this length Nanoseconds in the past is garbage collected
     * and is not used for calculating any metric
     *
     * @retval The current history length (in Nanoseconds)
     */
    uint64_t getHistoryLength() const;

    /**
     * Function used to log messages. It calls the message logging callback
     * if has been set, otherwise it logs to stdout
     */
    void logMessage(const std::string& log) const;

    /*
     * The functions below calculate different delay and loss
     * metrics based on the received feedback. Although they can
     * be considered part of the NADA algorithm, we have defined
     * them in the superclass because they could also be useful
     * to other algorithms
     * */

    /*
     * Calculate current queuing delay (qdelay)
     *
     * @param [out] qdelayUs Queuing delay in Nanoseconds during current history length
     * @retval False if the current history is empty (output parameter is not
     *         valid). True otherwise
     */
    bool getCurrentQdelay(uint64_t& qdelayUs) const;

    /**
     * Calculate current round trip time (rtt)
     *
     * @param [out] rttUs Round trip time in Nanoseconds during current history length
     * @retval False if the current history is empty (output parameter is not
     *         valid). True otherwise
     */
    bool getCurrentRTT(uint64_t& rttUs) const;

    /**
     * Calculate current info on packet losses
     *
     * @param [out] nLoss Number of packets lost during current history length
     * @param [out] plr Loss ratio (losses per packet) for the current history
     *                  length
     * @retval False if the current history does not contain enough packets to
     *         calculate the metrics (output parameter is not valid). True
     *         otherwise
     */
    bool getPktLossInfo(uint32_t& nLoss, float& plr) const;

    /**
     * Calculate current rate at which the receiver is receiving the media
     * packets (receive rate), in bits per second
     *
     * @param [out] rrateBps Current receive rate in bps
     * @retval False if the current history does not contain enough packets to
     *         calculate the metrics (output parameter is not valid). True
     *         otherwise
     */
    bool getCurrentRecvRate(float& rrateBps) const;

    /**
     * Calculate the current average inter-loss interval. A loss event is
     * the loss of one or more consecutive packets (loss burst). An
     * inter-loss interval is the number of packets received without sequence
     * gaps between two consecutive loss events. The average is weighted, based
     * on the coefficients specified by the TCP-friendly rate control
     * algorithm (TFRC). See rfc5348.
     *
     * @param [out] avgInterval Average inter-loss interval in packets
     * @param [out] rrateBps Current (most recent, growing) inter-loss interval in packets
     * @retval False if there have not been any losses yet, and therefore there are no
     *         inter-loss intervals to return; in this case, the output parameters are not
     *         valid). True otherwise
     */
    bool getLossIntervalInfo(float& avgInterval, uint32_t& currentInterval) const;

    bool m_firstSend; /**< true if at least one packet has been sent */
    uint16_t m_lastSequence; /**< sequence of the last packet sent */
    /**
     * Estimation of the network propagation delay, plus clock difference
     * between sender and receiver endpoints. In Nanoseconds
     */
    uint64_t m_baseDelayUs;
    /**
     * Sent packets for which feedback has not been received yet
     */
    std::deque<PacketRecord> m_inTransitPackets;
    /**
     * Packets for which feedback has already been received. Information
     * contained in these records will be used to calculate the different
     * metrics that congestion controllers use
     */
    std::deque<PacketRecord> m_packetHistory;
    /**
     * Maintains the sum of the size of all packets in #m_packetHistory .
     * This is done for efficiency reasons
     */
    uint32_t m_pktSizeSum;

    std::string m_id_control; /**< Id used for logging, and can be used for plotting */

    float m_initBw;
    float m_minBw;
    float m_maxBw;

    logCallback m_logCallback;

    InterLossState m_ilState;

private:
    uint64_t m_historyLengthUs; // in Nanoseconds
    std::string m_baseLogsDir; //!< Where the burst logs will be written to:
                                //!<   logs_dir/sag_nada_feedback_[id].csv

    void setDefaultId();
    void updateInterLossData(uint16_t sequence);
};

}

#endif /* SAG_RTP_CONTROLLER_H */
