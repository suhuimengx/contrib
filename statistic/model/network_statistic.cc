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

#include "network_statistic.h"
#include "ns3/sgp4coord.h"
#include "ns3/cppmap3d.hh"
#include "ns3/cppjson2structure.hh"

namespace ns3 {

    NS_LOG_COMPONENT_DEFINE ("Network_Statistic");

    Network_Statistic::Network_Statistic (Ptr<BasicSimulation> basicSimulation, Ptr<TopologySatelliteNetwork> topology){
    	if(basicSimulation->GetSystemId() != 0) return;
    	m_basicSimulation = basicSimulation;
    	m_topology = topology;
    	m_dynamicStateUpdateIntervalNs = parse_positive_int64(m_basicSimulation->GetConfigParamOrFail("dynamic_state_update_interval_ns"));

    }

    Network_Statistic::~Network_Statistic (){

	}

	void
	Network_Statistic::SetBasicSimHandle(Ptr<BasicSimulation> basicSimulation){
		m_basicSimulation = basicSimulation;
	}

	void
	Network_Statistic::ReadAccessAnalysisJson(){

	}

	void
	Network_Statistic::AccessScheduler(double time){

	}

	void
	Network_Statistic::AccessWriteResults(){

	}


}


