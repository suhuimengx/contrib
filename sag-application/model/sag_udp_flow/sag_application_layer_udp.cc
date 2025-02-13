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

#include "sag_application_layer_udp.h"

#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
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
#include "ns3/id_seq_tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SAGApplicationLayerUdp");

NS_OBJECT_ENSURE_REGISTERED (SAGApplicationLayerUdp);

TypeId
SAGApplicationLayerUdp::GetTypeId(void) 
{
    static TypeId tid = TypeId("ns3::SAGApplicationLayerUdp")
            .SetParent<SAGApplicationLayer>()
            .SetGroupName("Applications")
            .AddConstructor<SAGApplicationLayerUdp>()

            .AddAttribute("Port", "Port on which we listen for incoming packets.",
                            UintegerValue(9),
                            MakeUintegerAccessor(&SAGApplicationLayerUdp::m_port),
                            MakeUintegerChecker<uint16_t>());
    return tid;
}

SAGApplicationLayerUdp::SAGApplicationLayerUdp() 
{
    NS_LOG_FUNCTION(this);
    m_next_internal_burst_idx = 0;
}

SAGApplicationLayerUdp::~SAGApplicationLayerUdp() 
{
    NS_LOG_FUNCTION(this);
    m_socket = 0;
}

uint32_t 
SAGApplicationLayerUdp::GetMaxPayloadSizeByte() 
{
    return m_max_payload_size_byte;
}

void
SAGApplicationLayerUdp::RegisterOutgoingBurst(SAGBurstInfoUdp burstInfo, Ptr<Node> targetNode, uint16_t my_port_num, uint16_t port, bool enable_precise_logging)
{
    NS_ABORT_MSG_IF(burstInfo.GetFromNodeId() != this->GetNode()->GetId(), "Source node identifier is not that of this node.");
    if (m_outgoing_bursts.size() >= 1 && burstInfo.GetStartTimeNs() < std::get<0>(m_outgoing_bursts[m_outgoing_bursts.size() - 1]).GetStartTimeNs()) {
        throw std::runtime_error("Bursts must be added weakly ascending on start time");
    }
    m_dstPort = port;
    m_port = my_port_num;
    m_outgoing_bursts.push_back(std::make_tuple(burstInfo, targetNode));
    m_outgoing_bursts_packets_sent_counter.push_back(0);
    m_outgoing_bursts_packets_size_sent_counter.push_back(0);
    m_outgoing_bursts_event_id.push_back(EventId());
    m_outgoing_bursts_enable_precise_logging.push_back(enable_precise_logging);
//    if (enable_precise_logging) {
//        std::ofstream ofs;
//        ofs.open(m_baseLogsDir + "/" + format_string("burst_%" PRIu64 "_outgoing_timestamp.csv", burstInfo.GetBurstId()));
//        ofs.close();
//
//		ofs.open(m_baseLogsDir + "/" + format_string("burst_%" PRIu64 "_route_trace.csv", burstInfo.GetBurstId()));
//        ofs.close();
//
//		ofs.open(m_baseLogsDir + "/" + format_string("burst_%" PRIu64 "_delay.csv", burstInfo.GetBurstId()));
//        ofs.close();
//    }
}

void
SAGApplicationLayerUdp::RegisterIncomingBurst(SAGBurstInfoUdp burstInfo, uint16_t my_port_num, bool enable_precise_logging)
{
    NS_ABORT_MSG_IF(burstInfo.GetToNodeId() != this->GetNode()->GetId(), "Destination node identifier is not that of this node.");
    m_port = my_port_num;
    m_incoming_bursts.push_back(burstInfo);
    m_incoming_bursts_received_counter[burstInfo.GetBurstId()] = 0;
    m_incoming_bursts_size_received_counter[burstInfo.GetBurstId()] = 0;
    m_incoming_bursts_enable_precise_logging[burstInfo.GetBurstId()] = enable_precise_logging;
//    if (enable_precise_logging) {
//        std::ofstream ofs;
//        ofs.open(m_baseLogsDir + "/" + format_string("burst_%" PRIu64 "_incoming_timestamp.csv", burstInfo.GetBurstId()));
//        ofs.close();
//    }
}

