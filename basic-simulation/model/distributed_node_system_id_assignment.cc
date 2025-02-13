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
 * Author: Skypty <@163.com>
 */

#include "distributed_node_system_id_assignment.h"
#include "cppjson2structure.hh"

namespace ns3 {

//NS_LOG_COMPONENT_DEFINE("PartitioningTopology");
NS_OBJECT_ENSURE_REGISTERED (PartitioningTopology);

TypeId
PartitioningTopology::GetTypeId (void)
{
    static TypeId tid = TypeId("ns3::PartitioningTopology").
    		            SetParent<Object>().
						SetGroupName("SatelliteNetwork")
    ;
    return tid;
}

void
PartitioningTopology::SetBasicSim(Ptr<BasicSimulation> basicsimulation){
	m_basicsimulation = basicsimulation;
}

std::vector<int64_t>
PartitioningTopology::GetDistributedNodeSystemIdAssignment(std::string algorithm, std::string satellite_network_dir, int64_t orbits, int64_t sats, int64_t groundStation_counter, uint32_t systems_cnt){
	Ptr<AlgorithmForPartitioning> n = CreateObject<AlgorithmForPartitioning>();
	if (algorithm == "default") return n->DivideEvenlyInOrder(satellite_network_dir, orbits, sats, groundStation_counter, systems_cnt);
	//else if (algorithm == "DivideEvenlyAtRandom") return n->DivideEvenlyAtRandom(num, systems_cnt);
	else throw std::runtime_error(format_string("Unknown distributed Node to System Id Assign Algorithm type: %s", algorithm));
}

std::vector<int64_t>
PartitioningTopology::GetDistributedNodeSystemIdAssignment(std::string algorithm, std::string satellite_network_dir, int64_t satellite_counter, int64_t groundStation_counter, uint32_t systems_cnt){
	Ptr<AlgorithmForPartitioning> n = CreateObject<AlgorithmForPartitioning>();
	if (algorithm == "default"){
		return n->DivideEvenlyInOrder(satellite_network_dir, satellite_counter, groundStation_counter, systems_cnt);
	}
	else if(algorithm == "algorithm1"){
		return n->HandoverInterProcess(satellite_network_dir, satellite_counter, groundStation_counter, systems_cnt);
	}
	else if(algorithm == "customize"){
		return n->CustomizeMode(m_basicsimulation->GetRunDir(), satellite_counter, groundStation_counter, systems_cnt);
	}
	else
		throw std::runtime_error(format_string("Unknown distributed Node to System Id Assign Algorithm type: %s", algorithm));
}

std::vector<int64_t>
PartitioningTopology::GetDistributedNodeSystemIdAssignmentSat(std::string algorithm,int64_t num,uint32_t systems_cnt){
  Ptr<AlgorithmForPartitioning> n = CreateObject<AlgorithmForPartitioning>();
  if (algorithm == "default") return n->DivideEvenlyInOrder(num, systems_cnt);
   else if (algorithm == "DivideEvenlyAtRandom") return n->DivideEvenlyAtRandom(num, systems_cnt);
   else throw std::runtime_error(format_string("Unknown distributed Node to System Id Assign Algorithm type: %s", algorithm));

}

std::vector<int64_t>
PartitioningTopology::GetDistributedNodeSystemIdAssignmentGd(std::string algorithm,int64_t num,uint32_t systems_cnt){
	Ptr<AlgorithmForPartitioning> n = CreateObject<AlgorithmForPartitioning>();
	if (algorithm == "default") return n->DivideEvenlyInOrder(num ,systems_cnt);
	  else if (algorithm == "DivideEvenlyAtRandom") return n->DivideEvenlyAtRandom(num ,systems_cnt);
	  else throw std::runtime_error(format_string("Unknown distributed Node to System Id Assign Algorithm type: %s", algorithm));

}

std::vector<int>
PartitioningTopology::GetAndCheckNodeToSystemIdAssignment(std::vector<int64_t> node,std::string nodetype,uint32_t m_systems_count) {
	std::vector<int> system_id_counter(m_systems_count, 0);
    for (uint32_t i = 0; i < node.size(); i++) {
        if (node[i] < 0 || node[i] >= m_systems_count) {
            throw std::invalid_argument(
                    format_string(
                    "%s Node %d is assigned to an invalid system id %" PRId64 " (k=%" PRId64 ")",
				    nodetype.c_str(),
                    i,
                    node[i],
                    m_systems_count
            )
            );

        }
        system_id_counter[node[i]]++;
    }
   return system_id_counter;
}

//NS_LOG_COMPONENT_DEFINE("AlgorithmForPartitioning");
NS_OBJECT_ENSURE_REGISTERED (AlgorithmForPartitioning);

TypeId
AlgorithmForPartitioning::GetTypeId (void)
{
    static TypeId tid = TypeId("ns3::AlgorithmForPartitioning").
    		            SetParent<Object>().
						SetGroupName("SatelliteNetwork")
    ;
    return tid;
}

std::vector<int64_t>
AlgorithmForPartitioning::DivideEvenlyInOrder(std::string satellite_network_dir, int64_t satellite_counter, int64_t groundStation_counter, uint32_t systems_cnt){
	printf("Using Divide Evenly In Order Algorithm\n");
	// Initialize assignment of satellites
	std::vector<int64_t> assignment;
	int num_per_LP = satellite_counter / systems_cnt;
	for (uint32_t i = 0; i < satellite_counter; i++){// so begin i = 1
		int64_t belongtoLP = 0;
		if(i == 0){
			assignment.push_back(belongtoLP);
			continue;
		}
		belongtoLP = (int64_t)(i / num_per_LP);
		if(belongtoLP >= systems_cnt)
			belongtoLP = systems_cnt - 1;
		assignment.push_back(belongtoLP);
	}

	if(groundStation_counter == 0){
		return assignment;
	}

	// Considering ground stations
	std::ifstream fs;
	fs.open(satellite_network_dir + "/system_" + std::to_string(0)+"_topology_change_message.txt");
	NS_ABORT_MSG_UNLESS(fs.is_open(), "topology_change_message.txt");

	// Read ground station from each line
	std::unordered_map<uint32_t, std::vector<uint32_t>> gs2Sats;
	std::string line;
	while (std::getline(fs, line)) {

		std::vector<std::string> res = split_string(line, ",", 3);

		// All values
		//uint32_t time = parse_positive_int64(res[0]); // ms
		uint32_t gs = parse_positive_int64(res[1]);
		uint32_t sat = parse_positive_int64(res[2]);

		gs2Sats[gs].push_back(sat);

	}
	if(gs2Sats.size() != (uint32_t)groundStation_counter){
		throw std::runtime_error("AlgorithmForPartitioning::DivideEvenlyInOrder");
	}
	std::unordered_map<uint32_t, std::vector<uint32_t>> records_1;  // sat v<gs>
	for(uint32_t id = satellite_counter; id < satellite_counter+groundStation_counter; id++){
		if(gs2Sats[id].size() == 0){
			throw std::runtime_error("AlgorithmForPartitioning::DivideEvenlyInOrder.");
		}
		//uint32_t gs = id;
		std::vector<uint32_t> sats = gs2Sats[id];
		bool done = true;
		std::set<uint32_t> gss;
		for(auto sat: sats){
			if(records_1[sat].size() != 0){
				done = false;
				auto v = records_1[sat];
				gss.insert(v.begin(), v.end());
			}
		}
		if(done){
			for(uint32_t i = 0; i < sats.size(); i++){
				assignment[sats[i]] = assignment[sats[0]];
				records_1[sats[i]].push_back(id);
			}
			assignment.push_back(assignment[sats[0]]);
		}
		else{
			uint32_t gcur = *gss.begin();
			auto sid = assignment[gcur];
			for(auto g: gss){
				for(auto s: gs2Sats[g]){
					assignment[s] = sid;
				}
				assignment[g] = sid;
			}
			for(uint32_t i = 0; i < sats.size(); i++){
				assignment[sats[i]] = sid;
				records_1[sats[i]].push_back(id);
			}
			assignment.push_back(sid);

		}

	}

	std::ofstream assignmentTxt(satellite_network_dir + "/system_" +std::to_string(MpiInterface::GetSystemId()) +"_distributed_node_system_id_assignment.txt", std::ofstream::out);
	for(auto p: gs2Sats){
		assignmentTxt<<"GroundStation "<<p.first<<"  "<<assignment[p.first]<<std::endl;
		for(auto sat: p.second){
			assignmentTxt<<sat<<"  "<<assignment[sat]<<std::endl;
		}
	}
	assignmentTxt.close();



	return assignment;
}


std::vector<int64_t>
AlgorithmForPartitioning::HandoverInterProcess(std::string satellite_network_dir, int64_t satellite_counter, int64_t groundStation_counter, uint32_t systems_cnt){
	printf("Using Satellite-Ground Decoupled Cross-Process Algorithm\n");
	// Initialize assignment of satellites
	std::vector<int64_t> assignment;
	int num_per_LP = satellite_counter / (systems_cnt-1);
	for (uint32_t i = 0; i < satellite_counter; i++){// so begin i = 1
		int64_t belongtoLP = 0;
		if(i == 0){
			assignment.push_back(belongtoLP);
			continue;
		}
		belongtoLP = (int64_t)(i / num_per_LP);
		if(belongtoLP >= systems_cnt - 1)
			belongtoLP = systems_cnt - 2;
		assignment.push_back(belongtoLP);
	}

	if(groundStation_counter == 0){
		return assignment;
	}

	// Considering ground stations
	for (uint32_t gs = 0; gs < groundStation_counter; gs++){
		assignment.push_back(systems_cnt - 1);
	}



	return assignment;

}

std::vector<int64_t>
AlgorithmForPartitioning::CustomizeMode(std::string run_dir, int64_t satellite_counter, int64_t groundStation_counter, uint32_t systems_cnt){

	std::string filename = run_dir + "/coalitions.json";

	std::vector<int64_t> assignment(satellite_counter+groundStation_counter, 0);
	uint32_t ntotal = 0;
	// Check that the file exists
	if (!file_exists(filename)) {
		throw std::runtime_error(format_string("File %s does not exist.", filename.c_str()));
	}
	else{
		std::ifstream jfile(filename);
		if (jfile) {
			json j;
			jfile >> j;
			ntotal = j.size();
			for(uint32_t k = 0; k < ntotal; k++){
				for(uint32_t p = 0; p < j[k].size(); p++){
					assignment[j[k][p]] = k;
				}

			}
		}
		else{
			throw std::runtime_error(format_string("File %s could not be read.", filename.c_str()));
		}

		jfile.close();

	}

	return assignment;


}

std::vector<int64_t>
AlgorithmForPartitioning::DivideEvenlyInOrder(std::string satellite_network_dir, int64_t orbits, int64_t satsPerOrbit, int64_t groundStation_counter, uint32_t systems_cnt){
	printf("Using Divide Evenly In Order Algorithm\n");
	// Initialize assignment of satellites
	int64_t satellite_counter = orbits * satsPerOrbit;
	uint32_t satsPerProcess = satellite_counter / systems_cnt;

	std::vector<int64_t> assignment(satellite_counter, -1);
	float A = orbits / satsPerOrbit;
	float length_f = sqrt(systems_cnt / A);
	float width_f = A * length_f;
	uint32_t areaLength = (uint32_t)length_f;
	uint32_t areaWidth = (uint32_t)width_f;
	if(areaLength*areaWidth > systems_cnt){
		throw std::runtime_error("AlgorithmForPartitioning::DivideEvenlyInOrder!");
	}

	//<! there may be a bug when phase factor changes, which need to be optimised
	//<! there may be a bug when there has more than one layer, which need to be optimised
	//<! here is just a case
	uint32_t m_length = satsPerOrbit / areaLength;
	uint32_t m_width = orbits / areaWidth;
	while(m_length * m_width > satsPerProcess){
		if(m_length > m_width){
			m_length--;
		}
		else{
			m_width--;
		}
	}

	std::vector<uint32_t> remainNodes;
	uint32_t counter = 0;
	for(uint32_t s = 0; s < satellite_counter; s++){
		uint32_t orbitNum = s / satsPerOrbit; // start from 0
		uint32_t satelliteNum = s % satsPerOrbit; // start from 0

		if(satelliteNum / m_length > (areaLength-1) || orbitNum / m_width > (areaWidth-1)){
			remainNodes.push_back(s);
			continue;
		}
		uint32_t curArea = areaLength * (orbitNum / m_width) + satelliteNum / m_length;
		assignment[s] = curArea;
		counter++;
	}
	if(counter + remainNodes.size() != (uint32_t)satellite_counter){
		throw std::runtime_error("AlgorithmForPartitioning::DivideEvenlyInOrder!!");
	}
	if(remainNodes.size()!=0){
		uint32_t remainSystems = systems_cnt - areaLength*areaWidth;
		uint32_t systemNewStartId = areaLength*areaWidth;
		uint32_t newSatsPerSystem = remainNodes.size() / remainSystems;
		if(newSatsPerSystem == 0){
			throw std::runtime_error("AlgorithmForPartitioning::DivideEvenlyInOrder!!!");
		}
		for (uint32_t i = 0; i < remainNodes.size(); i++){
			int64_t belongtoLP = (int64_t)(i / newSatsPerSystem) + systemNewStartId;
			if(belongtoLP >= systems_cnt)
				belongtoLP = systems_cnt - 1;
			assignment[remainNodes[i]] = belongtoLP;
		}
	}

	if(groundStation_counter == 0){
		return assignment;
	}

	// Considering ground stations
	std::ifstream fs;
	fs.open(satellite_network_dir + "/system_" + std::to_string(0)+"_topology_change_message.txt");
	NS_ABORT_MSG_UNLESS(fs.is_open(), "topology_change_message.txt");

	// Read ground station from each line
	std::unordered_map<uint32_t, std::vector<uint32_t>> gs2Sats;
	std::string line;
	while (std::getline(fs, line)) {

		std::vector<std::string> res = split_string(line, ",", 3);

		// All values
		//uint32_t time = parse_positive_int64(res[0]); // ms
		uint32_t gs = parse_positive_int64(res[1]);
		uint32_t sat = parse_positive_int64(res[2]);

		gs2Sats[gs].push_back(sat);

	}
	if(gs2Sats.size() != (uint32_t)groundStation_counter){
		throw std::runtime_error("AlgorithmForPartitioning::DivideEvenlyInOrder");
	}
	std::unordered_map<uint32_t, std::vector<uint32_t>> records_1;  // sat v<gs>
	for(uint32_t id = satellite_counter; id < satellite_counter+groundStation_counter; id++){
		if(gs2Sats[id].size() == 0){
			throw std::runtime_error("AlgorithmForPartitioning::DivideEvenlyInOrder.");
		}
		//uint32_t gs = id;
		std::vector<uint32_t> sats = gs2Sats[id];
		bool done = true;
		std::set<uint32_t> gss;
		for(auto sat: sats){
			if(records_1[sat].size() != 0){
				done = false;
				auto v = records_1[sat];
				gss.insert(v.begin(), v.end());
			}
		}
		if(done){
			for(uint32_t i = 0; i < sats.size(); i++){
				assignment[sats[i]] = assignment[sats[0]];
				records_1[sats[i]].push_back(id);
			}
			assignment.push_back(assignment[sats[0]]);
		}
		else{
			uint32_t gcur = *gss.begin();
			auto sid = assignment[gcur];
			for(auto g: gss){
				for(auto s: gs2Sats[g]){
					assignment[s] = sid;
				}
				assignment[g] = sid;
			}
			for(uint32_t i = 0; i < sats.size(); i++){
				assignment[sats[i]] = sid;
				records_1[sats[i]].push_back(id);
			}
			assignment.push_back(sid);

		}

	}

	std::ofstream assignmentTxt(satellite_network_dir + "/system_" +std::to_string(MpiInterface::GetSystemId()) +"_distributed_node_system_id_assignment.txt", std::ofstream::out);
	for(auto p: gs2Sats){
		assignmentTxt<<"GroundStation "<<p.first<<"  "<<assignment[p.first]<<std::endl;
		for(auto sat: p.second){
			assignmentTxt<<sat<<"  "<<assignment[sat]<<std::endl;
		}
	}
	assignmentTxt.close();



	return assignment;
}


std::vector<int64_t>
AlgorithmForPartitioning::DivideEvenlyInOrder(int64_t num, uint32_t systems_cnt){
	printf("Using Divide Evenly In Order Algorithm\n");
	std::vector<int64_t> assignment;
	if (num == 0) { // zero can't be divided
		assignment.clear();
		return assignment;
	}
	//assignment.push_back(0);// node 0 assign to Rank 0
	int num_per_LP = num / systems_cnt;
	for (uint32_t i = 0; i < num; i++){// so begin i = 1
		int64_t belongtoLP = 0;
		if(i == 0){
			assignment.push_back(belongtoLP);
			continue;
		}
		belongtoLP = (int64_t)(i / num_per_LP);
		if(belongtoLP >= systems_cnt)
			belongtoLP = systems_cnt - 1;
		assignment.push_back(belongtoLP);
	}

	return assignment;

}

std::vector<int64_t>
AlgorithmForPartitioning::DivideEvenlyAtRandom(int64_t num,uint32_t systems_cnt){
	// check here ,haven't  been tested
    printf("Using Divide Evenly At Random Algorithm\n");
    std::vector<int64_t> ans;
    //int num_per_LP = num / systems_cnt;
    Ptr<UniformRandomVariable> LP_uv = CreateObject<UniformRandomVariable>();
    //Plus one here than minus before,here just to Make sure the split is equally
    LP_uv->SetAttribute ("Min", DoubleValue (1.0));
    LP_uv->SetAttribute ("Max", DoubleValue (double( systems_cnt+1 )) );
    for (uint32_t i = 0; i < num; i++){
    	double value = LP_uv->GetValue ();
    	int64_t belongtoLP = int64_t(value)-1;
    	ans.push_back(belongtoLP);
    	}
    return ans;

}



} // namespace ns3


