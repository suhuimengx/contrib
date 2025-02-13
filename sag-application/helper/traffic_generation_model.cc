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

#include "ns3/uinteger.h"
#include "ns3/names.h"
#include "ns3/string.h"
#include <string>
#include <vector>
#include <map>
#include <tuple>
#include <cstring>
#include <fstream>
#include <cinttypes>
#include <algorithm>
#include <regex>

#include "traffic_generation_model.h"
#include "ns3/exp-util.h"
#include "ns3/topology.h"
#include "ns3/sag_application_layer_udp.h"

namespace ns3 {

TrafficGenerationModelHelper::TrafficGenerationModelHelper(Ptr<BasicSimulation> basicSimulation, Ptr<TopologySatelliteNetwork> topology){
    m_basicSimulation = basicSimulation;
    m_topology = topology;
}

void
TrafficGenerationModelHelper::DoTrafficGeneration(){

	std::string filename = m_basicSimulation->GetRunDir() + "/config_protocol/application_global_attribute.json";

	bool enabled;
	// Check that the file exists
	if (!file_exists(filename)) {
		throw std::runtime_error(format_string("File %s does not exist.", filename.c_str()));
	}
	else{
		ifstream jfile(filename);
		if (jfile) {
			json j;
			jfile >> j;
			enabled = j["traffic_model"]["enabled"];
			if(enabled){
				NS_ASSERT_MSG (
					m_basicSimulation->IsDistributedEnabled() == false, "Traffic model enabling in distribued mode is not supported now!"
				);
				bool random_generate_model = j["traffic_model"]["random_generate_model"];
				bool geo_distribution_model = j["traffic_model"]["geo_distribution_model"];
				NS_ASSERT_MSG (!(random_generate_model&geo_distribution_model), "TrafficGenerationModelHelper Error: Only one model permitted");
				if(random_generate_model){
			    	RandomlyGeneratedModel trafficModel = RandomlyGeneratedModel(m_basicSimulation, m_topology);
				}
				else if(geo_distribution_model){
					// todo
				}
			}
		}
		else{
			throw std::runtime_error(format_string("File %s could not be read.", filename.c_str()));
		}

		jfile.close();

	}
}

TrafficGenerationModel::TrafficGenerationModel(Ptr<BasicSimulation> basicSimulation, Ptr<TopologySatelliteNetwork> topology)
{
    m_basicSimulation = basicSimulation;
    m_topology = topology;

}

void TrafficGenerationModel::TrafficGenerate()
{

}

uint32_t
TrafficGenerationModel::ReadTrafficModelSet(){

	std::string filename = m_basicSimulation->GetRunDir() + "/config_protocol/application_global_attribute.json";

	uint32_t ntotal = 0;
	// Check that the file exists
	if (!file_exists(filename)) {
		throw std::runtime_error(format_string("File %s does not exist.", filename.c_str()));
	}
	else{
		ifstream jfile(filename);
		if (jfile) {
			json j;
			jfile >> j;
			ntotal = j["udp_flow"]["total_flow_number"];
		}
		else{
			throw std::runtime_error(format_string("File %s could not be read.", filename.c_str()));
		}

		jfile.close();

	}
	return ntotal;
}

RandomlyGeneratedModel::RandomlyGeneratedModel(Ptr<BasicSimulation> basicSimulation, Ptr<TopologySatelliteNetwork> topology)
:TrafficGenerationModel(basicSimulation, topology)
{
	TrafficGenerate();
}

void RandomlyGeneratedModel::TrafficGenerate()
{
    // random seed
    std::srand(1);

	uint32_t ntotal = TrafficGenerationModel::ReadTrafficModelSet();
	uint32_t nearthStations = m_topology->GetGroundStationNodes().GetN();
	int64_t endTime = parse_positive_int64(m_basicSimulation->GetConfigParamOrFail("simulation_end_time_ns"));
    remove_file_if_exists(m_basicSimulation->GetRunDir() + "/config_traffic/application_schedule_udp.json");

	// udp flows
    nlohmann::ordered_json jsonObject;
    for(uint32_t n = 0; n < ntotal; n++){
    	nlohmann::ordered_json jsonFlowObject;
    	jsonFlowObject["flow_id"] = n;
    	uint32_t s = 0, d = 0;
    	while(s == d){
    		s = GetRandomNumber(0, nearthStations-1);
    		d = GetRandomNumber(0, nearthStations-1);
    	}
    	jsonFlowObject["sender"] = s;
    	jsonFlowObject["receiver"] = d;
    	jsonFlowObject["target_flow_rate_mbps"] = 4;
    	jsonFlowObject["start_time_ns"] = 1000000000;
		NS_ASSERT_MSG (endTime > 1000000000, "RandomlyGeneratedModel Error: endTime <= 1s");
    	jsonFlowObject["duration_time_ns"] = endTime - 1000000000;
    	jsonFlowObject["code_type"] = "SYNCODEC_TYPE_PERFECT";
    	jsonFlowObject["service_type"] = 0;
    	jsonObject["udp_flows"].push_back(jsonFlowObject);
    }

	std::ofstream pathRecord(m_basicSimulation->GetRunDir() + "/config_traffic/application_schedule_udp.json", std::ofstream::out);
	if (pathRecord.is_open()) {
		pathRecord << jsonObject.dump(4);  // 使用缩进格式将 JSON 内容写入文件
		pathRecord.close();
		//std::cout << "JSON file created successfully." << std::endl;
	} else {
		std::cout << "Failed to create JSON file." << std::endl;
	}
}

uint32_t GetRandomNumber(uint32_t min, uint32_t max) {
    static const double fraction = 1.0 / (RAND_MAX + 1.0);
    return min + static_cast<uint32_t>((max - min + 1) * (std::rand() * fraction));
}


}
