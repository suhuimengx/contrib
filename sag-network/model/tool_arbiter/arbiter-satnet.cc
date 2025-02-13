/*
 * Copyright (c) 2020 ETH Zurich
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
 */

#include "ns3/arbiter-satnet.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ArbiterSatnet");

NS_OBJECT_ENSURE_REGISTERED (ArbiterSatnet);
TypeId ArbiterSatnet::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::ArbiterSatnet")
            .SetParent<Arbiter> ()
            .SetGroupName("BasicSim")
    ;
    return tid;
}

ArbiterSatnet::ArbiterSatnet()
{
    
}

ArbiterSatnet::ArbiterSatnet(
        Ptr<Node> this_node,
        NodeContainer nodes)
{
    DoInitialize(this_node, nodes);
}

void ArbiterSatnet::DoInitialize(
    Ptr<Node> this_node,
    NodeContainer nodes)
{
    m_node_id = this_node->GetId();
    m_nodes = nodes;

//    // Store IP address to node id (each interface has an IP address, so multiple IPs per node)
//    for (uint32_t i = 0; i < m_nodes.GetN(); i++) {
//        for (uint32_t j = 1; j < m_nodes.Get(i)->GetObject<Ipv4>()->GetNInterfaces(); j++) {
//            m_ip_to_node_id.insert({m_nodes.Get(i)->GetObject<Ipv4>()->GetAddress(j, 0).GetLocal().Get(), i});
//        }
//
//        // for (uint32_t j = 1; j < m_nodes.Get(i)->GetObject<Ipv6>()->GetNInterfaces(); j++) {
//        //     uint8_t ipv6Buf[16] = {0};
//        //     m_nodes.Get(i)->GetObject<Ipv6>()->GetAddress(j, 1).GetAddress().GetBytes(ipv6Buf); //address 0 for multicast
//        //     std::pair<IPv6AddressBuf, uint32_t> entry = std::make_pair(ipv6Buf, i);
//        //     m_ipv6_to_node_id.emplace(entry);
//        // }
//        for (uint32_t j = 1; j < m_nodes.Get(i)->GetObject<Ipv6>()->GetNInterfaces(); j++) {
//        	uint32_t nadrs = m_nodes.Get(i)->GetObject<Ipv6>()->GetNAddresses(j);
//        	for(uint32_t k = 1; k < nadrs; k++){
//        		// if(j == 0 && k == 0){
//        		// 	continue;
//        		// }
//                uint8_t ipv6Buf[16] = {0};
//                m_nodes.Get(i)->GetObject<Ipv6>()->GetAddress(j, k).GetAddress().GetBytes(ipv6Buf); //address 0 for multicast
//                std::pair<IPv6AddressBuf, uint32_t> entry = std::make_pair(ipv6Buf, i);
//                m_ipv6_to_node_id.emplace(entry);
//        	}
//        }
//    }
}

void ArbiterSatnet::SetIpAddresstoNodeId(std::unordered_map<uint32_t, uint32_t>& Ip2NodeId){
	m_ip_to_node_id.clear();
    m_ip_to_node_id = Ip2NodeId;
}

uint32_t ArbiterSatnet::ResolveNodeIdFromIp(uint32_t ip) 
{
    std::unordered_map<uint32_t, uint32_t>::iterator ip_to_node_id_iter = m_ip_to_node_id.find(ip);
    if (ip_to_node_id_iter != m_ip_to_node_id.end()) {
        return ip_to_node_id_iter->second;
    }
    else {
       NS_LOG_LOGIC ("IP address " << Ipv4Address(ip)  << " (" << ip << ") is not mapped to a node id at node " << m_node_id);
//        throw std::invalid_argument(res.str());
        return UINT32_MAX;
    }
}

ArbiterResultIPv4 ArbiterSatnet::BaseDecide(Ptr<const Packet> pkt, Ipv4Header const &ipHeader) 
{

    // Retrieve the source node id
    uint32_t source_ip = ipHeader.GetSource().Get();
    uint32_t source_node_id;

    // Ipv4Address default constructor has IP 0x66666666 = 102.102.102.102 = 1717986918,
    // which is set by TcpSocketBase::SetupEndpoint to discover its actually source IP.
    bool is_socket_request_for_source_ip = source_ip == 1717986918;

    // If it is a request for source IP, the source node id is just the current node.
    // if (is_socket_request_for_source_ip) {
    //     source_node_id = m_node_id;
    // } else {
    //     source_node_id = ResolveNodeIdFromIp(source_ip);
    // }
    source_node_id = 0; //not used
    // if(source_node_id == UINT32_MAX)
    // 	return ArbiterResult(true, 0, 0); // Failed = no route (means either drop, or socket fails)

    uint32_t target_node_id = ResolveNodeIdFromIp(ipHeader.GetDestination().Get());
    if(target_node_id == UINT32_MAX){
        NS_LOG_LOGIC ("Not Found target_node_id for " << ipHeader.GetDestination() << " at node " << m_node_id);
        return ArbiterResultIPv4(true, 0, 0);
    }

    // Decide the next node
    return Decide(
                source_node_id,
                target_node_id,
                pkt,
                ipHeader,
                is_socket_request_for_source_ip
    );

}

