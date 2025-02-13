/*
 * Copyright (c) 2023 NJU
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Xiaoyu Liu <xyliu0119@163.com>
 */

#include "ns3/sag_application_layer.h"

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
#include "ns3/route_trace_tag.h"
#include "ns3/delay_trace_tag.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SAGApplicationLayer");

NS_OBJECT_ENSURE_REGISTERED (SAGApplicationLayer);

TypeId
SAGApplicationLayer::GetTypeId(void) 
{
    static TypeId tid = TypeId("ns3::SAGApplicationLayer")
            .SetParent<Application>()
            .SetGroupName("Applications")
            .AddConstructor<SAGApplicationLayer>()
            .AddAttribute ("BaseLogsDir",
                            "Base logging directory (logging will be placed here, i.e. logs_dir/burst_[burst id]_{incoming, outgoing}.csv",
                            StringValue (""),
                            MakeStringAccessor (&SAGApplicationLayer::m_baseLogsDir),
                            MakeStringChecker ())
			.AddAttribute ("BaseDir",
					        "Directory (trace file will be placed here",
				            StringValue (""),
				            MakeStringAccessor (&SAGApplicationLayer::m_baseDir),
				            MakeStringChecker ())
			.AddAttribute("EnableFlowLoggingToFile",
						  "True iff you want to track some aspects of this flow over time.",
						  BooleanValue(false),
						  MakeBooleanAccessor(&SAGApplicationLayer::m_enableDetailedLogging),
						  MakeBooleanChecker())
			.AddAttribute("MaxPayloadSizeByte", "Total  payload size (byte) before it gets fragmented.",
							UintegerValue(1472), // 1500 (point-to-point default) - 20 (IP) - 8 (UDP) = 1472
							MakeUintegerAccessor(&SAGApplicationLayer::m_max_payload_size_byte),
							MakeUintegerChecker<uint32_t>());
    return tid;
}

SAGApplicationLayer::SAGApplicationLayer() 
    :m_socket(0),
    m_isIPv4Networking(false),
    m_isIPv6Networking(false),
	m_fps{30.}
{
    NS_LOG_FUNCTION(this);
}

SAGApplicationLayer::~SAGApplicationLayer() 
{
    NS_LOG_FUNCTION(this);
    m_socket = 0;
}

void 
SAGApplicationLayer::SetBasicSimuAttr(Ptr<BasicSimulation> basicSimu)
{
    NS_LOG_FUNCTION(this);
    m_dynamicStateUpdateIntervalNs = parse_positive_int64(basicSimu->GetConfigParamOrFail("dynamic_state_update_interval_ns"));
    m_isIPv4Networking = parse_boolean(basicSimu->GetConfigParamOrDefault("enable_ipv4_addressing_protocol", "false"));
    m_isIPv6Networking = parse_boolean(basicSimu->GetConfigParamOrDefault("enable_ipv6_addressing_protocol", "false"));
}

void 
SAGApplicationLayer::SetTransportFactory(std::string transport_factory_name)
{
    NS_LOG_FUNCTION(this);
    m_transport_factory_name = transport_factory_name;
}

void 
SAGApplicationLayer::SetTransportSocket(std::string transport_socket_name)
{
    NS_LOG_FUNCTION(this);
    m_transport_socket_name = transport_socket_name;
}

void
SAGApplicationLayer::SetDestinationNode(Ptr<Node> dst)
{
	NS_LOG_FUNCTION(this);
	m_dstNode = dst;
}

Ptr<Node>
SAGApplicationLayer::GetDestinationNode()
{
	NS_LOG_FUNCTION(this);
	return m_dstNode;
}

void
SAGApplicationLayer::SetSourceNode(Ptr<Node> src){
	NS_LOG_FUNCTION(this);
	m_srcNode = src;
}

Ptr<Node>
SAGApplicationLayer::GetSourceNode(){
	return m_srcNode;
}

