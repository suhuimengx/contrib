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

#ifndef ROUTING_MODULE_MONITOR_H
#define ROUTING_MODULE_MONITOR_H

#include "ns3/basic-simulation.h"
#include "ns3/exp-util.h"
#include "ns3/topology.h"
#include "ns3/sag_routing_protocal.h"
#include "ns3/sag_routing_protocal_ipv6.h"

namespace ns3 {

class RoutingModuleMonitor
{

public:
	RoutingModuleMonitor(Ptr<BasicSimulation> basicSimulation, Ptr<Topology> topology);
    virtual void WriteResults();
    virtual void RoutingOverheadStatistics(Time time, uint32_t size, uint32_t nodeId);
    void RoutingCalculationStatistics(uint32_t nodeId, Time curTime, double calculationTime);

protected:
    Ptr<BasicSimulation> m_basicSimulation;
    Ptr<Topology> m_topology;
    bool m_enabled;
    NodeContainer m_nodes;
    std::vector<uint32_t> m_overheadCounterInByte;
    std::string m_logsDir;

};




}



#endif /* ROUTING_MODULE_MONITOR_H */
