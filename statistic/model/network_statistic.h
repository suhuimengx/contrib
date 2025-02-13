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

#ifndef SIMULATOR_CONTRIB_STATISTIC_MODEL_NETWORK_STATISTIC_H_
#define SIMULATOR_CONTRIB_STATISTIC_MODEL_NETWORK_STATISTIC_H_

#include <vector>
#include <cfloat>
#include "ns3/node-container.h"
#include "ns3/mobility-model.h"
#include "ns3/basic-simulation.h"
#include "ns3/topology-satellite-network.h"

namespace ns3 {

/**
 * \ingroup Statistics
 *
 * \brief Network_Statistic
 */
class Network_Statistic
{
public:
	// constructor
	Network_Statistic (Ptr<BasicSimulation> basicSimulation, Ptr<TopologySatelliteNetwork> topology);
	virtual ~Network_Statistic ();

	void SetBasicSimHandle(Ptr<BasicSimulation> basicSimulation);
	void ReadAccessAnalysisJson();

	/**
	 * \brief Update all ground stations' switching state
	 * \param gslRecord		Adjacency between satellite and ground station
	 * \param gslRecordCopy		Adjacency between satellite and ground station before switching
	 */
	void AccessScheduler(double time);

	/**
	 * \brief Update all ground stations' switching state
	 * \param gslRecord		Adjacency between satellite and ground station
	 * \param gslRecordCopy		Adjacency between satellite and ground station before switching
	 */
	void AccessWriteResults();

protected:
	Ptr<BasicSimulation> m_basicSimulation;
	Ptr<TopologySatelliteNetwork> m_topology;
	double m_dynamicStateUpdateIntervalNs;


};


}
#endif /* SIMULATOR_CONTRIB_STATISTIC_MODEL_NETWORK_STATISTIC_H_ */
