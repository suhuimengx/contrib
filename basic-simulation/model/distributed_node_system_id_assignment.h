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

#ifndef PARTITIONING_TOPOLOGY_H
#define PARTITIONING_TOPOLOGY_H


#include "ns3/core-module.h"
#include "ns3/random-variable-stream.h"
#include "ns3/node-container.h"
#include "ns3/net-device-container.h"
#include "ns3/ipv4-interface-container.h"
#include <string>
#include <vector>
#include <iostream>
#include "ns3/exp-util.h"
#include "ns3/mpi-interface.h"
#include "ns3/basic-simulation.h"

namespace ns3 {

class PartitioningTopology : public Object
{
public:
	static TypeId GetTypeId();
	void SetBasicSim(Ptr<BasicSimulation> basicsimulation);
	std::vector<int64_t> GetDistributedNodeSystemIdAssignment(std::string algorithm, std::string satellite_network_dir, int64_t orbits, int64_t sats, int64_t groundStation_counter, uint32_t systems_cnt);
	std::vector<int64_t> GetDistributedNodeSystemIdAssignment(std::string algorithm, std::string satellite_network_dir, int64_t satellite_counter, int64_t groundStation_counter, uint32_t systems_cnt);
	std::vector<int64_t> GetDistributedNodeSystemIdAssignmentSat(std::string algorithm,int64_t num,uint32_t systems_cnt);
	std::vector<int64_t> GetDistributedNodeSystemIdAssignmentGd(std::string algorithm,int64_t num,uint32_t systems_cnt);
	std::vector<int> GetAndCheckNodeToSystemIdAssignment(std::vector<int64_t> node,std::string nodetype,uint32_t systems_cnt);
	//void PrintSystemIdInformation();

private:
	Ptr<BasicSimulation> m_basicsimulation;

};

class AlgorithmForPartitioning : public Object
{
public:
	static TypeId GetTypeId();
	std::vector<int64_t> DivideEvenlyInOrder(std::string satellite_network_dir, int64_t satellite_counter, int64_t groundStation_counter, uint32_t systems_cnt);
	std::vector<int64_t> DivideEvenlyInOrder(std::string satellite_network_dir, int64_t orbits, int64_t satsPerOrbit, int64_t groundStation_counter, uint32_t systems_cnt);
	std::vector<int64_t> HandoverInterProcess(std::string satellite_network_dir, int64_t satellite_counter, int64_t groundStation_counter, uint32_t systems_cnt);
	std::vector<int64_t> CustomizeMode(std::string run_dir, int64_t satellite_counter, int64_t groundStation_counter, uint32_t systems_cnt);
	std::vector<int64_t> DivideEvenlyInOrder(int64_t num, uint32_t systems_cnt);
	std::vector<int64_t> DivideEvenlyAtRandom(int64_t num, uint32_t systems_cnt);


private:

};



}
#endif // PARTITIONING_TOPOLOGY_H
