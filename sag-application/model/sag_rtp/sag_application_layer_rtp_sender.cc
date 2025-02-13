#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/address-utils.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
//#include "ns3/sag_transport_socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/abort.h"
#include <cstring>
#include <string>
#include "ns3/string.h"
#include "ns3/udp-socket-factory.h"
#include "syncodecs.h"
#include <sys/stat.h>

#include "sag_application_layer_rtp_sender.h"
#include "sag_rtp_header.h"
#include "sag_rtp_dummy_controller.h"
#include "sag_rtp_nada_controller.h"
#include "ns3/route_trace_tag.h"
#include "ns3/id_seq_tag.h"
#include "ns3/delay_trace_tag.h"

//SagDummyController to SagNadaController


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SAGApplicationLayerRTPSender");

NS_OBJECT_ENSURE_REGISTERED (SAGApplicationLayerRTPSender);

TypeId
SAGApplicationLayerRTPSender::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::SAGApplicationLayerRTPSender")
            .SetParent<SAGApplicationLayer>()
            .SetGroupName("Applications")
            .AddConstructor<SAGApplicationLayerRTPSender>()

            .AddAttribute("Port", "Port on which we listen for incoming packets.",
                            UintegerValue(9),
                            MakeUintegerAccessor(&SAGApplicationLayerRTPSender::m_port),
                            MakeUintegerChecker<uint16_t>());
    return tid;
}

SAGApplicationLayerRTPSender::SAGApplicationLayerRTPSender()
: m_destIP{}
, m_port{0}
, m_initBw{1000*1000}
, m_minBw{150*1000}//150*1000
, m_maxBw{1500*1000}//1500*1000
, m_paused{false}
, m_ssrc{0}
, m_sequence{0}
, m_rtpTsOffset{0}
, m_socket{0}
, m_enqueueEvent{}
, m_sendEvent{}
, m_sendOversleepEvent{}
, m_rVin{0.}
, m_rSend{0.}
, m_rateShapingBytes{0}
, m_nextSendTstmpNs{0}
{
    NS_LOG_FUNCTION(this);
    m_next_internal_burst_idx = 0;
}

SAGApplicationLayerRTPSender::~SAGApplicationLayerRTPSender()
{
    NS_LOG_FUNCTION(this);
    m_socket = 0;
}

void
SAGApplicationLayerRTPSender ::PauseResume (bool pause)
{
    NS_ASSERT (pause != m_paused);
    if (pause) {
        Simulator::Cancel (m_enqueueEvent);
        Simulator::Cancel (m_sendEvent);
        Simulator::Cancel (m_sendOversleepEvent);
        m_rateShapingBuf.clear ();
        m_rateShapingBytes = 0;
    } else {
        m_rVin = m_initBw;
        m_rSend = m_initBw;
        m_enqueueEvent = Simulator::ScheduleNow (&SAGApplicationLayerRTPSender ::EnqueuePacket, this);
        m_nextSendTstmpNs = 0;
    }
    m_paused = pause;
}

void
SAGApplicationLayerRTPSender ::SetCodec (std::shared_ptr<syncodecs::Codec> codec)
{
    NS_LOG_FUNCTION(this);
    m_codec = codec;
}

