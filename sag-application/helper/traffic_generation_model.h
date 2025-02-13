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
#ifndef TRAFFIC_GENERATION_MODEL_H
#define TRAFFIC_GENERATION_MODEL_H

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

class TrafficGenerationModelHelper
{
public:
	TrafficGenerationModelHelper(Ptr<BasicSimulation> basicSimulation, Ptr<TopologySatelliteNetwork> topology);
	void DoTrafficGeneration();
protected:
    Ptr<BasicSimulation> m_basicSimulation;
    Ptr<TopologySatelliteNetwork> m_topology;
};

class TrafficGenerationModel
{

public:
	TrafficGenerationModel(Ptr<BasicSimulation> basicSimulation, Ptr<TopologySatelliteNetwork> topology);
    virtual void TrafficGenerate();
    uint32_t ReadTrafficModelSet();

protected:
    Ptr<BasicSimulation> m_basicSimulation;
    Ptr<TopologySatelliteNetwork> m_topology;
    bool m_enabled;

};

class RandomlyGeneratedModel: public TrafficGenerationModel
{

public:
	RandomlyGeneratedModel(Ptr<BasicSimulation> basicSimulation, Ptr<TopologySatelliteNetwork> topology);
    virtual void TrafficGenerate();

};

uint32_t GetRandomNumber(uint32_t min, uint32_t max);
}




#endif /* TRAFFIC_GENERATION_MODEL_H */