void
SAGApplicationLayerUdp::DoDispose(void) 
{
    NS_LOG_FUNCTION(this);
    SAGApplicationLayer::DoDispose();
}

void
SAGApplicationLayerUdp::SetSocketType(TypeId tid, TypeId socketTid)
{
    Ptr<SocketFactory> socketFactory = GetNode()->GetObject<SocketFactory> (tid);
    socketFactory->SetSocketType(socketTid);
}

void
SAGApplicationLayerUdp::StartApplication(void) 
{
    NS_LOG_FUNCTION(this);

    //DoSomeThingBeforeStartApplication();

    // Bind a socket to the SAG port
    if (m_socket == 0) {
        TypeId factoryTid = TypeId::LookupByName(m_transport_factory_name);
        TypeId socketTid = TypeId::LookupByName(m_transport_socket_name);
        SetSocketType(factoryTid, socketTid);
        m_socket = Socket::CreateSocket(GetNode(), factoryTid);
        if(m_isIPv4Networking){
            InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_port);
            if (m_socket->Bind(local) == -1) {
                NS_FATAL_ERROR("Failed to bind socket");
            }
        }
        else if(m_isIPv6Networking){
            Inet6SocketAddress localIPv6 = Inet6SocketAddress(Ipv6Address::GetAny(), m_port);
            if (m_socket->Bind(localIPv6) == -1) {
            NS_FATAL_ERROR("Failed to bind ipv6 socket");
            }
        }
    }

    // Receive of packets
    m_socket->SetRecvCallback(MakeCallback(&SAGApplicationLayerUdp::HandleRead, this));

    // First process call is for the start of the first burst
    if (m_outgoing_bursts.size() > 0) {
        m_startNextBurstEvent = Simulator::Schedule(NanoSeconds(std::get<0>(m_outgoing_bursts[0]).GetStartTimeNs()), &SAGApplicationLayerUdp::StartNextBurst, this);
    }


}

void
SAGApplicationLayerUdp::StartNextBurst()
{

	NS_LOG_FUNCTION(this);
    int64_t now_ns = Simulator::Now().GetNanoSeconds();

    // If this function is called, there must be a next burst
    if (m_next_internal_burst_idx >= m_outgoing_bursts.size() || std::get<0>(m_outgoing_bursts[m_next_internal_burst_idx]).GetStartTimeNs() != now_ns) {
        throw std::runtime_error("No next burst available; this function should not have been called.");
    }

    // Start the self-calling (and self-ending) process of sending out packets of the burst
    BurstSendOut(m_next_internal_burst_idx);

    // Schedule the start of the next burst if there are more
    m_next_internal_burst_idx += 1;
    if (m_next_internal_burst_idx < m_outgoing_bursts.size()) {
        m_startNextBurstEvent = Simulator::Schedule(NanoSeconds(std::get<0>(m_outgoing_bursts[m_next_internal_burst_idx]).GetStartTimeNs() - now_ns), &SAGApplicationLayerUdp::StartNextBurst, this);
    }

}