//void
//SAGApplicationLayerRTPSender ::SetTrace (TraceType traceType)
//{
//	//std::cout<<"Dir"<<m_baseLogsDir<<std::endl;
//    NS_LOG_FUNCTION(this);
//    if (traceType==TRACE_TYPE_CHAT)
//    	{
//    	    m_traceDir=m_baseDir+"/syncodecs/video_traces/chat_firefox_h264";
//            m_filePrefix = "chat";
//    	}
//    else if (traceType==TRACE_TYPE_BBB)
//    	{
//    		m_traceDir=m_baseDir+"/syncodecs/video_traces/big_buck_bunny";
//            m_filePrefix = "bbb_offset1900_1080p24fps";
//    	}
//    else if (traceType==TRACE_TYPE_CONCAT)
//    	{
//            m_traceDir=m_baseDir+"/syncodecs/video_traces/Concat";
//            m_filePrefix = "Concat_ProRes";
//    	}
//    else if (traceType==TRACE_TYPE_ED)
//    	{
//            m_traceDir=m_baseDir+"/syncodecs/video_traces/elephants_dream";
//            m_filePrefix = "ed_offset3900_1080p24fps";
//    	}
//    else if (traceType==TRACE_TYPE_FOREMAN)
//    {
//            m_traceDir=m_baseDir+"/syncodecs/video_traces/Foreman_lookahead_1";
//            m_filePrefix = "Foreman_ProRes";
//    	}
//    else if (traceType==TRACE_TYPE_NEWS)
//    	{
//            m_traceDir=m_baseDir+"/syncodecs/video_traces/News_lookahead_1";
//            m_filePrefix = "News_ProRes";
//    	}
//    else
//    	{
//    	    m_traceDir=m_baseLogsDir+"/syncodecs/video_traces/chat_firefox_h264";
//            m_filePrefix = "chat";
//    	}
//}
//
//// TODO (deferred): allow flexible input of video traffic trace path via config file, etc.
//void
//SAGApplicationLayerRTPSender ::SetCodecType (SyncodecType codecType)
//{
//    NS_LOG_FUNCTION(this);
//    syncodecs::Codec* codec = NULL;
//    switch (codecType) {
//        case SYNCODEC_TYPE_PERFECT:
//        {
//            codec = new syncodecs::PerfectCodec{DEFAULT_PACKET_SIZE};
//            break;
//        }
//        case SYNCODEC_TYPE_FIXFPS:
//        {
//            m_fps = SYNCODEC_DEFAULT_FPS;
//            auto innerCodec = new syncodecs::SimpleFpsBasedCodec{m_fps};
//            codec = new syncodecs::ShapedPacketizer{innerCodec, DEFAULT_PACKET_SIZE};
//            break;
//        }
//        case SYNCODEC_TYPE_STATS:
//        {
//            m_fps = SYNCODEC_DEFAULT_FPS;
//            auto innerStCodec = new syncodecs::StatisticsCodec{m_fps};
//            codec = new syncodecs::ShapedPacketizer{innerStCodec, DEFAULT_PACKET_SIZE};
//            break;
//        }
//        case SYNCODEC_TYPE_TRACE:
//        case SYNCODEC_TYPE_HYBRID:
//        {
//            const std::vector<std::string> candidatePaths = {
//                ".",      // If run from top directory (e.g., with gdb), from ns-3.26/
//                "../",    // If run from with test_new.py with designated directory, from ns-3.26/2017-xyz/
//                "../..",  // If run with test.py, from ns-3.26/testpy-output/201...
//            };
//
//            const std::string traceSubDir{"scratch/main_satnet/test_data/end_to_end/run/syncodecs/video_traces/chat_firefox_h264"};
//            std::string traceDir{};
//
//            for (auto c : candidatePaths) {
//                std::ostringstream currPathOss;
//                currPathOss << c << "/" << traceSubDir;
//                struct stat buffer;
//                if (::stat (currPathOss.str ().c_str (), &buffer) == 0) {
//                    //filename exists
//                    traceDir = currPathOss.str ();
//                    break;
//                }
//            }
//            NS_ASSERT_MSG (!m_traceDir.empty (), "Traces file not found in candidate paths");
//
//            auto innerCodec = (codecType == SYNCODEC_TYPE_TRACE) ?
//                                 new syncodecs::TraceBasedCodecWithScaling{
//                                    m_traceDir,        // path to traces directory
//                                    m_filePrefix,      // video filename
//                                    SYNCODEC_DEFAULT_FPS,             // Default FPS: 30fps
//                                    true} :          // fixed mode: image resolution doesn't change
//                                 new syncodecs::HybridCodec{
//                                    m_traceDir,        // path to traces directory
//                                    m_filePrefix,      // video filename
//                                    SYNCODEC_DEFAULT_FPS,             // Default FPS: 30fps
//                                    true};           // fixed mode: image resolution doesn't change
//	    m_fps = SYNCODEC_DEFAULT_FPS;
//            codec = new syncodecs::ShapedPacketizer{innerCodec, DEFAULT_PACKET_SIZE};
//            break;
//        }
//        case SYNCODEC_TYPE_SHARING:
//        {
//            auto innerShCodec = new syncodecs::SimpleContentSharingCodec{};
//            codec = new syncodecs::ShapedPacketizer{innerShCodec, DEFAULT_PACKET_SIZE};
//            break;
//        }
//        default:  // defaults to perfect codec
//            codec = new syncodecs::PerfectCodec{DEFAULT_PACKET_SIZE};
//    }
//
//    // update member variable
//    m_codec = std::shared_ptr<syncodecs::Codec>{codec};
//}

