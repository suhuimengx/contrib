/*
 * minimum_hop_count_routing_helper_ipv6.cc
 *
 *  Created on: 2022年12月27日
 *      Author: root
 */


#include "ns3/minimum_hop_count_routing_helper_ipv6.h"
#include "ns3/sag_rtp_constants.h"
#include "ns3/ipv6-minimum-hop-routing.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (Minimum_Hop_Count_Routing_Helper_IPv6);
TypeId Minimum_Hop_Count_Routing_Helper_IPv6::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::Minimum_Hop_Count_Routing_Helper_IPv6")
            .SetParent<SAGRoutingHelper> ()
            .SetGroupName("SagRouting")
            .AddConstructor<Minimum_Hop_Count_Routing_Helper_IPv6>()
    ;
    return tid;
}

Minimum_Hop_Count_Routing_Helper_IPv6::Minimum_Hop_Count_Routing_Helper_IPv6()
{
	m_factory.SetTypeId ("ns3::Ipv6MinimumHopRouting");
}

Minimum_Hop_Count_Routing_Helper_IPv6::Minimum_Hop_Count_Routing_Helper_IPv6(Ptr<BasicSimulation> basicSimulation, NodeContainer nodes)
{
	m_factory.SetTypeId ("ns3::Ipv6MinimumHopRouting");

}

void Minimum_Hop_Count_Routing_Helper_IPv6::InitializeArbiter(Ptr<BasicSimulation> basicSimulation, NodeContainer nodes){

	SAGRoutingHelperIPv6::InitializeArbiter(basicSimulation, nodes);
	// Read in initial forwarding state
	std::cout << "  > Create initial single forwarding state" << std::endl;
	//std::vector<std::vector<std::tuple<int32_t, int32_t, int32_t>>> initial_forwarding_state = InitialEmptyForwardingState();
	m_basicSimulation->RegisterTimestamp("Create initial single forwarding state");

	// Set the routing arbiters
	std::cout << "  > Setting the routing arbiter on each node" << std::endl;
	for (size_t i = 0; i < m_satellite.GetN(); i++) {
		Ptr<ArbiterSingleForward> arbiter = CreateObject<ArbiterSingleForward>(m_nodes.Get(i), m_nodes); //Class Type needs to change to subclass type in subclass sag_routing_protocal_helper
		m_arbiters.push_back(arbiter);

		Ptr<Ipv6ListRouting> routingList = m_satellite.Get(i)->GetObject<Ipv6>()->GetRoutingProtocol()->GetObject<Ipv6ListRouting>();
		uint32_t NRoutingProtocols = routingList->GetNRoutingProtocols();
		//bool found = false;
		for(uint32_t i = 0; i < NRoutingProtocols; i++){
			int16_t priority;
			if(routingList->GetRoutingProtocol(i, priority)->GetInstanceTypeId() == TypeId::LookupByName ("ns3::Ipv6MinimumHopRouting")){
				routingList->GetRoutingProtocol(i, priority)->GetObject<Ipv6MinimumHopRouting>()->SetArbiter(arbiter);
				//found = true;
			}
		}
//		if(!found){
//			throw std::runtime_error ("Minimum_Hop_Count_Routing_Helper_IPv6::InitializeArbiter.");
//		}
//		int16_t priority;
//		m_nodes.Get(i)->GetObject<Ipv4>()->GetRoutingProtocol()->GetObject<Ipv4ListRouting>()->GetRoutingProtocol(0,priority)->GetObject<Ipv4MinimumHopRouting>()->SetArbiter(arbiter);


	}
	m_dynamicStateUpdateIntervalNs = parse_positive_int64(m_basicSimulation->GetConfigParamOrFail("dynamic_state_update_interval_ns"));
	std::cout << "  > Forward state update interval: " << m_dynamicStateUpdateIntervalNs << "ns" << std::endl;
	std::cout << "  > Perform first forwarding state load for t=0" << std::endl;
	m_basicSimulation->RegisterTimestamp("Setup routing arbiter on each node");
	UpdateForwardingState(0);

}

