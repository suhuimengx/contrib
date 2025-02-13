#include "sag_application_layer_rtp_receiver.h"
#include "sag_rtp_constants.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/route_trace_tag.h"
#include "ns3/id_seq_tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SAGApplicationLayerRTPReceiver");

NS_OBJECT_ENSURE_REGISTERED (SAGApplicationLayerRTPReceiver);

TypeId
SAGApplicationLayerRTPReceiver::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::SAGApplicationLayerRTPReceiver")
            .SetParent<SAGApplicationLayer>()
            .SetGroupName("Applications")
            .AddConstructor<SAGApplicationLayerRTPReceiver>()
            .AddAttribute("Port", "Port on which we listen for incoming packets.",
                            UintegerValue(9),
                            MakeUintegerAccessor(&SAGApplicationLayerRTPReceiver::m_srcPort),
                            MakeUintegerChecker<uint16_t>())
            ;
    return tid;
}

SAGApplicationLayerRTPReceiver::SAGApplicationLayerRTPReceiver ()
: m_running{false}
, m_waiting{false}
, m_ssrc{0}
, m_remoteSsrc{0}
, m_srcIp{}
, m_srcPort{}
, m_socket{0}
, m_header{}
, m_sendEvent{}
, m_periodUs{SAG_RTP_FEEDBACK_PERIOD_US}
{}

SAGApplicationLayerRTPReceiver::~SAGApplicationLayerRTPReceiver () {

}

void
SAGApplicationLayerRTPReceiver::SetSocketType(TypeId tid, TypeId socketTid)
{
    Ptr<SocketFactory> socketFactory = GetNode()->GetObject<SocketFactory> (tid);
    socketFactory->SetSocketType(socketTid);
}

void
SAGApplicationLayerRTPReceiver::Setup (uint16_t port)
{
    NS_LOG_FUNCTION(this << port);
    //m_socket = Socket::CreateSocket (GetNode (), UdpSocketFactory::GetTypeId ());
    //auto local = InetSocketAddress{Ipv4Address::GetAny (), port};
    //auto ret = m_socket->Bind (local);
    //NS_ASSERT (ret == 0);

    if (m_socket == 0) {
        TypeId factoryTid = TypeId::LookupByName(m_transport_factory_name);
        TypeId socketTid = TypeId::LookupByName(m_transport_socket_name);
        SetSocketType(factoryTid, socketTid);
        m_socket = Socket::CreateSocket(GetNode(), factoryTid);
        InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), port);
        if (m_socket->Bind(local) == -1) {
            NS_FATAL_ERROR("Failed to bind socket");
        }
    }

    //std::cout<< "SAGApplicationLayerRTPReceiver::Setup:GetNode ()" << GetNode () << std::endl;
    //std::cout << ret << "socket" << m_socket << "local" << local << "node" << GetNode()  << std::endl;
    m_socket->SetRecvCallback (MakeCallback (&SAGApplicationLayerRTPReceiver::RecvPacket, this));
    m_running = false;
    m_waiting = true;
}

void
SAGApplicationLayerRTPReceiver::StartApplication ()
{
    NS_LOG_FUNCTION(this);
    //std::cout<< "SAGApplicationLayerRTPReceiver::StartApplication:GetNode ()" << GetNode () << std::endl;
	//Setup(1026);
    Setup(m_srcPort);
    m_running = true;
    m_ssrc = rand ();
    m_header.SetSendSsrc (m_ssrc);
    Time tFirst {NanoSeconds (m_periodUs)};
    m_sendEvent = Simulator::Schedule (tFirst, &SAGApplicationLayerRTPReceiver::SendFeedback, this, true);
}

void
SAGApplicationLayerRTPReceiver::StopApplication ()
{
    NS_LOG_FUNCTION(this);
    m_running = false;
    m_waiting = true;
    m_header.Clear ();
    Simulator::Cancel (m_sendEvent);
}

