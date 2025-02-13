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
#ifndef SAG_APPLICATION_SCHEDULER_H
#define SAG_APPLICATION_SCHEDULER_H

#include <map>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <chrono>
#include <stdexcept>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/random-variable-stream.h"
#include "ns3/command-line.h"

#include "ns3/basic-simulation.h"
#include "ns3/exp-util.h"
#include "ns3/topology-satellite-network.h"

#include "ns3/sag_application_helper.h"
#include "ns3/sag_burst_info.h"

namespace ns3 {

class SAGApplicationScheduler
{

public:
    SAGApplicationScheduler(Ptr<BasicSimulation> basicSimulation, Ptr<TopologySatelliteNetwork> topology);
    virtual void WriteResults();
    bool ReadApplicationEnable(std::string flowType);
protected:
    void ReadLoggingForApplicationIds(std::string flowType);
    Ptr<BasicSimulation> m_basicSimulation;
    int64_t m_simulation_end_time_ns;
    Ptr<TopologySatelliteNetwork> m_topology = nullptr;
    bool m_enabled;

    uint32_t m_system_id;
    bool m_enable_distributed;
    std::vector<int64_t> m_distributed_node_system_id_assignment;

    //std::vector<SAGBurstInfo> m_schedule;
    NodeContainer m_nodes;
    std::vector<ApplicationContainer> m_apps;  // used for write simulation result,not all app can be contained,e.g. tcpsink
    std::set<int64_t> m_enable_logging_for_sag_application_ids;

};




}


#endif /* SAG_APPLICATION_SCHEDULER_H */
