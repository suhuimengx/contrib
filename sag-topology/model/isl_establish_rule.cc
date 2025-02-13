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

#include "isl_establish_rule.h"

namespace ns3 {

    //NS_LOG_COMPONENT_DEFINE ("SwitchStrategyGSL");

    NS_OBJECT_ENSURE_REGISTERED (ISLEstablishRule);

    TypeId
	ISLEstablishRule::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::ISLEstablishRule")
                .SetParent<Object>()
                .SetGroupName("SatelliteNetwork")
                .AddConstructor<ISLEstablishRule>()
				;
        return tid;
    }

    ISLEstablishRule::ISLEstablishRule (){

    }

    ISLEstablishRule::~ISLEstablishRule (){

	}
    void ISLEstablishRule::SetBasicSimulationHandle(Ptr<BasicSimulation> basicSimulation){
    	m_basicSimulation = basicSimulation;
    }

    void ISLEstablishRule::SetTopologyHandle(std::vector<Ptr<Constellation>> constellations){
    	m_constellations = constellations;
    }

    void ISLEstablishRule::SetSatelliteNetworkDir(std::string satelliteNetworkDir){
    	m_satelliteNetworkDir = satelliteNetworkDir;
    }

    void ISLEstablishRule::ISLEstablish(std::vector<std::pair<uint32_t, uint32_t>>& islInterOrbit){

    }

    NS_OBJECT_ENSURE_REGISTERED (GridTypeBuilding);

    TypeId
	GridTypeBuilding::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::GridTypeBuilding")
                .SetParent<Object>()
                .SetGroupName("SatelliteNetwork")
                .AddConstructor<GridTypeBuilding>()
				.AddAttribute("ISLShift", "For ISL connection principle, 0 means connect to satellite at the adjacent orbit.",
							UintegerValue(0),
							MakeUintegerAccessor(&GridTypeBuilding::m_islShift),
							MakeUintegerChecker<uint16_t>())
				;
        return tid;
    }

    GridTypeBuilding::GridTypeBuilding ()
    : m_islShift(0)
    {

    }

    GridTypeBuilding::~GridTypeBuilding (){

	}

    void GridTypeBuilding::ISLEstablish(std::vector<std::pair<uint32_t, uint32_t>>& islInterOrbit){
		int idx_offset = 0; //<! for multi-layer
		for(auto cons : m_constellations){
			std::string name = cons->GetName();
			remove_file_if_exists(m_satelliteNetworkDir + "/" + "system_" + std::to_string(m_basicSimulation->GetSystemId()) + "_" + name +"_adjacency.txt");
			//std::ofstream fileISL(m_satelliteNetworkDir + "/" + name + "_adjacency.txt");
			std::ofstream fileISL(m_satelliteNetworkDir + "/" + "system_" + std::to_string(m_basicSimulation->GetSystemId()) + "_" + name +"_adjacency.txt");
			std::vector<std::pair<int,int>> list_isls;
			auto n_orbits = cons->GetOrbitNum();
			auto n_sats_per_orbit = cons->GetSatNum();
			auto n_phaseFactor = cons->GetPhase(); //<! the last orbit's ISL offset by phase factor
			auto type = cons->GetType();
			std::vector<std::pair<uint32_t, uint32_t>> islInterOrbit_CurCon;

			for(int i = 0; i < n_orbits - 1; i++){
				for(int j = 0; j < n_sats_per_orbit; j++){
					int sat = i * n_sats_per_orbit + j;
					//Link to the next in the orbit
					int sat_same_orbit = i * n_sats_per_orbit + ((j + 1) % n_sats_per_orbit);
					int sat_adjacent_orbit = ((i + 1) % n_orbits) * n_sats_per_orbit + ((j + m_islShift) % n_sats_per_orbit);
					//Same orbit
					list_isls.push_back(std::make_pair(idx_offset + std::min(sat, sat_same_orbit), idx_offset + std::max(sat, sat_same_orbit)));

					//Adjacent orbit
					list_isls.push_back(std::make_pair(idx_offset + std::min(sat, sat_adjacent_orbit), idx_offset + std::max(sat, sat_adjacent_orbit)));
					islInterOrbit_CurCon.push_back(std::make_pair(idx_offset + std::min(sat, sat_adjacent_orbit), idx_offset + std::max(sat, sat_adjacent_orbit)));
				}
			}
			// exception occurs for the (n_orbits - 1)th orbit
			if(type == "delta" || type == "Delta" ){
				for(int j = 0; j < n_sats_per_orbit; j++){
					int sat = (n_orbits - 1) * n_sats_per_orbit + j;
					//Link to the next in the orbit
					int sat_same_orbit = (n_orbits - 1) * n_sats_per_orbit + ((j + 1) % n_sats_per_orbit);
					int sat_adjacent_orbit = (((n_orbits - 1) + 1) % n_orbits) * n_sats_per_orbit + ((j + m_islShift + n_phaseFactor) % n_sats_per_orbit);
					//Same orbit
					list_isls.push_back(std::make_pair(idx_offset + std::min(sat, sat_same_orbit), idx_offset + std::max(sat, sat_same_orbit)));

					//Adjacent orbit
					list_isls.push_back(std::make_pair(idx_offset + std::min(sat, sat_adjacent_orbit), idx_offset + std::max(sat, sat_adjacent_orbit)));
					islInterOrbit_CurCon.push_back(std::make_pair(idx_offset + std::min(sat, sat_adjacent_orbit), idx_offset + std::max(sat, sat_adjacent_orbit)));

				}
			}
			else if(type == "star" || type == "Star"){
				for(int j = 0; j < n_sats_per_orbit; j++){
					int sat = (n_orbits - 1) * n_sats_per_orbit + j;
					//Link to the next in the orbit
					int sat_same_orbit = (n_orbits - 1) * n_sats_per_orbit + ((j + 1) % n_sats_per_orbit);
					//Same orbit
					list_isls.push_back(std::make_pair(idx_offset + std::min(sat, sat_same_orbit), idx_offset + std::max(sat, sat_same_orbit)));

					//Adjacent orbit None
				}

			}
			else{
				throw std::runtime_error("Wrong input of constellation type.");
			}



			idx_offset += n_orbits * n_sats_per_orbit;

			for(auto isl : list_isls){
				fileISL<<isl.first<<" "<<isl.second<<std::endl;
			}
			fileISL.close();

			// just for Star type walker constellation
			if(type == "Star" || type == "star" ){
				islInterOrbit.insert(islInterOrbit.end(), islInterOrbit_CurCon.begin(), islInterOrbit_CurCon.end());
			}

		}
    }

}