void
SAGApplicationLayerRTPReceiver::RecvPacket (Ptr<Socket> socket)
{

	//std::cout << "SAGApplicationLayerRTPReceiver::RecvPacket" <<std::endl;
	NS_LOG_FUNCTION(this << socket);
    if (!m_running) {
        return;
    }
    Ptr <Packet> packet;
    Address from;
    while ((packet = socket->RecvFrom(from))) {
        NS_LOG_INFO ("SAGApplicationLayerRTPReceiver::RecvPacket, " << packet->ToString ());
        // Extract burst identifier and packet sequence number
        SagRtpHeader incomingIdSeq{};
        packet->RemoveHeader (incomingIdSeq);

        IdSeqTag idTag;
        packet->PeekPacketTag(idTag);

        // Count packets from incoming bursts
	    m_incoming_bursts_received_counter.at(idTag.GetId()) += 1;
	    m_incoming_bursts_received_size_counter.at(idTag.GetId()) += packet->GetSize ();
        m_totalRxBytes = m_incoming_bursts_received_size_counter.at(idTag.GetId());
        m_totalRxPacketNumber = m_incoming_bursts_received_counter.at(idTag.GetId());


        // Log precise trace
		if (m_incoming_bursts_enable_precise_logging[idTag.GetId()]) {
			RecordDetailsLog(packet);
		}



	    auto srcIp = InetSocketAddress::ConvertFrom (from).GetIpv4 ();
	    const auto srcPort = InetSocketAddress::ConvertFrom (from).GetPort ();
	    if (m_waiting) {
	    	m_waiting = false;
	    	m_remoteSsrc = incomingIdSeq.GetSsrc ();
	        m_srcIp = srcIp;
	        m_srcPort = srcPort;
	    } else {
	    	// Only one flow supported
	        NS_ASSERT (m_remoteSsrc == incomingIdSeq.GetSsrc ());
	        //NS_ASSERT (m_srcIp == srcIp);
	        NS_ASSERT (m_srcPort == srcPort);
	    }

	    uint64_t recvTimestampNs = Simulator::Now ().GetNanoSeconds ();
	    AddFeedback (incomingIdSeq.GetSequence (), recvTimestampNs);

	    //const uint64_t now_ns = Simulator::Now ().GetNanoSeconds ();
	    //std::cout<<now_ns<<"  "<<incomingIdSeq.GetSequence ()<<std::endl;
    }
}

void
SAGApplicationLayerRTPReceiver::AddFeedback (uint16_t sequence,
                                 uint64_t recvTimestampNs)
{
    NS_LOG_FUNCTION(this << sequence << recvTimestampNs);
    auto res = m_header.AddFeedback (m_remoteSsrc, sequence, recvTimestampNs);
    if (res == CCFeedbackHeader::CCFB_TOO_LONG) {
        SendFeedback (false);
        res = m_header.AddFeedback (m_remoteSsrc, sequence, recvTimestampNs);
    }
    NS_ASSERT (res == CCFeedbackHeader::CCFB_NONE);
}

void
SAGApplicationLayerRTPReceiver::SendFeedback (bool reschedule)
{
	NS_LOG_FUNCTION(this);
    if (m_running && !m_header.Empty ()) {
        //TODO (authors): If packet empty, easiest is to send it as is. Propose to authors
        auto packet = Create<Packet> ();
        packet->AddHeader (m_header);
        NS_LOG_INFO ("SAGApplicationLayerRTPReceiver::SendFeedback, " << packet->ToString ());
	    // first up interface
	    InetSocketAddress adr(m_srcPort);
		for(uint32_t ifNum = 1; ifNum < m_srcNode->GetObject<Ipv4>()->GetNInterfaces(); ifNum++){
			if(m_srcNode->GetObject<Ipv4>()->IsUp(ifNum)){
				adr.SetIpv4(m_srcNode->GetObject<Ipv4>()->GetAddress(ifNum,0).GetLocal());
				break;
			}
		}
        m_socket->SendTo (packet, 0, adr);

        m_header.Clear ();
        m_header.SetSendSsrc (m_ssrc);
    }

    if (reschedule) {
        Time tNext {NanoSeconds (m_periodUs)};
        m_sendEvent = Simulator::Schedule (tNext, &SAGApplicationLayerRTPReceiver::SendFeedback, this, true);
    }
}

std::vector<std::tuple<SAGBurstInfoRtp, uint64_t>>
SAGApplicationLayerRTPReceiver::GetIncomingBurstsInformation()
{
    std::vector<std::tuple<SAGBurstInfoRtp, uint64_t>> result;
    for (size_t i = 0; i < m_incoming_bursts.size(); i++) {
        result.push_back(std::make_tuple(m_incoming_bursts[i], m_incoming_bursts_received_counter.at(m_incoming_bursts[i].GetBurstId())));
    }
    return result;
}

uint64_t
SAGApplicationLayerRTPReceiver::GetReceivedCounterOf(int64_t burst_id)
{
    return m_incoming_bursts_received_counter.at(burst_id);
}

uint64_t
SAGApplicationLayerRTPReceiver::GetReceivedSizeCounterOf(int64_t burst_id)
{
    return m_incoming_bursts_received_size_counter.at(burst_id);
}

void
SAGApplicationLayerRTPReceiver::RegisterIncomingBurst(SAGBurstInfoRtp burstInfo, bool enable_precise_logging)
{
    NS_ABORT_MSG_IF(burstInfo.GetToNodeId() != this->GetNode()->GetId(), "Destination node identifier is not that of this node.");
    m_incoming_bursts.push_back(burstInfo);
    m_incoming_bursts_received_counter[burstInfo.GetBurstId()] = 0;
    m_incoming_bursts_received_size_counter[burstInfo.GetBurstId()] = 0;
    m_incoming_bursts_enable_precise_logging[burstInfo.GetBurstId()] = enable_precise_logging;
}

uint32_t
SAGApplicationLayerRTPReceiver::GetMaxPayloadSizeByte()
{
    return m_max_payload_size_byte;
}

}
