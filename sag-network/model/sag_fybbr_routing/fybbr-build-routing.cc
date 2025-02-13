/*
 * ospf-build-routing.cc
 *
 */


#include "fybbr-build-routing.h"
#include "ns3/sag_routing_table_entry.h"
#include <utility>
#include <iostream>
#include <algorithm>
//#include "leo-satellite-config.h"

namespace ns3 {
namespace fybbr {


TypeId
FybbrBuildRouting::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::fybbr::FybbrBuildRouting")
    .SetParent<Object> ()
    .SetGroupName ("FYBBR")
    .AddConstructor<FybbrBuildRouting> ()
  ;
  return tid;
}

FybbrBuildRouting::FybbrBuildRouting(){

}

FybbrBuildRouting::~FybbrBuildRouting(){

}

void
FybbrBuildRouting::SetRouter(Ptr<SAGRoutingTable> routerTable){

	//uint32_t curNode = m_rootRouterId.Get();
	//m_routingTables[curNode] = routerTable;
	m_routingTable = routerTable;
}

void
FybbrBuildRouting::SetIpv4(Ptr<Ipv4> ipv4){
	m_ipv4 = ipv4;
}

void
FybbrBuildRouting::RouterCalculate (std::map<FYBBRLinkStateIdentifier, std::pair<LSAHeader,LSAPacket>>* db, Ipv4Address rootRouterId, std::map<Ipv4Address, std::pair<Ipv4Address, Ipv4Address>> interfaceAddress, Time routcalInterval){
	m_db = db;
	m_interfaceAddress = interfaceAddress;
	ConstructAdjacency();
	clock_t startTime, endTime;
	Ipv4Address calrootRouterId;
	if (m_adjacency.size() == m_satellitesNumber + m_groundStationNumber /*&& first==1 || Simulator::Now() > m_lastRouteCalculateTime + routcalInterval */){
		  first = 0;
		  routeCalculateTime.clear();
		  uint64_t totoalCalTime = 0;
		  //the number of calculate time
		  uint16_t seqLast = AddSeqForSelfOriginatedRTS();
		  SetlastrtsAge(seqLast);
		  //clock_t caltime1=clock();
		  for(std::map<FYBBRLinkStateIdentifier, std::pair<LSAHeader,LSAPacket>>::iterator it = m_db->begin(); it != m_db->end(); it++){
		   // for current node
		   //std::vector<Neighbors::Neighbor> m_nb=m_nbs.GetNB();
		   calrootRouterId = it->second.first.GetLinkStateID();
		   //std::cout<<calrootRouterId<<" m_db LinkStateID"<<std::endl;
		   std::map<Ipv4Address, std::pair<Ipv4Address, Ipv4Address>> interfaces;
		   std::vector<LSALinkData> lsalinkdata= it->second.second.GetLSALinkDatas();
		   for(auto i = lsalinkdata.begin(); i != lsalinkdata.end (); ++i){
			interfaces[i->GetlinkID()] = std::make_pair(i->Getlinkdata(), i->GetneighborlinkID());
		   }
				 //m_rootRouterId = curNodeRtr.Get();
		   std::unordered_map<uint32_t, uint32_t> calpreNode;
		   startTime = clock(); // route calculate starting time
		   calpreNode= UpdateRoute(calrootRouterId);
		   endTime = clock(); // route calculate end time
		   routeCalculateTime[calrootRouterId.Get()] = ((double)(endTime - startTime) / CLOCKS_PER_SEC) * 1e9 / m_serverComputingPower;

		   totoalCalTime += ((double)(endTime - startTime) / CLOCKS_PER_SEC) * 1e9 / m_serverComputingPower;
		   if(m_rtrCalTimeConsidered){
			   Simulator::Schedule(NanoSeconds(totoalCalTime), &FybbrBuildRouting::DoUpdateRoute, this, Simulator::Now(), calrootRouterId, calpreNode, interfaces);
		   }
		   else{
			   DoUpdateRoute (Simulator::Now(), calrootRouterId, calpreNode, interfaces);
		   }

		}
		if(m_rtrCalTimeConsidered){
			m_rtrCalCb(m_ipv4->GetObject<Node>()->GetId(), Simulator::Now(), totoalCalTime);
		}
		else{
			m_rtrCalCb(m_ipv4->GetObject<Node>()->GetId(), Simulator::Now(), 0);
		}
	}


}

void
FybbrBuildRouting::ConstructAdjacency (){

	m_address.clear();
	m_adjacency.clear();
	m_cost.clear();

	for(std::map<FYBBRLinkStateIdentifier, std::pair<LSAHeader,LSAPacket>>::iterator it = m_db->begin(); it != m_db->end(); it++){
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
}
void
FybbrBuildRouting::SetRoutingCalculationCallback(Callback<void, uint32_t, Time, double> rtrCalCb)
{
	m_rtrCalCb = rtrCalCb;
}

//void
std::unordered_map<uint32_t, uint32_t>
FybbrBuildRouting::UpdateRoute (Ipv4Address calrootRouterId){

	    double  maxdist = 9999999.9;
		uint32_t curNode = calrootRouterId.Get();
		//std::unordered_map<uint32_t, uint32_t> calpreNode;
		//Ptr<FybbrRoutingTable> routingTable;
		//routingTable->Clear();
		std::unordered_map<uint32_t, uint32_t> calpreNode;
		calpreNode.clear();
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
		calpreNode = pre;
		//m_preNode = pre;
		return calpreNode;

}

void
FybbrBuildRouting::DoUpdateRoute (Time t, Ipv4Address calrootRouterId, std::unordered_map<uint32_t, uint32_t> calpreNode, std::map<Ipv4Address, std::pair<Ipv4Address, Ipv4Address>> interfaces){

	uint32_t curNode = calrootRouterId.Get();

	std::vector<Ipv4Address> srcips = {};
	std::vector<Ipv4Address> dsts = {};
	//std::vector<Ipv4InterfaceAddress> ifaces = {};
	std::vector<Ipv4Address> nexthops = {};

	for(auto i : m_adjacency){
		uint32_t dstNode = i.first;
		if (dstNode != curNode && calpreNode.find(dstNode) != calpreNode.end()){//{find(pre.begin(),pre.end(),dstNode)
			uint32_t tempNode1 = calpreNode[dstNode];
			uint32_t tempNode2 = dstNode;
			bool found = true;
			while (tempNode1 != curNode){
				tempNode2 = tempNode1;
				if(calpreNode.find(tempNode1) != calpreNode.end()){//find(pre.begin(),pre.end(),tempNode1)
					tempNode1 = calpreNode[tempNode1];
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

			if(interfaces.find(nextHopRtr) != interfaces.end()){
				srcIP = interfaces[nextHopRtr].first;
				gateway = interfaces[nextHopRtr].second;
			}
			else{
				throw std::runtime_error("Process Wrong: FybbrBuildRouting::UpdateRoute1");

			}
			//Ptr<NetDevice> dec = m_ipv4->GetNetDevice(m_ipv4->GetInterfaceForAddress(srcIP));
			//Ipv4InterfaceAddress itrAddress = m_ipv4->GetAddress(m_ipv4->GetInterfaceForAddress(srcIP),0);
			Ipv4Address dstRtr = Ipv4Address(dstNode);
			if(m_address.find(dstRtr) != m_address.end()){
				for(auto d : m_address[dstRtr]){
					// d : dst IP
					//routingTable->AddRoute(dec, d, itrAddress, gateway);
					//SAGRoutingTableEntry rtEntry(dec, d, itrAddress, gateway);
					//routingTable->AddRoute(rtEntry);

					//store routing table
					srcips.push_back(srcIP);
					dsts.push_back(d);
					nexthops.push_back(gateway);

					//rtsData.push_back(RTSData(dec, d, itrAddress, gateway)); // metric = 1 by default todo
					//RTSPacket rtsPacket(m_routingTable[curNode]);
					//std::pair<RTSHeader,RTSPacket> rt={std::make_pair(RTSHeader, RTSPacket)};
				}
			}
			else{
				throw std::runtime_error("Process Wrong: FybbrBuildRouting::UpdateRoute2");
			}
		}
	}
	// Ip header 20 bytes
	// typeID 1 byte
	// FYBBR header 10 bytes
	// RTS header 12 bytes
	// LSUPacket

	//RTSHeader rtsHeadertest(/*uint16_t Pathage =*/ 0, /*uint16_t PathLength =*/ 0, /*Ipv4Address local_routerID =*/ 0, /*Ipv4Address local_areaID =*/ Ipv4Address::GetAny());

	uint32_t sentnum=0;//the number of scrips which have sent
	uint16_t rtsnum=0;//the number of rts amount for one node
	uint32_t maxRTSNum = (1400 - 20 - 12 - 1 - 10 - 4) / 12; // 92 // need a interface for mtu 1500 bytes   max sent scrips num
	RTSPacket sentList;
	//uint16_t Pathage=0;
	if(maxRTSNum >= srcips.size()){
		rtsnum++;
		RTSHeader rtsHeader(/*uint16_t Pathage =*/ m_lastrtsAge, /*uint16_t PathLength =*/ rtsnum, /*Ipv4Address local_routerID =*/ calrootRouterId, /*Ipv4Address local_areaID =*/ Ipv4Address::GetAny());
		RTSPacket rtsPacket(srcips,dsts,nexthops);
		//m_rts.push_back(std::make_pair(rtsHeader,rtsPacket));
		m_initialRTSTriggering(std::make_pair(rtsHeader,rtsPacket));
	}
	else{
		while (sentnum < (srcips.size()-1)){
			rtsnum++;
			uint32_t sentNum = srcips.size();
			std::vector<Ipv4Address> sentsrcips = {};
			std::vector<Ipv4Address> sentdsts = {};
			std::vector<Ipv4Address> sentnexthops = {};
			if(sentnum+maxRTSNum < srcips.size())
			{
				sentNum= sentnum+maxRTSNum;//the number of scrips once sent
			}
			for (uint32_t i=sentnum; i< sentNum; i++){
				sentsrcips.push_back(srcips[i]);
				sentdsts.push_back(dsts[i]);
				sentnexthops.push_back(nexthops[i]);
				sentnum=i;
			}
			RTSHeader rtsHeader(/*uint16_t Pathage =*/ m_lastrtsAge, /*uint16_t PathLength =*/ rtsnum, /*Ipv4Address local_routerID =*/ calrootRouterId, /*Ipv4Address local_areaID =*/ Ipv4Address::GetAny());
			RTSPacket rtsPacket(sentsrcips,sentdsts,sentnexthops);
			m_initialRTSTriggering(std::make_pair(rtsHeader,rtsPacket));

		}
	}

	m_sendFromServerQueue();
}

}
}
