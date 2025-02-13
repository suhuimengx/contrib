/*
 * tlr-build-routing.h
 *
 *  Created on: 2023年
 *      Author: 
 */

#ifndef TLR_BUILD_ROUTING_H
#define TLR_BUILD_ROUTING_H

#include <stdint.h>
#include <list>
#include <queue>
#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>
#include "ns3/tlr_link_state_packet.h"
#include "ns3/tlr-lsa-identifier.h"
#include "ns3/tlr-routing-table.h"
#include "ns3/ipv4-address.h"
//#include "global-router-interface.h"
//#include "global-route-manager-impl.h"
#include "ns3/ipv4-interface-address.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/exp-util.h"
#include "tlr-routing-table.h"

//class RoutingTable;
//class TLRLinkStateIdentifier;
//class LSAHeader;
//class LSAPacket;

namespace ns3 {
namespace tlr {

class TlrBuildRouting : public Object
{
public:
	static TypeId GetTypeId ();
	TlrBuildRouting();
	~TlrBuildRouting();
	void SetRouter(Ptr<TLRRoutingTable> routerTable);
	void SetIpv4(Ptr<Ipv4> ipv4);
	int32_t SatelliteMaxid;
	void SetMaxSatelliteId(int32_t mid);
	int32_t GetMaxSatelliteId();
	void RouterCalculate (std::map<TLRLinkStateIdentifier, std::pair<LSAHeader,LSAPacket>>* db, Ipv4Address rootRouterId, std::map<Ipv4Address, std::pair<Ipv4Address, Ipv4Address>> interfaceAddress);
	void ConstructAdjacency ();
	void UpdateRoute ();

	void DoUpdateRoute (Time t);
	void PrintRout(int32_t curNode,int32_t s,int type);
	bool FindSecondPath(uint32_t curNode,uint32_t t);
	void FindRoutForNb(uint32_t curNode);


	void SetRoutingCalculationCallback(Callback<void, uint32_t, Time, double> rtrCalCb)
	{
		m_rtrCalCb = rtrCalCb;
	}
	void SetRtrCalTimeEnable(bool rtrCalTimeConsidered){
		m_rtrCalTimeConsidered = rtrCalTimeConsidered;
	}


private:
	  std::map<TLRLinkStateIdentifier, std::pair<LSAHeader,LSAPacket>>* m_db;
	  Ptr<TLRRoutingTable> m_routingTable;
	  Ptr<Ipv4> m_ipv4;
	  Ipv4Address m_rootRouterId;
	  std::unordered_map<uint32_t, std::unordered_map<uint32_t, uint16_t>> m_cost;
	  std::unordered_map<uint32_t, std::vector<uint32_t>> m_adjacency;
	  std::map<Ipv4Address, std::pair<Ipv4Address, Ipv4Address>> m_interfaceAddress;
	  std::map<Ipv4Address, std::vector<Ipv4Address>> m_address; // rtr: interface address
	  std::unordered_map<int32_t, int32_t> m_preNode;
	  std::unordered_map<int32_t, int32_t> m_preNode2;
	  std::unordered_map<int32_t, std::unordered_map<int32_t,double>> m_dis;
	  std::unordered_map<uint32_t, double> m_dist1;  // cost for best nexthop
	  std::unordered_map<uint32_t, double> m_dist2;  // cost for second best nexthop
	  std::unordered_map<int32_t, int32_t> m_find1;
	  std::unordered_map<int32_t, int32_t> m_find2;
	  std::vector<uint32_t> vis;
	  Time m_lastRouteCalculateTime = Seconds(0);
	  Callback<void, uint32_t, Time, double> m_rtrCalCb;
	  bool m_rtrCalTimeConsidered;


};
}

}
#endif /* TLR_BUILD_ROUTING_H_ */