void Minimum_Hop_Count_Routing_Helper_IPv6::UpdateForwardingState(int32_t t)
{

	if(!Topology_CHANGE_GSL && t!=0){
		int64_t dynamicStateUpdateIntervalNsTemp;
		if(t == 0) dynamicStateUpdateIntervalNsTemp = m_dynamicStateUpdateIntervalNs + 2;
		else dynamicStateUpdateIntervalNsTemp = m_dynamicStateUpdateIntervalNs;
		int64_t next_update_ns = t + dynamicStateUpdateIntervalNsTemp;
		if (next_update_ns < m_basicSimulation->GetSimulationEndTimeNs()) {
			Simulator::Schedule(NanoSeconds(dynamicStateUpdateIntervalNsTemp),
					&Minimum_Hop_Count_Routing_Helper_IPv6::UpdateForwardingState, this,
					next_update_ns);
		}
		return;
	}

	UpdateIpv6AddresstoNodeId();

	for(auto a : m_arbiters){
		a->ClearNextHopList();
	}
	if(m_groundStation.GetN() != 0){
//		for(int32_t i = 0; i < (int32_t)m_satellite.GetN(); i++){
//			for(auto gsl : m_constellations[0]->GetGSLInformation()){
//				if(gsl.second == nullptr){
//					continue;
//				}
//				Ptr<Constellation> cons1 = m_satellite.Get(i)->GetObject<SatellitePositionMobilityModel>()->GetSatellite()->GetCons();
//				Ptr<Constellation> cons2 = gsl.second->GetObject<SatellitePositionMobilityModel>()->GetSatellite()->GetCons();
//				if(cons1->GetName() != cons2->GetName()){
//					continue;
//				}
//	//			if(gsl.second->GetId() == gsl_temp.second->GetId()){
//	//				continue;
//	//			}
//				if(i != (int32_t)gsl.second->GetId()){
//					std::vector<uint32_t> adjacency = cons1->GetAdjacency(i);
//					int32_t next_hop;
//					int32_t hops = 100000;
//
//					for(int32_t neighbour : adjacency){
//						int32_t tempHops = HopCount(neighbour, (int32_t)gsl.second->GetId(), cons1);
//
//						if(tempHops < hops){
//							next_hop = neighbour;
//							hops = tempHops;
//						}
//
//					}
//
//					SAGRoutingHelper::UpdateRoutingTable(
//							i,
//							gsl.first->GetId(),
//							next_hop,
//							cons1->GetISLInterfaceNumber(i,next_hop),
//							cons1->GetISLInterfaceNumber(next_hop,i)
//							);
//
//				}
//
//			}
//		}






		auto gs2sat_vec = m_constellations[0]->GetGSLInformation();
		std::unordered_map<Ptr<Node>, Ptr<Node>> gs2sat;
		for(auto gsl: gs2sat_vec){
			gs2sat[gsl.first] = gsl.second[0].second;
		}

		std::vector<std::pair<uint32_t, uint32_t>> pairNodeIds;
		for(int32_t i = 0; i < (int32_t)m_groundStation.GetN(); i++){
    		uint32_t nApps = m_groundStation.Get(i)->GetNApplications();
    		for(uint32_t j = 0; j < nApps; j++){
    			// to be modified
    			// if(m_groundStation.Get(i)->GetApplication(j)->GetInstanceTypeId() == TypeId::LookupByName ("ns3::Bgp")){
    			// 	continue;
    			// }
    			Ptr<SAGApplicationLayer> app = m_groundStation.Get(i)->GetApplication(j)->GetObject<SAGApplicationLayer>();
    			Ptr<Node> dstNode = app->GetDestinationNode();
    			Ptr<Node> srcNode = app->GetSourceNode();
    			if(srcNode == nullptr || dstNode == nullptr){
    				continue;
    				// maybe happen
    				throw std::runtime_error ("Minimum_Hop_Count_Routing_Helper_IPv6::UpdateForwardingState.");
    			}
    			Ptr<Node> srcSat = gs2sat[srcNode];
    			Ptr<Node> dstSat = gs2sat[dstNode];
    			if(gs2sat[srcNode] == nullptr || gs2sat[dstNode] == nullptr){
    				continue;
    				// maybe happen: ground station sleep state
    				throw std::runtime_error ("No satellite in sight!");
    			}
    			if(find(pairNodeIds.begin(), pairNodeIds.end(), std::make_pair(srcNode->GetId(), dstNode->GetId())) == pairNodeIds.end()){
    				pairNodeIds.push_back(std::make_pair(srcNode->GetId(), dstNode->GetId()));  // pair<srcSatelliteId, dstGroundStationId>
    			}
    			else{
    				continue;
    			}

    			int32_t cur_node = srcSat->GetId();
    			int32_t next_hop = srcSat->GetId();
    			while(cur_node != (int32_t)dstSat->GetId()){
    				Ptr<Constellation> cons1 = srcSat->GetObject<SatellitePositionMobilityModel>()->GetSatellite()->GetCons();
    				Ptr<Constellation> cons2 = dstSat->GetObject<SatellitePositionMobilityModel>()->GetSatellite()->GetCons();
					if(cons1->GetName() != cons2->GetName()){
						throw std::runtime_error ("Not the same satellite constellation.");
						continue;
					}

    				std::vector<uint32_t> adjacency = cons1->GetAdjacency(cur_node);
					//int32_t next_hop;
					int32_t hops = 100000;

					for(int32_t neighbour : adjacency){
						int32_t tempHops = HopCount(neighbour, dstSat->GetId(), cons1);

						if(tempHops < hops){
							next_hop = neighbour;
							hops = tempHops;
						}

					}

					SAGRoutingHelperIPv6::UpdateRoutingTable(
							cur_node,
							dstNode->GetId(),
							next_hop,
							cons1->GetISLInterfaceNumber(cur_node,next_hop),
							cons1->GetISLInterfaceNumber(next_hop,cur_node)
							);
					//std::cout<<cur_node<<"  "<<next_hop<<std::endl;
					cur_node = next_hop;


    			}

    		}
    	}
	}
	else{
		// only on satellites
		std::vector<std::pair<uint32_t, uint32_t>> pairNodeIds;
		for(int32_t i = 0; i < (int32_t)m_satellite.GetN(); i++){
    		uint32_t nApps = m_satellite.Get(i)->GetNApplications();
    		for(uint32_t j = 0; j < nApps; j++){
    			// to be modified
    			// if(m_satellite.Get(i)->GetApplication(j)->GetInstanceTypeId() == TypeId::LookupByName ("ns3::Bgp")){
    			// 	continue;
    			// }
    			Ptr<SAGApplicationLayer> app = m_satellite.Get(i)->GetApplication(j)->GetObject<SAGApplicationLayer>();
    			Ptr<Node> dstNode = app->GetDestinationNode();
    			Ptr<Node> srcNode = app->GetSourceNode();
    			if(srcNode == nullptr){
    				continue;
    			}
    			if(find(pairNodeIds.begin(), pairNodeIds.end(), std::make_pair(srcNode->GetId(), dstNode->GetId())) == pairNodeIds.end()){
    				pairNodeIds.push_back(std::make_pair(srcNode->GetId(), dstNode->GetId()));
    			}
    			else{
    				continue;
    			}


    			int32_t cur_node = srcNode->GetId();
    			int32_t next_hop = srcNode->GetId();
    			while(cur_node != (int32_t)dstNode->GetId()){
    				Ptr<Constellation> cons1 = m_satellite.Get(cur_node)->GetObject<SatellitePositionMobilityModel>()->GetSatellite()->GetCons();

    				std::vector<uint32_t> adjacency = cons1->GetAdjacency(cur_node);
					//int32_t next_hop;
					int32_t hops = 100000;

					for(int32_t neighbour : adjacency){
						int32_t tempHops = HopCount(neighbour, dstNode->GetId(), cons1);

						if(tempHops < hops){
							next_hop = neighbour;
							hops = tempHops;
						}

					}

					SAGRoutingHelperIPv6::UpdateRoutingTable(
							cur_node,
							dstNode->GetId(),
							next_hop,
							cons1->GetISLInterfaceNumber(cur_node,next_hop),
							cons1->GetISLInterfaceNumber(next_hop,cur_node)
							);
					//std::cout<<cur_node<<"  "<<next_hop<<std::endl;
					cur_node = next_hop;


    			}

    		}
    	}
	}


    //}
    // Plan the next update
	int64_t dynamicStateUpdateIntervalNsTemp;
	if(t == 0) dynamicStateUpdateIntervalNsTemp = m_dynamicStateUpdateIntervalNs + 2;
	else dynamicStateUpdateIntervalNsTemp = m_dynamicStateUpdateIntervalNs;
	int64_t next_update_ns = t + dynamicStateUpdateIntervalNsTemp;
	if (next_update_ns < m_basicSimulation->GetSimulationEndTimeNs()) {
		Simulator::Schedule(NanoSeconds(dynamicStateUpdateIntervalNsTemp),
				&Minimum_Hop_Count_Routing_Helper_IPv6::UpdateForwardingState, this,
				next_update_ns);
	}

}

