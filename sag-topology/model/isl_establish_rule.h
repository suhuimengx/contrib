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

#include <vector>
#include <cfloat>
#include "ns3/exp-util.h"
#include "ns3/node-container.h"
#include "ns3/mobility-model.h"
#include "ns3/basic-simulation.h"
#include "ns3/walker-constellation-structure.h"

#ifndef ISL_ESTABLISH_RULE_H
#define ISL_ESTABLISH_RULE_H

namespace ns3 {
/**
 * \ingroup SatelliteNetwork
 *
 * \brief Rule of ISL establishment
 */
class ISLEstablishRule : public Object
{
public:
	static TypeId GetTypeId (void);

	// constructor
	ISLEstablishRule ();
	virtual ~ISLEstablishRule ();

	void SetBasicSimulationHandle(Ptr<BasicSimulation> basicSimulation);
	void SetTopologyHandle(std::vector<Ptr<Constellation>> constellations);
	void SetSatelliteNetworkDir(std::string satelliteNetworkDir);
	virtual void ISLEstablish(std::vector<std::pair<uint32_t, uint32_t>>& islInterOrbit);

protected:
	Ptr<BasicSimulation> m_basicSimulation;       //<! Basic simulation instance
	std::string m_satelliteNetworkDir;		//<! Satellite network dir
	std::vector<Ptr<Constellation>> m_constellations;		//<! Constellation structure pointers

};

/**
 * \ingroup SatelliteNetwork
 *
 * \brief "Grid+" type
 */
class GridTypeBuilding : public ISLEstablishRule
{
public:
	static TypeId GetTypeId (void);

	// constructor
	GridTypeBuilding ();
	virtual ~GridTypeBuilding ();

	virtual void ISLEstablish(std::vector<std::pair<uint32_t, uint32_t>>& islInterOrbit);

private:
	uint16_t m_islShift;		//<! for ISL connection principle, 0 means connect to satellite at the adjacent orbit
};


}

#endif /* ISL_ESTABLISH_RULE_H */