void
SAGApplicationLayerRTPSender::SetController (std::shared_ptr<SagRtpController> controller)//SagNadaController
{
    NS_LOG_FUNCTION(this);
    m_controller = controller;
}

void
SAGApplicationLayerRTPSender::Setup (Ipv4Address destIP, uint16_t destPort)
{
    NS_LOG_FUNCTION(this);
    if (!m_codec) {
        m_codec = std::make_shared<syncodecs::PerfectCodec> (DEFAULT_PACKET_SIZE);
    }

    if (!m_controller) {
        m_controller = std::make_shared<SagNadaController> ();//SagNadaController SagDummyController
        SetDir(m_baseLogsDir);
        //SetAttribute (m_controller, "BaseLogsDir", StringValue(m_baseLogsDir));
    } else {
        m_controller->reset ();
        SetDir(m_baseLogsDir);
    }

    m_destIP = destIP;
    m_port = destPort;
}

uint32_t
SAGApplicationLayerRTPSender::GetMaxPayloadSizeByte()
{
    return m_max_payload_size_byte;
}

void
SAGApplicationLayerRTPSender::SetRinit (float r)
{
    NS_LOG_FUNCTION(this);
    m_initBw = r;
    if (m_controller) m_controller->setInitBw (m_initBw);
}

void
SAGApplicationLayerRTPSender::SetRmin (float r)
{
    NS_LOG_FUNCTION(this);
    m_minBw = r;
    if (m_controller) m_controller->setMinBw (m_minBw);
}

void
SAGApplicationLayerRTPSender::SetRmax (float r)
{
    NS_LOG_FUNCTION(this);
    m_maxBw = r;
    if (m_controller) m_controller->setMaxBw (m_maxBw);
}

void
SAGApplicationLayerRTPSender::SetDir (std::string baseLogsDir)
{
    NS_LOG_FUNCTION(this);
    if (m_controller)
    	m_controller->setDir (baseLogsDir);
    	//m_controller.SetAttribute("BaseLogsDir",baseLogsDir);
}

void
SAGApplicationLayerRTPSender::RegisterOutgoingBurst(SAGBurstInfoRtp burstInfo, Ptr<Node> targetNode, uint16_t port, bool enable_precise_logging)
{
    NS_ABORT_MSG_IF(burstInfo.GetFromNodeId() != this->GetNode()->GetId(), "Source node identifier is not that of this node.");
    if (m_outgoing_bursts.size() >= 1 && burstInfo.GetStartTimeNs() < std::get<0>(m_outgoing_bursts[m_outgoing_bursts.size() - 1]).GetStartTimeNs()) {
        throw std::runtime_error("Bursts must be added weakly ascending on start time");
    }
    //m_outgoing_bursts.push_back(std::make_tuple(burstInfo, targetAddress));
    m_dstPort = port;
    m_outgoing_bursts.push_back(std::make_tuple(burstInfo, targetNode));
    m_outgoing_bursts_packets_sent_counter.push_back(0);
    m_outgoing_bursts_packets_size_sent_counter.push_back(0);
    m_outgoing_bursts_event_id.push_back(EventId());
    m_outgoing_bursts_enable_precise_logging.push_back(enable_precise_logging);
}



void
SAGApplicationLayerRTPSender::DoDispose(void)
{
    NS_LOG_FUNCTION(this);
    SAGApplicationLayer::DoDispose();
}

void
SAGApplicationLayerRTPSender::SetSocketType(TypeId tid, TypeId socketTid)
{
    Ptr<SocketFactory> socketFactory = GetNode()->GetObject<SocketFactory> (tid);
    socketFactory->SetSocketType(socketTid);
}