int32_t Minimum_Hop_Count_Routing_Helper_IPv6::HopCount(int32_t cur_node, int32_t dst_node, Ptr<Constellation> cons){

	int32_t satsPerOrbit = cons->GetSatNum();
	int32_t orbits = cons->GetOrbitNum();
	int32_t F = cons->GetPhase();
	int32_t cur_x = cur_node / satsPerOrbit; // orbit number
	int32_t cur_y = cur_node % satsPerOrbit; // satellite number
	int32_t dst_x = dst_node / satsPerOrbit;
	int32_t dst_y = dst_node % satsPerOrbit;
	int32_t delta_x;
	int32_t delta_y;
	if(abs(cur_x - dst_x) <= (orbits / 2)){
		delta_x = abs(cur_x - dst_x);
	}
	else{
		delta_x = orbits - abs(cur_x - dst_x);
	}

	if(abs(cur_x - dst_x) <= (orbits / 2)){
		if(abs(cur_y - dst_y) >= (satsPerOrbit / 2)){
			delta_y = satsPerOrbit - abs(cur_y - dst_y);
		}
		else{
			delta_y = abs(cur_y - dst_y);
		}
	}
	else{
		int32_t cur_y_temp;
		int32_t dst_y_temp;
		if(cur_x > dst_x){
			cur_y_temp = cur_y + F;
			dst_y_temp = dst_y;
		}
		else{
			cur_y_temp = cur_y;
			dst_y_temp = dst_y + F;
		}
		if (abs(cur_y_temp - dst_y_temp) >= (satsPerOrbit / 2)) {
			delta_y = satsPerOrbit - abs(cur_y_temp - dst_y_temp);
		} else {
			delta_y = abs(cur_y_temp - dst_y_temp);
		}
	}

	return delta_x + delta_y;

//    	int32_t adj_node_x = ceil(cur_node*1.0 / 22.0) - 1;
//    	int32_t adj_node_y;
//        if(cur_node % 22 != 0){
//            adj_node_y = cur_node % 22 - 1;
//        }
//        else{
//            adj_node_y = 21;
//        }
//        int32_t dst_node_x = ceil(dst_node*1.0 / 22.0) - 1;
//        int32_t dst_node_y;
//        if(dst_node % 22 != 0){
//            dst_node_y = dst_node % 22 - 1;
//        }
//        else{
//            dst_node_y = 21;
//        }
//
//        int32_t xt = abs(dst_node_x - adj_node_x);
//        int32_t yt = abs(dst_node_y - adj_node_y);
//        int32_t xt1 = xt;
//        int32_t yt1 = yt;
//        if (xt > 30/2){
//            xt1 = 30 - std::max(dst_node_x, adj_node_x) + std::min(dst_node_x, adj_node_x);
//        }
//
//        int32_t dst_node_y1;
//        int32_t adj_node_y1;
//        if(dst_node_x > adj_node_x){
//            dst_node_y1 = dst_node_y + 1;
//            adj_node_y1 = adj_node_y;
//        }
//        else{
//            adj_node_y1 = adj_node_y + 1;
//            dst_node_y1 = dst_node_y;
//        }
//
//        if((yt >= 22/2) && (xt > 30/2)){
//            yt1 = 22 - std::max(dst_node_y1, adj_node_y1) + std::min(dst_node_y1, adj_node_y1);
//        }
//        else if((yt < 22/2) && (xt > 30/2)){
//            yt1 = abs(dst_node_y1 - adj_node_y1);
//        }
//        else if((yt > 22/2) && (xt <= 30/2)){
//            yt1 = 22 - std::max(dst_node_y, adj_node_y) + std::min(dst_node_y, adj_node_y);
//        }
//
//
//
//        int32_t hops = xt1 + yt1;
//        return hops;


}


}
