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

#ifndef MPLB_BUILD_ROUTING_H
#define MPLB_BUILD_ROUTING_H

//#include <stdint.h>
//#include <stdlib.h>
#include <list>
#include <queue>
#include <map>
#include <unordered_map>
#include <vector>

#include "ns3/ipv4-address.h"
#include "ns3/ipv4-interface-address.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/exp-util.h"
#include "ns3/sag_routing_table.h"
#include "ns3/walker-constellation-structure.h"


namespace ns3 {
namespace mplb {

struct pairHash
{
	template<typename T,typename U>
	size_t operator()(const std::pair<T, U> & p) const
	{
		return std::hash<T>()(p.first) ^ std::hash<U>()(p.second);
	}

	template<typename T, typename U>
	bool operator() (const std::pair<T, U> & p1, const std::pair<T, U> & p2) const
	{
		return p1.first == p2.first && p1.second == p2.second;
	}

};

class MplbBuildRouting : public Object
{
public:
	static TypeId GetTypeId ();
	MplbBuildRouting();
	~MplbBuildRouting();

	void SetRouter(Ptr<SAGRoutingTable> routerTable);
	void SetIpv4(Ptr<Ipv4> ipv4);
	void SetTopology(Ptr<Constellation> walkerConstellation);

	void RouterCalculate ();


	// FlexTE
	void UpdateRoute ();
	void FilteringNPath_symmetry (uint8_t nPath);
	void FilteringNPath(uint8_t nPath);
	void FilteringNPath_KSP(uint8_t nPath);
	std::vector<uint32_t> MappingPath(uint32_t oldNode, uint32_t newNode, std::vector<uint32_t> path, uint32_t satsPerOrbit, uint32_t orbits);
	uint32_t MappingNodePair(uint32_t oldsrc, uint32_t src, uint32_t dst, uint32_t satsPerOrbit, uint32_t orbits);

	// Segemnt routing
	void UpdateRouteSegment ();
	bool UpdateRouteSegment (uint32_t curLinkDownNumber, uint32_t sampleIndex);

	// SMORE
	struct RandomizedRoutingTreeNode{
		uint32_t hId;		// level id
		uint32_t nodeId;		// clustering node id
		std::vector<uint32_t> nodeSet;		// nodeset
		std::vector<uint32_t> posSet;
		RandomizedRoutingTreeNode* fatherTreeNode;

		RandomizedRoutingTreeNode(uint32_t h, uint32_t satId, std::vector<uint32_t> nodes, RandomizedRoutingTreeNode* fatherNode){
			hId = h;
			nodeId = satId;
			nodeSet = nodes;
			fatherTreeNode = fatherNode;
		}
		RandomizedRoutingTreeNode(){
			hId = 0;
			nodeId = 0;
			nodeSet = {};
			fatherTreeNode = nullptr;
		}


		void SetPosSet(std::vector<uint32_t> set){
			posSet = set;
		}
	};
	void UpdateRouteSMORE ();
	bool UpdateRouteSMORE (uint32_t curLinkDownNumber, uint32_t sampleIndex);
	bool UpdateAdjacency (uint32_t curnode, uint32_t neighbour);
	void WriteAdjacency(std::vector<std::pair<uint32_t, uint32_t>> links, uint32_t sampleIndex);
	bool Vertify(std::vector<std::pair<uint32_t, uint32_t>> links, uint32_t curLinkDownNumber);
	std::pair<double,double> Dijkstra (uint32_t curNode);
	void RandomizedRoutingTree (uint32_t mindist, uint32_t maxdist);
	std::vector<RandomizedRoutingTreeNode> Clustering(uint32_t level, RandomizedRoutingTreeNode rrtNode, RandomizedRoutingTreeNode* rrtNodeFather, double R, uint32_t k);
	bool Distance(uint32_t node1, uint32_t node2, double R, uint32_t k);
	std::vector<uint32_t> SelectPath(uint32_t src, uint32_t dst);
	std::vector<uint32_t> PathGenerate(std::vector<uint32_t> paths);