void
SAGApplicationLayerRTPSender::StartApplication(void)
{
    NS_LOG_FUNCTION(this);
    SetDir(m_baseLogsDir);
    // RTP initial values for sequence number and timestamp SHOULD be random (RFC 3550)
    //m_sequence = rand (); //Sequence number from 0
    m_rtpTsOffset = rand ();
    m_ssrc = rand ();

    NS_ASSERT (m_minBw <= m_initBw);
    NS_ASSERT (m_initBw <= m_maxBw);

    m_rVin = m_initBw;
    m_rSend = m_initBw;

    // Bind a socket to the SAG port
    if (m_socket == NULL) {
        TypeId factoryTid = TypeId::LookupByName(m_transport_factory_name);
        TypeId socketTid = TypeId::LookupByName(m_transport_socket_name);
        SetSocketType(factoryTid, socketTid);
        m_socket = Socket::CreateSocket(GetNode(), factoryTid);
        InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_port);
        m_destIP = Ipv4Address::GetAny();
        if (m_socket->Bind(local) == -1) {
            NS_FATAL_ERROR("Failed to bind socket");
        }
    }

    // Receive of packets
    m_socket->SetRecvCallback (MakeCallback (&SAGApplicationLayerRTPSender::HandleRead, this));

    /*
    // First process call is for the start of the first burst
    if (m_outgoing_bursts.size() > 0) {
        m_enqueueEvent = Simulator::Schedule(NanoSeconds(std::get<0>(m_outgoing_bursts[0]).GetStartTimeNs()), &SAGApplicationLayerRTPSender::EnqueuePacket, this);
        m_nextSendTstmpNs = 0;
    }
    */

    //m_enqueueEvent = Simulator::Schedule (NanoSeconds(std::get<0>(m_outgoing_bursts[0]).GetStartTimeNs()), &SAGApplicationLayerRTPSender::EnqueuePacket, this);//std::get<0>(m_outgoing_bursts[0]).GetStartTimeNs()) or Seconds (0.0)
    m_enqueueEvent = Simulator::Schedule (NanoSeconds(0), &SAGApplicationLayerRTPSender::EnqueuePacket, this);//std::get<0>(m_outgoing_bursts[0]).GetStartTimeNs()) or Seconds (0.0)

}

void
SAGApplicationLayerRTPSender::StopApplication()
{
    NS_LOG_FUNCTION(this);
    if (m_socket != 0) {
        m_socket->Close();
        m_socket->SetRecvCallback(MakeNullCallback < void, Ptr < Socket > > ());
        Simulator::Cancel(m_enqueueEvent);
        Simulator::Cancel (m_sendEvent);
        Simulator::Cancel (m_sendOversleepEvent);
        m_rateShapingBuf.clear ();
        m_rateShapingBytes = 0;
        for (EventId& eventId : m_outgoing_bursts_event_id) {
            Simulator::Cancel(eventId);
        }
    }
}

