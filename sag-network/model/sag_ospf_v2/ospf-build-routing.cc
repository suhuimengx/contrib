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


#include "ns3/ospf-build-routing.h"
#include "ns3/sag_routing_table_entry.h"
namespace ns3 {
namespace ospf {


TypeId
OspfBuildRouting::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ospf::OspfBuildRouting")
    .SetParent<Object> ()
    .SetGroupName ("OSPF")
    .AddConstructor<OspfBuildRouting> ()
  ;
  return tid;
}

OspfBuildRouting::OspfBuildRouting(){

}

OspfBuildRouting::~OspfBuildRouting(){

}

void
OspfBuildRouting::SetRouter(Ptr<SAGRoutingTable> routerTable){
	m_routingTable = routerTable;
}

void
OspfBuildRouting::SetIpv4(Ptr<Ipv4> ipv4){
	m_ipv4 = ipv4;
}

void
OspfBuildRouting::RouterCalculate (std::unordered_map<OSPFLinkStateIdentifier, std::pair<LSAHeader,LSAPacket>, hash_ospfIdt, equal_ospfIdt>* db,
		std::vector<std::pair<LSAHeader,LSAPacket>> lsaList,
		Ipv4Address rootRouterId,
		std::unordered_map<Ipv4Address, std::pair<Ipv4Address, Ipv4Address>, hash_adr, equal_adr> interfaceAddress){
	m_rootRouterId = rootRouterId;
	m_db = db;
	if(PromptMode){
		if(m_db->size() < m_satelliteNum+m_gsNum){
			return;
		}
		m_interfaceAddress = interfaceAddress;
		ConstructAdjacency(lsaList);

		//ReadRoute ();

		return;
	}
	m_interfaceAddress = interfaceAddress;

	ConstructAdjacency();

	//std::cout<<m_rtrCalTimeConsidered<<"    "<<m_rootRouterId.Get()<<std::endl;
    if(m_rtrCalTimeConsidered){
    	clock_t startTime, endTime;
    	startTime = clock(); // route calculate starting time
    	UpdateRoute();
    	endTime = clock(); // route calculate end time

    	double routeCalculateTime = ((double)(endTime - startTime) / CLOCKS_PER_SEC) * 1e9;

        m_rtrCalCb(m_ipv4->GetObject<Node>()->GetId(), Simulator::Now(), routeCalculateTime);

        Simulator::Schedule(NanoSeconds(routeCalculateTime), &OspfBuildRouting::DoUpdateRoute, this, Simulator::Now());
    }
    else{
       	UpdateRoute();
       	DoUpdateRoute(Simulator::Now());
       	m_rtrCalCb(m_ipv4->GetObject<Node>()->GetId(), Simulator::Now(), 0);
    }

}

bool
OspfBuildRouting::ConstructAdjacency (std::vector<std::pair<LSAHeader,LSAPacket>> lsaList){


	if(m_adjacencyPast.size() == 0){
		m_adjacency.clear();
		m_address.clear();
		for(std::unordered_map<OSPFLinkStateIdentifier, std::pair<LSAHeader,LSAPacket>, hash_ospfIdt, equal_ospfIdt>::iterator it = m_db->begin(); it != m_db->end(); it++){
			// for current node
			Ipv4Address curNodeRtr = it->second.first.GetLinkStateID();
			uint32_t curNode = curNodeRtr.Get();
			// for current node's neighbors
			for(auto link : it->second.second.GetLSALinkDatas()){
				m_adjacency[curNode].push_back(link.GetlinkID().Get());
				m_address[curNodeRtr].push_back(link.Getlinkdata());
			}
			if(curNode >= m_satelliteNum){
				if(it->second.second.GetLSALinkDatas().size()!=1){
					throw std::runtime_error ("Currently, gs only supports one gsl link in ospf prompt version");
				}
				m_gsConnection[curNode-m_satelliteNum] = it->second.second.GetLSALinkDatas()[0].GetlinkID().Get();
			}

		}
		ReadRoute ();
		m_adjacencyPast = m_adjacency;
	}
	else{
		for(auto lsa: lsaList){
			// for current node
			Ipv4Address curNodeRtr = lsa.first.GetLinkStateID();
			//uint32_t curNode = curNodeRtr.Get();
			// for current node's neighbors
			m_address[curNodeRtr].clear();
			for(auto link : lsa.second.GetLSALinkDatas()){
				m_address[curNodeRtr].push_back(link.Getlinkdata());
			}
		}


		for(auto lsa: lsaList){
			// for current node
			Ipv4Address curNodeRtr = lsa.first.GetLinkStateID();
			uint32_t curNode = curNodeRtr.Get();

			if(curNode >= m_satelliteNum){
				if(lsa.second.GetLSALinkDatas().size()!=1){
					throw std::runtime_error ("Currently, gs only supports one gsl link in ospf prompt version");
				}
				auto sat = lsa.second.GetLSALinkDatas()[0].GetlinkID().Get();
				if(m_gsConnection[curNode-m_satelliteNum] != (int)sat){
					// read route
					ReadUpdateRoute(curNode, sat);


					m_gsConnection[curNode-m_satelliteNum] = sat;
				}
			}
		}

	}

	return 1;
//	if(m_adjacency.size() >= m_satelliteNum+m_gsNum){
//		return true;
//	}
//	else{
//		return false;
//	}

}

bool
OspfBuildRouting::ConstructAdjacency (){

	m_adjacencyPast = m_adjacency;
	m_address.clear();
	m_adjacency.clear();
	m_cost.clear();
	for(std::unordered_map<OSPFLinkStateIdentifier, std::pair<LSAHeader,LSAPacket>, hash_ospfIdt, equal_ospfIdt>::iterator it = m_db->begin(); it != m_db->end(); it++){
		// for current node
		Ipv4Address curNodeRtr = it->second.first.GetLinkStateID();
		uint32_t curNode = curNodeRtr.Get();
		// for current node's neighbors
		for(auto link : it->second.second.GetLSALinkDatas()){
			m_adjacency[curNode].push_back(link.GetlinkID().Get());
			m_address[curNodeRtr].push_back(link.Getlinkdata());
			m_cost[curNode][link.GetlinkID().Get()] = link.Getmetric();
		}

	}


//	uint32_t totalN = 0;
//	for(auto adj: m_adjacency){
//		totalN += adj.second.size();
//	}
//
//	if(totalN == 2000 * 4){
//		return true;
//	}
//	else{
//		return false;
//	}


	if(m_adjacency.size() >= m_satelliteNum+m_gsNum){
		return true;
	}
	else{
		return false;
	}

	//return 1;


//	if(m_rootRouterId.Get()==65){
//		std::cout<<m_adjacency.size()<<std::endl;
//
//	}

}

void
OspfBuildRouting::ReadRoute (){

	// Open file
	uint32_t curNode = m_rootRouterId.Get();
	//std::cout<<curNode<<" ::::::::::::::::::"<<std::endl;
	std::ifstream config_file;
	if(m_adjacencyPast.size()==0){
		config_file.open(m_baseDir + "/config_topology/network_state/time_0/node_"+std::to_string(curNode)+".txt");
		NS_ABORT_MSG_UNLESS(config_file.is_open(), "File routes could not be opened, waiting to be modified");
	}
	
	//if(curNode==1584){
	//std::cout<<Simulator::Now().GetSeconds()<<"  "<<curNode<<" ::::::::::::::::::"<<std::endl;
	//}
	//std::cout<<Simulator::Now().GetSeconds()<<"  "<<curNode<<" ::::::::::::::::::"<<std::endl;
	m_routingTable->Clear();
	uint32_t counter=0;
	std::string line;
	if (config_file) {
		while (std::getline(config_file, line)) {
			// Skip commented lines
			if (trim(line).empty() || line.c_str()[0] == '#') {
				continue;
			}
			
			if(counter==0){
				counter++;
				continue;
			}

			// Split on =
			std::vector<std::string> res = split_string(line, ",", 5);

			uint32_t src = std::stoi(res[0]);
			uint32_t dst = std::stoi(res[1]);
			uint32_t next_hop = std::stoi(res[2]);
			uint32_t outIf = std::stoi(res[3]);
			//uint32_t inIf = std::stoi(res[4]);
			if(outIf==5){
			outIf=4;
			}



			if(src != curNode){
				std::cout<<src<<"  "<<curNode<<std::endl;
				throw std::runtime_error("Process Wrong: OspfBuildRouting::UpdateRoute222");
			}

			Ipv4Address srcIP;
			Ipv4Address gateway;
			Ipv4Address nextHopRtr = Ipv4Address(next_hop);
			if(m_interfaceAddress.find(nextHopRtr) != m_interfaceAddress.end()){
				srcIP = m_interfaceAddress[nextHopRtr].first;
				gateway = m_interfaceAddress[nextHopRtr].second;
			}
			else{
//				if(curNode == 100){
//					auto x = m_adjacency[curNode];
//				}
				throw std::runtime_error("Process Wrong: OspfBuildRouting::UpdateRoute1");

			}



			Ptr<NetDevice> dec = m_ipv4->GetNetDevice(outIf + 1);
			Ipv4InterfaceAddress itrAddress = m_ipv4->GetAddress(outIf + 1, 0);
			Ipv4Address dstRtr = Ipv4Address(dst);
			if(m_address.find(dstRtr) != m_address.end()){
				for(auto d : m_address[dstRtr]){
					// d : dst IP
					//m_routingTable->AddRoute(dec, d, itrAddress, gateway);
					SAGRoutingTableEntry rtEntry(dec, d, itrAddress, gateway);
					m_routingTable->AddRoute(rtEntry);

				}
				SAGRoutingTableEntry rtEntry(dec, dstRtr, itrAddress, gateway);
				m_routingTable->AddRoute(rtEntry);
			}

			counter++;



		}

	}

	config_file.close();
	
	//if(curNode==1584){
	//std::cout<<Simulator::Now().GetSeconds()<<"  "<<curNode<<" ::::::::::::::::::"<<std::endl;
	//}
	//std::cout<<curNode<<" ::::::::::::::::::"<<std::endl;

}


void
OspfBuildRouting::ReadUpdateRoute(uint32_t gs, uint32_t sat){
	std::ifstream config_file;
	config_file.open(m_baseDir + "/config_topology/network_state/"+std::to_string(gs)+","+std::to_string(sat)+"/node_"+std::to_string(m_rootRouterId.Get())+".txt");
	//std::cout<<m_baseDir + "/config_topology/network_state/"+std::to_string(gs)+","+std::to_string(sat)+"/node_"+std::to_string(m_rootRouterId.Get())+".txt"<<std::endl;
	NS_ABORT_MSG_UNLESS(config_file.is_open(), "File routes could not be opened, waiting to be modified");

	
	std::string line;
	uint32_t counter=0;
	if (config_file) {
		while (std::getline(config_file, line)) {
			// Skip commented lines
			if (trim(line).empty() || line.c_str()[0] == '#') {
				continue;
			}
			if(counter==0){
				uint32_t time = std::stoi(line);
				if(m_timePast <= time){
					m_routingTable->Clear();
					counter++;
					m_timePast = time;
					//if(m_rootRouterId.Get()==1){
					//std::cout<<"@@@@@@@@@@@@@@@@@@@@@@@@@@"<<Simulator::Now().GetSeconds()<<"  "<<gs<<","<<sat<<std::endl;
					//}
					continue;
				}
				else{
					//std::cout<<"!!!!!!!!!!!!!!!!!!!!!!!"<<Simulator::Now().GetSeconds()<<"  "<<gs<<","<<sat<<std::endl;
					break;	
				}
			}

			// Split on =
			std::vector<std::string> res = split_string(line, ",", 5);

			uint32_t src = std::stoi(res[0]);
			uint32_t dst = std::stoi(res[1]);
			uint32_t next_hop = std::stoi(res[2]);
			uint32_t outIf = std::stoi(res[3]);
			//uint32_t inIf = std::stoi(res[4]);
			if(outIf==5){
			outIf=4;
			}



			if(src != m_rootRouterId.Get()){
				std::cout<<src<<"  "<<m_rootRouterId.Get()<<std::endl;
				throw std::runtime_error("Process Wrong: OspfBuildRouting::UpdateRoute222");
			}

			Ipv4Address srcIP;
			Ipv4Address gateway;
			Ipv4Address nextHopRtr = Ipv4Address(next_hop);
			if(m_interfaceAddress.find(nextHopRtr) != m_interfaceAddress.end()){
				srcIP = m_interfaceAddress[nextHopRtr].first;
				gateway = m_interfaceAddress[nextHopRtr].second;
			}
			else{
//				if(curNode == 100){
//					auto x = m_adjacency[curNode];
//				}
				throw std::runtime_error("Process Wrong: OspfBuildRouting::UpdateRoute1");

			}



			Ptr<NetDevice> dec = m_ipv4->GetNetDevice(outIf + 1);
			Ipv4InterfaceAddress itrAddress = m_ipv4->GetAddress(outIf + 1, 0);
			Ipv4Address dstRtr = Ipv4Address(dst);
			if(m_address.find(dstRtr) != m_address.end()){
				for(auto d : m_address[dstRtr]){
					// d : dst IP
					//m_routingTable->AddRoute(dec, d, itrAddress, gateway);
					SAGRoutingTableEntry rtEntry(dec, d, itrAddress, gateway);
					m_routingTable->AddRoute(rtEntry);

				}
				SAGRoutingTableEntry rtEntry(dec, dstRtr, itrAddress, gateway);
				m_routingTable->AddRoute(rtEntry);
			}

			counter++;



		}

	}

	config_file.close();





}

void
OspfBuildRouting::SetRoutingCalculationCallback(Callback<void, uint32_t, Time, double> rtrCalCb)
{
	m_rtrCalCb = rtrCalCb;
}

void
OspfBuildRouting::UpdateRoute (){

	m_preNode.clear();
	//<**************
//	//std::vector<uint32_t> gs = {100, 101,102,103,104,105,106,107,108};
//	std::vector<uint32_t> gs = {100, 101, 104};
//	//std::vector<uint32_t> gs = {200, 201};
//	auto it1 = m_adjacency.find(gs[1]);
//	auto it2 = m_adjacency.find(gs[0]);
//	if(it1 == m_adjacency.end() || it2 == m_adjacency.end()){
//		return;
//	}
	//>**************

	double  maxdist = 9999999.9;
	uint32_t curNode = m_rootRouterId.Get();
	std::unordered_map<uint32_t, uint32_t> pre;  // 前溯
	std::unordered_map<uint32_t, bool> processedornot;
	std::unordered_map<uint32_t, double> dist;  // cost
	for (auto iter1 : m_adjacency) processedornot[iter1.first] = false;
	for (auto iter2 : m_adjacency) dist[iter2.first] = maxdist;
	for(auto nb : m_adjacency[curNode]){
		dist[nb] = m_cost[curNode][nb];
		pre[nb] = curNode;
	}

	dist[curNode] = 0;
	processedornot[curNode] = true;

	for (size_t i = 2; i <= m_adjacency.size(); i++) {

		uint32_t best = curNode;
		double temp = maxdist;
		for (auto iter3 : m_adjacency) {
			if (!processedornot[iter3.first] && dist[iter3.first] < temp)
			{
				temp = dist[iter3.first];
				best = iter3.first;
			}
		}
		processedornot[best] = true;
		//<**************
//		for(size_t it = 0; it < gs.size(); it++){
//			if(gs[it] == best){
//				gs.erase(gs.begin() + it);
//				break;
//			}
//		}
//		if(gs.empty()){
//			break;
//		}
		//>**************
		for (auto iter4 : m_adjacency[best])//更新dist和pre
		{
			if (!processedornot[iter4])//j要连通并且j没有被处理过
			{

				double newdist = dist[best] + m_cost[best][iter4];
				if (newdist < dist[iter4])
				{
					dist[iter4] = newdist;
					pre[iter4] = best;
				}
			}
		}

	}

	m_preNode = pre;


}

void
OspfBuildRouting::DoUpdateRoute (Time t){

	if(m_lastRouteCalculateTime > t){
		return;
	}

	m_routingTable->Clear();


	m_lastRouteCalculateTime = t;
	uint32_t curNode = m_rootRouterId.Get();
	for(auto it : m_adjacency){
		uint32_t dstNode = it.first;
		if (dstNode != curNode && m_preNode.find(dstNode) != m_preNode.end()){//{find(pre.begin(),pre.end(),dstNode)
			uint32_t tempNode1 = m_preNode[dstNode];
			uint32_t tempNode2 = dstNode;
			bool found = true;
			while (tempNode1 != curNode){
				tempNode2 = tempNode1;
				if(m_preNode.find(tempNode1) != m_preNode.end()){//find(pre.begin(),pre.end(),tempNode1)
					tempNode1 = m_preNode[tempNode1];
				}
				else{
					found = false;
					break;
				}
			}
			if(!found){
				continue;
			}
			// dst: dstNode
			// nexthop: tempNode2
			Ipv4Address nextHopRtr(tempNode2);
			Ipv4Address srcIP;
			Ipv4Address gateway;


			if(m_interfaceAddress.find(nextHopRtr) != m_interfaceAddress.end()){
				srcIP = m_interfaceAddress[nextHopRtr].first;
				gateway = m_interfaceAddress[nextHopRtr].second;
			}
			else{
//				if(curNode == 100){
//					auto x = m_adjacency[curNode];
//				}
				throw std::runtime_error("Process Wrong: OspfBuildRouting::UpdateRoute1");

			}
			Ptr<NetDevice> dec = m_ipv4->GetNetDevice(m_ipv4->GetInterfaceForAddress(srcIP));
			Ipv4InterfaceAddress itrAddress = m_ipv4->GetAddress(m_ipv4->GetInterfaceForAddress(srcIP),0);
			Ipv4Address dstRtr = Ipv4Address(dstNode);
			if(m_address.find(dstRtr) != m_address.end()){
				for(auto d : m_address[dstRtr]){
					// d : dst IP
					//m_routingTable->AddRoute(dec, d, itrAddress, gateway);
					SAGRoutingTableEntry rtEntry(dec, d, itrAddress, gateway);
					m_routingTable->AddRoute(rtEntry);

				}
				SAGRoutingTableEntry rtEntry(dec, dstRtr, itrAddress, gateway);
				m_routingTable->AddRoute(rtEntry);
			}
			else{
				throw std::runtime_error("Process Wrong: OspfBuildRouting::UpdateRoute2");
			}


		}
	}


}




}
}
