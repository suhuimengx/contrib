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

#include "ns3/sag_routing_helper_ipv6.h"
// #include "ns3/topology-satellite-network.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SAGRoutingHelperIPv6");
NS_OBJECT_ENSURE_REGISTERED (SAGRoutingHelperIPv6);

TypeId SAGRoutingHelperIPv6::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::SAGRoutingHelperIPv6")
            .SetParent<Ipv6RoutingHelper> ()
            .SetGroupName("SagRouting")
            .AddConstructor<SAGRoutingHelperIPv6>()
    ;
    return tid;
}

SAGRoutingHelperIPv6::SAGRoutingHelperIPv6 (){
	m_factory.SetTypeId ("ns3::SAGRoutingProtocalIPv6");
}

SAGRoutingHelperIPv6::SAGRoutingHelperIPv6 (const SAGRoutingHelperIPv6 &o)
{
    // Left empty intentionally
	//m_factory.SetTypeId(o.m_factory.GetTypeId());
	m_factory = o.m_factory;
    m_objectToBeInstall = o.m_objectToBeInstall;
}

SAGRoutingHelperIPv6::~SAGRoutingHelperIPv6 (){
	
}

SAGRoutingHelperIPv6* SAGRoutingHelperIPv6::Copy (void) const
{
  return new SAGRoutingHelperIPv6 (*this);
}


Ptr<Ipv6RoutingProtocol> SAGRoutingHelperIPv6::Create (Ptr<Node> node) const 
{
	//	return m_factory.Create<Ipv6RoutingProtocol> ();
  Ptr<Ipv6RoutingProtocol> agent = m_factory.Create<Ipv6RoutingProtocol> ();
  node->AggregateObject (agent);
  return agent;
}

void SAGRoutingHelperIPv6::Set (std::string name, const AttributeValue &value)
{
	m_factory.Set (name, value);
}

void SAGRoutingHelperIPv6::SetTopologyHandle(std::vector<Ptr<Constellation>> constellations){
	m_constellations = constellations;
}

void SAGRoutingHelperIPv6::InitializeArbiter(Ptr<BasicSimulation> basicSimulation, NodeContainer nodes)
{
    m_nodes = nodes;
	m_groundStation = m_constellations[0]->GetGroundStationNodes();
	for(auto cons: m_constellations){
		m_satellite.Add(cons->GetNodes());
	}
	m_basicSimulation = basicSimulation;
	//StoreIPAddresstoNodeId();

}


//void SAGRoutingHelperIPv6::SetTopologyHandle(Ptr<TopologySatelliteNetwork> topology)
//{
//    m_topology = topology;
//}

// std::vector<std::vector<std::tuple<int32_t, int32_t, int32_t>>>
// SAGRoutingHelperIPv6::InitialEmptyForwardingState() 
// {
//     std::vector<std::vector<std::tuple<int32_t, int32_t, int32_t>>> initial_forwarding_state;
//     for (size_t i = 0; i < m_nodes.GetN(); i++) {
//         std::vector <std::tuple<int32_t, int32_t, int32_t>> next_hop_list;
//         for (size_t j = 0; j < m_nodes.GetN(); j++) {
//             next_hop_list.push_back(std::make_tuple(-2, -2, -2)); // -2 indicates an invalid entry
//         }
//         initial_forwarding_state.push_back(next_hop_list);
//     }
//     return initial_forwarding_state;
// }

void SAGRoutingHelperIPv6::UpdateForwardingState(int32_t t)
{


}

void SAGRoutingHelperIPv6::UpdateRoutingTable(int32_t cur_node, int32_t dst_node, int32_t next_hop, int32_t outInterface, int32_t inInterface){

	m_arbiters.at(cur_node)->SetSingleForwardState(dst_node, next_hop, outInterface, inInterface);

}

void SAGRoutingHelperIPv6::StoreIPv6AddresstoNodeId()
{
    // Store IP address to node id (each interface has an IP address, so multiple IPs per node)
	m_ipv6_to_node_id.clear();
	for (uint32_t i = 0; i < m_nodes.GetN(); i++){ // add satellite and groundstation
    // for (uint32_t i = m_satellite.GetN(); i < m_nodes.GetN(); i++) { //only add groundstation
        for (uint32_t j = 1; j < m_nodes.Get(i)->GetObject<Ipv6>()->GetNInterfaces(); j++) {
        	uint32_t nadrs = m_nodes.Get(i)->GetObject<Ipv6>()->GetNAddresses(j);
        	for(uint32_t k = 1; k < nadrs; k++){
        		// if(j == 0 && k == 0){
        		// 	continue;
        		// }
                uint8_t ipv6Buf[16] = {0};
                m_nodes.Get(i)->GetObject<Ipv6>()->GetAddress(j, k).GetAddress().GetBytes(ipv6Buf); //address 0 for multicast
                std::pair<IPv6AddressBuf, uint32_t> entry = std::make_pair(ipv6Buf, i);
                m_ipv6_to_node_id.emplace(entry);
        	}
        }
    }
}

void SAGRoutingHelperIPv6::UpdateIpv6AddresstoNodeId(){
	StoreIPv6AddresstoNodeId();
	for(auto arbiter: m_arbiters){
		// arbiter->SetIpAddresstoNodeId(m_ip_to_node_id);
	    arbiter->SetIpv6AddresstoNodeId(m_ipv6_to_node_id);
	}


//	if(Simulator::Now()==Time(0)){
//		for(auto arbiter: m_arbiters){
//			arbiter->SetIpAddresstoNodeId(m_ip_to_node_id);
//		}
//	}

}

void SAGRoutingHelperIPv6::SetObjectNameString(std::string objectToBeInstall){
	m_objectToBeInstall = objectToBeInstall;
}

std::string SAGRoutingHelperIPv6::GetObjectNameString(){
	return m_objectToBeInstall;
}

TypeId
SAGRoutingHelperIPv6::GetObjectFactoryTypeId() const
{
    NS_LOG_FUNCTION(this);
    return m_factory.GetTypeId();
}

} // namespace ns3
