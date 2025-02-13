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


#include "mplb-build-routing.h"
#include "ns3/sag_routing_table_entry.h"
#include <algorithm>
#include "ns3/satellite-position-mobility-model.h"
namespace ns3 {
namespace mplb {


TypeId
MplbBuildRouting::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::mplb::MplbBuildRouting")
    .SetParent<Object> ()
    .SetGroupName ("MPLB")
    .AddConstructor<MplbBuildRouting> ()
  ;
  return tid;
}

MplbBuildRouting::MplbBuildRouting(){

}

MplbBuildRouting::~MplbBuildRouting(){

}

void
MplbBuildRouting::SetRouter(Ptr<SAGRoutingTable> routerTable){
	m_routingTable = routerTable;
}

void
MplbBuildRouting::SetIpv4(Ptr<Ipv4> ipv4){
	m_ipv4 = ipv4;
}

void
MplbBuildRouting::SetTopology(Ptr<Constellation> walkerConstellation){
	m_walkerConstellation = walkerConstellation;
}

void
MplbBuildRouting::RouterCalculate (){

    // record propagation delay
    remove_file_if_exists("/home/xiaoyuliu/sag_platform_v3/bake/source/ns-3.35/scratch/main_satnet/test_data/config_topology/propagation_delay.txt");
    std::ofstream fileProp("/home/xiaoyuliu/sag_platform_v3/bake/source/ns-3.35/scratch/main_satnet/test_data/config_topology/propagation_delay.txt");

    auto nodes = m_walkerConstellation->GetNodes();
    for(auto nbs: m_walkerConstellation->GetAdjacency()){
    	Ptr<Node> me = nodes.Get(nbs.first);
    	for(auto nb: nbs.second){
    		Ptr<Node> neighbor = nodes.Get(nb);
    		Ptr<MobilityModel> dstMobility = neighbor->GetObject<MobilityModel>();
    		Ptr<MobilityModel> myMobility = me->GetObject<MobilityModel>();
    		double delay = myMobility->GetDistanceFrom(dstMobility) / (3 * 1e5);
    		fileProp<<nbs.first<<","<<nb<<","<<delay<<std::endl;
    	}
    }
    fileProp.close();


	std::srand(500);
	std::string algorithName = "SegmentRouting"; // "SMORE" "SegmentRouting"
	m_nPath = 4;
	m_filtering = "symmetry"; // 'ksp' 'symmetry' just for segmentrouting
	m_segments = 6; //  just for segmentrouting

	m_hasLinkDwon = true;
	/// If m_hasLinkDwon = true, need to set the following options
	std::vector<uint32_t> faultLinkNumber = {1, 2};
	std::vector<uint32_t> linkDownSamples = {1, 5};

	if(m_hasLinkDwon){
		if(algorithName == "SMORE"){
			m_routeAllPairsSMORE.clear();
			uint32_t totalSampleIndex = 0;
			UpdateRouteSMORE();
			totalSampleIndex++;
			for(uint32_t i = 0; i < faultLinkNumber.size(); i++){
				uint32_t fn = faultLinkNumber[i];
				// m_adjacency = m_walkerConstellation->GetAdjacency();
				for(uint32_t j = 0; j < linkDownSamples[i]; j++){
					std::cout<<"Sample index:" + std::to_string(totalSampleIndex)<<std::endl;
					bool find = false;
					while(!find){
						find = UpdateRouteSMORE(fn, totalSampleIndex);
					}
					totalSampleIndex++;
				}
			}
		}
		else if(algorithName == "SegmentRouting"){
			m_routeAllPairs.clear();
			uint32_t totalSampleIndex = 0;
			UpdateRouteSegment();
			totalSampleIndex++;
			for(uint32_t i = 0; i < faultLinkNumber.size(); i++){
				uint32_t fn = faultLinkNumber[i];
				// m_adjacency = m_walkerConstellation->GetAdjacency();
				for(uint32_t j = 0; j < linkDownSamples[i]; j++){
					std::cout<<"Sample index:" + std::to_string(totalSampleIndex)<<std::endl;
					bool find = false;
					while(!find){
						find = UpdateRouteSegment(fn, totalSampleIndex);
					}
					totalSampleIndex++;
				}
			}
		}

	}
	else{
		//UpdateRoute(); # no use
		if(algorithName == "SMORE"){
			UpdateRouteSMORE();
		}
		else if(algorithName == "SegmentRouting"){
			UpdateRouteSegment();
		}
	}

	std::cout<<"Done!"<<std::endl;



}

void
MplbBuildRouting::SetRoutingCalculationCallback(Callback<void, uint32_t, Time, double> rtrCalCb)
{
	m_rtrCalCb = rtrCalCb;
}

struct TreeNode{
	uint32_t hId;		// level id
	uint32_t nodeId;		// node id
	uint8_t degree;		// node degree in MHBT
	std::pair<uint32_t, uint32_t> coordinate;		// coordinate of node in a coordinate graph (CG)
	uint32_t xHopCount;		// hop count in x direction
	uint32_t yHopCount;		// hop count in y direction
	std::pair<uint32_t, uint32_t> coordinateFather;		// coordinate of father node in CG
	TreeNode* fatherTreeNode;
	std::pair<uint8_t, uint8_t> direction;  // x, y