void
SAGApplicationLayerUdp::BurstSendOut(size_t internal_burst_idx)
{






//	TransmitFullPacket(internal_burst_idx, 1472);
//
//    // Schedule the next if the packet gap would not exceed the rate
//    uint64_t now_ns = Simulator::Now().GetNanoSeconds();
//    SAGBurstInfoUdp info = std::get<0>(m_outgoing_bursts[internal_burst_idx]);
//    uint64_t packet_gap_nanoseconds = std::ceil(1500.0 / (info.GetTargetRateMegabitPerSec() / 8000.0));
//    if (now_ns + packet_gap_nanoseconds < (uint64_t) (info.GetStartTimeNs() + info.GetDurationNs())) {
//        m_outgoing_bursts_event_id.at(internal_burst_idx) = Simulator::Schedule(NanoSeconds(packet_gap_nanoseconds), &SAGApplicationLayerUdp::BurstSendOut, this, internal_burst_idx);
//    }



	NS_LOG_FUNCTION(this);



    syncodecs::Codec& codec = *m_codec;

    SAGBurstInfoUdp info = std::get<0>(m_outgoing_bursts[internal_burst_idx]);
    float targetRate = info.GetTargetRateMegabitPerSec()*1e6; // bps
	codec.setTargetRate (targetRate); //m_rVin
	++codec; // Advance codec/packetizer to next frame/packet
	const auto bytesToSend = codec->first.size ();
	//std::cout<<"BytesToSend: "<<codec->first.size()<<std::endl;
	NS_ASSERT (bytesToSend > 0);
	//NS_ASSERT (bytesToSend <= m_max_payload_size_byte);

    // Send out the packet as desired
    TransmitFullPacket(internal_burst_idx, bytesToSend);

	double secsToNextEnqPacket = codec->second;
	//std::cout<<"SecsToNext: "<<codec->second<<std::endl;
	Time tNext = Seconds (secsToNextEnqPacket);
	//std::cout<<"tNext: "<<tNext<<std::endl;

	uint64_t now_ns = Simulator::Now().GetNanoSeconds();
	if (tNext.GetNanoSeconds() + now_ns < (uint64_t) (info.GetStartTimeNs() + info.GetDurationNs())) {
		m_outgoing_bursts_event_id.at(internal_burst_idx) = Simulator::Schedule(tNext, &SAGApplicationLayerUdp::BurstSendOut, this, internal_burst_idx);
	}


}