	// public
	void DoUpdateRoute (Time t);
	void SetRoutingCalculationCallback(Callback<void, uint32_t, Time, double> rtrCalCb);
	void SetRtrCalTimeEnable(bool rtrCalTimeConsidered){
		m_rtrCalTimeConsidered = rtrCalTimeConsidered;
	}


private:
	  uint8_t m_nPath;
	  Ptr<SAGRoutingTable> m_routingTable;
	  Ptr<Ipv4> m_ipv4;
	  std::map<Ipv4Address, std::pair<Ipv4Address, Ipv4Address>> m_interfaceAddress;
	  std::map<Ipv4Address, std::vector<Ipv4Address>> m_address; // rtr: interface address
	  Time m_lastRouteCalculateTime = Seconds(0);
	  Callback<void, uint32_t, Time, double> m_rtrCalCb;
	  bool m_rtrCalTimeConsidered;
	  Ptr<Constellation> m_walkerConstellation;
	  bool m_hasLinkDwon;
	  std::unordered_map<std::pair<uint32_t, uint32_t>, std::set<std::pair<uint32_t, uint32_t>>, pairHash> m_link2Routes;
	  std::unordered_map<uint32_t, std::vector<std::vector<std::pair<uint32_t, uint32_t>>>> m_downLinks;




	  // FlexTE
	  std::unordered_map<uint32_t, std::unordered_map<uint32_t,std::vector<std::vector<uint32_t>>>> m_pathDateBase;
	  std::unordered_map<uint32_t, std::unordered_map<uint32_t,std::vector<uint32_t>>> m_pathDateBaseSegmentNumber;
	  std::unordered_map<uint32_t, std::unordered_map<uint32_t,std::vector<std::pair<uint8_t, uint8_t>>>> m_pathDateBaseSegmentDirection; // (x, y)
	  std::unordered_map<uint32_t, std::unordered_map<uint32_t,std::vector<std::vector<uint32_t>>>> m_routeAllPairs;
	  std::unordered_map<uint32_t, std::unordered_map<uint32_t,std::vector<std::vector<uint32_t>>>> m_routeAllPairsFlexSATE;
	  std::unordered_map<uint32_t, std::unordered_map<uint32_t,std::vector<std::vector<uint32_t>>>> m_initialRouteAllPairsFlexSATE;
	  //std::unordered_map<std::pair<uint32_t, uint32_t>, uint16_t, pairHash> m_linkWeight;
	  std::unordered_map<uint32_t, std::vector<uint32_t>> m_initialAdjacency;
	  std::vector<uint32_t> m_nodes;
	  std::vector<std::vector<uint32_t>> m_linkWeight;
	  std::vector<std::vector<uint32_t>> m_initialLinkWeight;
	  std::string m_filtering;
	  uint32_t m_segments;




	  // SMORE
	  std::unordered_map<uint32_t, std::unordered_map<uint32_t, uint32_t>> m_preNode;
	  std::unordered_map<uint32_t, std::unordered_map<uint32_t, double>> m_dist;
	  std::unordered_map<uint32_t, std::vector<uint32_t>> m_adjacency;
	  std::unordered_map<uint32_t, std::unordered_map<uint32_t, uint32_t>> m_cost;
	  std::unordered_map<uint32_t, std::unordered_map<uint32_t, uint32_t>> m_initialCost;
	  std::unordered_map<uint32_t, std::unordered_map<uint32_t,std::vector<std::vector<uint32_t>>>> m_routeAllPairsSMORE;
	  std::unordered_map<uint32_t, std::unordered_map<uint32_t,std::vector<std::vector<uint32_t>>>> m_initialRouteAllPairsSMORE;
	  std::vector<uint32_t> m_shuffle;
	  //std::map<uint32_t, uint32_t> m_mappingNodeId2Shuffle;
	  std::map<uint32_t, uint32_t> m_mappingShuffle2NodeId;
	  std::vector<std::vector<RandomizedRoutingTreeNode>> rrt;
};
}

}
#endif /* MPLB_BUILD_ROUTING_H_ */