Address
SAGApplicationLayer::GetDestinationAddress()
{
	NS_LOG_FUNCTION(this);
    // first up interface
    InetSocketAddress adr(m_dstPort);
	for(uint32_t ifNum = 1; ifNum < m_dstNode->GetObject<Ipv4>()->GetNInterfaces(); ifNum++){
		if(m_dstNode->GetObject<Ipv4>()->IsUp(ifNum)){
			adr.SetIpv4(m_dstNode->GetObject<Ipv4>()->GetAddress(ifNum,0).GetLocal());
			//std::cout<<m_dstNode->GetId()<<"  "<<m_dstNode->GetObject<Ipv4>()->GetNAddresses(ifNum)<<" :::"<<m_dstNode->GetObject<Ipv4>()->GetAddress(ifNum,0).GetLocal()<<std::endl;
			break;
		}
	}
	return adr;
}

Ipv4Address
SAGApplicationLayer::GetMyAddress()
{
	NS_LOG_FUNCTION(this);
    // only for ground stations with only one interface
	if(this->GetNode()->GetObject<Ipv4>()->IsUp(1)){
		return this->GetNode()->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
	}
	return 0;

}

void
SAGApplicationLayer::NotifyAddressChange()
{

}


void
SAGApplicationLayer::DoDispose(void) 
{
    NS_LOG_FUNCTION(this);
    Application::DoDispose();
}

void
SAGApplicationLayer::SetSocketType(TypeId tid, TypeId socketTid)
{
    Ptr<SocketFactory> socketFactory = GetNode()->GetObject<SocketFactory> (tid);
    socketFactory->SetSocketType(socketTid);
}

void
SAGApplicationLayer::StartApplication(void) 
{
    NS_LOG_FUNCTION(this);

//    //DoSomeThingBeforeStartApplication();
//
//    // Bind a socket to the SAG port
//    if (m_socket == 0) {
//        //        TypeId factoryTid = TypeId::LookupByName("ns3::SAGTransportSocketFactory");
//        TypeId factoryTid = TypeId::LookupByName(m_transport_factory_name);
//        TypeId socketTid = TypeId::LookupByName(m_transport_socket_name);
//        SetSocketType(factoryTid, socketTid);
//        m_socket = Socket::CreateSocket(GetNode(), factoryTid);
//        // InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_port);
//        // if (m_socket->Bind(local) == -1) {
//        //     NS_FATAL_ERROR("Failed to bind socket");
//        // }
//    }
//
//    // Receive of packets
//    m_socket->SetRecvCallback(MakeCallback(&SAGApplicationLayer::HandleRead, this));
//
//    // First process call is for the start of the first burst
//    // if (m_outgoing_bursts.size() > 0) {
//    //     m_startNextBurstEvent = Simulator::Schedule(NanoSeconds(std::get<0>(m_outgoing_bursts[0]).GetStartTimeNs()), &SAGApplicationLayer::StartNextBurst, this);
//    // }


}



void
SAGApplicationLayer::StopApplication() 
{
    NS_LOG_FUNCTION(this);
//    if (m_socket != 0) {
//        m_socket->Close();
//        m_socket->SetRecvCallback(MakeNullCallback < void, Ptr < Socket > > ());
//        // Simulator::Cancel(m_startNextBurstEvent);
//        // for (EventId& eventId : m_outgoing_bursts_event_id) {
//        //     Simulator::Cancel(eventId);
//        // }
//    }
}

void
SAGApplicationLayer::HandleRead(Ptr <Socket> socket) 
{
    NS_LOG_FUNCTION(this << socket);
    // Ptr <Packet> packet;
    // Address from;
    // while ((packet = socket->RecvFrom(from))) {

    //     // Extract burst identifier and packet sequence number
    //     IdSeqHeader incomingIdSeq;
    //     packet->RemoveHeader (incomingIdSeq);

    //     // Count packets from incoming bursts
    //     m_incoming_bursts_received_counter.at(incomingIdSeq.GetId()) += 1;

    //     // Log precise timestamp received of the sequence packet if needed
    //     if (m_incoming_bursts_enable_precise_logging[incomingIdSeq.GetId()]) {
    //         std::ofstream ofs;
    //         ofs.open(m_baseLogsDir + "/" + format_string("burst_%" PRIu64 "_incoming.csv", incomingIdSeq.GetId()), std::ofstream::out | std::ofstream::app);
    //         ofs << incomingIdSeq.GetId() << "," << incomingIdSeq.GetSeq() << "," << Simulator::Now().GetNanoSeconds() << std::endl;
    //         ofs.close();
    //     }

    //     // Application layer do something when receive pkt
    //     DoSomeThingWhenReceivePkt(packet);

    // }
}



