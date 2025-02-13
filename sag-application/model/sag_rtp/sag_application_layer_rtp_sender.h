#ifndef RTP_SENDER_H
#define RTP_SENDER_H

#include "ns3/sag_application_layer.h"
#include "ns3/sag_burst_info.h"
#include "ns3/sag_rtp_header.h"
#include "ns3/sag_rtp_constants.h"
#include "syncodecs.h"
#include "ns3/socket.h"
#include "ns3/application.h"
#include "sag_rtp_controller.h"
#include "sag_rtp_nada_controller.h"
#include "sag_rtp_dummy_controller.h"
#include <memory>


namespace ns3 {

class Socket;
class Packet;

class SAGApplicationLayerRTPSender : public SAGApplicationLayer
{
public:
    static TypeId GetTypeId (void);
    SAGApplicationLayerRTPSender ();
    virtual ~SAGApplicationLayerRTPSender ();

    void PauseResume (bool pause);

    void SetCodec (std::shared_ptr<syncodecs::Codec> codec);

    void SetController (std::shared_ptr<SagRtpController> controller);//SagNadaController


    void SetRinit (float Rinit);
    void SetRmin (float Rmin);
    void SetRmax (float Rmax);
    void SetDir (std::string baseLogsDir);

    void Setup (Ipv4Address destIP, uint16_t dest_port);

    uint32_t GetMaxPayloadSizeByte();
    virtual void RegisterOutgoingBurst(SAGBurstInfoRtp burstInfo, Ptr<Node> targetNode, uint16_t port,  bool enable_precise_logging);
    std::vector<std::tuple<SAGBurstInfoRtp, uint64_t>> GetOutgoingBurstsInformation();
    uint64_t GetSentCounterOf(int64_t burst_id);
    uint64_t GetSentSizeCounterOf(int64_t burst_id);

protected:
    virtual void DoDispose (void);
    //Ptr<Socket> m_socket; //!< IPv4 Socket

private:
    virtual void StartApplication (void);
    virtual void StopApplication (void);

    /**
       * \brief enqueue packet
       */
    void EnqueuePacket ();
    /**
       * \brief send packet
       *
       * \param nsSlept send interval in nano second
       */
    void SendPacket (uint64_t nsSlept);

    /**
       * \brief handle the feedback
       *
       * \param socket the socket between transport layer and application layer
       */
    void HandleRead (Ptr<Socket> socket);
    /**
       * \brief calculate the buffer at current time
       *
       * \param now_ns current time in nano seconds
       */
    void CalcBufferParams (uint64_t now_ns);

    void SendOverSleep(uint32_t bytesToSend);

    void SetSocketType(TypeId tid, TypeId socketTid);

    //std::shared_ptr<syncodecs::Codec> m_codec;
    std::shared_ptr<SagRtpController> m_controller;
    Ipv4Address m_destIP;
    uint16_t m_port;
    float m_initBw;
    float m_minBw;
    float m_maxBw;
    bool m_paused;
    uint32_t m_ssrc;
    uint16_t m_sequence;
    uint32_t m_rtpTsOffset;
    Ptr<Socket> m_socket;
    EventId m_enqueueEvent;
    EventId m_sendEvent;
    EventId m_sendOversleepEvent;


    uint32_t m_max_payload_size_byte;  //!< Maximum size of payload before getting fragmented
    double m_rVin; //bps
    double m_rSend; //bps
    std::deque<uint32_t> m_rateShapingBuf;
    uint32_t m_rateShapingBytes;
    uint64_t m_nextSendTstmpNs;

    // Outgoing bursts
    //std::vector<std::tuple<SAGBurstInfoRtp, InetSocketAddress>> m_outgoing_bursts; //!< Weakly ascending on start time list of bursts
    std::vector<std::tuple<SAGBurstInfoRtp, Ptr<Node>>> m_outgoing_bursts; //!< Weakly ascending on start time list of bursts
    std::vector<uint64_t> m_outgoing_bursts_packets_sent_counter; //!< Amount of packets sent out already for each burst
    std::vector<uint64_t> m_outgoing_bursts_packets_size_sent_counter;
    std::vector<EventId> m_outgoing_bursts_event_id; //!< Event ID of the outgoing burst send loop
    std::vector<bool> m_outgoing_bursts_enable_precise_logging; //!< True iff enable precise logging for each burst
    size_t m_next_internal_burst_idx; //!< Next burst index to send out


    // Incoming bursts
    std::vector<SAGBurstInfoRtp> m_incoming_bursts;
    std::map<int64_t, uint64_t> m_incoming_bursts_received_counter;       //!< Counter for how many packets received
    std::map<int64_t, uint64_t> m_incoming_bursts_enable_precise_logging; //!< True iff enable precise logging for each burst
};


} // namespace ns3

#endif /* RTP_SENDER_H */
