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

#ifndef OSPF_BUILD_ROUTING_H
#define OSPF_BUILD_ROUTING_H

#include <stdint.h>
#include <list>
#include <queue>
#include <map>
#include <unordered_map>
#include <vector>

#include "ns3/ospf_link_state_packet.h"
#include "ns3/ospf-lsa-identifier.h"
#include "ns3/ipv4-address.h"
//#include "global-router-interface.h"
//#include "global-route-manager-impl.h"
#include "ns3/ipv4-interface-address.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/exp-util.h"
#include "ns3/sag_routing_table.h"
#include "ns3/sag_rtp_constants.h"
//class RoutingTable;
//class OSPFLinkStateIdentifier;
//class LSAHeader;
//class LSAPacket;

namespace ns3 {

namespace ospf {
class OspfBuildRouting : public Object
{
public:
	static TypeId GetTypeId ();
	OspfBuildRouting();
	~OspfBuildRouting();
	void SetRouter(Ptr<SAGRoutingTable> routerTable);
	void SetIpv4(Ptr<Ipv4> ipv4);
	void RouterCalculate (std::unordered_map<OSPFLinkStateIdentifier, std::pair<LSAHeader,LSAPacket>, hash_ospfIdt, equal_ospfIdt>* db,
			std::vector<std::pair<LSAHeader,LSAPacket>> lsaList,
			Ipv4Address rootRouterId,
			std::unordered_map<Ipv4Address, std::pair<Ipv4Address, Ipv4Address>, hash_adr, equal_adr> interfaceAddress);
	bool ConstructAdjacency ();
	bool ConstructAdjacency (std::vector<std::pair<LSAHeader,LSAPacket>> lsaList);
	void UpdateRoute ();
	void ReadRoute ();
	void ReadUpdateRoute(uint32_t gs, uint32_t sat);

	void DoUpdateRoute (Time t);
	void SetRoutingCalculationCallback(Callback<void, uint32_t, Time, double> rtrCalCb);
	void SetRtrCalTimeEnable(bool rtrCalTimeConsidered){
		m_rtrCalTimeConsidered = rtrCalTimeConsidered;
	}
	void SetBaseDir(std::string baseDir){
		m_baseDir = baseDir;
	}
	void SetNodeNum(uint32_t satelliteNum, uint32_t gsNum){
		m_satelliteNum = satelliteNum;
		m_gsNum = gsNum;
		std::vector<int32_t> gsConnection(m_gsNum, -1);
		m_gsConnection = gsConnection;
	}

	void SetPromptMode(bool prompt){
		PromptMode = prompt;
	}



private:
	  std::unordered_map<OSPFLinkStateIdentifier, std::pair<LSAHeader,LSAPacket>, hash_ospfIdt, equal_ospfIdt>* m_db;
	  Ptr<SAGRoutingTable> m_routingTable;
	  Ptr<Ipv4> m_ipv4;
	  Ipv4Address m_rootRouterId;
	  std::unordered_map<uint32_t, std::unordered_map<uint32_t, uint8_t>> m_cost;
	  std::unordered_map<uint32_t, std::vector<uint32_t>> m_adjacency;
	  std::unordered_map<uint32_t, std::vector<uint32_t>> m_adjacencyPast;
	  std::unordered_map<Ipv4Address, std::pair<Ipv4Address, Ipv4Address>, hash_adr, equal_adr> m_interfaceAddress;
	  std::unordered_map<Ipv4Address, std::vector<Ipv4Address>, hash_adr, equal_adr> m_address; // rtr: interface address
	  std::unordered_map<uint32_t, uint32_t> m_preNode;
	  Time m_lastRouteCalculateTime = Seconds(0);
	  Callback<void, uint32_t, Time, double> m_rtrCalCb;
	  bool m_rtrCalTimeConsidered;
	  std::string m_baseDir;
	  uint32_t m_satelliteNum;
	  uint32_t m_gsNum;
	  bool PromptMode = false;
	  uint32_t m_timePast = 0;
	  std::vector<int32_t> m_gsConnection;


};
}

}
#endif /* OSPF_BUILD_ROUTING_H_ */
