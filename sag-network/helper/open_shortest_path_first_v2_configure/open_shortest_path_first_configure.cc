/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019 ETH Zurich
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
 */

#include "open_shortest_path_first_configure.h"

namespace ns3 {

OpenShortestPathFirstConfigure::OpenShortestPathFirstConfigure(Ptr<BasicSimulation> basicSimulation, std::vector<std::pair<SAGRoutingHelper, int16_t>>& sagRoutings)
{

    printf("PROTOCOL CONFIGURATION OPEN SHORTEST PATH FIRST\n");
    m_basicSimulation = basicSimulation;
    // Check if it is enabled explicitly
    m_enabled = parse_boolean(basicSimulation->GetConfigParamOrDefault("enable_open_shortest_path_first", "false"));
	if (!m_enabled) {
		std::cout << "  > Not enabled explicitly for open shortest path first, so disabled" << std::endl;
	}
	else{
		//<! ip_global_attribute.json
		std::string filename = basicSimulation->GetRunDir() + "/config_protocol/ip_global_attribute.json";

		// Check that the file exists
		if (!file_exists(filename)) {
			throw std::runtime_error(format_string("File %s does not exist.", filename.c_str()));
		}
		else{
			ifstream jfile(filename);
			if (jfile) {
				json j;
				jfile >> j;
				jsonns::ip_routing vi = j.at("routing");
				jsonns::ip_ospf_info vj = vi.ospf;

				int16_t priority = vj.priority;
				m_routingHelper = Sag_Open_Shortest_Path_First_Helper();
				m_routingHelper.SetObjectNameString(vj.installation_scope);
				m_routingHelper.Set("PromptMode", BooleanValue (vj.prompt_mode));

				/// set attributes...
				m_routingHelper.Set("HelloInterval", TimeValue (Seconds (vj.hello_interval_s)));
				m_routingHelper.Set("RouterDeadInterval", TimeValue (Seconds (vj.router_dead_interval_s)));
				m_routingHelper.Set("RetransmitInterval", TimeValue (Seconds (vj.retransmit_interval_s)));
				m_routingHelper.Set("LSRefreshTime", TimeValue (Seconds (vj.LSRefreshTime_s)));

				sagRoutings.push_back(std::make_pair(m_routingHelper, priority));

			}
			else{
				throw std::runtime_error(format_string("File %s could not be read.", filename.c_str()));
			}


			jfile.close();

		}

	}

    std::cout << std::endl;
}


void OpenShortestPathFirstConfigure::Initialize(Ptr<TopologySatelliteNetwork> topology){
	if (m_enabled) {
		m_routingHelper.SetTopologyHandle(topology->GetConstellations());
		m_routingHelper.InitializeArbiter(m_basicSimulation, topology->GetNodes());
	}

}




}