void
SAGApplicationLayer::SetTrace (TraceType traceType)
{
	//std::cout<<"Dir"<<m_baseLogsDir<<std::endl;
    NS_LOG_FUNCTION(this);
    if (traceType==TRACE_TYPE_CHAT)
    	{
    	    m_traceDir=m_baseDir+"/syncodecs/video_traces/chat_firefox_h264";
            m_filePrefix = "chat";
    	}
    else if (traceType==TRACE_TYPE_BBB)
    	{
    		m_traceDir=m_baseDir+"/syncodecs/video_traces/big_buck_bunny";
            m_filePrefix = "bbb_offset1900_1080p24fps";
    	}
    else if (traceType==TRACE_TYPE_CONCAT)
    	{
            m_traceDir=m_baseDir+"/syncodecs/video_traces/Concat";
            m_filePrefix = "Concat_ProRes";
    	}
    else if (traceType==TRACE_TYPE_ED)
    	{
            m_traceDir=m_baseDir+"/syncodecs/video_traces/elephants_dream";
            m_filePrefix = "ed_offset3900_1080p24fps";
    	}
    else if (traceType==TRACE_TYPE_FOREMAN)
    {
            m_traceDir=m_baseDir+"/syncodecs/video_traces/Foreman_lookahead_1";
            m_filePrefix = "Foreman_ProRes";
    	}
    else if (traceType==TRACE_TYPE_NEWS)
    	{
            m_traceDir=m_baseDir+"/syncodecs/video_traces/News_lookahead_1";
            m_filePrefix = "News_ProRes";
    	}
    else
    	{
    	    m_traceDir=m_baseLogsDir+"/syncodecs/video_traces/chat_firefox_h264";
            m_filePrefix = "chat";
    	}
}

// TODO (deferred): allow flexible input of video traffic trace path via config file, etc.
void
SAGApplicationLayer::SetCodecType (SyncodecType codecType)
{
    NS_LOG_FUNCTION(this);
    syncodecs::Codec* codec = NULL;
    switch (codecType) {
        case SYNCODEC_TYPE_PERFECT:
        {
            codec = new syncodecs::PerfectCodec{m_max_payload_size_byte};
            break;
        }
        case SYNCODEC_TYPE_FIXFPS:
        {
            m_fps = SYNCODEC_DEFAULT_FPS;
            auto innerCodec = new syncodecs::SimpleFpsBasedCodec{m_fps};
            codec = new syncodecs::ShapedPacketizer{innerCodec, m_max_payload_size_byte};
            break;
        }
        case SYNCODEC_TYPE_STATS:
        {
            m_fps = SYNCODEC_DEFAULT_FPS;
            auto innerStCodec = new syncodecs::StatisticsCodec{m_fps};
            codec = new syncodecs::ShapedPacketizer{innerStCodec, m_max_payload_size_byte};
            break;
        }
        case SYNCODEC_TYPE_TRACE:
        {
//			codec = new syncodecs::TraceBasedCodec(m_traceDir, m_filePrefix, 25.);
//			break;

        	m_fps = SYNCODEC_DEFAULT_FPS;
            auto innerCodec = new syncodecs::TraceBasedCodec(m_traceDir, m_filePrefix, m_fps);
            codec = new syncodecs::ShapedPacketizer{innerCodec, m_max_payload_size_byte};
            break;
        }
        case SYNCODEC_TYPE_HYBRID:
        {
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
            NS_ASSERT_MSG (!m_traceDir.empty (), "Traces file not found in candidate paths");

            auto innerCodec = (codecType == SYNCODEC_TYPE_TRACE) ?
                                 new syncodecs::TraceBasedCodecWithScaling{
                                    m_traceDir,        // path to traces directory
                                    m_filePrefix,      // video filename
                                    SYNCODEC_DEFAULT_FPS,             // Default FPS: 30fps
                                    true} :          // fixed mode: image resolution doesn't change
                                 new syncodecs::HybridCodec{
                                    m_traceDir,        // path to traces directory
                                    m_filePrefix,      // video filename
                                    SYNCODEC_DEFAULT_FPS,             // Default FPS: 30fps
                                    true};           // fixed mode: image resolution doesn't change
	        m_fps = SYNCODEC_DEFAULT_FPS;
            codec = new syncodecs::ShapedPacketizer{innerCodec, m_max_payload_size_byte};
            break;
        }
        case SYNCODEC_TYPE_SHARING:
        {
            auto innerShCodec = new syncodecs::SimpleContentSharingCodec{};
            codec = new syncodecs::ShapedPacketizer{innerShCodec, m_max_payload_size_byte};
            break;
        }
        default:  // defaults to perfect codec
            codec = new syncodecs::PerfectCodec{DEFAULT_PACKET_SIZE};
    }

    // update member variable
    m_codec = std::shared_ptr<syncodecs::Codec>{codec};
}