ArbiterResultIPv4 ArbiterSatnet::Decide(
        int32_t source_node_id,
        int32_t target_node_id,
        ns3::Ptr<const ns3::Packet> pkt,
        ns3::Ipv4Header const &ipHeader,
        bool is_socket_request_for_source_ip) 
{

    // Decide the next node
    std::tuple<int32_t, int32_t, int32_t> next_node_id_my_if_next_if = TopologySatelliteNetworkDecide(
                source_node_id,
                target_node_id,
                pkt,
                ipHeader,
                is_socket_request_for_source_ip
    );

    // Retrieve the components
    int32_t next_node_id = std::get<0>(next_node_id_my_if_next_if);
    int32_t own_if_id = std::get<1>(next_node_id_my_if_next_if);
    int32_t next_if_id = std::get<2>(next_node_id_my_if_next_if);

    // If the result is invalid
    // If invalid, feedback no route, and then check other routing protocols
    //NS_ABORT_MSG_IF(next_node_id == -2 || own_if_id == -2 || next_if_id == -2, "Forwarding state is not set for this node to this target node (invalid).");

    // Check whether it is a drop or not
    uint32_t select_ip_gateway = 0;
    if (next_node_id != -2) {

        // Retrieve the IP gateway
    	auto x=m_nodes.Get(next_node_id)->GetObject<Ipv4>();
    	select_ip_gateway = x->GetAddress(next_if_id, 0).GetLocal().Get();
        //uint32_t select_ip_gateway = m_nodes.Get(next_node_id)->GetObject<Ipv4>()->GetAddress(next_if_id, 0).GetLocal().Get();

        // We succeeded in finding the interface and gateway to the next hop
        return ArbiterResultIPv4(false, own_if_id, select_ip_gateway);

    } else {
        return ArbiterResultIPv4(true, 0, select_ip_gateway); // Failed = no route (means either drop, or socket fails)
    }

}

void ArbiterSatnet::SetIpv6AddresstoNodeId(std::map<IPv6AddressBuf, uint32_t, IPv6AddressBufComparator>& Ipv62NodeId){
	m_ipv6_to_node_id.clear();
    m_ipv6_to_node_id = Ipv62NodeId;
}

uint32_t ArbiterSatnet::ResolveNodeIdFromIp(uint8_t ipv6[16]) 
{
    // std::array<uint8_t, 16> ipv6Key;
    // std::memcpy(ipv6Key.data(), ipv6, 16);
    // const std::array<unsigned char, 16>& ipv6Key = *reinterpret_cast<std::array<unsigned char, 16>*>(ipv6);
    IPv6AddressBuf ipv6Key(ipv6);

    auto ipv6_to_node_id_iter = m_ipv6_to_node_id.find(ipv6Key);
    if (ipv6_to_node_id_iter != m_ipv6_to_node_id.end()) {
        return ipv6_to_node_id_iter->second;
    } else {
        NS_LOG_LOGIC ("IPv6 address " << Ipv6Address(ipv6)  << " is not mapped to a node id at node " << m_node_id);
        // throw std::invalid_argument(res.str()); //may not found when switch
        return UINT32_MAX;
    }
    return 0;
}

ArbiterResultIPv6 ArbiterSatnet::BaseDecide(Ptr<const Packet> pkt, Ipv6Header const &ipHeader) 
{

    // Retrieve the source node id
    uint8_t source_ip[16] = {0};
    ipHeader.GetSource().GetBytes(source_ip);
    uint32_t source_node_id;

    // source_node_id = ResolveNodeIdFromIp(source_ip);
    source_node_id = 0; //no used

    uint8_t destination_ip[16] = {0};
    ipHeader.GetDestination().GetBytes(destination_ip);
    uint32_t target_node_id = ResolveNodeIdFromIp(destination_ip);
    if(target_node_id == UINT32_MAX){
        NS_LOG_LOGIC ("Not Found target_node_id for " << ipHeader.GetDestination() << " at node " << m_node_id);
        uint8_t select_ip_gateway[16] = {0};
        return ArbiterResultIPv6(true, 0, select_ip_gateway);
    }


    // Decide the next node
    return Decide(
                source_node_id,
                target_node_id,
                pkt,
                ipHeader,
                false //is_socket_request_for_source_ip
    );

}

ArbiterResultIPv6 ArbiterSatnet::Decide(
        int32_t source_node_id,
        int32_t target_node_id,
        ns3::Ptr<const ns3::Packet> pkt,
        ns3::Ipv6Header const &ipHeader,
        bool is_socket_request_for_source_ip) 
{

    // Decide the next node
    std::tuple<int32_t, int32_t, int32_t> next_node_id_my_if_next_if = TopologySatelliteNetworkDecide(
                source_node_id,
                target_node_id,
                pkt,
                ipHeader,
                is_socket_request_for_source_ip
    );

    // Retrieve the components
    int32_t next_node_id = std::get<0>(next_node_id_my_if_next_if);
    int32_t own_if_id = std::get<1>(next_node_id_my_if_next_if);
    int32_t next_if_id = std::get<2>(next_node_id_my_if_next_if);

    // If the result is invalid
    // If invalid, feedback no route, and then check other routing protocols
    //NS_ABORT_MSG_IF(next_node_id == -2 || own_if_id == -2 || next_if_id == -2, "Forwarding state is not set for this node to this target node (invalid).");

    // Check whether it is a drop or not
    uint8_t select_ip_gateway[16] = {0};
    if (next_node_id != -2) {

        // Retrieve the IPv6 gateway
    	auto x=m_nodes.Get(next_node_id)->GetObject<Ipv6>();
    	x->GetAddress(next_if_id, 0).GetAddress().GetBytes(select_ip_gateway);

        // We succeeded in finding the interface and gateway to the next hop
        return ArbiterResultIPv6(false, own_if_id, select_ip_gateway);

    } else {
        return ArbiterResultIPv6(true, 0, select_ip_gateway); // Failed = no route (means either drop, or socket fails)
    }

}

}