void
SAGApplicationLayerRTPSender::EnqueuePacket()
{
	NS_LOG_FUNCTION(this);

/*
    // If this function is called, there must be a next burst
    if (m_next_internal_burst_idx >= m_outgoing_bursts.size() || std::get<0>(m_outgoing_bursts[m_next_internal_burst_idx]).GetStartTimeNs() != now_ns) {
        throw std::runtime_error("No next burst available; this function should not have been called.");
    }
*/
//	if(!m_codec){
//		m_codec = std::make_shared<syncodecs::PerfectCodec> (DEFAULT_PACKET_SIZE);//PerfectCodec TraceBasedCodec HybridCodec
//	}
	if(!m_controller){
	    m_controller = std::make_shared<SagNadaController> ();//SagNadaController SagDummyController
	}

	// Trace-based video source
	//SetCodecType(SYNCODEC_TYPE_TRACE);
    //syncodecs::TraceBasedCodecWithScaling* innerCodec = new syncodecs::TraceBasedCodecWithScaling(
    //                  "/home/zanxin99/eclipse-workspace/SAG_Platform/simulator/contrib/communication-protocal/model/sag_rtp/syncodecs/video_traces/chat_firefox_h264",
    //                  "chat", 30., true);
    //syncodecs::Codec* m_codec = new syncodecs::ShapedPacketizer(innerCodec, DEFAULT_PACKET_SIZE, 0);//IPV4_UDP_OVERHEAD);

	syncodecs::Codec& codec = *m_codec;
	//m_rVin=10e6; //delete later, target rate
	codec.setTargetRate (m_rVin);//m_rVin
	++codec; // Advance codec/packetizer to next frame/packet
	const auto bytesToSend = codec->first.size ();
	//std::cout<<m_rVin<<std::endl;
	NS_ASSERT (bytesToSend > 0);
	NS_ASSERT (bytesToSend <= DEFAULT_PACKET_SIZE);

    m_rateShapingBuf.push_back (bytesToSend);
    m_rateShapingBytes += bytesToSend;

    NS_LOG_INFO ("SAGApplicationLayerRTPSender::EnqueuePacket, packet enqueued, packet length: " << bytesToSend
                 << ", buffer size: " << m_rateShapingBuf.size ()
                 << ", buffer bytes: " << m_rateShapingBytes);

    double secsToNextEnqPacket = codec->second;
	//std::cout<<"SecsToNext: "<<codec->second<<std::endl;
    Time tNext{Seconds (secsToNextEnqPacket)};

    // ++codec; // Advance codec/packetizer to next frame/packet
    //m_enqueueEvent = Simulator::Schedule (tNext, &SAGApplicationLayerRTPSender::EnqueuePacket, this);
	uint64_t now_ns = Simulator::Now().GetNanoSeconds();
	auto info = std::get<0>(m_outgoing_bursts[m_next_internal_burst_idx]);
	if (tNext.GetNanoSeconds() + now_ns < (uint64_t) (info.GetStartTimeNs() + info.GetDurationNs())) {
		m_enqueueEvent = Simulator::Schedule (tNext, &SAGApplicationLayerRTPSender::EnqueuePacket, this);
	}
    //std::cout<<tNext.GetNanoSeconds() + now_ns<<std::endl;

    m_outgoing_bursts_packets_sent_counter[m_next_internal_burst_idx] += 1;//internal_burst_idx
    m_outgoing_bursts_packets_size_sent_counter[m_next_internal_burst_idx] += bytesToSend;


    // Schedule the start of the next burst if there are more
 /*
    m_next_internal_burst_idx += 1;
    if (m_next_internal_burst_idx < m_outgoing_bursts.size()) {
        m_enqueueEvent = Simulator::Schedule(NanoSeconds(std::get<0>(m_outgoing_bursts[m_next_internal_burst_idx]).GetStartTimeNs() - now_ns), &SAGApplicationLayerRTPSender::EnqueuePacket, this);
    }
*/
    if (!USE_BUFFER) {
        m_sendEvent = Simulator::ScheduleNow (&SAGApplicationLayerRTPSender::SendPacket, this,
                                              secsToNextEnqPacket);
        return;
    }

    if (m_rateShapingBuf.size () == 1) {
        // Buffer was empty
        const uint64_t now_ns = Simulator::Now ().GetNanoSeconds ();
        const uint64_t nsToNextSentPacket = now_ns < m_nextSendTstmpNs ?
                                                    m_nextSendTstmpNs - now_ns : 0;
        NS_LOG_INFO ("(Re-)starting the send timer: now_ns " << now_ns
                     << ", bytesToSend " << bytesToSend
                     << ", nsToNextSentPacket " << nsToNextSentPacket
                     << ", m_rSend " << m_rSend
                     << ", m_rVin " << m_rVin
                     << ", secsToNextEnqPacket " << secsToNextEnqPacket);

        Time tNext{NanoSeconds (nsToNextSentPacket)};
        //std::cout<<"tNext: "<<tNext<<std::endl;
        m_sendEvent = Simulator::Schedule (tNext, &SAGApplicationLayerRTPSender::SendPacket, this, nsToNextSentPacket);
    }
}