void
SAGApplicationLayer::RecordDetailsLog(Ptr<Packet> pkt){

	if(m_totalRxPacketNumber % 100 != 1){
		return;
	}
	m_recordTimeStampLog_us.push_back(Simulator::Now().GetMicroSeconds());

    // Extract route relay nodes
    RouteTraceTag rtTrTag;
    NS_ASSERT_MSG(pkt->PeekPacketTag(rtTrTag), "No Route Trace Packet Tag");

    DelayTraceTag delayTag;
    NS_ASSERT_MSG(pkt->PeekPacketTag(delayTag), "No Delay Trace Packet Tag");

	// route log
	std::vector<uint32_t> route = rtTrTag.GetRouteTrace();
	RecordRouteDetailsLog(route);

	// delay log
	double delay_us = ((uint64_t)Simulator::Now().GetNanoSeconds() - delayTag.GetStartTime()) / 1e3;
	m_delaymsDetailsTimeStampLog_us.push_back(delay_us);
	if(m_minDelay_us > delay_us){
		m_minDelay_us = delay_us;
	}
	if(m_maxDelay_us < delay_us){
		m_maxDelay_us = delay_us;
	}

	// rate log
	m_pktSizeBytes.push_back(m_totalRxBytes);

}

void
SAGApplicationLayer::RecordDetailsLogRouteOnly(Ptr<Packet> pkt){

	if(m_totalRxPacketNumber % 100 != 1){
		return;
	}

    // Extract route relay nodes
    RouteTraceTag rtTrTag;
    //NS_ASSERT_MSG(pkt->PeekPacketTag(rtTrTag), "No Route Trace Packet Tag");
    if(!pkt->PeekPacketTag(rtTrTag)){
    	return;
    }

	m_recordTimeStampLog_us.push_back(Simulator::Now().GetMicroSeconds());


	// route log
	std::vector<uint32_t> route = rtTrTag.GetRouteTrace();
	RecordRouteDetailsLog(route);

}

void
SAGApplicationLayer::RecordRouteDetailsLog(std::vector<uint32_t> route){

	if(m_routeDetailsLog.empty()){
		m_routeDetailsLog.push_back(route);
		m_routeDetailsTimeStampLog_us.push_back(Simulator::Now().GetMicroSeconds());
	}
	else{
		std::vector<uint32_t> route_last = m_routeDetailsLog[m_routeDetailsLog.size() - 1];
		if(route_last != route){
			m_routeDetailsLog.push_back(route);
			m_routeDetailsTimeStampLog_us.push_back(Simulator::Now().GetMicroSeconds());
		}
	}

}



} // Namespace ns3
