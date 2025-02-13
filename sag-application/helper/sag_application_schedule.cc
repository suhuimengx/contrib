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

#include "ns3/sag_application_schedule.h"
#include "ns3/exp-util.h"
#include "ns3/topology.h"
#include "ns3/sag_application_layer_udp.h"

namespace ns3 {

SAGApplicationScheduler::SAGApplicationScheduler(Ptr<BasicSimulation> basicSimulation, Ptr<TopologySatelliteNetwork> topology)
{
    m_basicSimulation = basicSimulation;
    m_topology = topology;
    m_nodes = m_topology->GetNodes();
    m_simulation_end_time_ns = m_basicSimulation->GetSimulationEndTimeNs();
    m_system_id = m_basicSimulation->GetSystemId();
    m_enable_distributed = m_basicSimulation->IsDistributedEnabled();
    m_distributed_node_system_id_assignment = m_basicSimulation->GetDistributedNodeSystemIdAssignment();

}

void SAGApplicationScheduler::WriteResults() 
{

}

void SAGApplicationScheduler::ReadLoggingForApplicationIds(std::string flowType){
	//<! config_analysis/network_statistic.json
	std::string filename = m_basicSimulation->GetRunDir() + "/config_analysis/network_statistic.json";

	// Check that the file exists
	if (!file_exists(filename)) {
		throw std::runtime_error(format_string("File %s does not exist.", filename.c_str()));
	}
	else{
		ifstream jfile(filename);
		if (jfile) {
			json j;
			jfile >> j;
			json list = j["object_statistics"][flowType]["enabled_list"];
			auto vi = list.size();

			for (int i = 0; i < (int)vi; i++)
			{
				int64_t id = list[i];
				m_enable_logging_for_sag_application_ids.insert(id);
			}

		}
		else{
			throw std::runtime_error(format_string("File %s could not be read.", filename.c_str()));
		}

		jfile.close();

	}


}

bool
SAGApplicationScheduler::ReadApplicationEnable(std::string flowType){
	//<! config_analysis/network_statistic.json
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
			enabled = j[flowType]["application_enabled"];
		}
		else{
			throw std::runtime_error(format_string("File %s could not be read.", filename.c_str()));
		}

		jfile.close();

	}
	return enabled;
}

}