void
SAGApplicationLayerUdp::TransmitFullPacket(size_t internal_burst_idx, uint32_t payload_size_byte)
{
	NS_LOG_FUNCTION(this);
//    // Header with (burst_id, seq_no)
//    IdSeqHeader idSeq;
//    idSeq.SetId(std::get<0>(m_outgoing_bursts[internal_burst_idx]).GetBurstId());
//    idSeq.SetSeq(m_outgoing_bursts_packets_sent_counter[internal_burst_idx]);

    IdSeqTag idSeq;
    idSeq.SetId(std::get<0>(m_outgoing_bursts[internal_burst_idx]).GetBurstId());
    idSeq.SetSeq(m_outgoing_bursts_packets_sent_counter[internal_burst_idx]);


    // One more packet will be sent out
    m_outgoing_bursts_packets_sent_counter[internal_burst_idx] += 1;
    m_outgoing_bursts_packets_size_sent_counter[internal_burst_idx] += payload_size_byte;
    m_totalTxBytes = m_outgoing_bursts_packets_size_sent_counter[internal_burst_idx];
    m_totalTxPacketNumber = m_outgoing_bursts_packets_sent_counter[internal_burst_idx];


//    // Log precise timestamp sent away of the sequence packet if needed
//    if (m_outgoing_bursts_enable_precise_logging[internal_burst_idx]) {
//        std::ofstream ofs;
//        ofs.open(m_baseLogsDir + "/" + format_string("burst_%" PRIu64 "_outgoing_timestamp.csv", idSeq.GetId()), std::ofstream::out | std::ofstream::app);
//        ofs << idSeq.GetId() << "," << idSeq.GetSeq() << "," << Simulator::Now().GetNanoSeconds() << std::endl;
//        ofs.close();
//    }

    // A full payload packet
    Ptr<Packet> p = Create<Packet>(payload_size_byte);

//    p->AddHeader(idSeq);
    p->AddPacketTag(idSeq);


    // Tag for trace
    if (m_outgoing_bursts_enable_precise_logging[internal_burst_idx]){
        RouteTraceTag rtTrTag;
        p->AddPacketTag(rtTrTag);

        DelayTraceTag delayTag(Simulator::Now().GetNanoSeconds());
        p->AddPacketTag(delayTag);
    }


    // first up interface
    Ptr<Node> dstNode = std::get<1>(m_outgoing_bursts[internal_burst_idx]);

    if(m_isIPv4Networking){
        InetSocketAddress adr(m_dstPort);
        for(uint32_t ifNum = 1; ifNum < dstNode->GetObject<Ipv4>()->GetNInterfaces(); ifNum++){
            if(dstNode->GetObject<Ipv4>()->IsUp(ifNum)){
//                if(Simulator::Now() > Seconds(100) && ifNum == 1){
//                	continue;
//                }
                adr.SetIpv4(dstNode->GetObject<Ipv4>()->GetAddress(ifNum,0).GetLocal());
                //std::cout<<dstNode->GetObject<Ipv4>()->GetAddress(ifNum,0).GetLocal().Get()<<std::endl;
                break;
            }
        }

        NS_LOG_LOGIC ("Send IPv4 address " << Ipv4Address(adr.GetIpv4()) << " to node " << dstNode->GetId());

        // Send out the packet to the target address
        m_socket->SendTo(p, 0, adr);
    }
    else if(m_isIPv6Networking){
        Inet6SocketAddress adr(m_dstPort);
        bool bHasAdr = false;
        for(uint32_t ifNum = 1; ifNum < dstNode->GetObject<Ipv6>()->GetNInterfaces(); ifNum++){
            if(dstNode->GetObject<Ipv6>()->IsUp(ifNum)){
                for(uint32_t adrNum = 1; adrNum < dstNode->GetObject<Ipv6>()->GetNAddresses(ifNum); adrNum++){
                    adr.SetIpv6(dstNode->GetObject<Ipv6>()->GetAddress(ifNum,adrNum).GetAddress());
                    //std::cout<<dstNode->GetObject<Ipv6>()->GetAddress(ifNum,adrNum).GetAddress().Get()<<std::endl;
                    bHasAdr = true;
                    break;
                }
            }
            if(bHasAdr) break;
        }
        // Ipv6Address dstAddress = dstNode->GetObject<Ipv6L3Protocol>()->GetAddress(1,1).GetAddress();
        // adr.SetIpv6(dstAddress);

        NS_LOG_LOGIC ("Send IPv6 address " << Ipv6Address(adr.GetIpv6()) << " to node " << dstNode->GetId());

        // Send out the packet to the target address
        m_socket->SendTo(p, 0, adr);
    }

}

void
SAGApplicationLayerUdp::StopApplication() 
{
    NS_LOG_FUNCTION(this);
    if (m_socket != 0) {
        m_socket->Close();
        m_socket->SetRecvCallback(MakeNullCallback < void, Ptr < Socket > > ());
        Simulator::Cancel(m_startNextBurstEvent);
        for (EventId& eventId : m_outgoing_bursts_event_id) {
            Simulator::Cancel(eventId);
        }
    }
}

void
SAGApplicationLayerUdp::HandleRead(Ptr <Socket> socket) 
{
    NS_LOG_FUNCTION(this << socket);
    Ptr <Packet> packet;
    Address from;
    while ((packet = socket->RecvFrom(from))) {

        // Extract burst identifier and packet sequence number
    	IdSeqTag incomingIdSeq;
    	packet->PeekPacketTag(incomingIdSeq);

//    	std::cout<<incomingIdSeq.GetId()<<"  "<<packet->GetSize ()<<std::endl;

        // Count packets from incoming bursts
        m_incoming_bursts_received_counter.at(incomingIdSeq.GetId()) += 1;
        m_incoming_bursts_size_received_counter.at(incomingIdSeq.GetId()) += packet->GetSize ();
        m_totalRxBytes = m_incoming_bursts_size_received_counter.at(incomingIdSeq.GetId());
        m_totalRxPacketNumber = m_incoming_bursts_received_counter.at(incomingIdSeq.GetId());


        // Log precise trace
		if (m_incoming_bursts_enable_precise_logging[incomingIdSeq.GetId()]) {
			RecordDetailsLog(packet);
		}

    }
}

