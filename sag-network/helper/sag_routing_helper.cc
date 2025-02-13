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

#include "ns3/sag_routing_helper.h"
//#include "ns3/topology-satellite-network.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SAGRoutingHelper");
NS_OBJECT_ENSURE_REGISTERED (SAGRoutingHelper);

TypeId SAGRoutingHelper::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::SAGRoutingHelper")
            .SetParent<Ipv4RoutingHelper> ()
            .SetGroupName("SagRouting")
            .AddConstructor<SAGRoutingHelper>()
    ;
    return tid;
}

SAGRoutingHelper::SAGRoutingHelper (){
	m_factory.SetTypeId ("ns3::SAGRoutingProtocal");
}

SAGRoutingHelper::SAGRoutingHelper (const SAGRoutingHelper &o)
{
    // Left empty intentionally
	//m_factory.SetTypeId(o.m_factory.GetTypeId());
	m_factory = o.m_factory;
	m_objectToBeInstall = o.m_objectToBeInstall;
}

SAGRoutingHelper::~SAGRoutingHelper (){

}

SAGRoutingHelper* SAGRoutingHelper::Copy (void) const
{
  return new SAGRoutingHelper (*this);
}


Ptr<Ipv4RoutingProtocol> SAGRoutingHelper::Create (Ptr<Node> node) const 
{
	//	return m_factory.Create<Ipv4RoutingProtocol> ();
  Ptr<Ipv4RoutingProtocol> agent = m_factory.Create<Ipv4RoutingProtocol> ();
  node->AggregateObject (agent);
  return agent;
}

void SAGRoutingHelper::Set (std::string name, const AttributeValue &value)
{
	m_factory.Set (name, value);
}

void SAGRoutingHelper::SetTopologyHandle(std::vector<Ptr<Constellation>> constellations){
	m_constellations = constellations;
}


void SAGRoutingHelper::InitializeArbiter(Ptr<BasicSimulation> basicSimulation, NodeContainer nodes)
{
	m_nodes = nodes;
	m_groundStation = m_constellations[0]->GetGroundStationNodes();
	for(auto cons: m_constellations){
		m_satellite.Add(cons->GetNodes());
	}
	m_basicSimulation = basicSimulation;
	//StoreIPAddresstoNodeId();
}


//void SAGRoutingHelper::SetTopologyHandle(Ptr<TopologySatelliteNetwork> topology)
//{
//    m_topology = topology;
//}

//std::vector<std::vector<std::tuple<int32_t, int32_t, int32_t>>>
//SAGRoutingHelper::InitialEmptyForwardingState()
//{
//    std::vector<std::vector<std::tuple<int32_t, int32_t, int32_t>>> initial_forwarding_state;
//    for (size_t i = 0; i < m_nodes.GetN(); i++) {
//        std::vector <std::tuple<int32_t, int32_t, int32_t>> next_hop_list;
//        for (size_t j = 0; j < m_nodes.GetN(); j++) {
//            next_hop_list.push_back(std::make_tuple(-2, -2, -2)); // -2 indicates an invalid entry
//        }
//        initial_forwarding_state.push_back(next_hop_list);
//    }
//    return initial_forwarding_state;
//}

void SAGRoutingHelper::UpdateForwardingState(int32_t t)
{


}

void SAGRoutingHelper::UpdateRoutingTable(int32_t cur_node, int32_t dst_node, int32_t next_hop, int32_t outInterface, int32_t inInterface){

	m_arbiters.at(cur_node)->SetSingleForwardState(dst_node, next_hop, outInterface, inInterface);

}

void SAGRoutingHelper::StoreIPAddresstoNodeId()
{
//    // Store IP address to node id (each interface has an IP address, so multiple IPs per node)
//	m_ip_to_node_id.clear();
//	for (uint32_t i = 0; i < m_nodes.GetN(); i++){
////    for (uint32_t i = m_satellite.GetN(); i < m_nodes.GetN(); i++) {
//        for (uint32_t j = 0; j < m_nodes.Get(i)->GetObject<Ipv4>()->GetNInterfaces(); j++) {
//        	uint32_t nadrs = m_nodes.Get(i)->GetObject<Ipv4>()->GetNAddresses(j);
//        	for(uint32_t k = 0; k < nadrs; k++){
//        		if(j == 0 && k == 0){
//        			continue;
//        		}
//        		m_ip_to_node_id.insert({m_nodes.Get(i)->GetObject<Ipv4>()->GetAddress(j, k).GetLocal().Get(), i});
//        	}
//        }
//    }


    // Store IP address to node id (each interface has an IP address, so multiple IPs per node)
	m_ip_to_node_id.clear();
	for (uint32_t i = 0; i < m_groundStation.GetN(); i++){
//    for (uint32_t i = m_satellite.GetN(); i < m_nodes.GetN(); i++) {
        for (uint32_t j = 0; j < m_groundStation.Get(i)->GetObject<Ipv4>()->GetNInterfaces(); j++) {
        	uint32_t nadrs = m_groundStation.Get(i)->GetObject<Ipv4>()->GetNAddresses(j);
        	for(uint32_t k = 0; k < nadrs; k++){
        		if(j == 0 && k == 0){
        			continue;
        		}
        		m_ip_to_node_id.insert({m_groundStation.Get(i)->GetObject<Ipv4>()->GetAddress(j, k).GetLocal().Get(), m_groundStation.Get(i)->GetId()});
        	}
        }
    }
}

void SAGRoutingHelper::UpdateIpAddresstoNodeId(){
	StoreIPAddresstoNodeId();
	for(auto arbiter: m_arbiters){
		arbiter->SetIpAddresstoNodeId(m_ip_to_node_id);
	}
//	if(Simulator::Now()==Time(0)){
//		for(auto arbiter: m_arbiters){
//			arbiter->SetIpAddresstoNodeId(m_ip_to_node_id);
//		}
//	}

}

void SAGRoutingHelper::SetObjectNameString(std::string objectToBeInstall){
	m_objectToBeInstall = objectToBeInstall;
}

std::string SAGRoutingHelper::GetObjectNameString(){
	return m_objectToBeInstall;
}

TypeId
SAGRoutingHelper::GetObjectFactoryTypeId() const
{
    NS_LOG_FUNCTION(this);
    return m_factory.GetTypeId();
}


} // namespace ns3
