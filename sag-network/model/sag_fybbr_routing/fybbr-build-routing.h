/*
 * ospf-build-routing.h
 *
 *  Created on: 2023年1月31日
 *      Author: xiaoyu
 */

#ifndef FYBBR_BUILD_ROUTING_H
#define FYBBR_BUILD_ROUTING_H

#include <stdint.h>
#include <list>
#include <queue>
#include <map>
#include <unordered_map>
#include <vector>


#include "ns3/fybbr_link_state_packet.h"
#include "ns3/fybbr-lsa-identifier.h"
#include "ns3/fybbr-routing-table.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-interface-address.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/exp-util.h"

#include "ns3/sag_routing_table.h"
#include "ns3/centralized_packet.h"
//#include "ns3/fybbr-neighbor.h"


//#include "global-router-interface.h"
//#include "global-route-manager-impl.h"

//#include "leo-satellite-config.h"

//class RoutingTable;
//class OSPFLinkStateIdentifier;
//class LSAHeader;
//class LSAPacket;

namespace ns3 {
namespace fybbr {

class FybbrBuildRouting : public Object
{
public:
	static TypeId GetTypeId ();
    FybbrBuildRouting();
	~FybbrBuildRouting();
	void SetRouter(Ptr<SAGRoutingTable> routerTable);
	void SetIpv4(Ptr<Ipv4> ipv4);
	void RouterCalculate (std::map<FYBBRLinkStateIdentifier, std::pair<LSAHeader,LSAPacket>>* db, Ipv4Address rootRouterId, std::map<Ipv4Address, std::pair<Ipv4Address, Ipv4Address>> interfaceAddress, Time routcalInterval);
	void ConstructAdjacency ();
	//void UpdateRoute ();
	std::unordered_map<uint32_t, uint32_t> UpdateRoute (Ipv4Address calrootRouterId);
	//void UpdateRoute (Ipv4Address calrootRouterId);
	//void DoUpdateRoute (Time t);
	void DoUpdateRoute (Time t, Ipv4Address calrootRouterId, std::unordered_map<uint32_t, uint32_t> calpreNode, std::map<Ipv4Address, std::pair<Ipv4Address, Ipv4Address>> interfaces);
	//void WriteRoute(Ipv4Address rootRouterId);
	//void SetSendInialRTSCallback (Callback<void, Ipv4Address, Ipv4Address, std::pair<RTSHeader,RTSPacket>> cb);
	/*{
		m_SendInialRTSTriggering = cb;
	}*/
	uint16_t AddSeqForSelfOriginatedRTS(){
		m_lastrtsAge++;
		if(m_lastrtsAge == UINT32_MAX){
			throw std::runtime_error("An attempt is made to increment the sequence number past the maximum value. Waiting for development!");
		}
		return m_lastrtsAge;
	}
	uint16_t GetlastrtsAge()  {
	    return m_lastrtsAge;
	}

	void SetlastrtsAge(uint16_t lastrtsAge)  {
		m_lastrtsAge = lastrtsAge;
	 }
	//send rts
	/*void SetIntialRTSTriggeringCallback (Callback<void, Ipv4Address, Ipv4Address,std::vector<std::pair<RTSHeader,RTSPacket>>> cb)
	{
		m_initialRTSTriggering = cb;
	}*/
	void SetIntialRTSTriggeringCallback (Callback<void, std::pair<RTSHeader,RTSPacket>> cb)
	{
		m_initialRTSTriggering = cb;
	}
	void SetSendFromServerQueueCallback (Callback<void> cb)
	{
		m_sendFromServerQueue = cb;
	}
	void SetRoutingCalculationCallback(Callback<void, uint32_t, Time, double> rtrCalCb);

	void SetRtrCalTimeEnable(bool rtrCalTimeConsidered){
		m_rtrCalTimeConsidered = rtrCalTimeConsidered;
	}
	void SetSatNum(uint32_t satellitesNumber){
		m_satellitesNumber = satellitesNumber;
	}
	void SetGndNum(uint32_t groundStationNumber){
		m_groundStationNumber = groundStationNumber;
	}
	void SetServerComputPower(uint16_t serverComputingPower){
		m_serverComputingPower = serverComputingPower;
	}

public:

	std::vector<std::pair<RTSHeader,RTSPacket>> m_rts={};
	std::vector<std::pair<RTSHeader,RTSPacket>> rtstosend={};
	//double routeCalculateTime;
	std::map<uint32_t,double> routeCalculateTime;
	//bool flagsforrtssend=false;

private:

	  //Callback<void, Ipv4Address, Ipv4Address, std::pair<RTSHeader,RTSPacket>> m_SendInialRTSTriggering;

	  std::map<FYBBRLinkStateIdentifier, std::pair<LSAHeader,LSAPacket>>* m_db;

	  Ptr<SAGRoutingTable> m_routingTable;
	  //std::unordered_map<uint32_t, Ptr<SAGRoutingTable>> m_routingTables;
	  //calculate completed or not
	  //extern bool m_calculRoutflag=false;
	  //extern std::vector<std::pair<RTSHeader,RTSPacket>> m_rts={};

	  //std::unordered_map<uint32_t, bool> m_calculRoutflag;

	  //Neighbors m_nbs;

	  Ptr<Ipv4> m_ipv4;
	  Ipv4Address m_rootRouterId;
	  std::unordered_map<uint32_t, std::unordered_map<uint32_t, uint8_t>> m_cost;
	  std::unordered_map<uint32_t, std::vector<uint32_t>> m_adjacency;
	  std::map<Ipv4Address, std::pair<Ipv4Address, Ipv4Address>> m_interfaceAddress;
	  std::map<Ipv4Address, std::vector<Ipv4Address>> m_address; // rtr: interface address
	  std::unordered_map<uint32_t, uint32_t> m_preNode;
	  Time m_lastRouteCalculateTime = Seconds(0);
	  uint16_t m_lastrtsAge=0;
	  uint16_t first=1;
	  Callback<void, uint32_t, Time, double> m_rtrCalCb;

	  //Callback<void, Ipv4Address, Ipv4Address,std::vector<std::pair<RTSHeader,RTSPacket>>> m_initialRTSTriggering;
	  Callback<void, std::pair<RTSHeader,RTSPacket>> m_initialRTSTriggering;
	  Callback<void> m_sendFromServerQueue;

	  bool m_rtrCalTimeConsidered;
	  uint32_t m_satellitesNumber;
	  uint32_t m_groundStationNumber;
	  uint16_t m_serverComputingPower;
};
}

}
#endif /* CONTRIB_COMMUNICATION_PROTOCAL_MODEL_OSPF_BUILD_ROUTING_H_ */