//void
//SAGApplicationLayerUdp::DoSomeThingBeforeStartApplication(void)
//{
//    std::cout << "SAG test Application Layer DoSth Before Start " << std::endl;
//}

void
SAGApplicationLayerUdp::DoSomeThingWhenSendPkt(Ptr <Packet> packet) 
{
    //std::cout << "SAGApplicationLayerUdp:"+ std::to_string(GetNode()->GetId())+" Application Layer Send Data Packet  " << std::endl;
}

void
SAGApplicationLayerUdp::DoSomeThingWhenReceivePkt(Ptr <Packet> packet)
{
    //std::cout << "SAGApplicationLayerUdp:"+ std::to_string(GetNode()->GetId())+" Application Layer Receive Data Packet " << std::endl;

}

//InetSocketAddress SAGApplicationLayerUdp::GetTargetValidInetSocketAddress(Ptr<Node> dstNode){
//	// first up interface
//	for(uint32_t ifNum = 1; ifNum < dstNode->GetObject<Ipv4>()->GetNInterfaces(); ifNum++){
//		if(dstNode->GetObject<Ipv4>()->IsUp(ifNum)){
//			return InetSocketAddress(dstNode->GetObject<Ipv4>()->GetAddress(ifNum,0).GetLocal(), m_dstPort);
//		}
//	}
//	return nullptr;
//}

std::vector<std::tuple<SAGBurstInfoUdp, uint64_t>>
SAGApplicationLayerUdp::GetOutgoingBurstsInformation() 
{
    std::vector<std::tuple<SAGBurstInfoUdp, uint64_t>> result;
    for (size_t i = 0; i < m_outgoing_bursts.size(); i++) {
        result.push_back(std::make_tuple(std::get<0>(m_outgoing_bursts[i]), m_outgoing_bursts_packets_sent_counter[i]));
    }
    return result;
}

std::vector<std::tuple<SAGBurstInfoUdp, uint64_t>>
SAGApplicationLayerUdp::GetIncomingBurstsInformation() 
{
    std::vector<std::tuple<SAGBurstInfoUdp, uint64_t>> result;
    for (size_t i = 0; i < m_incoming_bursts.size(); i++) {
        result.push_back(std::make_tuple(m_incoming_bursts[i], m_incoming_bursts_received_counter.at(m_incoming_bursts[i].GetBurstId())));
    }
    return result;
}

uint64_t
SAGApplicationLayerUdp::GetSentCounterOf(int64_t burst_id) 
{
    std::vector<std::tuple<SAGBurstInfoUdp, uint64_t>> result;
    for (size_t i = 0; i < m_outgoing_bursts.size(); i++) {
        if (std::get<0>(m_outgoing_bursts[i]).GetBurstId() == burst_id) {
            return m_outgoing_bursts_packets_sent_counter[i];
        }
    }
    throw std::runtime_error("Sent counter for unknown burst ID was requested");
}

uint64_t
SAGApplicationLayerUdp::GetReceivedCounterOf(int64_t burst_id) 
{
    return m_incoming_bursts_received_counter.at(burst_id);
}


uint64_t
SAGApplicationLayerUdp::GetSentCounterSizeOf(int64_t burst_id){
    std::vector<std::tuple<SAGBurstInfoUdp, uint64_t>> result;
    for (size_t i = 0; i < m_outgoing_bursts.size(); i++) {
        if (std::get<0>(m_outgoing_bursts[i]).GetBurstId() == burst_id) {
            return m_outgoing_bursts_packets_size_sent_counter[i];
        }
    }
    throw std::runtime_error("Sent counter for unknown burst ID was requested");
}

uint64_t
SAGApplicationLayerUdp::GetReceivedCounterSizeOf(int64_t burst_id){
	return m_incoming_bursts_size_received_counter.at(burst_id);
}



} // Namespace ns3
