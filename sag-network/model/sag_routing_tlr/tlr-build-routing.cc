/*
 * tlr-build-routing.cc
 *
 */


#include "ns3/tlr-build-routing.h"
#include "ns3/tlr-routing-table-entry.h"
namespace ns3 {
namespace tlr {


TypeId
TlrBuildRouting::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::tlr::TlrBuildRouting")
    .SetParent<Object> ()
    .SetGroupName ("TLR")
    .AddConstructor<TlrBuildRouting> ()
  ;
  return tid;
}

TlrBuildRouting::TlrBuildRouting(){

}

TlrBuildRouting::~TlrBuildRouting(){

}


void
TlrBuildRouting::SetRouter(Ptr<TLRRoutingTable> routerTable){
	m_routingTable = routerTable;
}

void
TlrBuildRouting::SetIpv4(Ptr<Ipv4> ipv4){
	m_ipv4 = ipv4;
}

void
TlrBuildRouting::SetMaxSatelliteId(int32_t mid){
	SatelliteMaxid = mid;
}

int32_t
TlrBuildRouting::GetMaxSatelliteId(){
	return SatelliteMaxid;
}
void
TlrBuildRouting::RouterCalculate (std::map<TLRLinkStateIdentifier, std::pair<LSAHeader,LSAPacket>>* db, Ipv4Address rootRouterId, std::map<Ipv4Address, std::pair<Ipv4Address, Ipv4Address>> interfaceAddress){
	m_rootRouterId = rootRouterId;
	m_db = db;
	m_interfaceAddress = interfaceAddress;
	ConstructAdjacency();
	//this->SetMaxSatelliteId(65);

    if(m_rtrCalTimeConsidered){
    	clock_t startTime, endTime;
    	startTime = clock(); // route calculate starting time
    	UpdateRoute();
    	endTime = clock(); // route calculate end time

    	double routeCalculateTime = ((double)(endTime - startTime) / CLOCKS_PER_SEC) * 1e9;

        m_rtrCalCb(m_ipv4->GetObject<Node>()->GetId(), Simulator::Now(), routeCalculateTime);

        Simulator::Schedule(NanoSeconds(routeCalculateTime), &TlrBuildRouting::DoUpdateRoute, this, Simulator::Now());
    }
    else{
       	UpdateRoute();
       	DoUpdateRoute(Simulator::Now());
       	m_rtrCalCb(m_ipv4->GetObject<Node>()->GetId(), Simulator::Now(), 0);
    }

}

void
TlrBuildRouting::ConstructAdjacency (){

	m_address.clear();
	m_adjacency.clear();
	m_cost.clear();
	for(std::map<TLRLinkStateIdentifier, std::pair<LSAHeader,LSAPacket>>::iterator it = m_db->begin(); it != m_db->end(); it++){
		// for current node
		Ipv4Address curNodeRtr = it->second.first.GetLinkStateID();
		uint32_t curNode = curNodeRtr.Get();
		// for current node's neighbors
		for(auto link : it->second.second.GetLSALinkDatas()){
			m_adjacency[curNode].push_back(link.GetlinkID().Get());
			m_address[curNodeRtr].push_back(link.Getlinkdata());
			m_cost[curNode][link.GetlinkID().Get()] = link.Getmetric();
			//std::cout<<"curNode "<<curNode<<" link "<<link.GetlinkID().Get()<<" metric "<<link.Getmetric()<<std::endl;

		}

	}

}

struct dijkstra_node {
	uint32_t u;// curNode
	double dis;//distanse
	//int type;//0:best path 1:second best path
};
bool operator > (const dijkstra_node& x, const dijkstra_node& y) {
	return x.dis > y.dis;
}
void
TlrBuildRouting::UpdateRoute (){

	m_preNode.clear();
	m_dist1.clear();
	std::priority_queue<dijkstra_node, std::vector<dijkstra_node>, std::greater<dijkstra_node>> que;
	while(!que.empty()) que.pop();
	double  maxdist = 9999999.9;
	uint32_t curNode = m_rootRouterId.Get();
	std::unordered_map<int32_t, int32_t> pre1;  // 前溯
	std::unordered_map<uint32_t, bool> processedornot1;
	std::unordered_map<uint32_t, double> dist1;  // cost for best nexthop
	for (auto iter1 : m_adjacency)
	{
		processedornot1[iter1.first] = false;
		pre1[iter1.first]=-1;
	}
	for (auto iter2 : m_adjacency)
		dist1[iter2.first] = maxdist;
	for(auto nb : m_adjacency[curNode])
		pre1[nb] = curNode;

	dist1[curNode] = 0;
	que.push({ curNode,0 });
	while (!que.empty()) {
		auto t = que.top();
		que.pop();
		uint32_t u = t.u;
		double nowdist = t.dis;
		if ( processedornot1[u]==true) continue;
	    processedornot1[u] = true;
	    if (m_adjacency.find(u) == m_adjacency.end()) continue;
		for (auto ed : m_adjacency[u])
		{
			double newdist = nowdist + m_cost[u][ed];
			if (dist1[ed]>newdist){
				dist1[ed] =newdist;
				que.push({ ed , dist1[ed]});
				pre1[ed] = u;

			}

		}
	}
	m_preNode = pre1;
	m_dist1 =dist1;

	for (auto it : m_adjacency) {
	uint32_t dstNode = it.first;
	if (dstNode != curNode && m_dist1[dstNode] < maxdist) {
		uint32_t tempNode1 = m_preNode[dstNode];
		uint32_t tempNode2 = dstNode;
		/*std::cout<<" the best routing"<<curNode<<" "<<dstNode<<" "<<m_dist1[dstNode]<<std::endl;
		PrintRout(dstNode,curNode,0);
		std::cout<<" "<<std::endl;*/
		vis.clear();
		while (tempNode1 != curNode){
			if (int(tempNode2)<= SatelliteMaxid && m_preNode[tempNode1]<=SatelliteMaxid)
				vis.push_back(tempNode1);
			tempNode2 = tempNode1;
		    tempNode1 = m_preNode[tempNode1];
			}
		m_find1[dstNode]=tempNode2;
		bool found2 = FindSecondPath(curNode,dstNode);//already have the best next hop,then found second best next hop
		uint32_t sectempNode1 ;
		uint32_t sectempNode2 ;//second best next hop
		if (found2==false )
		{
			sectempNode2=tempNode2;
			m_find2[dstNode]=tempNode2;
		}
			else
			{
				sectempNode1=m_preNode2[dstNode];
				sectempNode2=dstNode;
				 while (sectempNode1 != curNode)
				 {
					 sectempNode2 = sectempNode1;
					 sectempNode1 = m_preNode2[sectempNode1];
				  }
				 m_find2[dstNode]=sectempNode2;
			 }
	}
	}
}
void
TlrBuildRouting::PrintRout(int32_t curNode,int32_t s,int type)
{
	if (curNode==-1) return ;
	if (curNode==s)
	{
		std::cout<<s<<" ";
		return;
	}
	if (type==0)
	{
		if (m_preNode[curNode]!=-1)
		 PrintRout(m_preNode[curNode],s,0);
	}
	else if (type==1)
	{
		if (m_preNode2[curNode]!=-1)
			 PrintRout(m_preNode2[curNode],s,1);
		//else {
		//	if ( m_preNode[curNode]!=-1)  PrintRout(m_preNode[curNode],s,0);
		//}
	}
	std::cout<<curNode<<" ";
	return;
}

bool
TlrBuildRouting::FindSecondPath(uint32_t curNode,uint32_t t)//s:start node t:terminus node
{
	m_preNode2.clear();
	m_dist2.clear();
	std::priority_queue<dijkstra_node, std::vector<dijkstra_node>, std::greater<dijkstra_node>> que;
	while(!que.empty()) que.pop();
	double  maxdist = 9999999.9;
	std::unordered_map<int32_t, int32_t> pre2;  // 前溯
	std::unordered_map<uint32_t, bool> processedornot2;
    std::unordered_map<uint32_t, double> dist2;  // cost for best nexthop
		for (auto iter1 : m_adjacency)
		{
			processedornot2[iter1.first] = false;
			pre2[iter1.first]=-1;
		}
		for (auto iter2 : m_adjacency)
			dist2[iter2.first] = maxdist;
		for(auto nb : m_adjacency[curNode])
			pre2[nb] = curNode;
		dist2[curNode] = 0;
		que.push({ curNode,0 });

		while (!que.empty()) {
			auto t = que.top();
			que.pop();
			uint32_t u = t.u;
			double nowdist = t.dis;
			if ( processedornot2[u]==true) continue;
			 processedornot2[u] = true;
			 if (m_adjacency.find(u) == m_adjacency.end()) continue;
			 for (auto ed : m_adjacency[u])
			 {
				double newdist = nowdist + m_cost[u][ed];
				 if (dist2[ed]>newdist){
				  std::vector<uint32_t>::iterator it = find(vis.begin(),vis.end(),ed);
				  if(it != vis.end() ) continue;

					dist2[ed] =newdist;
					que.push({ ed , dist2[ed]});
					pre2[ed] = u;

				 }

				}
			}
		m_preNode2 = pre2;
		m_dist2 =dist2;
		if (dist2[t]<maxdist ) return true;
		return false;
}
void
TlrBuildRouting::DoUpdateRoute (Time t){
	double  maxdist = 9999999.9;
	if(m_lastRouteCalculateTime > t){
		return;
	}

	m_routingTable->Clear();

	m_lastRouteCalculateTime = t;
	uint32_t curNode = m_rootRouterId.Get();
	for(auto it : m_adjacency){
		uint32_t dstNode = it.first;
		if (dstNode != curNode && m_dist1[dstNode]<maxdist){//{find(pre.begin(),pre.end(),dstNode)
			// dst: dstNode
			// nexthop: tempNode2
			//secondnexthop:sectempNode2
		    uint32_t tempNode2 = m_find1[dstNode];
		    uint32_t sectempNode2 = m_find2[dstNode];
			Ipv4Address nextHopRtr(tempNode2);
			Ipv4Address secnextHopRtr(sectempNode2);
			Ipv4Address srcIP;
			Ipv4Address gateway;
			Ipv4Address secsrcIP;
			Ipv4Address secondhopgateway;
			if(m_interfaceAddress.find(nextHopRtr) != m_interfaceAddress.end()){
				srcIP = m_interfaceAddress[nextHopRtr].first;
				gateway = m_interfaceAddress[nextHopRtr].second;
			}
			else throw std::runtime_error("Process Wrong: TlrBuildRouting::UpdateRoute1");
			if (m_interfaceAddress.find(secnextHopRtr) != m_interfaceAddress.end())
			{
				secsrcIP = m_interfaceAddress[secnextHopRtr].first;
				secondhopgateway = m_interfaceAddress[secnextHopRtr].second;
			}
              else  throw std::runtime_error("Process Wrong: TlrBuildRouting::UpdateRoute1 No Found secnextHop");

			Ptr<NetDevice> dec = m_ipv4->GetNetDevice(m_ipv4->GetInterfaceForAddress(srcIP));
			Ptr<NetDevice> secdec = m_ipv4->GetNetDevice(m_ipv4->GetInterfaceForAddress(secsrcIP));
			Ipv4InterfaceAddress itrAddress = m_ipv4->GetAddress(m_ipv4->GetInterfaceForAddress(srcIP),0);
			Ipv4InterfaceAddress secitrAddress = m_ipv4->GetAddress(m_ipv4->GetInterfaceForAddress(secsrcIP),0);
			Ipv4Address dstRtr = Ipv4Address(dstNode);
			if(m_address.find(dstRtr) != m_address.end()){
				int cnt=0;
				for(auto d : m_address[dstRtr]){
					// d : dst IP
					TLRRoutingTableEntry rtEntry(dec, d, itrAddress, gateway, tempNode2,secdec, d, secitrAddress, secondhopgateway, sectempNode2);
					//std::cout<<gateway<<"   "<<gateway.Get()<<"   "<<tempNode2<<"   "<<nextHopRtr<<"   "<<nextHopRtr.Get()<<std::endl;
					m_routingTable->AddRoute(rtEntry);
					if (++cnt==1) m_routingTable->AddInferfaceForNode(dstNode,d);
					/*if (gateway!=secondhopgateway){
										std::cout<<' '<<std::endl;
										std::cout<<"dec   "<<dec<<std::endl;
										std::cout<<"d  "<<d<<std::endl;
										//std::cout<<"itrAddress"<<itrAddress<<std::endl;
										std::cout<<"gateway   "<<gateway<<std::endl;
										std::cout<<"secondhop  "<<secondhopgateway<<std::endl;
										std::cout<<' '<<std::endl;
										std::cout<<" the best routing"<<curNode<<" "<<dstNode<<" "<<m_dist1[dstNode]<<std::endl;
										//PrintRout(dstNode,curNode,0);
										std::cout<<nextHopRtr<<" "<<m_interfaceAddress[nextHopRtr].first<<" "<<m_interfaceAddress[nextHopRtr].second<<std::endl;
										std::cout<<" the second best routing "<<curNode<<" "<<dstNode<<" "<<m_dist2[dstNode]<<std::endl;
										//PrintRout(dstNode,curNode,1);
										std::cout<<secnextHopRtr<<" "<<m_interfaceAddress[secnextHopRtr].first<<" "<<m_interfaceAddress[secnextHopRtr].second<<std::endl;
										std::cout<<" "<<std::endl;
										}
										//system("pause");
										m_routingTable->AddRoute(rtEntry);

									}*/
			}
			}
			else throw std::runtime_error("Process Wrong: TlrBuildRouting::UpdateRoute");


		}
	}












}







}
}