void
SAGApplicationLayerRTPSender::SendPacket(uint64_t nsSlept)
{
    NS_LOG_FUNCTION(this << nsSlept);
    NS_ASSERT (m_rateShapingBuf.size () > 0);
    NS_ASSERT (m_rateShapingBytes < MAX_QUEUE_SIZE_SANITY);

    const auto bytesToSend = m_rateShapingBuf.front ();
    NS_ASSERT (bytesToSend > 0);
    NS_ASSERT (bytesToSend <= DEFAULT_PACKET_SIZE);
    m_rateShapingBuf.pop_front ();
    NS_ASSERT (m_rateShapingBytes >= bytesToSend);
    m_rateShapingBytes -= bytesToSend;

    NS_LOG_INFO ("SAGApplicationLayerRTPSender::SendPacket, packet dequeued, packet length: " << bytesToSend
                 << ", buffer size: " << m_rateShapingBuf.size ()
                 << ", buffer bytes: " << m_rateShapingBytes);

    // Synthetic oversleep: random uniform [0% .. 1%]
    uint64_t oversleepUs = nsSlept * (rand () % 100) / 10000; //unit
    Time tOver{NanoSeconds (oversleepUs)};
    //std::cout<<"tOver: "<<tOver<<std::endl;
    m_sendOversleepEvent = Simulator::Schedule (tOver, &SAGApplicationLayerRTPSender::SendOverSleep, this, bytesToSend);

    // schedule next sendData
    //std::cout<<"m_rSend: "<<  m_rSend <<std::endl;
    //std::cout<<"bytesToSend: "<<bytesToSend<<std::endl;
    const double usToNextSentPacketD = double (bytesToSend) / m_rSend * 8. * 1000. * 1000. * 1000.;
    //std::cout<< "usToNextSentPacketD: "<< usToNextSentPacketD <<std::endl;
    const uint64_t usToNextSentPacket = uint64_t (usToNextSentPacketD);
    //std::cout<< "usToNextSentPacket: "<< usToNextSentPacket <<std::endl;

    if (!USE_BUFFER || m_rateShapingBuf.size () == 0) {
        // Buffer became empty
        const auto now_ns = Simulator::Now ().GetNanoSeconds ();
        m_nextSendTstmpNs = now_ns + usToNextSentPacket;
        return;
    }

    Time tNext{NanoSeconds (usToNextSentPacket)};
    //std::cout<<"tNext: "<<tNext<<std::endl;
    m_sendEvent = Simulator::Schedule (tNext, &SAGApplicationLayerRTPSender::SendPacket, this, usToNextSentPacket);

/*
	//DoSomeThingBeforeStartApplication();

    // Send out the packet as desired
    TransmitFullPacket(bytesToSend);

    // Schedule the next if the packet gap would not exceed the rate
    uint64_t now_ns = Simulator::Now().GetNanoSeconds();
    SAGBurstInfoRtp info = std::get<0>(m_outgoing_bursts[internal_burst_idx]);
    uint64_t packet_gap_nanoseconds = std::ceil(1500.0 / (info.GetTargetRateMegabitPerSec() / 8000.0));
    if (now_ns + packet_gap_nanoseconds < (uint64_t) (info.GetStartTimeNs() + info.GetDurationNs())) {
        m_outgoing_bursts_event_id.at(internal_burst_idx) = Simulator::Schedule(NanoSeconds(packet_gap_nanoseconds), &SAGApplicationLayerRTPSender::SendPacket, this, internal_burst_idx);
    }
*/
}

