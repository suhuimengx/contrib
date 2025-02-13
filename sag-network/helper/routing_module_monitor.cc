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
#include "routing_module_monitor.h"

namespace ns3 {

RoutingModuleMonitor::RoutingModuleMonitor(Ptr<BasicSimulation> basicSimulation, Ptr<Topology> topology){

	printf("Routing Module Monitor\n");
	// Check if it is enabled explicitly
	m_enabled = parse_boolean(basicSimulation->GetConfigParamOrDefault("enable_routing_status_tracing", "false"));
	if (!m_enabled) {
		std::cout << "  > Not enabled explicitly, so disabled" << std::endl;

	} else {
		std::cout << "  > SAG applicaiton scheduler is enabled" << std::endl;
		m_basicSimulation = basicSimulation;
		m_topology = topology;
		m_nodes = m_topology->GetNodes();
		std::vector<uint32_t> overheadCounter(m_nodes.GetN(), 0);
		m_overheadCounterInByte = overheadCounter;
		m_logsDir = m_basicSimulation->GetLogsDir();


		for(uint32_t n = 0; n < m_nodes.GetN(); n++){
			Ptr<Ipv4> ipv4 = m_nodes.Get(n)->GetObject<Ipv4>();
			// set send callback
			ipv4->GetRoutingProtocol()->GetObject<SAGRoutingProtocal>()
					->SetSendCallback(MakeCallback(&RoutingModuleMonitor::RoutingOverheadStatistics, this));

			ipv4->GetRoutingProtocol()->GetObject<SAGRoutingProtocal>()
					->SetRoutingCalculationCallback(MakeCallback(&RoutingModuleMonitor::RoutingCalculationStatistics, this));

			Ptr<Ipv6> ipv6 = m_nodes.Get(n)->GetObject<Ipv6>();
			// set send callback
			ipv6->GetRoutingProtocol()->GetObject<SAGRoutingProtocalIPv6>()
					->SetSendCallback(MakeCallback(&RoutingModuleMonitor::RoutingOverheadStatistics, this));

			ipv6->GetRoutingProtocol()->GetObject<SAGRoutingProtocalIPv6>()
					->SetRoutingCalculationCallback(MakeCallback(&RoutingModuleMonitor::RoutingCalculationStatistics, this));

		}
	}

    std::cout << std::endl;


}


void
RoutingModuleMonitor::WriteResults(){

	std::cout << "STORE ROUTING OVERHEAD RESULTS" << std::endl;
	// Check if it is enabled explicitly
	if (!m_enabled) {
		std::cout << "  > Not enabled, so no routing overhead results are written" << std::endl;

	} else {

		uint32_t totalOverhead = 0; // KB
		std::string dir = m_logsDir + "/router_overhead.txt";
		remove_file_if_exists(dir);
		std::ofstream file(dir);

		for(uint32_t id = 0; id < m_nodes.GetN(); id++){
			totalOverhead += m_overheadCounterInByte.at(id)/1e3;
			file << id << "," << m_overheadCounterInByte.at(id) << std::endl;
		}
		std::cout<<totalOverhead<<" KB"<<std::endl;

		file.close();
	}

}

void
RoutingModuleMonitor::RoutingOverheadStatistics(Time time, uint32_t size, uint32_t nodeId){

	m_overheadCounterInByte.at(nodeId) += size;

}

void
RoutingModuleMonitor::RoutingCalculationStatistics(uint32_t nodeId, Time curTime, double calculationTime){

	std::ofstream ofs;
    ofs.open(m_logsDir + "/" + format_string("route_calculation_node_%" PRIu64 ".csv", nodeId), std::ofstream::out | std::ofstream::app);
    ofs << nodeId << "," << curTime.GetNanoSeconds() << "," << calculationTime << std::endl;
    //std::cout << ((double)(endTime - startTime) / CLOCKS_PER_SEC) * 1e9 << std::endl;
    ofs.close();

}


}
