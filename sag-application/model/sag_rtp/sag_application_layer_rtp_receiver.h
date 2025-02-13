#ifndef RTP_RECEIVER_H
#define RTP_RECEIVER_H

#include "ns3/sag_rtp_header.h"
#include "ns3/socket.h"
#include "ns3/application.h"
#include "ns3/sag_application_layer.h"
#include "ns3/sag_rtp_constants.h"
#include "ns3/sag_burst_info.h"
#include "syncodecs.h"
#include "ns3/socket.h"
#include "ns3/application.h"
#include "sag_rtp_controller.h"
#include <memory>

namespace ns3 {

class SAGApplicationLayerRTPReceiver: public SAGApplicationLayer
{
public:
    static TypeId GetTypeId (void);
    SAGApplicationLayerRTPReceiver ();
    virtual ~SAGApplicationLayerRTPReceiver ();

    void Setup (uint16_t port);
    std::vector<std::tuple<SAGBurstInfoRtp, uint64_t>> GetIncomingBurstsInformation();
    virtual void RegisterIncomingBurst(SAGBurstInfoRtp burstInfo, bool enable_precise_logging);
    uint64_t GetReceivedCounterOf(int64_t burst_id);
    uint64_t GetReceivedSizeCounterOf(int64_t burst_id);
    uint32_t GetMaxPayloadSizeByte();

private:
    virtual void StartApplication ();
    virtual void StopApplication ();
    /**
       * \brief receive packet
       *
       * \param socket socket bind at the receiver
       */
    void RecvPacket (Ptr<Socket> socket);
    /**
       * \brief add feedback
       *
       * \param sequence
       * \param recvTimestampUs
       */
    void AddFeedback (uint16_t sequence,
                      uint64_t recvTimestampUs);
    /**
       * \brief send feedback
       *
       * \param reschedule
       */
    void SendFeedback (bool reschedule);

    void SetSocketType(TypeId tid, TypeId socketTid);

private:
    bool m_running;
    bool m_waiting;
    uint32_t m_ssrc;
    uint32_t m_remoteSsrc;
    Ipv4Address m_srcIp;
    uint16_t m_srcPort;
    Ptr<Socket> m_socket;
    CCFeedbackHeader m_header;
    EventId m_sendEvent;
    uint64_t m_periodUs;
    uint32_t m_max_payload_size_byte;  //!< Maximum size of payload before getting fragmented


    // Incoming bursts
    std::vector<SAGBurstInfoRtp> m_incoming_bursts;
    std::map<int64_t, uint64_t> m_incoming_bursts_received_counter;       //!< Counter for how many packets received
    std::map<int64_t, uint64_t> m_incoming_bursts_received_size_counter;       //!< Counter for how many packets received
    std::map<int64_t, uint64_t> m_incoming_bursts_enable_precise_logging; //!< True iff enable precise logging for each burst
};

}

#endif /* RTP_RECEIVER_H */
