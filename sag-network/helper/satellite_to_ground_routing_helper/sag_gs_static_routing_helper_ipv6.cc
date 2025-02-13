/*
 * sag_gs_static_routing_helper_ipv6.cc
 *
 *  Created on: 2022年12月2日
 *      Author: root
 */


#include "sag_gs_static_routing_helper_ipv6.h"
#include "ns3/mpi-interface.h"
#include "ns3/sag_rtp_constants.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (SAG_GS_Static_Routing_Helper_IPv6);
TypeId SAG_GS_Static_Routing_Helper_IPv6::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::SAG_GS_Static_Routing_Helper_IPv6")
            .SetParent<SAGRoutingHelperIPv6> ()
            .SetGroupName("SagRouting")
            .AddConstructor<SAG_GS_Static_Routing_Helper_IPv6>()
    ;
    return tid;
}

SAG_GS_Static_Routing_Helper_IPv6::SAG_GS_Static_Routing_Helper_IPv6()
{
	m_factory.SetTypeId ("ns3::SAGRoutingProtocalIPv6");
}

SAG_GS_Static_Routing_Helper_IPv6::SAG_GS_Static_Routing_Helper_IPv6(Ptr<BasicSimulation> basicSimulation, NodeContainer nodes)
{
	m_factory.SetTypeId ("ns3::SAGRoutingProtocalIPv6");

}

void SAG_GS_Static_Routing_Helper_IPv6::InitializeArbiter(Ptr<BasicSimulation> basicSimulation, NodeContainer nodes)
{
	SAGRoutingHelperIPv6::InitializeArbiter(basicSimulation, nodes);
	// Read in initial forwarding state
	std::cout << "  > Create initial single forwarding state" << std::endl;
	//std::vector<std::vector<std::tuple<int32_t, int32_t, int32_t>>> initial_forwarding_state = SAGRoutingHelperIPv6::InitialEmptyForwardingState();
	m_basicSimulation->RegisterTimestamp("Create initial SAG_GS_Static_Routing_Helper_IPv6 state");

	// Set the routing arbiters
	std::cout << "  > Setting the routing arbiter on each node" << std::endl;
	for (size_t i = 0; i < m_nodes.GetN(); i++) {
		//Ptr<SAG_Routing_Protocal> arbiter = m_routingFactory.Create<SAG_Routing_Protocal> ();
		//arbiter->DoInitialize(m_nodes.Get(i), m_nodes, initial_forwarding_state[i]);
		Ptr<ArbiterSingleForward> arbiter = CreateObject<ArbiterSingleForward>(m_nodes.Get(i), m_nodes); //Class Type needs to change to subclass type in subclass sag_routing_protocal_helper
		m_arbiters.push_back(arbiter);

		Ptr<Ipv6ListRouting> routingList = m_nodes.Get(i)->GetObject<Ipv6>()->GetRoutingProtocol()->GetObject<Ipv6ListRouting>();
		uint32_t NRoutingProtocols = routingList->GetNRoutingProtocols();
		//bool found = false;
		for(uint32_t i = 0; i < NRoutingProtocols; i++){
			int16_t priority;
			if(routingList->GetRoutingProtocol(i, priority)->GetInstanceTypeId() == TypeId::LookupByName ("ns3::SAGRoutingProtocalIPv6")){
				routingList->GetRoutingProtocol(i, priority)->GetObject<SAGRoutingProtocalIPv6>()->SetArbiter(arbiter);
				//found = true;
			}
		}
//		if(!found){
//			throw std::runtime_error ("Minimum_Hop_Count_Routing_Helper::InitializeArbiter.");
//		}

	}
	m_basicSimulation->RegisterTimestamp("Setup SAG_GS_Static_Routing_Helper_IPv6 arbiter on each node");

	// Load first forwarding state
	m_dynamicStateUpdateIntervalNs = parse_positive_int64(m_basicSimulation->GetConfigParamOrFail("dynamic_state_update_interval_ns"));
	std::cout << "  > Forward state update interval: " << m_dynamicStateUpdateIntervalNs << "ns" << std::endl;
	std::cout << "  > Perform first forwarding state load for t=0" << std::endl;
	UpdateForwardingState(0);

}


