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

#include "gsl_switch_strategy.h"
#include "ns3/sag_rtp_constants.h"
#include <math.h>
#define pi 3.14159265358979323846
namespace ns3 {

    //NS_LOG_COMPONENT_DEFINE ("SwitchStrategyGSL");

    NS_OBJECT_ENSURE_REGISTERED (SwitchStrategyGSL);

    TypeId
	SwitchStrategyGSL::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::SwitchStrategyGSL")
                .SetParent<Object>()
                .SetGroupName("SatelliteNetwork")
                .AddConstructor<SwitchStrategyGSL>()
				.AddAttribute("FeederLinkMaxNumber", "Max number of FeederLinks of each satellite.",
							UintegerValue(5),
							MakeUintegerAccessor(&SwitchStrategyGSL::m_feederLinkNum),
							MakeUintegerChecker<uint32_t>())
				;
        return tid;
    }

    SwitchStrategyGSL::SwitchStrategyGSL (){

    }

    SwitchStrategyGSL::~SwitchStrategyGSL (){

	}

    void
	SwitchStrategyGSL::SetTopologyHandle(std::vector<Ptr<Constellation>> constellations, NodeContainer& groundStations, std::vector<Ptr<GroundStation>> &groundStationsModel){

    	m_constellations = constellations;
    	m_groundStations = groundStations;
    	m_groundStationsModel = groundStationsModel;

    }

    void
    SwitchStrategyGSL::InitializeSatelliteConnectEntry(){
    	if(m_satelliteConnectionState.size() != 0){
    		return;
    	}
    	for(uint32_t i = 0; i < m_groundStations.GetN(); i++) {
    		Ptr<Node> groundStation = m_groundStations.Get(i);
    		//uint32_t Ndev = groundStation->GetNDevices();
    		uint32_t Ndev = 2;  // only consider one antenna
    		std::vector<SatelliteConnectState> states;
			for(uint32_t ndev = 1; ndev < Ndev; ndev++){ // lookback netdevice: 0
				states.push_back(waitConnect);
			}
			m_satelliteConnectionState.push_back(states);
			//std::cout<<m_satelliteConnectionState.size()<<std::endl;
    	}

    }

    void
	SwitchStrategyGSL::SetBasicSimHandle(Ptr<BasicSimulation> basicSimulation){

    	m_basicSimulation = basicSimulation;
    	std::string st = m_basicSimulation->GetConfigParamOrDefault("minimum_elevation_angle_deg", "10.0");
    	m_elevation = parse_double(st);
    	m_feederLinkNum = parse_positive_int64(m_basicSimulation->GetConfigParamOrDefault("maximum_feeder_link_number", "5"));

    	m_baseLogsDir = basicSimulation->GetLogsDir();
    }

    void
	SwitchStrategyGSL::UpdateSwitch(std::vector<std::pair<Ptr<Node>, std::vector<std::pair<uint32_t, Ptr<Node>>>>>& gslRecord, std::vector<std::pair<Ptr<Node>, std::vector<std::pair<uint32_t, Ptr<Node>>>>>& gslRecordCopy){

    	InitializeSatelliteConnectEntry();

		m_feederLink.clear();
		m_feederLinkUnique.clear();

    }

	double
	SwitchStrategyGSL::GetMaxVisibleDistance(double orbitHeight){

//		double h = orbitHeight + R;
//		double alpha = pi / 3;
//		double maxDistance = (h * cos(alpha) - sqrt(pow(R, 2) - pow(h * sin(alpha), 2)));
//
//		return maxDistance;
//
//		double satelliteConeRadiusKm = orbitHeight / tan(pi / 180.0 * m_elevation);
//		double maxDistance = sqrt(pow(satelliteConeRadiusKm, 2) + pow(orbitHeight, 2));
//
//		return maxDistance;

		if(m_heightToDistTable.count(orbitHeight) != 0){
			return m_heightToDistTable[orbitHeight];
		}
		double satelliteConeRadiusKm = orbitHeight / tan(pi / 180.0 * m_elevation);
		double maxDistance = sqrt(pow(satelliteConeRadiusKm, 2) + pow(orbitHeight, 2));

		m_heightToDistTable[orbitHeight] = maxDistance;
		return maxDistance;
	}

	std::unordered_map <uint32_t,uint32_t>
	SwitchStrategyGSL::GetRecentSatelliteByPosition(Vector gsPosition, uint32_t interface){
		uint32_t validRecodeInterval = 300;
		double timeNow = Simulator::Now().GetSeconds();
		std::unordered_map <uint32_t, uint32_t> Satellites;
		for(auto entry:m_positionToSatelliteRecord){
			if(entry.first.first != gsPosition || entry.first.second != interface) continue;
			for(auto subEntry:entry.second) {
				if(timeNow-subEntry.m_updateTime>validRecodeInterval) continue;
				Satellites[subEntry.m_layer]=subEntry.m_satellite;
			}
		}
		return Satellites;
	}

	void
	SwitchStrategyGSL::SetRecentSatelliteByPosition(Vector gsPosition, uint32_t layer, uint32_t satelliteNum, uint32_t interface){
		int32_t recordIdex = -1;
		double timeNow = Simulator::Now().GetSeconds();
		for (uint32_t i=0; i<m_positionToSatelliteRecord.size(); i++){
			if(m_positionToSatelliteRecord[i].first.first == gsPosition && m_positionToSatelliteRecord[i].first.second == interface) {
				recordIdex = i;
				break;
			}
		}
		if(recordIdex == -1) {
			std::vector<SatelliteConnectEntry> entry;
			entry.push_back(SatelliteConnectEntry(layer, timeNow, satelliteNum));
			m_positionToSatelliteRecord.push_back(std::make_pair(std::make_pair(gsPosition, interface), entry));
		}
		else{
			for(uint32_t j=0; j<m_positionToSatelliteRecord[recordIdex].second.size(); j++){
				// 一层只维护一个节点?
				if(m_positionToSatelliteRecord[recordIdex].second[j].m_layer == layer) {
					if(m_positionToSatelliteRecord[recordIdex].second[j].m_updateTime == timeNow) return;
					m_positionToSatelliteRecord[recordIdex].second[j].m_satellite = satelliteNum;
					m_positionToSatelliteRecord[recordIdex].second[j].m_updateTime = timeNow;
					return;
				}
			}
			m_positionToSatelliteRecord[recordIdex].second.push_back(SatelliteConnectEntry(layer, timeNow, satelliteNum));
		}
	}