void
SAGApplicationLayerRTPSender::SendOverSleep(uint32_t bytesToSend)
{
    NS_LOG_FUNCTION(this);
    const auto now_ns = Simulator::Now ().GetNanoSeconds ();

    SetDir(m_baseLogsDir);
    m_controller->processSendPacket (now_ns, m_sequence, bytesToSend);
    //std::cout << "Sender: sequence:" << m_sequence <<std::endl;

    // outgoingIdSeq with (burst_id, seq_no)
    ns3::SagRtpHeader outgoingIdSeq{96}; //96:Dynamic payload type, according to RFC 3551
    IdSeqTag idtag;
    idtag.SetId(std::get<0>(m_outgoing_bursts[m_next_internal_burst_idx]).GetBurstId());

    //outgoingIdSeq.SetId(std::get<0>(m_outgoing_bursts[m_next_internal_burst_idx]).GetBurstId());//internal_burst_idx

    outgoingIdSeq.SetSequence(m_sequence++);//internal_burst_idx

    //std::cout << "m_next_internal_burst_idx" << m_next_internal_burst_idx
    //		  << "Sender: sequence:" << m_outgoing_bursts_packets_sent_counter[m_next_internal_burst_idx] << std::endl;

    NS_ASSERT (now_ns >= 0);
    // Most video payload types in RFC 3551, Table 5, use a 90 KHz clock
    // Therefore, assuming 90 KHz clock for RTP timestamps
    outgoingIdSeq.SetTimestamp (m_rtpTsOffset + uint32_t (now_ns * 90 / 1000 / 1000));;
    outgoingIdSeq.SetSsrc (m_ssrc);

//    // One more packet will be sent out
//    m_outgoing_bursts_packets_sent_counter[m_next_internal_burst_idx] += 1;//internal_burst_idx
//    m_outgoing_bursts_packets_size_sent_counter[m_next_internal_burst_idx] += bytesToSend;

    // A full payload packet
    Ptr<Packet> packet = Create<Packet>(bytesToSend);
    packet->AddHeader(outgoingIdSeq);
    // Send out the packet to the target address
    NS_LOG_INFO ("SAGApplicationLayerRTPSender::SendOverSleep, " << packet->ToString ());

    // Tag for trace
    if (m_outgoing_bursts_enable_precise_logging[m_next_internal_burst_idx]){
        RouteTraceTag rtTrTag;
        packet->AddPacketTag(rtTrTag);

        DelayTraceTag delayTag(Simulator::Now().GetNanoSeconds());
        packet->AddPacketTag(delayTag);
    }
    packet->AddPacketTag(idtag);


    // first up interface
    Ptr<Node> dstNode = std::get<1>(m_outgoing_bursts[m_next_internal_burst_idx]);
    InetSocketAddress adr(m_dstPort);
	for(uint32_t ifNum = 1; ifNum < dstNode->GetObject<Ipv4>()->GetNInterfaces(); ifNum++){
		if(dstNode->GetObject<Ipv4>()->IsUp(ifNum)){
			adr.SetIpv4(dstNode->GetObject<Ipv4>()->GetAddress(ifNum,0).GetLocal());
			break;
		}
	}

	 m_socket->SendTo(packet, 0, adr);
    //m_socket->SendTo(packet, 0, std::get<1>(m_outgoing_bursts[m_next_internal_burst_idx]));//internal_burst_idx
}



void
SAGApplicationLayerRTPSender::HandleRead(Ptr <Socket> socket)
{
	const uint64_t now_ns = Simulator::Now ().GetNanoSeconds ();
	//std::cout<<"time: "<< now_ns<<std::endl;
    NS_LOG_FUNCTION(this << socket);
    Ptr <Packet> packet;
    Address from;
    while ((packet = socket->RecvFrom(from))) {
        //auto rIPAddress = InetSocketAddress::ConvertFrom (from).GetIpv4 ();
        auto rport = InetSocketAddress::ConvertFrom (from).GetPort ();
        //NS_ASSERT (rIPAddress == m_destIP);
        NS_ASSERT (rport == m_dstPort);

    	//get the feedback header
    	//const uint64_t now_ns= Simulator::Now ().GetNanoSeconds ();

        // Extract burst identifier and packet sequence number
        CCFeedbackHeader incomingheader{};
        NS_LOG_INFO ("SAGApplicationLayerRTPSender::HandleRead, " << packet->ToString ());
        packet->RemoveHeader (incomingheader);

        // Extract route relay nodes
        //RouteTraceTag rtTrTag;
        //packet->PeekPacketTag(rtTrTag);

        std::set<uint32_t> ssrcList{};
        incomingheader.GetSsrcList (ssrcList);
        if (ssrcList.count (m_ssrc) == 0) {
            NS_LOG_INFO ("SAGApplicationLayerRTPSender::Received Feedback packet with no data for SSRC " << m_ssrc);
            CalcBufferParams (now_ns);
            return;
        }
        std::vector<std::pair<uint16_t,
                              CCFeedbackHeader::MetricBlock> > feedback{};
        const bool res = incomingheader.GetMetricList (m_ssrc, feedback);
        NS_ASSERT (res);
        std::vector<ns3::SagRtpController::FeedbackItem> fbBatch{};
        for (auto& item : feedback) {
            const ns3::SagRtpController::FeedbackItem fbItem{
                .sequence = item.first,
                .rxTimestampUs = item.second.m_timestampUs,
                .ecn = item.second.m_ecn
            };
            fbBatch.push_back (fbItem);
        }
        m_controller->processFeedbackBatch (now_ns, fbBatch);
        CalcBufferParams (now_ns);
    }

}