void SAG_GS_Static_Routing_Helper_IPv6::UpdateForwardingState(int64_t t)
{
	if(!Topology_CHANGE_GSL){
		int64_t dynamicStateUpdateIntervalNsTemp;
		if(t == 0) dynamicStateUpdateIntervalNsTemp = m_dynamicStateUpdateIntervalNs + 2;
		else dynamicStateUpdateIntervalNsTemp = m_dynamicStateUpdateIntervalNs;
		int64_t next_update_ns = t + dynamicStateUpdateIntervalNsTemp;
		if (next_update_ns < m_basicSimulation->GetSimulationEndTimeNs()) {
			Simulator::Schedule(NanoSeconds(dynamicStateUpdateIntervalNsTemp),
					&SAG_GS_Static_Routing_Helper_IPv6::UpdateForwardingState, this,
					next_update_ns);
		}
		return;
	}


	UpdateIpv6AddresstoNodeId();

	for(auto a : m_arbiters){
		a->ClearNextHopList();
	}
	m_basicSimulation->RegisterTimestamp("Create initial SAG_GS_Static_Routing state");
	GSLSwitch(t);
	// Plan the next update
	int64_t dynamicStateUpdateIntervalNsTemp;
	if(t == 0) dynamicStateUpdateIntervalNsTemp = m_dynamicStateUpdateIntervalNs + 2;
	else dynamicStateUpdateIntervalNsTemp = m_dynamicStateUpdateIntervalNs;
	int64_t next_update_ns = t + dynamicStateUpdateIntervalNsTemp;
	if (next_update_ns < m_basicSimulation->GetSimulationEndTimeNs()) {
		Simulator::Schedule(NanoSeconds(dynamicStateUpdateIntervalNsTemp),
				&SAG_GS_Static_Routing_Helper_IPv6::UpdateForwardingState, this,
				next_update_ns);
	}
}

void SAG_GS_Static_Routing_Helper_IPv6::GSLSwitch(int64_t t){

	auto gs2sat_vec = m_constellations[0]->GetGSLInformation();
	std::vector<std::pair<Ptr<Node>, Ptr<Node>>> gslSwitchs;
	for(auto gsl: gs2sat_vec){
		gslSwitchs.push_back(std::make_pair(gsl.first, gsl.second[0].second));
	}

//	const std::vector<std::pair<Ptr<Node>, Ptr<Node>>> gslSwitchs = m_constellations[0]->GetGSLInformation();
	for(uint32_t i = 0; i < gslSwitchs.size(); i++){

		if(gslSwitchs.at(i).second!=nullptr){
			//update routingTable
			UpdateGSRoutingTable(gslSwitchs.at(i).first, gslSwitchs.at(i).second);
			UpdateSATRoutingTable(gslSwitchs.at(i).first, gslSwitchs.at(i).second);
		}


	}

//	std::cout<< "     >> Update ground station static routing table"<<std::endl;
//	std::cout<< "     >> Update satellite static routing table"<<std::endl;

}

void SAG_GS_Static_Routing_Helper_IPv6::UpdateGSRoutingTable(Ptr<Node> groundStation, Ptr<Node> satellite){
	if(groundStation!=nullptr and satellite!=nullptr){
		for(uint32_t i = 0; i < m_constellations[0]->GetGroundStationNodes().GetN(); i++){
			auto curGS = m_constellations[0]->GetGroundStationNodes().Get(i);
			//std::cout<<curGS->GetId()<<std::endl;
	        //std::cout<<MpiInterface::GetSystemId()<<":"<<groundStation->GetId()<<","<<curGS->GetId()<<","<<satellite->GetId()<<std::endl;

			m_arbiters.at(groundStation->GetId())->SetSingleForwardState(
					curGS->GetId(),
					satellite->GetId(), 1 + 0, // GS OUTPUT Interface 0, Skip the loop-back interface: +1
					1 + 4 // satellite INPUT Interface 4, Skip the loop-back interface: +1
							);
		}

		m_arbiters.at(groundStation->GetId())->SetSingleForwardState(
					satellite->GetId(),
					satellite->GetId(), 1 + 0, // GS OUTPUT Interface 0, Skip the loop-back interface: +1
					1 + 4 // satellite INPUT Interface 4, Skip the loop-back interface: +1
							);

	}


}

void SAG_GS_Static_Routing_Helper_IPv6::UpdateSATRoutingTable(Ptr<Node> groundStation, Ptr<Node> satellite){
	if (groundStation != nullptr and satellite != nullptr) {

		m_arbiters.at(satellite->GetId())->SetSingleForwardState(
				groundStation->GetId(), groundStation->GetId(), 1 + 4, // GS OUTPUT Interface 0, Skip the loop-back interface: +1
				1 + 0 // satellite INPUT Interface 4, Skip the loop-back interface: +1
						);

	}

}



}