//	void SwitchStrategyGSL::SetGroundStationState(u_int32_t index, GroundStationState state){
//		m_groundStationsModel[index]->SetState(state);
//	}

	bool
	SwitchStrategyGSL::RecordFeederLinkForSatellites(Ptr<Node> sat){


		if(m_feederLink.find(sat->GetId()) != m_feederLink.end()){
			if(m_feederLink[sat->GetId()] + 1 > m_feederLinkNum){
				return false;
			}
			else{
				m_feederLink[sat->GetId()]++;
				return true;
			}
		}
		else{
			m_feederLink[sat->GetId()] = 1;
			return true;
		}

	}

	bool
	SwitchStrategyGSL::CheckFeederLinkNumber(Ptr<Node> sat){
		if(m_feederLink.find(sat->GetId()) != m_feederLink.end()){
			if(m_feederLink[sat->GetId()] + 1 > m_feederLinkNum){
				return false;
			}
			else{
				return true;
			}
		}
		else{
			return true;
		}
	}

	void
	SwitchStrategyGSL::RecordFeederLinkUnique(Ptr<Node> sat, Ptr<Node> gs){
		m_feederLinkUnique[gs->GetId()].push_back(sat->GetId());
	}

	bool
	SwitchStrategyGSL::CheckFeederLinkUnique(Ptr<Node> sat, Ptr<Node> gs){
		if(m_feederLinkUnique.find(gs->GetId()) != m_feederLinkUnique.end()){
			std::vector<uint32_t> sats = m_feederLinkUnique[gs->GetId()];
			if(find(sats.begin(), sats.end(), sat->GetId()) != sats.end()){
				return false;
			}
			else{
				return true;
			}
		}
		else{
			return true;
		}
	}

	//NS_LOG_COMPONENT_DEFINE ("DistanceNearestFirst");

	NS_OBJECT_ENSURE_REGISTERED (DistanceNearestFirst);

	TypeId
	DistanceNearestFirst::GetTypeId(void)
	{
		static TypeId tid = TypeId("ns3::DistanceNearestFirst")
				.SetParent<SwitchStrategyGSL>()
				.SetGroupName("SatelliteNetwork")
				.AddConstructor<DistanceNearestFirst>()
				;
		return tid;
	}

	DistanceNearestFirst::DistanceNearestFirst (){

	}

	DistanceNearestFirst::~DistanceNearestFirst (){

	}

	void
	DistanceNearestFirst::UpdateSwitch(std::vector<std::pair<Ptr<Node>, std::vector<std::pair<uint32_t, Ptr<Node>>>>>& gslRecord, std::vector<std::pair<Ptr<Node>, std::vector<std::pair<uint32_t, Ptr<Node>>>>>& gslRecordCopy){

//		m_feederLink.clear();
//
//		for(uint32_t i = 0; i < m_groundStations.GetN(); i++){
//			Ptr<Node> gs2sat = nullptr;
//			Ptr<Node> groundStation = m_groundStations.Get(i);
//			Ptr<MobilityModel> gsMobility = groundStation->GetObject<MobilityModel>();
//			Ptr<Node> satelliteOld = nullptr;
//
//			double minDist = DBL_MAX;
//			if(gs2sat == nullptr){
//				for(auto cons : m_constellations){
//
//					double minDistTemp = GetMaxVisibleDistance(cons->GetAltitude());
//
//					for(uint32_t j = 0; j < cons->GetNodes().GetN(); j++){
//						auto satellite = cons->GetNodes().Get(j);
//						Ptr<MobilityModel> satMobility = satellite->GetObject<MobilityModel>();
//						double distance = gsMobility->GetDistanceFrom (satMobility) / 1000.0;
//
//						if(distance < minDistTemp && distance < minDist && CheckFeederLinkNumber(satellite)){
//							minDistTemp = distance;
//							minDist = minDistTemp;
//							gs2sat = satellite;
//						}
//
//					}
//				}
//			}
//
//			if(gs2sat != nullptr){
//				RecordFeederLinkForSatellites(gs2sat);
//				gslRecord.push_back(std::make_pair(groundStation, gs2sat));
//			}
//			else{
//				gslRecord.push_back(std::make_pair(groundStation, nullptr));
//				std::cout << ">> No satellite in light of sight, ground station ID: " +  std::to_string(groundStation->GetId())<< std::endl;
//			}
//
//
//		}

	}


	NS_OBJECT_ENSURE_REGISTERED (SwitchTriggeredByInvisible);

	TypeId
	SwitchTriggeredByInvisible::GetTypeId(void)
	{
		static TypeId tid = TypeId("ns3::SwitchTriggeredByInvisible")
				.SetParent<SwitchStrategyGSL>()
				.SetGroupName("SatelliteNetwork")
				.AddConstructor<SwitchTriggeredByInvisible>()
				;
		return tid;
	}

	SwitchTriggeredByInvisible::SwitchTriggeredByInvisible (){

	}

	SwitchTriggeredByInvisible::~SwitchTriggeredByInvisible (){

	}

	void
	SwitchTriggeredByInvisible::UpdateSwitch(std::vector<std::pair<Ptr<Node>, std::vector<std::pair<uint32_t, Ptr<Node>>>>>& gslRecord, std::vector<std::pair<Ptr<Node>, std::vector<std::pair<uint32_t, Ptr<Node>>>>>& gslRecordCopy){
		// Link switching only occurs when invisible

		// for initializing
		SwitchStrategyGSL::UpdateSwitch(gslRecord, gslRecordCopy);
		// for each ground station
		for(uint32_t i = 0; i < m_groundStationsModel.size(); i++) {
			Ptr<GroundStation> gsModel = m_groundStationsModel[i];
			Ptr<Node> groundStation = gsModel->m_groundStationNodeP;
			Ptr<MobilityModel> gsMobility = groundStation->GetObject<MobilityModel>();
			std::vector<std::pair<uint32_t, Ptr<Node>>> gs2sat_vec;

			// for each connection
			for(uint32_t j_index = 0; j_index < m_satelliteConnectionState[i].size(); j_index++){
				Ptr<Node> gs2sat = nullptr;
				Ptr<Node> satelliteOld = nullptr;
				if(m_satelliteConnectionState[i][j_index] == waitConnect){
					uint32_t interface = j_index + 1;
					// std::cout<<"Time:"<<timeNow<< " ground station "<<gsModel->GetGid()<<" is wait connect"<< std::endl;
					double minDist = DBL_MAX;
					//clock_t start = clock();
					Vector gsPosition = gsMobility->GetPosition();
					std::unordered_map<uint32_t,uint32_t> lastRecord = GetRecentSatelliteByPosition(gsPosition, interface);
					uint32_t layer = 0;
					for (auto cons : m_constellations){
						double a = cons->GetAltitude();
						double MaxDistance = GetMaxVisibleDistance(a);
						uint32_t lastSatellite = 0;
						uint32_t searchOrbitStart = 0;
						uint32_t searchOrbitSize = cons->GetOrbitNum();
						uint32_t searchSatStart = 0;
						uint32_t searchSatSize = cons->GetSatNum();
						if(lastRecord.count(cons->GetIndex())>0) {
							lastSatellite = lastRecord[cons->GetIndex()];
							uint32_t lastOrbit = lastSatellite/cons->GetSatNum();
							lastSatellite = lastSatellite%cons->GetSatNum();
							searchOrbitStart = (cons->GetOrbitNum()+lastOrbit-SearchOrbitBond)%cons->GetOrbitNum();
							searchSatStart = (cons->GetSatNum()+lastSatellite-SearchSateBond)%cons->GetSatNum();
							searchOrbitSize = 2*SearchOrbitBond+1;
							searchSatSize = 2*SearchSateBond+1;
						}
						// std::cout<<"Last connect Satellite : "<<lastSatellite<<std::endl;

						double minDistTemp = MaxDistance;
						for (uint32_t o = searchOrbitStart;o<searchOrbitStart+searchOrbitSize;o++){
							for(uint32_t s = searchSatStart;s<searchSatStart+searchSatSize;s++){
								uint32_t orbit = o%cons->GetOrbitNum();
								uint32_t sat = s%cons->GetSatNum();
								uint32_t sateNum = orbit*cons->GetSatNum()+sat;
								auto satellite = cons->GetNodes().Get(sateNum);
								Ptr<MobilityModel> satMobility = satellite->GetObject<MobilityModel>();
								double distance = gsMobility->GetDistanceFrom(satMobility) / 1000.0;
								//std::cout<<distance<<std::endl;
								if (distance < minDistTemp && distance < minDist && CheckFeederLinkNumber(satellite) && CheckFeederLinkUnique(satellite, groundStation))
								{
									// std::cout<<"Temp Satellite : "<<sateNum<<std::endl;
									minDist = distance;
									minDistTemp = distance;
									gs2sat = satellite;
									layer = cons->GetIndex();
								}
							}
						}
						break; // todo waiting to be deleted, this just for avoiding traffic down
					}
					//clock_t end = clock();
					//std::cout<<"Satellite select time: "<<double(end-start)*1000/CLOCKS_PER_SEC<<"ms"<<std::endl;
					if(gs2sat != nullptr){
						Topology_CHANGE_GSL = true;
						gs2sat_vec.push_back(std::make_pair(interface, gs2sat));
						m_satelliteConnectionState[i][j_index] = connedted;
						RecordFeederLinkUnique(gs2sat, groundStation);
						RecordFeederLinkForSatellites(gs2sat);
						SetRecentSatelliteByPosition(gsPosition, layer, gs2sat->GetId(), interface);
						// std::cout << "Time:" << timeNow << " ground station " << gsModel->GetGid() << " is connected" << std::endl;
						// std::cout<<"Connect Satellite : "<<gs2sat->GetId()<<std::endl;
					}
					else
					{
						gs2sat_vec.push_back(std::make_pair(interface, nullptr));
						std::cout << ">> No satellite in light of sight, ground station ID: " +  std::to_string(groundStation->GetId())<< std::endl;
					}
				}
				else if(m_satelliteConnectionState[i][j_index] == connedted){
					std::pair<uint32_t, Ptr<Node>> connectionEntry = gslRecordCopy[i].second[j_index];
					satelliteOld = connectionEntry.second;
					uint32_t interface = connectionEntry.first;
					Ptr<MobilityModel> satOldMobility = satelliteOld->GetObject<MobilityModel>();
					double dist = gsMobility->GetDistanceFrom(satOldMobility) / 1000.0;
					Ptr<Constellation> itsCons = satOldMobility->GetObject<SatellitePositionMobilityModel>()->GetSatellite()->GetCons();
					double a = itsCons->GetAltitude();
					double maxDistance = GetMaxVisibleDistance(a);

					// if the old link is valid, maintain
					if (dist < maxDistance && CheckFeederLinkNumber(satelliteOld) && CheckFeederLinkUnique(satelliteOld, groundStation))
					{
						gs2sat = satelliteOld;
						gs2sat_vec.push_back(std::make_pair(interface, gs2sat));
						RecordFeederLinkUnique(gs2sat, groundStation);
						RecordFeederLinkForSatellites(gs2sat);
					}
					else
					{
						// std::cout << "Time:" << timeNow << " ground station " << gsModel->GetGid() << " is sleeped" << std::endl;
						m_satelliteConnectionState[i][j_index] = sleep;
						Topology_CHANGE_GSL = true;
						// todo
						//Simulator::Schedule(Seconds(gsModel->GetSleepTime()), &SwitchTriggeredByInvisible::SetGroundStationState, this, i, waitConnect);
						m_satelliteConnectionState[i][j_index] = waitConnect;

						if (m_satelliteConnectionState[i][j_index] == waitConnect){
							// if not, switch to the new satellite that is nearest to the ground station
							if(gs2sat != nullptr) {
								NS_ASSERT("ground station state is not consist with record!");
							}
							// std::cout<<"Time:"<<timeNow<< " ground station "<<gsModel->GetGid()<<" is wait connect"<< std::endl;
							double minDist = DBL_MAX;
							//clock_t start = clock();
							Vector gsPosition = gsMobility->GetPosition();
							std::unordered_map<uint32_t,uint32_t> lastRecord = GetRecentSatelliteByPosition(gsPosition, interface);
							uint32_t layer = 0;
							for (auto cons : m_constellations){
								double a = cons->GetAltitude();
								double MaxDistance = GetMaxVisibleDistance(a);
								uint32_t lastSatellite = 0;
								uint32_t searchOrbitStart = 0;
								uint32_t searchOrbitSize = cons->GetOrbitNum();
								uint32_t searchSatStart = 0;
								uint32_t searchSatSize = cons->GetSatNum();
								if(lastRecord.count(cons->GetIndex())>0) {
									lastSatellite = lastRecord[cons->GetIndex()];
									uint32_t lastOrbit = lastSatellite/cons->GetSatNum();
									lastSatellite = lastSatellite%cons->GetSatNum();
									searchOrbitStart = (cons->GetOrbitNum()+lastOrbit-SearchOrbitBond)%cons->GetOrbitNum();
									searchSatStart = (cons->GetSatNum()+lastSatellite-SearchSateBond)%cons->GetSatNum();
									searchOrbitSize = 2*SearchOrbitBond+1;
									searchSatSize = 2*SearchSateBond+1;
								}
								// std::cout<<"Last connect Satellite : "<<lastSatellite<<std::endl;

								double minDistTemp = MaxDistance;
								for (uint32_t o = searchOrbitStart;o<searchOrbitStart+searchOrbitSize;o++){
									for(uint32_t s = searchSatStart;s<searchSatStart+searchSatSize;s++){
										uint32_t orbit = o%cons->GetOrbitNum();
										uint32_t sat = s%cons->GetSatNum();
										uint32_t sateNum = orbit*cons->GetSatNum()+sat;
										auto satellite = cons->GetNodes().Get(sateNum);
										Ptr<MobilityModel> satMobility = satellite->GetObject<MobilityModel>();
										double distance = gsMobility->GetDistanceFrom(satMobility) / 1000.0;
										//std::cout<<distance<<std::endl;
										if (distance < minDistTemp && distance < minDist && CheckFeederLinkNumber(satellite) && CheckFeederLinkUnique(satellite, groundStation))
										{
											// std::cout<<"Temp Satellite : "<<sateNum<<std::endl;
											minDist = distance;
											minDistTemp = distance;
											gs2sat = satellite;
											layer = cons->GetIndex();
										}
									}
								}
								break; // todo waiting to be deleted, this just for avoiding traffic down

							}
							//clock_t end = clock();
							//std::cout<<"Satellite select time: "<<double(end-start)*1000/CLOCKS_PER_SEC<<"ms"<<std::endl;
							if(gs2sat != nullptr){
								Topology_CHANGE_GSL = true;
								m_satelliteConnectionState[i][j_index] = connedted;
								gs2sat_vec.push_back(std::make_pair(interface, gs2sat));
								RecordFeederLinkUnique(gs2sat, groundStation);
								RecordFeederLinkForSatellites(gs2sat);
								SetRecentSatelliteByPosition(gsPosition, layer, gs2sat->GetId(), interface);
								// std::cout << "Time:" << timeNow << " ground station " << gsModel->GetGid() << " is connected" << std::endl;
								// std::cout<<"Connect Satellite : "<<gs2sat->GetId()<<std::endl;
							}
							else
							{
								gs2sat_vec.push_back(std::make_pair(interface, nullptr));
								std::cout << ">> No satellite in light of sight, ground station ID: " +  std::to_string(groundStation->GetId())<< std::endl;
							}

						}
					}
				}
				else if(m_satelliteConnectionState[i][j_index] == sleep){
					std::pair<uint32_t, Ptr<Node>> connectionEntry = gslRecordCopy[i].second[j_index];
					uint32_t interface = connectionEntry.first;
					gs2sat_vec.push_back(std::make_pair(interface, nullptr));
					std::cout << ">> Ground station ID: " +  std::to_string(groundStation->GetId()) + " is in sleep state"<< std::endl;
				}
				else{
					NS_ASSERT("Connect state invalid!");
					continue;
				}

			}

			gslRecord.push_back(std::make_pair(groundStation, gs2sat_vec));

		}
	}


	NS_OBJECT_ENSURE_REGISTERED (GeographicInformationAwareSwitching);

	TypeId
	GeographicInformationAwareSwitching::GetTypeId(void)
	{
		static TypeId tid = TypeId("ns3::GeographicInformationAwareSwitching")
				.SetParent<SwitchStrategyGSL>()
				.SetGroupName("SatelliteNetwork")
				.AddConstructor<GeographicInformationAwareSwitching>()
				;
		return tid;
	}

	GeographicInformationAwareSwitching::GeographicInformationAwareSwitching (){
	}

	GeographicInformationAwareSwitching::~GeographicInformationAwareSwitching (){

	}

	void
	GeographicInformationAwareSwitching::InitializeGeographicPartition(){

//        NS_ASSERT_MSG(m_constellations.size() == 1, "Currently only supports one layer constellation in Geographic Networking.");
//        Ptr<Constellation> cons = m_constellations[0];
//
//        std::ofstream ofs1;
//        ofs1.open(m_baseLogsDir + "/AscendingPartition_"+ std::to_string(int(Simulator::Now().GetSeconds()))+".csv");
//        ofs1.close();
//
//        std::ofstream ofs2;
//        ofs2.open(m_baseLogsDir + "/DescendingPartition_"+ std::to_string(int(Simulator::Now().GetSeconds()))+".csv");
//        ofs2.close();
//
////        // Initialize the ascending orbit partition
////        std::vector<std::pair<double, double>> initialCoor = cons->GetInitialSatelliteCoordinates();
////        std::vector<std::vector<std::pair<double, double>>> partitionCoor;
////        uint32_t satNum = cons->GetSatNum();
////        uint32_t orbitNum = cons->GetOrbitNum();
////        for(uint32_t i = 0; i < orbitNum; i++){
////        	uint32_t index = i * satNum;
////        	std::vector<std::pair<double, double>> satCoor;
////        	satCoor.insert(satCoor.end(), initialCoor.begin() + index, initialCoor.begin() + index + (satNum / 4) + 1);
////        	satCoor.insert(satCoor.begin(), initialCoor.begin() + index + (satNum - satNum / 4), initialCoor.begin() + index + (satNum));
////        	partitionCoor.push_back(satCoor);
////        }
////
////        uint32_t totalPartitionNumberLLA = 0;
////        // Totoal partiton number =  OrbitNum * (satNum / 2)
////        for(uint32_t j = 0; j < satNum / 2; j++){
////        	for(uint32_t k = 0; k < orbitNum; k++){
////        		// bottom left, top left, bottom right, top right
////        		// Numbering from bottom to top
////        		m_ascendingPartitionLLA[++totalPartitionNumberLLA] = {partitionCoor[k][j], partitionCoor[k][j + 1],
////        						partitionCoor[(k + 1) % orbitNum][j], partitionCoor[(k + 1) % orbitNum][j + 1]};
////        	}
////        }
////
////        // Initialize the descending orbit partition
////        std::vector<std::vector<std::pair<double, double>>> partitionCoorDescending;
////		for(uint32_t j = 0; j < orbitNum; j++){
////			uint32_t index = j * satNum;
////			std::vector<std::pair<double, double>> satCoor;
////			satCoor.insert(satCoor.end(), initialCoor.begin() + index + (satNum / 4), initialCoor.begin() + index + (satNum * 3 / 4) + 1);
////			partitionCoorDescending.push_back(satCoor);
////		}
////
////		uint32_t totalPartitionNumberLLADescending = 0;
////		// Totoal partiton number =  OrbitNum * (satNum / 2)
////		for(uint32_t j = 0; j < satNum / 2; j++){
////			for(uint32_t k = 0; k < orbitNum; k++){
////				// bottom left, top left, bottom right, top right
////				// Numbering from bottom to top
////				m_descendingPartitionLLA[++totalPartitionNumberLLADescending] = {partitionCoorDescending[k][j], partitionCoorDescending[k][j + 1],
////						partitionCoorDescending[(k + 1) % orbitNum][j], partitionCoorDescending[(k + 1) % orbitNum][j + 1]};
////			}
////		}
////
////
////        // Initialize the mapping relationship between partitions and satellites, there is only one satellite in each partition.
////        //UpdateMappingOfPartitionAndSatellite();



	}

	uint32_t
	GeographicInformationAwareSwitching::DetermineWhichAscendingPartitionSatelliteBelongTo(Ptr<Node> satellite){

		NS_ASSERT_MSG(m_constellations.size() == 1, "Currently only supports one layer constellation in Geographic Networking.");
		Ptr<Constellation> cons = m_constellations[0];
		int satNum = cons->GetSatNum();
		int orbitNum = cons->GetOrbitNum();


		// Solve the Angular Distance of the Ascending Node and the Right Ascension of the Ascending Node
		Vector satPosition = satellite->GetObject<MobilityModel>()->GetPosition();
		double yOPC = satPosition.z / sin(cons->GetInclination() / 180.0 * pi);
		double yOPC2 = pow(yOPC, 2);

		double xOPC2 = pow(satPosition.x, 2) + pow(satPosition.y, 2) - pow(yOPC * cos(cons->GetInclination() / 180.0 * pi), 2);
		double xOPC = sqrt(xOPC2); // > 0

		double AL;
		if(xOPC2 == 0){
			if(satPosition.z > 0){
				AL = 90;
			}
			else if(satPosition.z < 0){
				AL = 270;
			}
		}
		else{
			double tanAL = sqrt(yOPC2 / xOPC2);
			if(satPosition.z >= 0){
				AL = atan(tanAL) * 180 / pi;
			}
			else if(satPosition.z < 0){
				AL = (-1) * atan(tanAL) * 180 / pi + 360;
			}
		}
		//std::cout<<AL<<std::endl;

		double cosOmega =
				(satPosition.y * yOPC * cos(cons->GetInclination() / 180.0 * pi) + satPosition.x * xOPC)
				/ (pow(satPosition.x, 2) + pow(satPosition.y, 2));


		double omegaTemp = acos(cosOmega) * 180 / pi;
		std::vector<double> omegas;
		if(omegaTemp > 90){
			omegas.push_back(omegaTemp);
			omegas.push_back((180 + 180 - omegaTemp));
		}
		else if(omegaTemp <= 90){
			omegas.push_back(omegaTemp);
			omegas.push_back((360 - omegaTemp));
		}


		omegaTemp = 0;
		double deltaMin = 1000000000000;
		for(auto curOmega : omegas){
			double deltaX = abs(satPosition.x
					- xOPC * cos(curOmega / 180.0 * pi)
			+ yOPC * cos(cons->GetInclination() / 180.0 * pi) * sin(curOmega / 180.0 * pi));
			if(deltaX < deltaMin){
				omegaTemp = curOmega;
				deltaMin = deltaX;

			}

		}


		// Determine which partition the satellite belong to
		// Only supports the number of satellites in orbit satisfying the multiple of 4 todo
		int xCoordinate = int(omegaTemp / (360.0 / orbitNum)) + 1;
		int yCoordinate = 0;
		int p = ceil(AL / (360.0 / satNum));
		if(p >  satNum * 0.75){
			yCoordinate = p - satNum * 0.75;
		}
		else if(p <= satNum * 0.25){
			yCoordinate = p + satNum * 0.25;
		}

//		if(yCoordinate == 0){
//			std::cout<<yCoordinate<<"  "<<xCoordinate<<std::endl;
//		}
		NS_ASSERT_MSG(yCoordinate != 0, "Todo.");

		//std::cout<<yCoordinate<<"  "<<xCoordinate<<std::endl;
		return (yCoordinate - 1) * orbitNum + xCoordinate;



	}

	uint32_t
	GeographicInformationAwareSwitching::DetermineWhichDescendingPartitionSatelliteBelongTo(Ptr<Node> satellite){

		NS_ASSERT_MSG(m_constellations.size() == 1, "Currently only supports one layer constellation in Geographic Networking.");
		Ptr<Constellation> cons = m_constellations[0];
		int satNum = cons->GetSatNum();
		int orbitNum = cons->GetOrbitNum();


		// Solve the Angular Distance of the Ascending Node and the Right Ascension of the Ascending Node
		Vector satPosition = satellite->GetObject<MobilityModel>()->GetPosition();
		double yOPC = satPosition.z / sin(cons->GetInclination() / 180.0 * pi);
		double yOPC2 = pow(yOPC, 2);

		double xOPC2 = pow(satPosition.x, 2) + pow(satPosition.y, 2) - pow(yOPC * cos(cons->GetInclination() / 180.0 * pi), 2);
		double xOPC = (-1) * sqrt(xOPC2); // > 0

		double AL;
		if(xOPC2 == 0){
			if(satPosition.z > 0){
				AL = 90;
			}
			else if(satPosition.z < 0){
				AL = 270;
			}
		}
		else{
			double tanAL = sqrt(yOPC2 / xOPC2);
			if(satPosition.z >= 0){
				AL = (-1) * atan(tanAL) * 180 / pi + 180;
			}
			else if(satPosition.z < 0){
				AL = atan(tanAL) * 180 / pi + 180;
			}
		}
		//std::cout<<AL<<std::endl;

		double cosOmega =
				(satPosition.y * yOPC * cos(cons->GetInclination() / 180.0 * pi) + satPosition.x * xOPC)
				/ (pow(satPosition.x, 2) + pow(satPosition.y, 2));


		double omegaTemp = acos(cosOmega) * 180 / pi;
		std::vector<double> omegas;
		if(omegaTemp > 90){
			omegas.push_back(omegaTemp);
			omegas.push_back((180 + 180 - omegaTemp));
		}
		else if(omegaTemp <= 90){
			omegas.push_back(omegaTemp);
			omegas.push_back((360 - omegaTemp));
		}


		omegaTemp = 0;
		double deltaMin = 1000000000000;
		for(auto curOmega : omegas){
			double deltaX = abs(satPosition.x
					- xOPC * cos(curOmega / 180.0 * pi)
			+ yOPC * cos(cons->GetInclination() / 180.0 * pi) * sin(curOmega / 180.0 * pi));
			if(deltaX < deltaMin){
				omegaTemp = curOmega;
				deltaMin = deltaX;

			}

		}


		// Determine which partition the satellite belong to
		// Only supports the number of satellites in orbit satisfying the multiple of 4 todo
		int xCoordinate = int(omegaTemp / (360.0 / orbitNum)) + 1;
		int yCoordinate = 0;
		int p = ceil(AL / (360.0 / satNum));
		NS_ASSERT_MSG(satNum * 0.25 < p && p <=  satNum * 0.75, "Wrong AL.");
		yCoordinate = p - satNum * 0.25;

		NS_ASSERT_MSG(yCoordinate != 0, "Todo.");

		//std::cout<<yCoordinate<<"  "<<xCoordinate<<std::endl;
		return (yCoordinate - 1) * orbitNum + xCoordinate;


	}

	void
	GeographicInformationAwareSwitching::UpdateMappingOfPartitionAndSatellite(){

		// Note ascending orbit satellites and descending orbit satellites
		NS_ASSERT_MSG(m_constellations.size() == 1, "Currently only supports one layer constellation in Geographic Networking.");
		Ptr<Constellation> cons = m_constellations[0];

		m_mapOfAscendingPartitionAndSatellite.clear();
		m_mapOfSatelliteAndAscendingPartition.clear();
		m_mapOfDescendingPartitionAndSatellite.clear();
		m_mapOfSatelliteAndDescendingPartition.clear();

        uint32_t satNum = cons->GetSatNum();
        uint32_t orbitNum = cons->GetOrbitNum();

        for(uint32_t x = 0; x < cons->GetNodes().GetN(); x++){
        	Ptr<Node> satellite = cons->GetNodes().Get(x);
        	// Focus only on ascending orbits
        	uint32_t partitionId;
    		if(satellite->GetObject<MobilityModel>()->GetVelocity().z >= 0){
    			partitionId = DetermineWhichAscendingPartitionSatelliteBelongTo(satellite);
            	m_mapOfAscendingPartitionAndSatellite[partitionId].push_back(satellite);
            	m_mapOfSatelliteAndAscendingPartition[satellite] = partitionId;
            	if(m_mapOfAscendingPartitionAndSatellite[partitionId].size() > 1){
            		std::cout<<"Ascending -> Time(s) "<<std::to_string(int(Simulator::Now().GetSeconds()))<<" Partition: "<< partitionId<<" has "<< m_mapOfAscendingPartitionAndSatellite[partitionId].size()<< " satellites!"<<std::endl;
            	}
            	NS_ASSERT_MSG(partitionId <= satNum * orbitNum / 2, "Wrong partition Id.");
                std::ofstream ofs;
                ofs.open(m_baseLogsDir + "/AscendingPartition_"+ std::to_string(int(Simulator::Now().GetSeconds()))+".csv", std::ofstream::out | std::ofstream::app);
                ofs << satellite->GetId()<<","<<partitionId<< std::endl;
                ofs.close();
    		}
    		else if(satellite->GetObject<MobilityModel>()->GetVelocity().z < 0){
    			partitionId = DetermineWhichDescendingPartitionSatelliteBelongTo(satellite);
    			m_mapOfDescendingPartitionAndSatellite[partitionId].push_back(satellite);
    			m_mapOfSatelliteAndDescendingPartition[satellite] = partitionId;
            	if(m_mapOfDescendingPartitionAndSatellite[partitionId].size() > 1){
            		std::cout<<"Descending -> Time(s) "<<std::to_string(int(Simulator::Now().GetSeconds()))<<" Partition: "<< partitionId<<" has "<< m_mapOfDescendingPartitionAndSatellite[partitionId].size()<< " satellites!"<<std::endl;
            	}
    			NS_ASSERT_MSG(partitionId <= satNum * orbitNum / 2, "Wrong partition Id.");
    	        std::ofstream ofs;
    	        ofs.open(m_baseLogsDir + "/DescendingPartition_"+ std::to_string(int(Simulator::Now().GetSeconds()))+".csv", std::ofstream::out | std::ofstream::app);
    	        ofs << satellite->GetId()<<","<<partitionId<< std::endl;
    	        ofs.close();
    		}

        }

        SupplementMappingForSomePartitions();

//		// Plan the next update
//        int64_t dynamicStateUpdateIntervalNs = 10 * 1e9;
//		int64_t next_update_ns = Simulator::Now().GetNanoSeconds() + dynamicStateUpdateIntervalNs;
//		if (next_update_ns < m_basicSimulation->GetSimulationEndTimeNs()) {
//			Simulator::Schedule(NanoSeconds(dynamicStateUpdateIntervalNs),
//					&GeographicInformationAwareSwitching::UpdateMappingOfPartitionAndSatellite, this);
//		}

	}

	void
	GeographicInformationAwareSwitching::SupplementMappingForSomePartitions(){

		// Note ascending orbit satellites and descending orbit satellites
		NS_ASSERT_MSG(m_constellations.size() == 1, "Currently only supports one layer constellation in Geographic Networking.");
		Ptr<Constellation> cons = m_constellations[0];

        uint32_t satNum = cons->GetSatNum();
        uint32_t orbitNum = cons->GetOrbitNum();
        std::map<uint32_t, std::vector<Ptr<Node>>> mapOfAscendingPartitionAndSatellite;
        std::map<uint32_t, std::vector<Ptr<Node>>> mapOfDescendingPartitionAndSatellite;

        uint32_t totalP = m_mapOfDescendingPartitionAndSatellite.size() + m_mapOfAscendingPartitionAndSatellite.size();
        uint32_t remainP = satNum * orbitNum - totalP;
        std::cout<<"    >> "<< totalP<<" partitions are successfully mapped to satellites, and the remaining " << remainP<<" partitions need to be supplemented"<<std::endl;
        if(remainP==0){
    		return;
        }


        // Ascending partitions
        for(uint32_t pid = 1; pid <= satNum * orbitNum / 2; pid++){
        	if(m_mapOfAscendingPartitionAndSatellite.find(pid) == m_mapOfAscendingPartitionAndSatellite.end()){
        		bool found = false;
        		std::vector<uint32_t> neighborPartitions;

        		uint32_t p2; // bottom
        		if(pid > orbitNum){
        			p2 = pid - orbitNum;
        			neighborPartitions.push_back(p2);
        		}

        		uint32_t p1; // left
				if((pid - 1) % orbitNum == 0){
					p1 = pid + orbitNum - 1;
				}
				else{
					p1 = pid - 1;
				}
				neighborPartitions.push_back(p1);

				uint32_t p3; // top
        		if(pid + orbitNum <= satNum * orbitNum / 2){
        			p3 = pid + orbitNum;
        			neighborPartitions.push_back(p3);
        		}

        		uint32_t p4; // right
        		if(pid % orbitNum == 0){
        			p4 = pid - (orbitNum - 1);
				}
				else{
					p4 = pid + 1;
				}
				neighborPartitions.push_back(p4);

				for(auto curPid : neighborPartitions){
					if(m_mapOfAscendingPartitionAndSatellite.find(curPid) == m_mapOfAscendingPartitionAndSatellite.end()){
						continue;
					}
					found = true;
					mapOfAscendingPartitionAndSatellite[pid].push_back(m_mapOfAscendingPartitionAndSatellite[curPid][0]);
					break;
				}
				NS_ASSERT_MSG(found = true, "No optional satellites");


        	}
        }
        for(auto c : mapOfAscendingPartitionAndSatellite){
        	m_mapOfAscendingPartitionAndSatellite.insert(c);
            for(auto node: c.second){
            	std::ofstream ofs;
				ofs.open(m_baseLogsDir + "/AscendingPartition_"+ std::to_string(int(Simulator::Now().GetSeconds()))+".csv", std::ofstream::out | std::ofstream::app);
				ofs << node->GetId()<<","<<c.first<<","<<0<< std::endl;
				ofs.close();
            }
        }

        // Descending partitions
        for(uint32_t pid = 1; pid <= satNum * orbitNum / 2; pid++){
			if(m_mapOfDescendingPartitionAndSatellite.find(pid) == m_mapOfDescendingPartitionAndSatellite.end()){
				bool found = false;
				std::vector<uint32_t> neighborPartitions;

				uint32_t p2; // bottom
				if(pid > orbitNum){
					p2 = pid - orbitNum;
					neighborPartitions.push_back(p2);
				}

				uint32_t p1; // left
				if((pid - 1) % orbitNum == 0){
					p1 = pid + orbitNum - 1;
				}
				else{
					p1 = pid - 1;
				}
				neighborPartitions.push_back(p1);

				uint32_t p3; // top
				if(pid + orbitNum <= satNum * orbitNum / 2){
					p3 = pid + orbitNum;
					neighborPartitions.push_back(p3);
				}

				uint32_t p4; // right
				if(pid % orbitNum == 0){
					p4 = pid - (orbitNum - 1);
				}
				else{
					p4 = pid + 1;
				}
				neighborPartitions.push_back(p4);

				for(auto curPid : neighborPartitions){
					if(m_mapOfDescendingPartitionAndSatellite.find(curPid) == m_mapOfDescendingPartitionAndSatellite.end()){
						continue;
					}
					found = true;
					mapOfDescendingPartitionAndSatellite[pid].push_back(m_mapOfDescendingPartitionAndSatellite[curPid][0]);
					break;
				}
				NS_ASSERT_MSG(found = true, "No optional satellites");


			}
		}
		for(auto c : mapOfDescendingPartitionAndSatellite){
			m_mapOfDescendingPartitionAndSatellite.insert(c);
            for(auto node: c.second){
            	std::ofstream ofs;
				ofs.open(m_baseLogsDir + "/DescendingPartition_"+ std::to_string(int(Simulator::Now().GetSeconds()))+".csv", std::ofstream::out | std::ofstream::app);
				ofs << node->GetId()<<","<<c.first<<","<<0<< std::endl;
				ofs.close();
            }
		}

		NS_ASSERT_MSG(m_mapOfDescendingPartitionAndSatellite.size() + m_mapOfAscendingPartitionAndSatellite.size() == satNum * orbitNum, "No optional satellites");


	}

	void
	GeographicInformationAwareSwitching::UpdateSwitch(std::vector<std::pair<Ptr<Node>, std::vector<std::pair<uint32_t, Ptr<Node>>>>>& gslRecord, std::vector<std::pair<Ptr<Node>, std::vector<std::pair<uint32_t, Ptr<Node>>>>>& gslRecordCopy){
//
//		// Initialize partition if haven't
//		if(m_mapOfAscendingPartitionAndSatellite.size() == 0)
//			InitializeGeographicPartition();
//
//		// First update the mapping relationship
//		UpdateMappingOfPartitionAndSatellite();
//
//		// Update ground-to-satellite handover
//		for(uint32_t i = 0; i < m_groundStations.GetN(); i++){
//
//
//			Ptr<Node> groundStation = m_groundStations.Get(i);
//			Ptr<Node> gs2sat = nullptr;
//			Ptr<MobilityModel> gsMobility = groundStation->GetObject<MobilityModel>();
//
//
//			if(!gslRecordCopy.empty()
//			   && groundStation->GetId() == gslRecordCopy.at(i).first->GetId()){
//
//				NS_ASSERT_MSG(gslRecordCopy.at(i).second != nullptr, "No optional satellites");
//
//				bool satDirection = m_mapOfGroundStationAndSatelliteFlyingDirection[groundStation];
//				if(satDirection){
//					//uint32_t partitionIdAscending = DetermineWhichAscendingPartitionSatelliteBelongTo(groundStation);
//					uint32_t partitionIdAscending = m_mapOfGroundStationAndAscendingPartition[groundStation];
//
//					NS_ASSERT_MSG(m_mapOfAscendingPartitionAndSatellite.find(partitionIdAscending) != m_mapOfAscendingPartitionAndSatellite.end(), "No optional satellites");
//
//					NS_ASSERT_MSG(m_mapOfAscendingPartitionAndSatellite[partitionIdAscending].size() > 0, "No optional satellites");
//
//					auto sat = find(m_mapOfAscendingPartitionAndSatellite[partitionIdAscending].begin(),
//												m_mapOfAscendingPartitionAndSatellite[partitionIdAscending].end(),
//												gslRecordCopy.at(i).second);
//					if(sat != m_mapOfAscendingPartitionAndSatellite[partitionIdAscending].end()){
//						gs2sat = *sat;
//					}
//					else{
//						// todo
//						// If the previous satellite is invisible, select the next satellite in the same direction.
//						Topology_CHANGE_GSL = true;
//						gs2sat = m_mapOfAscendingPartitionAndSatellite[partitionIdAscending][0];
//					}
//
//
//				}
//				else{
//					//uint32_t partitionIdDescending = DetermineWhichDescendingPartitionSatelliteBelongTo(groundStation);
//					uint32_t partitionIdDescending = m_mapOfGroundStationAndDescendingPartition[groundStation];
//
//					NS_ASSERT_MSG(m_mapOfDescendingPartitionAndSatellite.find(partitionIdDescending) != m_mapOfDescendingPartitionAndSatellite.end(), "No optional satellites");
//
//					NS_ASSERT_MSG(m_mapOfDescendingPartitionAndSatellite[partitionIdDescending].size() > 0, "No optional satellites");
//
//					auto sat = find(m_mapOfDescendingPartitionAndSatellite[partitionIdDescending].begin(),
//							m_mapOfDescendingPartitionAndSatellite[partitionIdDescending].end(),
//												gslRecordCopy.at(i).second);
//					if(sat != m_mapOfDescendingPartitionAndSatellite[partitionIdDescending].end()){
//						gs2sat = *sat;
//					}
//					else{
//						// todo
//						// If the previous satellite is invisible, select the next satellite in the same direction.
//						Topology_CHANGE_GSL = true;
//						gs2sat = m_mapOfDescendingPartitionAndSatellite[partitionIdDescending][0];
//					}
//
//
//				}
//
//			}
//
//
//			// Initialization
//			if(m_mapOfGroundStationAndSatelliteFlyingDirection.find(groundStation) == m_mapOfGroundStationAndSatelliteFlyingDirection.end()){
//
//				uint32_t partitionIdAscending = DetermineWhichAscendingPartitionSatelliteBelongTo(groundStation);
//				uint32_t partitionIdDescending = DetermineWhichDescendingPartitionSatelliteBelongTo(groundStation);
//
//				NS_ASSERT_MSG(m_mapOfAscendingPartitionAndSatellite.find(partitionIdAscending) != m_mapOfAscendingPartitionAndSatellite.end(), "No optional satellites");
//
//				NS_ASSERT_MSG(m_mapOfAscendingPartitionAndSatellite[partitionIdAscending].size() > 0, "No optional satellites");
//
//				// If the previous satellite is invisible, select the next satellite in the same direction.
//				gs2sat = m_mapOfAscendingPartitionAndSatellite[partitionIdAscending][0];
//
//				m_mapOfGroundStationAndAscendingPartition[groundStation] = partitionIdAscending;
//				m_mapOfGroundStationAndDescendingPartition[groundStation] = partitionIdDescending;
//				m_mapOfGroundStationAndSatelliteFlyingDirection[groundStation] = 1;
//
//				Topology_CHANGE_GSL = true;
//			}
//
//
//			NS_ASSERT_MSG(gs2sat != nullptr, "No optional satellites");
//			if(gs2sat != nullptr){
//				//RecordFeederLinkForSatellites(gs2sat);
//				gslRecord.push_back(std::make_pair(groundStation, gs2sat));
//			}
//			else{
//				gslRecord.push_back(std::make_pair(groundStation, nullptr));
//				std::cout << ">> No satellite in light of sight, ground station ID: " +  std::to_string(groundStation->GetId())<< std::endl;
//			}
//
//
//		}


	}

	std::pair<uint32_t, bool>
	GeographicInformationAwareSwitching::GetPartitionIdOfGroundStation(Ptr<Node> groundStation){

		auto iterGsFlyingDirection = m_mapOfGroundStationAndSatelliteFlyingDirection.find(groundStation);
		NS_ASSERT_MSG(iterGsFlyingDirection != m_mapOfGroundStationAndSatelliteFlyingDirection.end(), "No such ground station");
		std::map<Ptr<Node>, uint32_t>::iterator iterGs;
		if(iterGsFlyingDirection->second){
			iterGs = m_mapOfGroundStationAndAscendingPartition.find(groundStation);
			NS_ASSERT_MSG(iterGs != m_mapOfGroundStationAndAscendingPartition.end(), "No such ground station");
		}
		else{
			iterGs = m_mapOfGroundStationAndDescendingPartition.find(groundStation);
			NS_ASSERT_MSG(iterGs != m_mapOfGroundStationAndDescendingPartition.end(), "No such ground station");
		}
		return std::make_pair(iterGs->second, iterGsFlyingDirection->second);

	}

	uint32_t
	GeographicInformationAwareSwitching::GetSatelliteCorrespondingToPartition(uint32_t partitionId, bool flyingDirection){

		if(flyingDirection){
			return m_mapOfAscendingPartitionAndSatellite[partitionId][0]->GetId();
		}
		else{
			return m_mapOfDescendingPartitionAndSatellite[partitionId][0]->GetId();
		}

	}

}