void
SAGApplicationLayerRTPSender::CalcBufferParams (uint64_t now_ns)
{
    NS_LOG_FUNCTION(this);
    //Calculate rate shaping buffer parameters
    const auto r_ref = m_controller->getBandwidth (now_ns); // bandwidth in bps
    float bufferLen;
    //Purpose: smooth out timing issues between send and receive
    // feedback for the common case: buffer oscillating between 0 and 1 packets
    if (m_rateShapingBuf.size () > 1) {
        bufferLen = static_cast<float> (m_rateShapingBytes);
    } else {
        bufferLen = 0;
    }

    syncodecs::Codec& codec = *m_codec;

    // TODO (deferred): encapsulate rate shaping buffer in a separate class
    if (USE_BUFFER && static_cast<bool> (codec)) {
        float r_diff = 8. * bufferLen * m_fps;
        float r_diff_v = std::min<float>(BETA_V*r_diff, r_ref*0.05);  // limit change to 5% of reference rate
        float r_diff_s = std::min<float>(BETA_S*r_diff, r_ref*0.05);  // limit change to 5% of reference rate
        m_rVin = std::max<float> (m_minBw, r_ref - r_diff_v);
        m_rSend = std::min<float>(m_maxBw, r_ref + r_diff_s);

        NS_LOG_INFO ("New rate shaping buffer parameters: r_ref " << r_ref/1000/1000. // in Mbps
                     << ", m_rVin " << m_rVin/1000/1000.
                     << ", m_rSend " << m_rSend/1000/1000.
                     << ", fps " << m_fps
                     << ", buffer length " << bufferLen);  // in Bytes
    } else {
        m_rVin = r_ref;
        m_rSend = r_ref;
    }
}

std::vector<std::tuple<SAGBurstInfoRtp, uint64_t>>
SAGApplicationLayerRTPSender::GetOutgoingBurstsInformation()
{
    std::vector<std::tuple<SAGBurstInfoRtp, uint64_t>> result;
    for (size_t i = 0; i < m_outgoing_bursts.size(); i++) {
        result.push_back(std::make_tuple(std::get<0>(m_outgoing_bursts[i]), m_outgoing_bursts_packets_sent_counter[i]));
    }
    return result;
}

uint64_t
SAGApplicationLayerRTPSender::GetSentCounterOf(int64_t burst_id)
{
    std::vector<std::tuple<SAGBurstInfoRtp, uint64_t>> result;
    for (size_t i = 0; i < m_outgoing_bursts.size(); i++) {
        if (std::get<0>(m_outgoing_bursts[i]).GetBurstId() == burst_id) {
            return m_outgoing_bursts_packets_sent_counter[i];
        }
    }
    throw std::runtime_error("Sent counter for unknown burst ID was requested");
}

uint64_t
SAGApplicationLayerRTPSender::GetSentSizeCounterOf(int64_t burst_id){
    std::vector<std::tuple<SAGBurstInfoRtp, uint64_t>> result;
    for (size_t i = 0; i < m_outgoing_bursts.size(); i++) {
        if (std::get<0>(m_outgoing_bursts[i]).GetBurstId() == burst_id) {
            return m_outgoing_bursts_packets_size_sent_counter[i];
        }
    }
    throw std::runtime_error("Sent counter for unknown burst ID was requested");

}


} // Namespace ns3