	TreeNode(uint32_t h, uint32_t satId, uint32_t hx, uint32_t hy, std::pair<uint32_t, uint32_t> cdt, std::pair<uint32_t, uint32_t> cdtFather, TreeNode* fatherNode){
		hId = h;
		nodeId = satId;
		uint32_t deg = 0;
		if(hx > 0){
			deg++;
		}
		if(hy > 0){
			deg++;
		}
		degree = deg;
		coordinate = cdt;
		xHopCount = hx;
		yHopCount = hy;
		coordinateFather = cdtFather;
		fatherTreeNode = fatherNode;
	}
	TreeNode(uint32_t h, uint32_t satId, uint32_t hx, uint32_t hy, std::pair<uint32_t, uint32_t> cdt, std::pair<uint32_t, uint32_t> cdtFather, TreeNode* fatherNode, std::pair<uint8_t, uint8_t> curdirection){
		hId = h;
		nodeId = satId;
		uint32_t deg = 0;
		if(hx > 0){
			deg++;
		}
		if(hy > 0){
			deg++;
		}
		degree = deg;
		coordinate = cdt;
		xHopCount = hx;
		yHopCount = hy;
		coordinateFather = cdtFather;
		fatherTreeNode = fatherNode;
		direction = curdirection;
	}
};

void
MplbBuildRouting::UpdateRoute (){


	// 1. construct a minimum-hop binary tree (MHBT)
	uint32_t satsPerOrbit = m_walkerConstellation->GetSatNum();
	uint32_t orbits = m_walkerConstellation->GetOrbitNum();
	uint32_t phaseFactor = m_walkerConstellation->GetPhase();
	uint32_t totalSats = satsPerOrbit * orbits;

	NS_ASSERT_MSG(phaseFactor == 0, "Currently only supports 0 phase offset.");

	for(uint32_t srcSatId = 0; srcSatId < totalSats; srcSatId++){
		// source id in CG
		uint32_t orbitNumber = srcSatId / satsPerOrbit;
		uint32_t satNumber = srcSatId % satsPerOrbit;

		// calculate four destination ids: (orbit number, sat number)
		std::pair<uint32_t, uint32_t> dstSatIdRightTop, dstSatIdRightBottom;
		std::pair<uint32_t, uint32_t> dstSatIdLeftTop, dstSatIdLeftBottom;
		// record minimum hop
		uint32_t minHopRightTop, minHopRightBottom;
		uint32_t minHopLeftTop, minHopLeftBottom;
		// hop count in two direction: (x, y)
		std::pair<uint32_t, uint32_t> hopRightTop, hopRightBottom;
		std::pair<uint32_t, uint32_t> hopLeftTop, hopLeftBottom;

		uint32_t orbitFloor = floor(orbits / 2.0);
		uint32_t orbitCeil = ceil(orbits / 2.0);
		uint32_t satFloor = floor(satsPerOrbit / 2.0);
		uint32_t satCeil = ceil(satsPerOrbit / 2.0);
		dstSatIdRightTop = std::make_pair((orbitNumber + orbitFloor) % orbits, (satNumber + satFloor) % satsPerOrbit);
		minHopRightTop = orbitFloor + satFloor;
		hopRightTop = std::make_pair(orbitFloor, satFloor);

		dstSatIdRightBottom = std::make_pair((orbitNumber + orbitFloor) % orbits, (satNumber + satCeil) % satsPerOrbit);
		minHopRightBottom = orbitFloor + satFloor;
		hopRightBottom = std::make_pair(orbitFloor, satFloor);

		dstSatIdLeftTop = std::make_pair((orbitNumber + orbitCeil) % orbits, (satNumber + satFloor) % satsPerOrbit);
		minHopLeftTop = orbitFloor + satFloor;
		hopLeftTop = std::make_pair(orbitFloor, satFloor);

		dstSatIdLeftBottom = std::make_pair((orbitNumber + orbitCeil) % orbits, (satNumber + satCeil) % satsPerOrbit);
		minHopLeftBottom = orbitFloor + satFloor;
		hopLeftBottom = std::make_pair(orbitFloor, satFloor);

		if(dstSatIdRightTop.first == dstSatIdLeftTop.first){
			dstSatIdLeftTop.first = (dstSatIdLeftTop.first + 1) % orbits;
			dstSatIdLeftBottom.first = dstSatIdLeftTop.first;
			minHopLeftTop--;
			minHopLeftBottom--;
			hopLeftTop.first = hopLeftTop.first - 1;
			hopLeftBottom.first = hopLeftBottom.first - 1;
		}
		if(dstSatIdRightTop.second == dstSatIdRightBottom.second){
			dstSatIdRightBottom.second = (dstSatIdRightBottom.second + 1) % satsPerOrbit;
			dstSatIdLeftBottom.second = dstSatIdRightBottom.second;
			minHopRightBottom--;
			minHopLeftBottom--;
			hopRightBottom.second = hopRightBottom.second -1;
			hopLeftBottom.second = hopLeftBottom.second - 1;
		}
		std::vector<std::pair<uint32_t, uint32_t>> regions = {dstSatIdRightTop, dstSatIdRightBottom, dstSatIdLeftTop, dstSatIdLeftBottom};
		std::vector<uint32_t> minHops = {minHopRightTop, minHopRightBottom, minHopLeftTop, minHopLeftBottom};
		std::vector<std::pair<uint32_t, uint32_t>> minHopInTwoDirection = {hopRightTop, hopRightBottom, hopLeftTop, hopLeftBottom};
		// moving direction
		std::vector<std::pair<int32_t, int32_t>> directions = {std::make_pair(1,1), std::make_pair(1,-1), std::make_pair(-1,1), std::make_pair(-1,-1)};

		// four minimum hop path (MHP) region
		for(uint8_t i = 0; i < 4; i++){
			uint32_t H_T = minHops[i] + 1; // the height of the tree
			uint32_t H_x = minHopInTwoDirection[i].first;
			uint32_t H_y = minHopInTwoDirection[i].second;
			int32_t Direction_x = directions[i].first;
			int32_t Direction_y = directions[i].second;

			// source tree node
			std::vector<std::vector<TreeNode>> MHBT;
			TreeNode srcTreeNode(1, srcSatId, H_x, H_y, std::make_pair(orbitNumber, satNumber), std::make_pair(orbitNumber, satNumber), nullptr);
			std::vector<TreeNode> VH = {srcTreeNode};
			MHBT.push_back(VH);

			// for each level in MHBT
			for(uint32_t h = 2; h <= H_T; h++){
				std::vector<TreeNode> VHTemp;
				uint32_t j = 0;
				// for each tree node in higher level
				for(uint32_t k = 0; k < MHBT[h-2].size(); k++){
					TreeNode treeNode = MHBT[h-2][k];
					std::pair<uint32_t, uint32_t> lastNode = treeNode.coordinate;
					if(treeNode.xHopCount > 0){
						j++;
						std::pair<uint32_t, uint32_t> curNode;
						if(Direction_x == 1){
							curNode = std::make_pair((lastNode.first + 1) % orbits, lastNode.second);
						}
						else{
							curNode = std::make_pair((lastNode.first + orbits - 1) % orbits, lastNode.second);
						}
						uint32_t satId = curNode.first * satsPerOrbit + curNode.second;
						TreeNode curTreeNode(h, satId, treeNode.xHopCount - 1, treeNode.yHopCount, curNode, lastNode, &MHBT[h-2][k]);
						VHTemp.push_back(curTreeNode);
					}
					if(treeNode.yHopCount > 0){
						j++;
						std::pair<uint32_t, uint32_t> curNode;
						if(Direction_y == 1){
							curNode = std::make_pair(lastNode.first, (lastNode.second + 1) % satsPerOrbit);
						}
						else{
							curNode = std::make_pair(lastNode.first, (lastNode.second + satsPerOrbit - 1) % satsPerOrbit);
						}
						uint32_t satId = curNode.first * satsPerOrbit + curNode.second;
						TreeNode curTreeNode(h, satId, treeNode.xHopCount, treeNode.yHopCount - 1, curNode, lastNode, &MHBT[h-2][k]);
						VHTemp.push_back(curTreeNode);
					}
				}
				VH = VHTemp;
				std::cout<<"::"<<VHTemp.size()<<std::endl;
				//MHBT.push_back(VHTemp);

				// calculate all path
				std::unordered_map<uint32_t, std::vector<uint32_t>> record;
				for(auto curTreeNode : VHTemp){
					//std::cout<<h<<std::endl;
					// Duplicate paths are not re-added if they are already included in other regions.
					if(curTreeNode.coordinate.first == srcTreeNode.coordinate.first || curTreeNode.coordinate.second == srcTreeNode.coordinate.second){
						auto iter = m_pathDateBase[srcSatId].find(curTreeNode.nodeId);
						if(iter != m_pathDateBase[srcSatId].end()){
							continue;
						}
					}
					// Add paths
					if(curTreeNode.fatherTreeNode->nodeId == srcSatId){
						std::vector<uint32_t> path;
						path.push_back(srcSatId);
						path.push_back(curTreeNode.nodeId);
						m_pathDateBase[srcSatId][curTreeNode.nodeId].push_back(path);
					}
					else{
						auto iter = find(record[curTreeNode.fatherTreeNode->nodeId].begin(), record[curTreeNode.fatherTreeNode->nodeId].end(), curTreeNode.nodeId);
						if(iter != record[curTreeNode.fatherTreeNode->nodeId].end()){
							//std::cout<<"1"<<std::endl;
							continue;
						}
						record[curTreeNode.fatherTreeNode->nodeId].push_back(curTreeNode.nodeId);
						std::vector<std::vector<uint32_t>> paths = m_pathDateBase[srcSatId][curTreeNode.fatherTreeNode->nodeId];
						NS_ASSERT_MSG(paths.size() != 0, "No available paths.");
						std::cout<<"--"<<paths.size()<<std::endl;
						for(auto curPath : paths){
							curPath.push_back(curTreeNode.nodeId);
							m_pathDateBase[srcSatId][curTreeNode.nodeId].push_back(curPath);
						}
					}
				}

				std::vector<TreeNode> ft;
				std::vector<uint32_t> rec;
				for(auto tn: VHTemp){
					if(find(rec.begin(), rec.end(), tn.nodeId) == rec.end()){
						ft.push_back(tn);
						rec.push_back(tn.nodeId);
					}
				}
				MHBT.push_back(ft);

			}
			//std::cout<<m_pathDateBase[srcSatId].size()<<std::endl;
		}




	}

	// Select N capacity-aware optimal paths between each node pair
	//FilteringNPath(4);
	FilteringNPath_KSP(4);
	//FilteringNPath_symmetry(4);

//	// 2. Initialize link weights. Is it possible to optimize the lookup process, array? todo
//	std::vector<std::vector<uint32_t>> linkWeight(totalSats, std::vector<uint32_t>(totalSats, UINT32_MAX));
//	m_linkWeight = linkWeight;
//	std::unordered_map<uint32_t, std::vector<uint32_t>> adj = m_walkerConstellation->GetAdjacency();
//	for(auto links : adj){
//		NS_ASSERT_MSG(links.second.size() == 4, "Currently only inclined orbit constellations are supported.");
//		for(auto link : links.second){
//			//m_linkWeight[std::make_pair(links.first, link)] = 1;
//			m_linkWeight[links.first][link] = 1;
//		}
//	}
//	// 3. Select N capacity-aware optimal paths between each node pair
//	FilteringNPath(4);

}

//void
//MplbBuildRouting::FilteringNPath_symmetry (uint8_t nPath){
//
//	uint32_t satsPerOrbit = m_walkerConstellation->GetSatNum();
//	uint32_t orbits = m_walkerConstellation->GetOrbitNum();
//	uint32_t totalSats = satsPerOrbit * orbits;
//
//	// 2. Initialize link weights. Is it possible to optimize the lookup process, array? todo
//	std::vector<std::vector<uint32_t>> linkWeight(totalSats, std::vector<uint32_t>(totalSats, 0));
//	m_linkWeight = linkWeight;
//	std::unordered_map<uint32_t, std::vector<uint32_t>> adj = m_walkerConstellation->GetAdjacency();
//	for(auto links : adj){
//		//NS_ASSERT_MSG(links.second.size() == 4, "Currently only inclined orbit constellations are supported.");
//		for(auto link : links.second){
//			//m_linkWeight[std::make_pair(links.first, link)] = 1;
//			m_linkWeight[links.first][link] = 0;
//		}
//	}
//
//	// 3. Select N capacity-aware optimal paths between each node pair
//	for(auto src2all : m_pathDateBase){
//		src2all = m_pathDateBase[0];
//		for(auto paths2dst: src2all.second){
//			// for searching n path
//			for(uint8_t n = 0; n < nPath; n++){
//				uint32_t minpcost = UINT32_MAX;
//				std::vector<uint32_t> pathWithMinCost;
//				for(std::vector<uint32_t> path : paths2dst.second){
//					uint32_t pcost = 0;
//					// calculate weight
//					std::vector<uint32_t> tempv;
//					for(uint32_t s = 0; s < path.size() - 1; s++){
//						pcost += m_linkWeight[path[s]][path[s+1]];
//						//NS_ASSERT_MSG(m_linkWeight[path[s]][path[s+1]] < UINT32_MAX, "Wrong link adjacency.");
//						//m_linkWeight[path[s]][path[s+1]]++; // update weight
//						//tempv.push_back(m_linkWeight[path[s]][path[s+1]]);
//
//					}
//					//pcost = *max_element(tempv.begin(), tempv.end());
//					if(minpcost > pcost){
//						minpcost = pcost;
//						pathWithMinCost = path;
//					}
//				}
//				// update link weight
//				for(uint32_t s = 0; s < pathWithMinCost.size() - 1; s++){
//					m_linkWeight[pathWithMinCost[s]][pathWithMinCost[s+1]]++;
//				}
//				// select it
//				m_routeAllPairs[src2all.first][paths2dst.first].push_back(pathWithMinCost);
//			}
//		}
//		// Just calculate all the routes starting from one node, and copy them to other starting points.
//		uint32_t curNodesId = src2all.first;
//		for(uint32_t nodeId = 0; nodeId < totalSats; nodeId++){
//			if(nodeId == curNodesId){
//				continue;
//			}
//
//			for(auto paths2dstInAllInAll : m_routeAllPairs[curNodesId]){
//				//uint32_t dstNodeId = paths2dstInAllInAll.first;
//				for(auto path : paths2dstInAllInAll.second){
//					auto newPath = MappingPath(curNodesId, nodeId, path, satsPerOrbit, orbits);
//					m_routeAllPairs[nodeId][*(newPath.end()-1)].push_back(newPath);
//				}
//			}
//		}
//		break;
//
//	}
//
//
//
//	// 4. Print network and paths information for DRL training in a numerical environment
//	remove_file_if_exists("/home/xiaoyu/eclipseSAG/simulator/scratch/main_satnet/test_data/config_topology/MPLB_mapping.txt");
//	std::ofstream mplb_txt("/home/xiaoyu/eclipseSAG/simulator/scratch/main_satnet/test_data/config_topology/MPLB_mapping.txt");
//
//	// m, n
//	// route1
//	// ...
//	// route2
//
//	mplb_txt<<satsPerOrbit<<","<<orbits<<std::endl;
//
//	for(auto src2all : m_routeAllPairs){
//		NS_ASSERT_MSG(m_routeAllPairs.size() == totalSats, "Wrong route.");
//		//mplb_txt<<src2all.first<<std::endl;
//
//		for(auto paths2dst: src2all.second){
//			NS_ASSERT_MSG(src2all.second.size() == totalSats - 1, "Wrong route.");
//			//uint32_t dstId = paths2dst.first;
//			std::vector<std::vector<uint32_t>> paths = paths2dst.second;
//			NS_ASSERT_MSG(paths.size() == nPath, "Wrong route.");
//
//			//mplb_txt<<src2all.first<<","<<dstId<<std::endl;
//
//			for(auto path : paths){
//				for(uint32_t s = 0; s < path.size() - 1; s++){
//					uint32_t curSat = path[s];
//					mplb_txt<<curSat<<",";
//				}
//				mplb_txt<<path[path.size() - 1]<<std::endl;
//			}
//		}
//	}
//	mplb_txt.close();
//
//	std::cout<<"Finished"<<std::endl;
//
//}




void
MplbBuildRouting::FilteringNPath_symmetry (uint8_t nPath){


//    // record propagation delay
//    remove_file_if_exists("/home/xiaoyuliu/sag_platform_v3/bake/source/ns-3.35/scratch/main_satnet/test_data/config_topology/propagation_delay.txt");
//    std::ofstream fileProp("/home/xiaoyuliu/sag_platform_v3/bake/source/ns-3.35/scratch/main_satnet/test_data/config_topology/propagation_delay.txt");
//
//    auto nodes = m_walkerConstellation->GetNodes();
//    for(auto nbs: m_walkerConstellation->GetAdjacency()){
//    	Ptr<Node> me = nodes.Get(nbs.first);
//    	for(auto nb: nbs.second){
//    		Ptr<Node> neighbor = nodes.Get(nb);
//    		Ptr<MobilityModel> dstMobility = neighbor->GetObject<MobilityModel>();
//    		Ptr<MobilityModel> myMobility = me->GetObject<MobilityModel>();
//    		double delay = myMobility->GetDistanceFrom(dstMobility) / (3 * 1e5);
//    		fileProp<<nbs.first<<","<<nb<<","<<delay<<std::endl;
//    	}
//    }
//    fileProp.close();



	uint32_t satsPerOrbit = m_walkerConstellation->GetSatNum();
	uint32_t orbits = m_walkerConstellation->GetOrbitNum();
	uint32_t totalSats = satsPerOrbit * orbits;

	// 2. Initialize link weights. Is it possible to optimize the lookup process, array? todo
	std::vector<std::vector<uint32_t>> linkWeight(totalSats, std::vector<uint32_t>(totalSats, 0));
	m_linkWeight = linkWeight;
	std::unordered_map<uint32_t, std::vector<uint32_t>> adj = m_walkerConstellation->GetAdjacency();
	for(auto links : adj){
		//NS_ASSERT_MSG(links.second.size() == 4, "Currently only inclined orbit constellations are supported.");
		for(auto link : links.second){
			//m_linkWeight[std::make_pair(links.first, link)] = 1;
			m_linkWeight[links.first][link] = 0;
		}
	}

	// 3. Select N capacity-aware optimal paths between each node pair

//	auto src2all = m_pathDateBase[0];
//	for(auto paths2dst: src2all){
//		// for searching n path
//		for(uint8_t n = 0; n < nPath; n++){
//			uint32_t minpcost = UINT32_MAX;
//			std::vector<uint32_t> pathWithMinCost;
//			for(std::vector<uint32_t> path : paths2dst.second){
//				uint32_t pcost = 0;
//				// calculate weight
//				std::vector<uint32_t> tempv;
//				for(uint32_t s = 0; s < path.size() - 1; s++){
//					pcost += m_linkWeight[path[s]][path[s+1]];
//					//NS_ASSERT_MSG(m_linkWeight[path[s]][path[s+1]] < UINT32_MAX, "Wrong link adjacency.");
//					//m_linkWeight[path[s]][path[s+1]]++; // update weight
//					//tempv.push_back(m_linkWeight[path[s]][path[s+1]]);
//
//				}
//				//pcost = *max_element(tempv.begin(), tempv.end());
//				if(minpcost > pcost){
//					minpcost = pcost;
//					pathWithMinCost = path;
//				}
//			}
//			// update link weight
//			if(find(m_nodes.begin(), m_nodes.end(), pathWithMinCost[0]) != m_nodes.end()
//					&& find(m_nodes.begin(), m_nodes.end(), pathWithMinCost[pathWithMinCost.size() - 1]) != m_nodes.end()){
//				for(uint32_t s = 0; s < pathWithMinCost.size() - 1; s++){
//					m_linkWeight[pathWithMinCost[s]][pathWithMinCost[s+1]]++;
//				}
//			}
//			// select it
//			m_routeAllPairs[0][paths2dst.first].push_back(pathWithMinCost);
//		}
//	}
//	// Just calculate all the routes starting from one node, and copy them to other starting points.
//	uint32_t curNodesId = 0;
//	for(uint32_t nodeId = 0; nodeId < totalSats; nodeId++){
//		if(nodeId == curNodesId){
//			continue;
//		}
//
//		for(auto paths2dstInAllInAll : m_routeAllPairs[curNodesId]){
//			//uint32_t dstNodeId = paths2dstInAllInAll.first;
//			for(auto path : paths2dstInAllInAll.second){
//				auto newPath = MappingPath(curNodesId, nodeId, path, satsPerOrbit, orbits);
//				m_routeAllPairs[nodeId][*(newPath.end()-1)].push_back(newPath);
//				// update link weight
//				if(find(m_nodes.begin(), m_nodes.end(), nodeId) != m_nodes.end()
//						&& find(m_nodes.begin(), m_nodes.end(),*(newPath.end()-1)) != m_nodes.end()){
//					for(uint32_t s = 0; s < newPath.size() - 1; s++){
//						m_linkWeight[newPath[s]][newPath[s+1]]++;
//					}
//				}
//			}
//		}
//	}
//
//	m_initialLinkWeight = m_linkWeight;


	auto src2all = m_pathDateBase[0];
	for(uint32_t src: m_nodes){
		for(uint32_t dst: m_nodes){
			if(src == dst){
				continue;
			}
			uint32_t olddst = MappingNodePair(0, src, dst, satsPerOrbit, orbits);
			// for searching n path
			for(uint8_t n = 0; n < nPath; n++){
				uint32_t minpcost = UINT32_MAX;
				std::vector<uint32_t> pathWithMinCost;
				for(std::vector<uint32_t> path : src2all[olddst]){
					auto newPath = MappingPath(0, src, path, satsPerOrbit, orbits);

					uint32_t pcost = 0;
					// calculate weight
					std::vector<uint32_t> tempv;
					for(uint32_t s = 0; s < newPath.size() - 1; s++){
						pcost += m_linkWeight[newPath[s]][newPath[s+1]];
						//NS_ASSERT_MSG(m_linkWeight[path[s]][path[s+1]] < UINT32_MAX, "Wrong link adjacency.");
						//m_linkWeight[path[s]][path[s+1]]++; // update weight
						//tempv.push_back(m_linkWeight[path[s]][path[s+1]]);

					}
					//pcost = *max_element(tempv.begin(), tempv.end());
					if(minpcost > pcost){
						minpcost = pcost;
						pathWithMinCost = newPath;
					}
				}
				// update link weight
				if(pathWithMinCost[pathWithMinCost.size() - 1] != dst){
					throw std::runtime_error ("pathWithMinCost[pathWithMinCost.size() - 1] != dst.");
				}
				for(uint32_t s = 0; s < pathWithMinCost.size() - 1; s++){
					m_linkWeight[pathWithMinCost[s]][pathWithMinCost[s+1]]++;
				}
				// select it
				m_routeAllPairs[src][dst].push_back(pathWithMinCost);
			}


		}
	}


	m_initialLinkWeight = m_linkWeight;








	// 4. Print network and paths information for DRL training in a numerical environment
	remove_file_if_exists("/home/xiaoyuliu/sag_platform_v3/bake/source/ns-3.35/scratch/main_satnet/test_data/flexsate/MPLB_mapping_"+std::to_string(0)+".txt");
	std::ofstream mplb_txt("/home/xiaoyuliu/sag_platform_v3/bake/source/ns-3.35/scratch/main_satnet/test_data/flexsate/MPLB_mapping_"+std::to_string(0)+".txt");

	// m, n
	// route1
	// ...
	// route2

	mplb_txt<<satsPerOrbit<<","<<orbits<<std::endl;

	for(auto src2all : m_routeAllPairs){
		NS_ASSERT_MSG(m_routeAllPairs.size() == m_nodes.size(), "Wrong route.");
//		if(find(m_nodes.begin(), m_nodes.end(), src2all.first) == m_nodes.end()){
//			continue;
//		}
		//mplb_txt<<src2all.first<<std::endl;

		for(auto paths2dst: src2all.second){
			NS_ASSERT_MSG(src2all.second.size() == m_nodes.size() - 1, "Wrong route.");
//			if(find(m_nodes.begin(), m_nodes.end(), paths2dst.first) == m_nodes.end()){
//				continue;
//			}
			//uint32_t dstId = paths2dst.first;
			std::vector<std::vector<uint32_t>> paths = paths2dst.second;
			NS_ASSERT_MSG(paths.size() == nPath, "Wrong route.");

			//mplb_txt<<src2all.first<<","<<dstId<<std::endl;
			m_initialRouteAllPairsFlexSATE[src2all.first][paths2dst.first] = paths;
			for(auto path : paths){
				for(uint32_t s = 0; s < path.size() - 1; s++){
					m_link2Routes[std::make_pair(path[s], path[s + 1])].insert(std::make_pair(path[0], path[path.size() - 1]));
					uint32_t curSat = path[s];
					mplb_txt<<curSat<<",";
				}
				mplb_txt<<path[path.size() - 1]<<std::endl;
			}
		}
	}
	mplb_txt.close();

	std::cout<<"Finished"<<std::endl;


}

std::vector<uint32_t>
MplbBuildRouting::MappingPath(uint32_t oldNode, uint32_t newNode, std::vector<uint32_t> path, uint32_t satsPerOrbit, uint32_t orbits){
	uint32_t orbitNumberOldNode = oldNode / satsPerOrbit;
	uint32_t satNumberOldNode = oldNode % satsPerOrbit;
	uint32_t orbitNumberNewNode = newNode / satsPerOrbit;
	uint32_t satNumberNewNode = newNode % satsPerOrbit;
	bool x;
	bool y;
	uint32_t x_hop;
	uint32_t y_hop;
	if(orbitNumberNewNode < orbitNumberOldNode){
		x = false;
	}
	else{
		x = true;
	}
	if(satNumberNewNode < satNumberOldNode){
		y = false;
	}
	else{
		y = true;
	}
	x_hop = std::abs(int(orbitNumberOldNode - orbitNumberNewNode));
	if(x_hop > orbits / 2){
		x_hop = orbits - x_hop;
		x = not x;
	}
	y_hop = std::abs(int(satNumberOldNode - satNumberNewNode));
	if(y_hop > satsPerOrbit / 2){
		y_hop = satsPerOrbit - y_hop;
		y = not y;
	}
	std::vector<uint32_t> mapping_path;
	for(auto passNodeId : path){
		// update passNodeId
		uint32_t orbitNumberPassNode = passNodeId / satsPerOrbit;
		uint32_t satNumberPassNode = passNodeId % satsPerOrbit;
		uint32_t orbitNumberPassNodeNew;
		uint32_t satNumberPassNodeNew;
		if(x){
			orbitNumberPassNodeNew = (orbitNumberPassNode + x_hop) % orbits;
		}
		else{
			orbitNumberPassNodeNew = (orbits + orbitNumberPassNode - x_hop) % orbits;
		}
		if(y){
			satNumberPassNodeNew = (satNumberPassNode + y_hop) % satsPerOrbit;
		}
		else{
			satNumberPassNodeNew = (satsPerOrbit + satNumberPassNode - y_hop) % satsPerOrbit;
		}
		mapping_path.push_back(orbitNumberPassNodeNew * satsPerOrbit + satNumberPassNodeNew);

	}
	return mapping_path;
}

uint32_t
MplbBuildRouting::MappingNodePair(uint32_t oldsrc, uint32_t src, uint32_t dst, uint32_t satsPerOrbit, uint32_t orbits){
	uint32_t orbitNumberOldNode = oldsrc / satsPerOrbit;
	uint32_t satNumberOldNode = oldsrc % satsPerOrbit;
	uint32_t orbitNumbersrc = src / satsPerOrbit;
	uint32_t satNumbersrc = src % satsPerOrbit;
	bool x;
	bool y;
	uint32_t x_hop;
	uint32_t y_hop;
	if(orbitNumbersrc < orbitNumberOldNode){
		x = false;
	}
	else{
		x = true;
	}
	if(satNumbersrc < satNumberOldNode){
		y = false;
	}
	else{
		y = true;
	}
	x_hop = std::abs(int(orbitNumberOldNode - orbitNumbersrc));
	if(x_hop > orbits / 2){
		x_hop = orbits - x_hop;
		x = not x;
	}
	y_hop = std::abs(int(satNumberOldNode - satNumbersrc));
	if(y_hop > satsPerOrbit / 2){
		y_hop = satsPerOrbit - y_hop;
		y = not y;
	}


	// update dst
	uint32_t orbitNumberdst = dst / satsPerOrbit;
	uint32_t satNumberdst = dst % satsPerOrbit;
	uint32_t orbitNumberPassNodeNew;
	uint32_t satNumberPassNodeNew;
	if(!x){
		orbitNumberPassNodeNew = (orbitNumberdst + x_hop) % orbits;
	}
	else{
		orbitNumberPassNodeNew = (orbits + orbitNumberdst - x_hop) % orbits;
	}
	if(!y){
		satNumberPassNodeNew = (satNumberdst + y_hop) % satsPerOrbit;
	}
	else{
		satNumberPassNodeNew = (satsPerOrbit + satNumberdst - y_hop) % satsPerOrbit;
	}
	//mapping_path.push_back(orbitNumberPassNodeNew * satsPerOrbit + satNumberPassNodeNew);


	return orbitNumberPassNodeNew * satsPerOrbit + satNumberPassNodeNew;
}

void
MplbBuildRouting::FilteringNPath(uint8_t nPath){

	uint32_t satsPerOrbit = m_walkerConstellation->GetSatNum();
	uint32_t orbits = m_walkerConstellation->GetOrbitNum();
	uint32_t totalSats = satsPerOrbit * orbits;


//    // record propagation delay
//    remove_file_if_exists("/home/xiaoyuliu/sag_platform_v3/bake/source/ns-3.35/scratch/main_satnet/test_data/config_topology/propagation_delay.txt");
//    std::ofstream fileProp("/home/xiaoyuliu/sag_platform_v3/bake/source/ns-3.35/scratch/main_satnet/test_data/config_topology/propagation_delay.txt");
//
    auto nodes = m_walkerConstellation->GetNodes();
//    for(auto nbs: m_walkerConstellation->GetAdjacency()){
//    	Ptr<Node> me = nodes.Get(nbs.first);
//    	for(auto nb: nbs.second){
//    		Ptr<Node> neighbor = nodes.Get(nb);
//    		Ptr<MobilityModel> dstMobility = neighbor->GetObject<MobilityModel>();
//    		Ptr<MobilityModel> myMobility = me->GetObject<MobilityModel>();
//    		double delay = myMobility->GetDistanceFrom(dstMobility) / (3 * 1e5);
//    		fileProp<<nbs.first<<","<<nb<<","<<delay<<std::endl;
//    	}
//    }
//    fileProp.close();





	// 2. Initialize link weights. Is it possible to optimize the lookup process, array? todo
	std::vector<std::vector<uint32_t>> linkWeight(totalSats, std::vector<uint32_t>(totalSats, UINT32_MAX));
	m_linkWeight = linkWeight;
	std::unordered_map<uint32_t, std::vector<uint32_t>> adj = m_walkerConstellation->GetAdjacency();
	for(auto links : adj){
		//NS_ASSERT_MSG(links.second.size() == 4, "Currently only inclined orbit constellations are supported.");
		for(auto link : links.second){
			//m_linkWeight[std::make_pair(links.first, link)] = 1;
			m_linkWeight[links.first][link] = 1;
		}
	}

	// 3. Select N capacity-aware optimal paths between each node pair
	for(auto src2all : m_pathDateBase){
		for(auto paths2dst: src2all.second){
			// for searching n path
			for(uint8_t n = 0; n < nPath; n++){
				uint32_t minpcost = UINT32_MAX;
				std::vector<uint32_t> pathWithMinCost;
				for(std::vector<uint32_t> path : paths2dst.second){
					uint32_t pcost = 0;
					// calculate weight
					std::vector<uint32_t> tempv;
					for(uint32_t s = 0; s < path.size() - 1; s++){
						//pcost += m_linkWeight[path[s]][path[s+1]];
						//NS_ASSERT_MSG(m_linkWeight[path[s]][path[s+1]] < UINT32_MAX, "Wrong link adjacency.");
						//m_linkWeight[path[s]][path[s+1]]++; // update weight
						tempv.push_back(m_linkWeight[path[s]][path[s+1]]);

					}
					pcost = *max_element(tempv.begin(), tempv.end());
					if(minpcost > pcost){
						minpcost = pcost;
						pathWithMinCost = path;
					}
				}
				// update link weight
				for(uint32_t s = 0; s < pathWithMinCost.size() - 1; s++){
					m_linkWeight[pathWithMinCost[s]][pathWithMinCost[s+1]]++;
				}
				// select it
				m_routeAllPairs[src2all.first][paths2dst.first].push_back(pathWithMinCost);
			}
		}
	}


	// 4. Print network and paths information for DRL training in a numerical environment
	remove_file_if_exists("/home/xiaoyuliu/sag_platform_v3/bake/source/ns-3.35/scratch/main_satnet/test_data/config_topology/MPLB.txt");
	std::ofstream mplb_txt("/home/xiaoyuliu/sag_platform_v3/bake/source/ns-3.35/scratch/main_satnet/test_data/config_topology/MPLB.txt");

	// m, n
	// route1
	// ...
	// route2

	mplb_txt<<satsPerOrbit<<","<<orbits<<std::endl;

	for(auto src2all : m_routeAllPairs){
		NS_ASSERT_MSG(m_routeAllPairs.size() == totalSats, "Wrong route.");
		//mplb_txt<<src2all.first<<std::endl;

		for(auto paths2dst: src2all.second){
			NS_ASSERT_MSG(src2all.second.size() == totalSats - 1, "Wrong route.");
			//uint32_t dstId = paths2dst.first;
			std::vector<std::vector<uint32_t>> paths = paths2dst.second;
			NS_ASSERT_MSG(paths.size() == nPath, "Wrong route.");

			//mplb_txt<<src2all.first<<","<<dstId<<std::endl;

			for(auto path : paths){
				for(uint32_t s = 0; s < path.size() - 1; s++){
					uint32_t curSat = path[s];
					mplb_txt<<curSat<<",";
				}
				mplb_txt<<path[path.size() - 1]<<std::endl;
			}
		}
	}
	mplb_txt.close();

	std::cout<<"Finished"<<std::endl;

}

void
MplbBuildRouting::FilteringNPath_KSP(uint8_t nPath){

	uint32_t satsPerOrbit = m_walkerConstellation->GetSatNum();
	uint32_t orbits = m_walkerConstellation->GetOrbitNum();
	uint32_t totalSats = satsPerOrbit * orbits;


	// 2. Randomly select the top K shortest paths
	auto src2all = m_pathDateBase[0];
	for(auto paths2dst: src2all){
		//std::cout<<src2all.first<<"  "<<paths2dst.first<<std::endl;
		// for searching n path
		if(paths2dst.second.size() > nPath){
			uint32_t a = 0;
			uint32_t b = paths2dst.second.size();
			std::vector<uint32_t> pathsIndex;
			while(pathsIndex.size() < nPath){
				uint32_t index = rand() % (b-a) + a;
				if(find(pathsIndex.begin(), pathsIndex.end(), index) == pathsIndex.end()){
					pathsIndex.push_back(index);
				}
				//std::cout<<src2all.first<<"  "<<paths2dst.first<<std::endl;
			}
			for(auto indx : pathsIndex){
				m_routeAllPairs[0][paths2dst.first].push_back(paths2dst.second[indx]);
			}
		}
		else if(paths2dst.second.size() == nPath){
			m_routeAllPairs[0][paths2dst.first] = paths2dst.second;
		}
		else{
			std::vector<uint32_t> pathsIndex;
			uint32_t i = 0;
			while(pathsIndex.size() < nPath){
				pathsIndex.push_back(i % paths2dst.second.size());
				i++;
			}
			for(auto indx : pathsIndex){
				m_routeAllPairs[0][paths2dst.first].push_back(paths2dst.second[indx]);
			}
		}
	}

	// Just calculate all the routes starting from one node, and copy them to other starting points.
	uint32_t curNodesId = 0;
	for(uint32_t nodeId = 0; nodeId < totalSats; nodeId++){
		if(nodeId == curNodesId){
			continue;
		}

		for(auto paths2dstInAllInAll : m_routeAllPairs[curNodesId]){
			//uint32_t dstNodeId = paths2dstInAllInAll.first;
			for(auto path : paths2dstInAllInAll.second){
				auto newPath = MappingPath(curNodesId, nodeId, path, satsPerOrbit, orbits);
				m_routeAllPairs[nodeId][*(newPath.end()-1)].push_back(newPath);
			}
		}
	}


	// 3. Print network and paths information for DRL training in a numerical environment
	remove_file_if_exists("/home/xiaoyuliu/sag_platform_v3/bake/source/ns-3.35/scratch/main_satnet/test_data/config_topology/MPLB_KSP.txt");
	std::ofstream mplb_txt("/home/xiaoyuliu/sag_platform_v3/bake/source/ns-3.35/scratch/main_satnet/test_data/config_topology/MPLB_KSP.txt");

	// m, n
	// route1
	// ...
	// route2

	mplb_txt<<satsPerOrbit<<","<<orbits<<std::endl;

	for(auto src2all : m_routeAllPairs){
		NS_ASSERT_MSG(m_routeAllPairs.size() == totalSats, "Wrong route.");
		//mplb_txt<<src2all.first<<std::endl;

		for(auto paths2dst: src2all.second){
			NS_ASSERT_MSG(src2all.second.size() == totalSats - 1, "Wrong route.");
			//uint32_t dstId = paths2dst.first;
			std::vector<std::vector<uint32_t>> paths = paths2dst.second;
			NS_ASSERT_MSG(paths.size() == nPath, "Wrong route.");

			//mplb_txt<<src2all.first<<","<<dstId<<std::endl;

			for(auto path : paths){
				for(uint32_t s = 0; s < path.size() - 1; s++){
					m_link2Routes[std::make_pair(path[s], path[s + 1])].insert(std::make_pair(path[0], path[path.size() - 1]));
					uint32_t curSat = path[s];
					mplb_txt<<curSat<<",";
				}
				mplb_txt<<path[path.size() - 1]<<std::endl;
			}
		}
	}
	mplb_txt.close();

	std::cout<<"Finished"<<std::endl;

}

void
MplbBuildRouting::UpdateRouteSegment (){
	uint8_t nPath = m_nPath;
	std::string al = m_filtering; // 'ksp' 'symmetry'
	uint32_t segmentRouting = m_segments;
	uint32_t satsPerOrbit = m_walkerConstellation->GetSatNum();
	uint32_t orbits = m_walkerConstellation->GetOrbitNum();
	uint32_t phaseFactor = m_walkerConstellation->GetPhase();
	uint32_t totalSats = satsPerOrbit * orbits;

	bool part_network = true;
	m_adjacency = m_walkerConstellation->GetAdjacency();

	std::vector<uint32_t> nodes; // 6*10 产生流量的节点
	if(part_network){
		// part network, noting if no use
		//std::vector<uint32_t> nodes; // 6*10 产生流量的节点
		uint32_t ort = int(orbits / 5.1); // 6:6  8:4.5  7:5.1
		uint32_t satp = int(satsPerOrbit / 1.8);// 10:2  12:1.6   11:1.8
		for(uint32_t k1 = 0; k1 < ort; k1++){
			for(uint32_t k2 = 0; k2 < satp; k2++){
				nodes.push_back(k1*satsPerOrbit + k2);
			}
		}
		for(uint32_t k2 = 0; k2 < satp; k2++){
			//nodes.push_back(35*satsPerOrbit + k2); // 8 12
			//nodes.push_back(34*satsPerOrbit + k2);
		}
		for(uint32_t k1 = 0; k1 < ort; k1++){
			//nodes.push_back(k1*satsPerOrbit + 19); // 8 12
			//nodes.push_back(k1*satsPerOrbit + 18);
		}

		for(auto iter = m_adjacency.begin(); iter != m_adjacency.end();){
			if(find(nodes.begin(), nodes.end(), (*iter).first) == nodes.end()){
				iter = m_adjacency.erase(iter);
			}
			else{
				std::vector<uint32_t> temp_nodes;
				for(auto n : (*iter).second){
					if(find(nodes.begin(), nodes.end(), n) != nodes.end()){
						temp_nodes.push_back(n);
					}
				}
				(*iter).second = temp_nodes;
				iter++;
			}
		}
	}
	else{
		for(uint32_t sat = 0; sat < totalSats; sat++){
			nodes.push_back(sat);
		}
	}
	m_nodes = nodes;
	m_initialAdjacency = m_adjacency;




	// 1. construct a minimum-hop binary tree (MHBT)

	NS_ASSERT_MSG(phaseFactor == 0, "Currently only supports 0 phase offset.");

	for(uint32_t srcSatId = 0; srcSatId < totalSats; srcSatId++){
		// source id in CG
		uint32_t orbitNumber = srcSatId / satsPerOrbit;
		uint32_t satNumber = srcSatId % satsPerOrbit;

		// calculate four destination ids: (orbit number, sat number)
		std::pair<uint32_t, uint32_t> dstSatIdRightTop, dstSatIdRightBottom;
		std::pair<uint32_t, uint32_t> dstSatIdLeftTop, dstSatIdLeftBottom;
		// record minimum hop
		uint32_t minHopRightTop, minHopRightBottom;
		uint32_t minHopLeftTop, minHopLeftBottom;
		// hop count in two direction: (x, y)
		std::pair<uint32_t, uint32_t> hopRightTop, hopRightBottom;
		std::pair<uint32_t, uint32_t> hopLeftTop, hopLeftBottom;

		uint32_t orbitFloor = floor(orbits / 2.0);
		uint32_t orbitCeil = ceil(orbits / 2.0);
		uint32_t satFloor = floor(satsPerOrbit / 2.0);
		uint32_t satCeil = ceil(satsPerOrbit / 2.0);
		dstSatIdRightTop = std::make_pair((orbitNumber + orbitFloor) % orbits, (satNumber + satFloor) % satsPerOrbit);
		minHopRightTop = orbitFloor + satFloor;
		hopRightTop = std::make_pair(orbitFloor, satFloor);

		dstSatIdRightBottom = std::make_pair((orbitNumber + orbitFloor) % orbits, (satNumber + satCeil) % satsPerOrbit);
		minHopRightBottom = orbitFloor + satFloor;
		hopRightBottom = std::make_pair(orbitFloor, satFloor);

		dstSatIdLeftTop = std::make_pair((orbitNumber + orbitCeil) % orbits, (satNumber + satFloor) % satsPerOrbit);
		minHopLeftTop = orbitFloor + satFloor;
		hopLeftTop = std::make_pair(orbitFloor, satFloor);

		dstSatIdLeftBottom = std::make_pair((orbitNumber + orbitCeil) % orbits, (satNumber + satCeil) % satsPerOrbit);
		minHopLeftBottom = orbitFloor + satFloor;
		hopLeftBottom = std::make_pair(orbitFloor, satFloor);

		if(dstSatIdRightTop.first == dstSatIdLeftTop.first){
			dstSatIdLeftTop.first = (dstSatIdLeftTop.first + 1) % orbits;
			dstSatIdLeftBottom.first = dstSatIdLeftTop.first;
			minHopLeftTop--;
			minHopLeftBottom--;
			hopLeftTop.first = hopLeftTop.first - 1;
			hopLeftBottom.first = hopLeftBottom.first - 1;
		}
		if(dstSatIdRightTop.second == dstSatIdRightBottom.second){
			dstSatIdRightBottom.second = (dstSatIdRightBottom.second + 1) % satsPerOrbit;
			dstSatIdLeftBottom.second = dstSatIdRightBottom.second;
			minHopRightBottom--;
			minHopLeftBottom--;
			hopRightBottom.second = hopRightBottom.second -1;
			hopLeftBottom.second = hopLeftBottom.second - 1;
		}
		std::vector<std::pair<uint32_t, uint32_t>> regions = {dstSatIdRightTop, dstSatIdRightBottom, dstSatIdLeftTop, dstSatIdLeftBottom};
		std::vector<uint32_t> minHops = {minHopRightTop, minHopRightBottom, minHopLeftTop, minHopLeftBottom};
		std::vector<std::pair<uint32_t, uint32_t>> minHopInTwoDirection = {hopRightTop, hopRightBottom, hopLeftTop, hopLeftBottom};
		// moving direction
		std::vector<std::pair<int32_t, int32_t>> directions = {std::make_pair(1,1), std::make_pair(1,-1), std::make_pair(-1,1), std::make_pair(-1,-1)};

		// four minimum hop path (MHP) region
		for(uint8_t i = 0; i < 4; i++){
			uint32_t H_T = minHops[i] + 1; // the height of the tree
			uint32_t H_x = minHopInTwoDirection[i].first;
			uint32_t H_y = minHopInTwoDirection[i].second;
			int32_t Direction_x = directions[i].first;
			int32_t Direction_y = directions[i].second;

			// source tree node
			std::vector<std::vector<TreeNode>> MHBT;
			TreeNode srcTreeNode(1, srcSatId, H_x, H_y, std::make_pair(orbitNumber, satNumber), std::make_pair(orbitNumber, satNumber), nullptr);
			std::vector<TreeNode> VH = {srcTreeNode};
			MHBT.push_back(VH);

			// for each level in MHBT
			for(uint32_t h = 2; h <= H_T; h++){
				std::cout<<"Level: "<<h<<std::endl;
				std::vector<TreeNode> VHTemp;
				uint32_t j = 0;
				// for each tree node in higher level
				for(uint32_t k = 0; k < MHBT[h-2].size(); k++){
					TreeNode treeNode = MHBT[h-2][k];
					std::pair<uint32_t, uint32_t> lastNode = treeNode.coordinate;
					if(treeNode.xHopCount > 0){
						j++;
						std::pair<uint32_t, uint32_t> curNode;
						if(Direction_x == 1){
							curNode = std::make_pair((lastNode.first + 1) % orbits, lastNode.second);
						}
						else{
							curNode = std::make_pair((lastNode.first + orbits - 1) % orbits, lastNode.second);
						}
						uint32_t satId = curNode.first * satsPerOrbit + curNode.second;
						TreeNode curTreeNode(h, satId, treeNode.xHopCount - 1, treeNode.yHopCount, curNode, lastNode, &MHBT[h-2][k], std::make_pair(1, 0));
						VHTemp.push_back(curTreeNode);
					}
					if(treeNode.yHopCount > 0){
						j++;
						std::pair<uint32_t, uint32_t> curNode;
						if(Direction_y == 1){
							curNode = std::make_pair(lastNode.first, (lastNode.second + 1) % satsPerOrbit);
						}
						else{
							curNode = std::make_pair(lastNode.first, (lastNode.second + satsPerOrbit - 1) % satsPerOrbit);
						}
						uint32_t satId = curNode.first * satsPerOrbit + curNode.second;
						TreeNode curTreeNode(h, satId, treeNode.xHopCount, treeNode.yHopCount - 1, curNode, lastNode, &MHBT[h-2][k], std::make_pair(0, 1));
						VHTemp.push_back(curTreeNode);
					}
				}
				VH = VHTemp;
				std::cout<<"TreeNodes: "<<VHTemp.size()<<std::endl;
				// MHBT.push_back(VHTemp);

				// calculate all path
				std::unordered_map<uint32_t, std::vector<uint32_t>> record;
				for(auto curTreeNode : VHTemp){
					//std::cout<<h<<std::endl;
					// Duplicate paths are not re-added if they are already included in other regions.
					if(curTreeNode.coordinate.first == srcTreeNode.coordinate.first || curTreeNode.coordinate.second == srcTreeNode.coordinate.second){
						auto iter = m_pathDateBase[srcSatId].find(curTreeNode.nodeId);
						if(iter != m_pathDateBase[srcSatId].end()){
							continue;
						}
					}
					// Add paths
					if(curTreeNode.fatherTreeNode->nodeId == srcSatId){
						std::vector<uint32_t> path;
						path.push_back(srcSatId);
						path.push_back(curTreeNode.nodeId);
						m_pathDateBaseSegmentNumber[srcSatId][curTreeNode.nodeId].push_back(1);
						m_pathDateBaseSegmentDirection[srcSatId][curTreeNode.nodeId].push_back(curTreeNode.direction);
						m_pathDateBase[srcSatId][curTreeNode.nodeId].push_back(path);
					}
					else{
						auto iter = find(record[curTreeNode.fatherTreeNode->nodeId].begin(), record[curTreeNode.fatherTreeNode->nodeId].end(), curTreeNode.nodeId);
						if(iter != record[curTreeNode.fatherTreeNode->nodeId].end()){
							//std::cout<<"1"<<std::endl;
							continue;
						}
						record[curTreeNode.fatherTreeNode->nodeId].push_back(curTreeNode.nodeId);
						std::vector<std::vector<uint32_t>> paths = m_pathDateBase[srcSatId][curTreeNode.fatherTreeNode->nodeId];
						std::vector<uint32_t> segments = m_pathDateBaseSegmentNumber[srcSatId][curTreeNode.fatherTreeNode->nodeId];
						std::vector<std::pair<uint8_t, uint8_t>> directons = m_pathDateBaseSegmentDirection[srcSatId][curTreeNode.fatherTreeNode->nodeId];
						//std::pair<uint8_t, uint8_t> directon = curTreeNode.fatherTreeNode->direction;
						NS_ASSERT_MSG(directons.size() == paths.size(), "Wrong.");
						//std::cout<<paths.size()<<std::endl;
						for(uint32_t p = 0; p < paths.size(); p++){
							auto directon = directons[p];
							auto curPath = paths[p];
							uint32_t seg = segments[p];
							if(directon == curTreeNode.direction){
								if(seg > segmentRouting){
									continue;
								}
								curPath.push_back(curTreeNode.nodeId);
								m_pathDateBase[srcSatId][curTreeNode.nodeId].push_back(curPath);
								m_pathDateBaseSegmentNumber[srcSatId][curTreeNode.nodeId].push_back(seg);
								m_pathDateBaseSegmentDirection[srcSatId][curTreeNode.nodeId].push_back(curTreeNode.direction);
							}
							else{
								if(seg + 1 > segmentRouting){
									continue;
								}
								curPath.push_back(curTreeNode.nodeId);
								m_pathDateBase[srcSatId][curTreeNode.nodeId].push_back(curPath);
								m_pathDateBaseSegmentNumber[srcSatId][curTreeNode.nodeId].push_back(seg + 1);
								m_pathDateBaseSegmentDirection[srcSatId][curTreeNode.nodeId].push_back(curTreeNode.direction);


							}

						}

//						NS_ASSERT_MSG(paths.size() != 0, "No available paths.");
//						std::cout<<"--"<<paths.size()<<std::endl;
//						for(auto curPath : paths){
//							curPath.push_back(curTreeNode.nodeId);
//							m_pathDateBase[srcSatId][curTreeNode.nodeId].push_back(curPath);
//						}
					}
				}

				std::vector<TreeNode> ft;
				std::vector<uint32_t> rec;
				for(auto tn: VHTemp){
					if(find(rec.begin(), rec.end(), tn.nodeId) == rec.end()){
						ft.push_back(tn);
						rec.push_back(tn.nodeId);
					}
				}
				MHBT.push_back(ft);

			}
			std::cout<<m_pathDateBase[srcSatId].size()<<std::endl;
		}


		break;

	}

	// Select N capacity-aware optimal paths between each node pair
	//FilteringNPath(nPath);
	if(al == "symmetry"){
		FilteringNPath_symmetry(nPath); // 'ksp' 'symmetry'
	}
	else if(al == "ksp"){
		FilteringNPath_KSP(nPath);
	}

}

bool MplbBuildRouting::UpdateRouteSegment (uint32_t curLinkDownNumber, uint32_t sampleIndex){
	uint8_t nPath = m_nPath;
	//bool part_network = true;


	uint32_t satsPerOrbit = m_walkerConstellation->GetSatNum();
	uint32_t orbits = m_walkerConstellation->GetOrbitNum();
	uint32_t phaseFactor = m_walkerConstellation->GetPhase();
	//uint32_t totalSats = satsPerOrbit * orbits;
	m_adjacency = m_initialAdjacency;

	NS_ASSERT_MSG(phaseFactor == 0, "Currently only supports 0 phase offset.");

    std::vector<uint32_t> nodes = m_nodes;
    std::vector<std::pair<uint32_t, uint32_t>> links;
    uint32_t tempNodeId = 0;
    if(!UpdateAdjacency(tempNodeId, tempNodeId + satsPerOrbit)){
    	exit(1);
    }
    links.push_back(std::make_pair(tempNodeId, tempNodeId + satsPerOrbit));

    uint32_t linksSelected = 1;
    std::srand(10*sampleIndex);
    while(linksSelected != curLinkDownNumber){
		// [a,b]
		uint32_t a = 0;
		uint32_t b = nodes.size() - 1;
		uint32_t randoxNumber = a + rand() % ( b -a +1 ) ;
		uint32_t curnode = nodes[randoxNumber];
		std::cout<<a<<" "<<b<<" randoxNumber: "<<randoxNumber<<std::endl;
		if(find(links.begin(), links.end(), std::make_pair(curnode, curnode + satsPerOrbit)) == links.end()){
			bool find = UpdateAdjacency(curnode, curnode + satsPerOrbit);
			if(find){
				links.push_back(std::make_pair(curnode, curnode + satsPerOrbit));
				linksSelected++;
			}
		}
    }
    if(!Vertify(links, curLinkDownNumber)){
    	return false;
    }
    for(auto n : links){
    	std::cout<<n.first<<"  "<<n.second<<std::endl;
    }
    m_downLinks[curLinkDownNumber].push_back(links);
    WriteAdjacency(links, sampleIndex);


    std::set<std::pair<uint32_t, uint32_t>> reroutes;
    for(auto link : links){
    	for(auto n : m_link2Routes[link]){
    		reroutes.insert(n);
    	}
    }
    std::cout<<"Reroutes: "+std::to_string(reroutes.size())<<std::endl;


	// 1. Update cost
    m_routeAllPairsFlexSATE = m_initialRouteAllPairsFlexSATE;
    m_linkWeight = m_initialLinkWeight;
    for(auto curroute : reroutes){
    	uint32_t s = curroute.first;
    	uint32_t d = curroute.second;
    	std::vector<std::vector<uint32_t>> routes = m_routeAllPairsFlexSATE[s][d];
    	if(routes.size() == 0){
			throw std::runtime_error ("MplbBuildRouting::UpdateRouteSegment (uint32_t curLinkDownNumber, uint32_t sampleIndex).");
    	}
    	m_routeAllPairsFlexSATE[s][d].clear();
    	for(auto p : routes){
    		for(uint32_t s = 0; s < p.size() - 1; s++){
    			m_linkWeight[p[s]][p[s + 1]] = m_linkWeight[p[s]][p[s + 1]] - 1;
    		}
    	}
    }



	// modify++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	auto src2all = m_pathDateBase[0];
	for(auto curroute : reroutes){
		uint32_t src = curroute.first;
		uint32_t dst = curroute.second;
		uint32_t olddst = MappingNodePair(0, src, dst, satsPerOrbit, orbits);
		// for searching n path
		for(uint8_t n = 0; n < nPath; n++){
			uint32_t minpcost = UINT32_MAX;
			std::vector<uint32_t> pathWithMinCost;
			for(std::vector<uint32_t> path : src2all[olddst]){
				auto newPath = MappingPath(0, src, path, satsPerOrbit, orbits);

				uint32_t pcost = 0;
				// calculate weight
				std::vector<uint32_t> tempv;
				for(uint32_t s = 0; s < newPath.size() - 1; s++){
					pcost += m_linkWeight[newPath[s]][newPath[s+1]];
					//NS_ASSERT_MSG(m_linkWeight[path[s]][path[s+1]] < UINT32_MAX, "Wrong link adjacency.");
					//m_linkWeight[path[s]][path[s+1]]++; // update weight
					//tempv.push_back(m_linkWeight[path[s]][path[s+1]]);

				}
				//pcost = *max_element(tempv.begin(), tempv.end());
				if(minpcost > pcost){
					minpcost = pcost;
					pathWithMinCost = newPath;
				}
			}
			// update link weight
			for(uint32_t s = 0; s < pathWithMinCost.size() - 1; s++){
				m_linkWeight[pathWithMinCost[s]][pathWithMinCost[s+1]]++;
			}
			// select it
			m_routeAllPairsFlexSATE[src][dst].push_back(pathWithMinCost);
		}


	}
	// modify++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


	// 3. Print network and paths information for DRL training in a numerical environment
	remove_file_if_exists("/home/xiaoyuliu/sag_platform_v3/bake/source/ns-3.35/scratch/main_satnet/test_data/flexsate/MPLB_mapping_"+std::to_string(sampleIndex)+".txt");
	std::ofstream mplb_txt("/home/xiaoyuliu/sag_platform_v3/bake/source/ns-3.35/scratch/main_satnet/test_data/flexsate/MPLB_mapping_"+std::to_string(sampleIndex)+".txt");
	// m, n
	// route1
	// ...
	// route2

	mplb_txt<<satsPerOrbit<<","<<orbits<<std::endl;

	for(auto src2all : m_routeAllPairsFlexSATE){
		NS_ASSERT_MSG(m_routeAllPairsFlexSATE.size() == m_nodes.size(), "Wrong route.");
		if(find(m_nodes.begin(), m_nodes.end(), src2all.first) == m_nodes.end()){
			throw std::runtime_error ("1MplbBuildRouting::UpdateRouteSegment (uint32_t curLinkDownNumber, uint32_t sampleIndex).");
		}
		//mplb_txt<<src2all.first<<std::endl;

		for(auto paths2dst: src2all.second){
			NS_ASSERT_MSG(src2all.second.size() == m_nodes.size() - 1, "Wrong route.");
			if(find(m_nodes.begin(), m_nodes.end(), paths2dst.first) == m_nodes.end()){
				throw std::runtime_error ("2MplbBuildRouting::UpdateRouteSegment (uint32_t curLinkDownNumber, uint32_t sampleIndex).");
			}
			//uint32_t dstId = paths2dst.first;
			std::vector<std::vector<uint32_t>> paths = paths2dst.second;
			NS_ASSERT_MSG(paths.size() == nPath, "Wrong route.");

			for(auto path : paths){
				for(uint32_t s = 0; s < path.size() - 1; s++){
					uint32_t curSat = path[s];
					mplb_txt<<curSat<<",";
				}
				mplb_txt<<path[path.size() - 1]<<std::endl;
			}
		}
	}
	mplb_txt.close();

	std::cout<<"Finished"<<std::endl;
	return true;
}



void
MplbBuildRouting::UpdateRouteSMORE (){
	uint8_t nPath = m_nPath;
	bool part_network = true;
	uint32_t satsPerOrbit = m_walkerConstellation->GetSatNum();
	uint32_t orbits = m_walkerConstellation->GetOrbitNum();
	uint32_t phaseFactor = m_walkerConstellation->GetPhase();
	uint32_t totalSats = satsPerOrbit * orbits;

	NS_ASSERT_MSG(phaseFactor == 0, "Currently only supports 0 phase offset.");

	m_adjacency = m_walkerConstellation->GetAdjacency();
	m_routeAllPairsSMORE.clear();


	m_shuffle.clear();
	std::vector<uint32_t> nodes; // 6*10 产生流量的节点
	if(part_network){
		// part network, noting if no use
		//std::vector<uint32_t> nodes; // 6*10 产生流量的节点
		uint32_t ort = int(orbits / 5.1); // 6:6  8:4.5  7:5.1
		uint32_t satp = int(satsPerOrbit / 1.8);// 10:2  12:1.6   11:1.8
		for(uint32_t k1 = 0; k1 < ort; k1++){
			for(uint32_t k2 = 0; k2 < satp; k2++){
				nodes.push_back(k1*satsPerOrbit + k2);
			}
		}
		for(uint32_t k2 = 0; k2 < satp; k2++){
			//nodes.push_back(35*satsPerOrbit + k2); // 8 12
			//nodes.push_back(34*satsPerOrbit + k2);
		}
		for(uint32_t k1 = 0; k1 < ort; k1++){
			//nodes.push_back(k1*satsPerOrbit + 19); // 8 12
			//nodes.push_back(k1*satsPerOrbit + 18);
		}

		for(auto iter = m_adjacency.begin(); iter != m_adjacency.end();){
			if(find(nodes.begin(), nodes.end(), (*iter).first) == nodes.end()){
				iter = m_adjacency.erase(iter);
			}
			else{
				std::vector<uint32_t> temp_nodes;
				for(auto n : (*iter).second){
					if(find(nodes.begin(), nodes.end(), n) != nodes.end()){
						temp_nodes.push_back(n);
					}
				}
				(*iter).second = temp_nodes;
				iter++;
			}
		}
		// part network
		m_shuffle = nodes;
	}
	else{
		for(uint32_t sat = 0; sat < totalSats; sat++){
			m_shuffle.push_back(sat);
		}
	}

	// 1. Initialize cost
	m_cost.clear();
//	for(uint32_t curNode = 0; curNode < m_adjacency.size(); curNode++){
//		for(auto nb: m_adjacency[curNode]){
//			m_cost[curNode][nb] = 1;
//		}
//	}
	for(auto t : m_adjacency){
		for(auto nb: t.second){
			m_cost[t.first][nb] = 1;
		}
	}

//	// 2. Calculate route
//	for(uint8_t p = 0; p < nPath; p++){
//		for(uint32_t curNode = 0; curNode < m_adjacency.size(); curNode++){
//			// (1)Dijkstra
//			m_preNode.clear();
//			std::pair<double, double> res = std::make_pair(0.0, 100000000.0);
//			for(uint32_t i = 0; i < m_adjacency.size(); i++){
//
//				std::pair<double, double> res_temp = Dijkstra(i);
//				NS_ASSERT_MSG(res_temp.second != 0, "Wrong.");
//				if(res_temp.first > res.first){
//					res.first = res_temp.first;
//				}
//				if(res_temp.second < res.second){
//					res.second = res_temp.second;
//				}
//			}
//			// (2)Generate a randomized routing tree (RRT)
//			rrt.clear();
//			RandomizedRoutingTree (res.second, res.first);
//			for(uint32_t dstNode = 0; dstNode < m_adjacency.size(); dstNode++){
//				// select routes
//				if(curNode != dstNode){
//					auto finalPath = SelectPath(curNode, dstNode);
//					// update cost
//					for(uint32_t k = 0; k < finalPath.size() - 1; k++){
//						uint32_t node1 = finalPath[k];
//						uint32_t node2 = finalPath[k+1];
//						std::unordered_map<uint32_t, uint32_t> cost = m_cost[node1];
//						auto it = cost.find(node2); //find(cost.begin(), cost.end(), node2);
//						NS_ASSERT_MSG(it != cost.end(), "Wrong.");
//						if(node1==10 && node2==11){
//							std::cout<<m_cost[node1][node2]<<std::endl;
//						}
//						m_cost[node1][node2]++;
//					}
//					m_routeAllPairsSMORE[curNode][dstNode].push_back(finalPath);
//				}
//			}
//		}
//	}



	if(part_network){
		for(uint8_t p = 0; p < nPath; p++){
			std::cout<<"Route: "<<p<<std::endl;
			for(auto ts : m_adjacency){
				uint32_t curNode = ts.first;
				// (1)Dijkstra
				m_preNode.clear();
				std::pair<double, double> res = std::make_pair(0.0, 100000000.0);
				for(auto t : m_adjacency){

					std::pair<double, double> res_temp = Dijkstra(t.first);
					NS_ASSERT_MSG(res_temp.second != 0, "Wrong.");
					if(res_temp.first > res.first){
						res.first = res_temp.first;
					}
					if(res_temp.second < res.second){
						res.second = res_temp.second;
					}
				}
				// (2)Generate a randomized routing tree (RRT)
				rrt.clear();
				RandomizedRoutingTree (res.second, res.first);
	//			for(auto ts : m_adjacency){
	//				uint32_t curNode = ts.first;
	//				for(auto td : m_adjacency){
	//					uint32_t dstNode = td.first;
	//					// select routes
	//					if(curNode != dstNode){
	//						auto finalPath = SelectPath(curNode, dstNode);
	//						// update cost
	//						for(uint32_t k = 0; k < finalPath.size() - 1; k++){
	//							uint32_t node1 = finalPath[k];
	//							uint32_t node2 = finalPath[k+1];
	//							std::unordered_map<uint32_t, uint32_t> cost = m_cost[node1];
	//							auto it = cost.find(node2); //find(cost.begin(), cost.end(), node2);
	//							NS_ASSERT_MSG(it != cost.end(), "Wrong.");
	//							m_cost[node1][node2]++;
	//						}
	//						m_routeAllPairsSMORE[curNode][dstNode].push_back(finalPath);
	//					}
	//				}
	//			}
				for(auto td : m_adjacency){
					uint32_t dstNode = td.first;
					// select routes
					if(curNode != dstNode){
						auto finalPath = SelectPath(curNode, dstNode);
						// update cost
						for(uint32_t k = 0; k < finalPath.size() - 1; k++){
							uint32_t node1 = finalPath[k];
							uint32_t node2 = finalPath[k+1];
							std::unordered_map<uint32_t, uint32_t> cost = m_cost[node1];
							auto it = cost.find(node2); //find(cost.begin(), cost.end(), node2);
							NS_ASSERT_MSG(it != cost.end(), "Wrong.");
							m_cost[node1][node2]++;
						}
						m_routeAllPairsSMORE[curNode][dstNode].push_back(finalPath);
					}
				}
			}

		}

	}
	else{
		// 2. mapping
		uint32_t curNode = 0;
		for(uint8_t p = 0; p < nPath; p++){
			std::cout<<"Route: "<<p<<std::endl;
			// (1)Dijkstra
			m_preNode.clear();
			std::pair<double, double> res = std::make_pair(0.0, 100000000.0);
			for(auto t : m_adjacency){

				std::pair<double, double> res_temp = Dijkstra(t.first);
				NS_ASSERT_MSG(res_temp.second != 0, "Wrong.");
				if(res_temp.first > res.first){
					res.first = res_temp.first;
				}
				if(res_temp.second < res.second){
					res.second = res_temp.second;
				}
			}
			// (2)Generate a randomized routing tree (RRT)
			rrt.clear();
			RandomizedRoutingTree (res.second, res.first);
			for(auto td : m_adjacency){
				uint32_t dstNode = td.first;
	//			// (1)Dijkstra
	//			m_preNode.clear();
	//			std::pair<double, double> res = std::make_pair(0.0, 100000000.0);
	//			for(uint32_t i = 0; i < m_adjacency.size(); i++){
	//
	//				std::pair<double, double> res_temp = Dijkstra(i);
	//				NS_ASSERT_MSG(res_temp.second != 0, "Wrong.");
	//				if(res_temp.first > res.first){
	//					res.first = res_temp.first;
	//				}
	//				if(res_temp.second < res.second){
	//					res.second = res_temp.second;
	//				}
	//			}
	//			// (2)Generate a randomized routing tree (RRT)
	//			rrt.clear();
	//			RandomizedRoutingTree (res.second, res.first);
				// select routes
				if(curNode != dstNode){
					auto finalPath = SelectPath(curNode, dstNode);
					// update cost
					for(uint32_t k = 0; k < finalPath.size() - 1; k++){
						uint32_t node1 = finalPath[k];
						uint32_t node2 = finalPath[k+1];
						std::unordered_map<uint32_t, uint32_t> cost = m_cost[node1];
						auto it = cost.find(node2); //find(cost.begin(), cost.end(), node2);
						NS_ASSERT_MSG(it != cost.end(), "Wrong.");
						m_cost[node1][node2]++;
					}
					m_routeAllPairsSMORE[curNode][dstNode].push_back(finalPath);
				}
			}
		}

		// Just calculate all the routes starting from one node, and copy them to other starting points.
		for(uint32_t nodeId = 0; nodeId < totalSats; nodeId++){
			if(nodeId == curNode){
				continue;
			}

			for(auto paths2dstInAllInAll : m_routeAllPairsSMORE[curNode]){
				//uint32_t dstNodeId = paths2dstInAllInAll.first;
				for(auto path : paths2dstInAllInAll.second){
					auto newPath = MappingPath(curNode, nodeId, path, satsPerOrbit, orbits);
					m_routeAllPairsSMORE[nodeId][*(newPath.end()-1)].push_back(newPath);
					for(uint32_t k = 0; k < newPath.size() - 1; k++){
						uint32_t node1 = newPath[k];
						uint32_t node2 = newPath[k+1];
						std::unordered_map<uint32_t, uint32_t> cost = m_cost[node1];
						auto it = cost.find(node2); //find(cost.begin(), cost.end(), node2);
						NS_ASSERT_MSG(it != cost.end(), "Wrong.");
						m_cost[node1][node2]++;
					}
				}
			}
		}
	}



	// 3. Print network and paths information for DRL training in a numerical environment
	remove_file_if_exists("/home/xiaoyuliu/sag_platform_v3/bake/source/ns-3.35/scratch/main_satnet/test_data/flexsate/MPLB_SMORE_"+std::to_string(0)+".txt");
	std::ofstream mplb_txt("/home/xiaoyuliu/sag_platform_v3/bake/source/ns-3.35/scratch/main_satnet/test_data/flexsate/MPLB_SMORE_"+std::to_string(0)+".txt");

	// m, n
	// route1
	// ...
	// route2

	mplb_txt<<satsPerOrbit<<","<<orbits<<std::endl;

	if(part_network){
		NS_ASSERT_MSG(m_routeAllPairsSMORE.size() == nodes.size(), "Wrong route.");

	}else{
		NS_ASSERT_MSG(m_routeAllPairsSMORE.size() == totalSats, "Wrong route.");
	}
	for(auto src2all : m_routeAllPairsSMORE){
		//mplb_txt<<src2all.first<<std::endl;
		if(part_network){
			NS_ASSERT_MSG(src2all.second.size() == nodes.size() - 1, "Wrong route.");
		}else{
			NS_ASSERT_MSG(src2all.second.size() == totalSats - 1, "Wrong route.");
		}
		for(auto paths2dst: src2all.second){
			//uint32_t dstId = paths2dst.first;
			std::vector<std::vector<uint32_t>> paths = paths2dst.second;
			NS_ASSERT_MSG(paths.size() == nPath, "Wrong route.");

			//mplb_txt<<src2all.first<<","<<dstId<<std::endl;

			for(auto path : paths){
				for(uint32_t s = 0; s < path.size() - 1; s++){
					m_link2Routes[std::make_pair(path[s], path[s + 1])].insert(std::make_pair(path[0], path[path.size() - 1]));
					uint32_t curSat = path[s];
					mplb_txt<<curSat<<",";
				}
				mplb_txt<<path[path.size() - 1]<<std::endl;
			}


		}
	}
	mplb_txt.close();

	m_initialRouteAllPairsSMORE = m_routeAllPairsSMORE;
	m_initialCost = m_cost;
	std::cout<<"Finished"<<std::endl;

}

bool
MplbBuildRouting::UpdateAdjacency (uint32_t curnode, uint32_t neighbour)
{
	auto it = m_adjacency.find(curnode);
	auto it1 = m_adjacency.find(neighbour);
	if(it != m_adjacency.end() && it1 != m_adjacency.end()){
		auto curIter = find(m_adjacency[curnode].begin(), m_adjacency[curnode].end(), neighbour);
		auto curIter1 = find(m_adjacency[neighbour].begin(), m_adjacency[neighbour].end(), curnode);
		if(curIter == m_adjacency[curnode].end() || m_adjacency[curnode].size() <= 1 || curIter1 == m_adjacency[neighbour].end() || m_adjacency[neighbour].size() <= 1){
			std::cout<<"2: link down generation retry!"<<std::endl;
			return false;
		}
		else{
			m_adjacency[curnode].erase(curIter);
			m_adjacency[neighbour].erase(curIter1);
			return true;
		}
	}
	else{
		std::cout<<"1: link down generation retry!"<<std::endl;
		return false;
	}

}

bool
MplbBuildRouting::Vertify(std::vector<std::pair<uint32_t, uint32_t>> links, uint32_t curLinkDownNumber){

	for(auto exits : m_downLinks[curLinkDownNumber]){
		bool find1 = true;
		for(uint32_t i = 0; i < curLinkDownNumber; i++){
			auto iter = find(exits.begin(), exits.end(), links[i]);
			if(iter == exits.end()){
				find1 = false;
				break;
			}
		}
		if(find1 == true){
			return false;
		}
	}
	return true;

}

void
MplbBuildRouting::WriteAdjacency(std::vector<std::pair<uint32_t, uint32_t>> links, uint32_t sampleIndex){
	remove_file_if_exists("/home/xiaoyuliu/sag_platform_v3/bake/source/ns-3.35/scratch/main_satnet/test_data/flexsate/Adjacency_"+std::to_string(sampleIndex)+".txt");
	std::ofstream fileISL("/home/xiaoyuliu/sag_platform_v3/bake/source/ns-3.35/scratch/main_satnet/test_data/flexsate/Adjacency_"+std::to_string(sampleIndex)+".txt");
	auto list_isls = m_walkerConstellation->GetIslFromToUnique();

	for(auto lk : links){
		auto iter = find(list_isls.begin(), list_isls.end(), lk);
		if(iter == list_isls.end()){
			std::cout<<"MplbBuildRouting::WriteAdjacency"<<std::endl;
			exit(1);
		}
		else{
			list_isls.erase(iter);
		}
	}

	for(auto isl : list_isls){
		fileISL<<isl.first<<" "<<isl.second<<std::endl;
	}
	fileISL.close();
}

bool
MplbBuildRouting::UpdateRouteSMORE (uint32_t curLinkDownNumber, uint32_t sampleIndex){
	uint8_t nPath = m_nPath;
	bool part_network = true;


	uint32_t satsPerOrbit = m_walkerConstellation->GetSatNum();
	uint32_t orbits = m_walkerConstellation->GetOrbitNum();
	uint32_t phaseFactor = m_walkerConstellation->GetPhase();
	uint32_t totalSats = satsPerOrbit * orbits;

	NS_ASSERT_MSG(phaseFactor == 0, "Currently only supports 0 phase offset.");

	m_adjacency = m_walkerConstellation->GetAdjacency();
	//m_routeAllPairsSMORE.clear();
	m_shuffle.clear();


	/// pre-process: select partial network nodes
	std::vector<uint32_t> nodes_temp; // 6*10 产生流量的节点
    if(part_network){
    	std::vector<uint32_t> nodes; // 6*10 产生流量的节点
		uint32_t ort = int(orbits / 5.1); // 6:6  8:4.5  7:5.1
		uint32_t satp = int(satsPerOrbit / 1.8);// 10:2  12:1.6   11:1.8
		for(uint32_t k1 = 0; k1 < ort; k1++){
			for(uint32_t k2 = 0; k2 < satp; k2++){
				nodes.push_back(k1*satsPerOrbit + k2);
			}
		}
//		for(uint32_t k1 = 0; k1 < 6; k1++){
//			for(uint32_t k2 = 0; k2 < 10; k2++){
//				nodes_temp.push_back(k1*satsPerOrbit + k2);
//			}
//		}
		nodes_temp = nodes;
		for(uint32_t k2 = 0; k2 < satp; k2++){
			//nodes.push_back(35*satsPerOrbit + k2); // 8 12
			//nodes.push_back(34*satsPerOrbit + k2);
		}
		for(uint32_t k1 = 0; k1 < ort; k1++){
			//nodes.push_back(k1*satsPerOrbit + 19); // 8 12
			//nodes.push_back(k1*satsPerOrbit + 18);
		}

		for(auto iter = m_adjacency.begin(); iter != m_adjacency.end();){
			if(find(nodes.begin(), nodes.end(), (*iter).first) == nodes.end()){
				iter = m_adjacency.erase(iter);
			}
			else{
				std::vector<uint32_t> temp_nodes;
				for(auto n : (*iter).second){
					if(find(nodes.begin(), nodes.end(), n) != nodes.end()){
						temp_nodes.push_back(n);
					}
				}
				(*iter).second = temp_nodes;
				iter++;
			}
		}
		m_shuffle = nodes;

    }
	else{
		for(uint32_t sat = 0; sat < totalSats; sat++){
			m_shuffle.push_back(sat);
		}
		nodes_temp = m_shuffle;
	}

    /// pre-process: generate link down {
//    std::vector<uint32_t> nodes;
//	uint32_t ort = int(orbits / 2);
//	uint32_t satp = int(satsPerOrbit / 2);
//	for(uint32_t k1 = 0; k1 < ort; k1++){
//		for(uint32_t k2 = 0; k2 < satp; k2++){
//			nodes.push_back(k1 * satsPerOrbit + k2);
//		}
//	}
    std::vector<uint32_t> nodes = nodes_temp;
    std::vector<std::pair<uint32_t, uint32_t>> links;
    uint32_t tempNodeId = 0;
    if(!UpdateAdjacency(tempNodeId, tempNodeId + satsPerOrbit)){
    	exit(1);
    }
    links.push_back(std::make_pair(tempNodeId, tempNodeId + satsPerOrbit));

    uint32_t linksSelected = 1;
    std::srand(10*sampleIndex);
    while(linksSelected != curLinkDownNumber){
		// [a,b]
		uint32_t a = 0;
		uint32_t b = nodes.size() - 1;
		uint32_t randoxNumber = a + rand() % ( b -a +1 ) ;
		uint32_t curnode = nodes[randoxNumber];
		std::cout<<a<<" "<<b<<" randoxNumber: "<<randoxNumber<<std::endl;
		if(find(links.begin(), links.end(), std::make_pair(curnode, curnode + satsPerOrbit)) == links.end()){
			bool find = UpdateAdjacency(curnode, curnode + satsPerOrbit);
			if(find){
				links.push_back(std::make_pair(curnode, curnode + satsPerOrbit));
				linksSelected++;
			}
		}
    }
    if(!Vertify(links, curLinkDownNumber)){
    	return false;
    }
    for(auto n : links){
    	std::cout<<n.first<<"  "<<n.second<<std::endl;
    }
    m_downLinks[curLinkDownNumber].push_back(links);
    WriteAdjacency(links, sampleIndex);


    std::set<std::pair<uint32_t, uint32_t>> reroutes;
    for(auto link : links){
    	for(auto n : m_link2Routes[link]){
    		reroutes.insert(n);
    	}
    }
    std::cout<<"Reroutes: "+std::to_string(reroutes.size())<<std::endl;


	// 1. Update cost
    m_routeAllPairsSMORE = m_initialRouteAllPairsSMORE;
    m_cost = m_initialCost;
    for(auto curroute : reroutes){
    	uint32_t s = curroute.first;
    	uint32_t d = curroute.second;
    	std::vector<std::vector<uint32_t>> routes = m_routeAllPairsSMORE[s][d];
    	m_routeAllPairsSMORE[s][d].clear();
    	for(auto p : routes){
    		for(uint32_t s = 0; s < p.size() - 1; s++){
    			m_cost[p[s]][p[s + 1]] = m_cost[p[s]][p[s + 1]] - 1;
    		}
    	}
    }


	for(uint8_t p = 0; p < nPath; p++){
		for(auto ts : reroutes){
			uint32_t curNode = ts.first;
			// (1)Dijkstra
			m_preNode.clear();
			std::pair<double, double> res = std::make_pair(0.0, 100000000.0);
			for(auto t : m_adjacency){

				std::pair<double, double> res_temp = Dijkstra(t.first);
				NS_ASSERT_MSG(res_temp.second != 0, "Wrong.");
				if(res_temp.first > res.first){
					res.first = res_temp.first;
				}
				if(res_temp.second < res.second){
					res.second = res_temp.second;
				}
			}
			// (2)Generate a randomized routing tree (RRT)
			rrt.clear();
			RandomizedRoutingTree (res.second, res.first);

			uint32_t dstNode = ts.second;
			// select routes
			if(curNode != dstNode){
				auto finalPath = SelectPath(curNode, dstNode);
				// update cost
				for(uint32_t k = 0; k < finalPath.size() - 1; k++){
					uint32_t node1 = finalPath[k];
					uint32_t node2 = finalPath[k+1];
					std::unordered_map<uint32_t, uint32_t> cost = m_cost[node1];
					auto it = cost.find(node2); //find(cost.begin(), cost.end(), node2);
					NS_ASSERT_MSG(it != cost.end(), "Wrong.");
					m_cost[node1][node2]++;
				}
				m_routeAllPairsSMORE[curNode][dstNode].push_back(finalPath);
			}

		}

	}





	// 3. Print network and paths information for DRL training in a numerical environment
	remove_file_if_exists("/home/xiaoyuliu/sag_platform_v3/bake/source/ns-3.35/scratch/main_satnet/test_data/flexsate/MPLB_SMORE_"+std::to_string(sampleIndex)+".txt");
	std::ofstream mplb_txt("/home/xiaoyuliu/sag_platform_v3/bake/source/ns-3.35/scratch/main_satnet/test_data/flexsate/MPLB_SMORE_"+std::to_string(sampleIndex)+".txt");

	// m, n
	// route1
	// ...
	// route2

	mplb_txt<<satsPerOrbit<<","<<orbits<<std::endl;

	if(part_network){
		NS_ASSERT_MSG(m_routeAllPairsSMORE.size() == nodes.size(), "Wrong route.");

	}else{
		NS_ASSERT_MSG(m_routeAllPairsSMORE.size() == totalSats, "Wrong route.");
	}
	for(auto src2all : m_routeAllPairsSMORE){
		//mplb_txt<<src2all.first<<std::endl;
		if(part_network){
			NS_ASSERT_MSG(src2all.second.size() == nodes.size() - 1, "Wrong route.");
		}else{
			NS_ASSERT_MSG(src2all.second.size() == totalSats - 1, "Wrong route.");
		}
		for(auto paths2dst: src2all.second){
			//uint32_t dstId = paths2dst.first;
			std::vector<std::vector<uint32_t>> paths = paths2dst.second;
			NS_ASSERT_MSG(paths.size() == nPath, "Wrong route.");

			//mplb_txt<<src2all.first<<","<<dstId<<std::endl;

			for(auto path : paths){
				for(uint32_t s = 0; s < path.size() - 1; s++){
					uint32_t curSat = path[s];
					mplb_txt<<curSat<<",";
				}
				mplb_txt<<path[path.size() - 1]<<std::endl;
			}
		}
	}
	mplb_txt.close();

	std::cout<<"Finished"<<std::endl;
	return true;

}

//定义比较的函数
bool cmp_value(const std::pair<uint32_t, uint32_t> left,const std::pair<uint32_t,uint32_t> right)
{
	return left.second < right.second;
}

bool cmp_value_min(const std::pair<uint32_t, uint32_t> left,const std::pair<uint32_t,uint32_t> right)
{
	return !(left.second < right.second);
}

std::pair<double,double>
MplbBuildRouting::Dijkstra (uint32_t curNode){

	//m_preNode.clear();

	double  maxdist = 99999999.9;
	std::unordered_map<uint32_t, uint32_t> pre;  // 前溯
	std::unordered_map<uint32_t, bool> processedornot;
	std::unordered_map<uint32_t, double> dist;  // cost
	for (auto iter1 : m_adjacency) processedornot[iter1.first] = false;
	for (auto iter2 : m_adjacency) dist[iter2.first] = maxdist;
	for(auto nb : m_adjacency[curNode]){
		dist[nb] = m_cost[curNode][nb];
		pre[nb] = curNode;
	}
	auto xx = dist;

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

	m_preNode[curNode] = pre;
	m_dist[curNode] = dist;
	//std::sort(dist.begin(), dist.end(), cmp_value);
	auto max = std::max_element(dist.begin(), dist.end(), cmp_value);
	auto it = dist.find(curNode);
	dist.erase(it);
	auto min = std::max_element(dist.begin(), dist.end(), cmp_value_min);
	if(min->second==0){
		std::cout<<xx.size()<<std::endl;
	}
	return std::make_pair(max->second, min->second);

}


void
MplbBuildRouting::RandomizedRoutingTree (uint32_t mindist, uint32_t maxdist){
	double k = 2 / mindist;
	double temp = k;
	if(k > 1){
		while(temp > 1){
			k = temp;
			mindist = mindist * k;
			temp = 2 / mindist;
		}
	}
	else{
		k = 1;
	}

	// Randomly renumber nodes
//	random_shuffle(m_shuffle.begin(), m_shuffle.end());
//	for(uint32_t j = 0; j < m_shuffle.size(); j++){
//		m_mappingShuffle2NodeId[m_shuffle[j]] = j;
//	}
	// Find delta
	uint32_t delta = 0;
	while(std::pow(2, delta) < maxdist * k){
		delta++;
	}
	// Find U [1,2)       [0,100)/100 + 1
	double U = ((rand() % (100-0))+ 0) / 100.0 + 1.0;

	uint32_t i = delta - 1;
	double R = std::pow(2, i-1) * U;

	// construct Construct node id set of the RRT root node
	std::vector<uint32_t> S;
//	for(std::map<uint32_t, uint32_t>::iterator it = m_mappingShuffle2NodeId.begin(); it != m_mappingShuffle2NodeId.end(); it++){
//		S.push_back(it->first);
//	}
	random_shuffle(m_shuffle.begin(), m_shuffle.end());
	S = m_shuffle;

	//std::vector<std::vector<RandomizedRoutingTreeNode>> rrt;
	rrt.push_back({RandomizedRoutingTreeNode(0, S[0], S, nullptr)});
	uint32_t delta_x = 1;
	for(uint32_t level = 0; level < delta_x; level++){
		std::vector<RandomizedRoutingTreeNode> sub_rrt;
		for(uint32_t pos = 0; pos < rrt[level].size(); pos++){
			// split
			if(rrt[level][pos].nodeSet.size() > 1){
				std::vector<RandomizedRoutingTreeNode> cur_sub_rrt = Clustering(level+1, rrt[level][pos], &rrt[level][pos], R, k);
				std::vector<uint32_t> newset;
				newset.push_back(sub_rrt.size());
				newset.push_back(sub_rrt.size() + cur_sub_rrt.size() - 1);
				rrt[level][pos].SetPosSet(newset);
				sub_rrt.insert(sub_rrt.end(), cur_sub_rrt.begin(), cur_sub_rrt.end());
			}

		}
		if(sub_rrt.size() > 0){
			rrt.push_back(sub_rrt);
			delta_x++;
		}
		i--;
		R = std::pow(2, i-1) * U;
	}


}

std::vector<MplbBuildRouting::RandomizedRoutingTreeNode>
MplbBuildRouting::Clustering(uint32_t level, RandomizedRoutingTreeNode rrtNode, RandomizedRoutingTreeNode* rrtNodeFather, double R, uint32_t k){

	uint32_t cnode = rrtNode.nodeId;
	std::vector<uint32_t> nodes = rrtNode.nodeSet;
	std::vector<RandomizedRoutingTreeNode> newrrtNodes;
	while(nodes.size() > 0){
		cnode = nodes[0];
		std::vector<uint32_t> newNodeSet;
		newNodeSet.push_back(cnode);
		auto pos = find(nodes.begin(), nodes.end(), cnode);
		if(pos != nodes.end()){
			nodes.erase(pos);
		}
		for(auto it = nodes.begin(); it != nodes.end();){
			if(Distance(cnode, *it, R, k)){
				newNodeSet.push_back(*it);
				it = nodes.erase(it);
			}
			else{
				it++;
			}
		}
		newrrtNodes.push_back(RandomizedRoutingTreeNode(level, cnode, newNodeSet, rrtNodeFather));
	}
	return newrrtNodes;

}

// Whether it is within the clustering range of node 1
bool MplbBuildRouting::Distance(uint32_t node1, uint32_t node2, double R, uint32_t k){
//	auto pre = m_preNode[node1];
//	uint32_t tempNode = node2;
//	std::vector<uint32_t> pathres;
//	pathres.push_back(node2);
//	while(tempNode != node1){
//		tempNode = pre[tempNode];
//		pathres.push_back(tempNode);
//	}
//	std::reverse(pathres.begin(),pathres.end());
	auto dist = m_dist[node1];
	if(dist[node2] * k <= R){
		return true;
	}
	return false;

}

std::vector<uint32_t>
MplbBuildRouting::SelectPath(uint32_t src, uint32_t dst){
	uint32_t levels = rrt.size();
	// find src in rrt
	RandomizedRoutingTreeNode srcRRTNode;
	bool found = false;
	uint32_t curLevel = levels - 1;
	for(uint32_t i = levels - 1; i >= 0; i--){
		std::vector<RandomizedRoutingTreeNode> rrtNodes = rrt[i];
		for(auto rrtnode: rrtNodes){
			if(rrtnode.nodeId == src){
				NS_ASSERT_MSG(rrtnode.nodeSet.size() == 1, "Wrong.");
				srcRRTNode = rrtnode;
				found = true;
				break;
			}
		}
		if(found){
			break;
		}
		curLevel--;
	}
	NS_ASSERT_MSG(found == true, "Wrong.");

	// find pass node
	// up
	auto fnode = srcRRTNode.fatherTreeNode;
	curLevel--;
	auto curset = fnode->nodeSet;
	auto it = find(curset.begin(), curset.end(), dst);
	std::vector<uint32_t> paths;
	paths.push_back(src);
	while(it == curset.end()){
		paths.push_back(fnode->nodeId);
		fnode = fnode->fatherTreeNode;
		NS_ASSERT_MSG(fnode != nullptr, "Wrong.");
		curLevel--;
		curset = fnode->nodeSet;
		it = find(curset.begin(), curset.end(), dst);
	}
	paths.push_back(fnode->nodeId);

	// down
	auto nextNode = *fnode;
	while(nextNode.nodeId != dst){
		auto pos = nextNode.posSet;
		curLevel++;
		NS_ASSERT_MSG(curLevel < rrt.size(), "Wrong.");
		bool found = false;
		for(uint32_t s = pos[0]; s <= pos[1]; s++){
			nextNode = rrt[curLevel][s];
			if(find(nextNode.nodeSet.begin(), nextNode.nodeSet.end(), dst) != nextNode.nodeSet.end()){
				found = true;
				break;
			}
		}
		NS_ASSERT_MSG(found == true, "Wrong.");
		paths.push_back(nextNode.nodeId);
	}



//	if(src == 0 && dst == 40){
//		std::cout<<"1"<<std::endl;
//	}
	auto finalPaths = PathGenerate(paths);
	NS_ASSERT_MSG(finalPaths[0] == src, "Wrong.");
	NS_ASSERT_MSG(*(finalPaths.end()-1) == dst, "Wrong.");

	return finalPaths;

}


std::vector<uint32_t> MplbBuildRouting::PathGenerate(std::vector<uint32_t> paths){
	std::vector<uint32_t> newpaths;
	std::vector<uint32_t> finalPaths;
	for (auto it1 = paths.begin(); it1 < paths.end(); it1++)
	{
		auto it2 = find(paths.begin(), it1, *it1);
		if (it2 == it1)
			newpaths.push_back(*it1);
	}
	uint32_t node1;
	uint32_t node2;
	for(uint32_t i = 0; i < newpaths.size()-1; i++){
		node1 = newpaths[i];
		node2 = newpaths[i+1];
		auto pre = m_preNode[node1];
		uint32_t tempNode = node2;
		std::vector<uint32_t> pathres;
		pathres.push_back(node2);
		while(tempNode != node1){
			tempNode = pre[tempNode];
			pathres.push_back(tempNode);
		}
		std::reverse(pathres.begin(),pathres.end());
		if(finalPaths.size() > 0){
			finalPaths.erase(finalPaths.end()-1);
		}
		finalPaths.insert(finalPaths.end(), pathres.begin(), pathres.end());

	}

	// Eliminate loops
	//uint32_t src = finalPaths[0];
	uint32_t dst = finalPaths[finalPaths.size()-1];
	std::vector<uint32_t> p;
	for(uint32_t k = 0; k < finalPaths.size(); k++){
		if(finalPaths[k] == dst){
			p.push_back(finalPaths[k]);
			break;
		}
		auto it = find(p.begin(),p.end(),finalPaths[k]);
		if(it != p.end()){
			p.erase(it, p.end());
			p.push_back(finalPaths[k]);
		}
		else{
			p.push_back(finalPaths[k]);
		}
	}


	return p;


}



















void
MplbBuildRouting::DoUpdateRoute (Time t){

	if(m_lastRouteCalculateTime > t){
		return;
	}

	m_routingTable->Clear();

// todo
//	m_lastRouteCalculateTime = t;
//	uint32_t curNode = m_rootRouterId.Get();
//	for(auto it : m_adjacency){
//		uint32_t dstNode = it.first;
//		if (dstNode != curNode && m_preNode.find(dstNode) != m_preNode.end()){//{find(pre.begin(),pre.end(),dstNode)
//			uint32_t tempNode1 = m_preNode[dstNode];
//			uint32_t tempNode2 = dstNode;
//			bool found = true;
//			while (tempNode1 != curNode){
//				tempNode2 = tempNode1;
//				if(m_preNode.find(tempNode1) != m_preNode.end()){//find(pre.begin(),pre.end(),tempNode1)
//					tempNode1 = m_preNode[tempNode1];
//				}
//				else{
//					found = false;
//					break;
//				}
//			}
//			if(!found){
//				continue;
//			}
//			// dst: dstNode
//			// nexthop: tempNode2
//			Ipv4Address nextHopRtr(tempNode2);
//			Ipv4Address srcIP;
//			Ipv4Address gateway;
//
//
//			if(m_interfaceAddress.find(nextHopRtr) != m_interfaceAddress.end()){
//				srcIP = m_interfaceAddress[nextHopRtr].first;
//				gateway = m_interfaceAddress[nextHopRtr].second;
//			}
//			else{
////				if(curNode == 100){
////					auto x = m_adjacency[curNode];
////				}
//				throw std::runtime_error("Process Wrong: OspfBuildRouting::UpdateRoute1");
//
//			}
//			Ptr<NetDevice> dec = m_ipv4->GetNetDevice(m_ipv4->GetInterfaceForAddress(srcIP));
//			Ipv4InterfaceAddress itrAddress = m_ipv4->GetAddress(m_ipv4->GetInterfaceForAddress(srcIP),0);
//			Ipv4Address dstRtr = Ipv4Address(dstNode);
//			if(m_address.find(dstRtr) != m_address.end()){
//				for(auto d : m_address[dstRtr]){
//					// d : dst IP
//					//m_routingTable->AddRoute(dec, d, itrAddress, gateway);
//					SAGRoutingTableEntry rtEntry(dec, d, itrAddress, gateway);
//					m_routingTable->AddRoute(rtEntry);
//
//				}
//			}
//			else{
//				throw std::runtime_error("Process Wrong: OspfBuildRouting::UpdateRoute2");
//			}
//
//
//		}
//	}


}




}
}
