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
 * Author: Yuze Liu
 */

#ifndef LLDP_CONFIGURE_H
#define LLDP_CONFIGURE_H

#include "stdio.h"
#include <arpa/inet.h>
#include <vector>
#include "ns3/bgp.h"
#include "ns3/log.h"
#include "ns3/enum.h"
#include "ns3/ipv4-list-routing.h"
#include "ns3/ipv4-static-routing.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/simulator.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/topology-satellite-network.h"
//#include "ns3/basic-simulation.h"

namespace ns3 {

class LLDPConfigure : public Object
{
public:
    static TypeId GetTypeId (void);
    LLDPConfigure();
	virtual ~LLDPConfigure();
	LLDPConfigure(Ptr<BasicSimulation> basicSimulation, Ptr<TopologySatelliteNetwork> topology);

protected:
	bool m_enabled;                           //<! True to enable bgp
	Ptr<TopologySatelliteNetwork> m_topology;    //<! Satellite networking instance
	Ptr<BasicSimulation> m_basicSimulation;      //<! Basic simulation instance
	NodeContainer m_satelliteNodes;

};

}
#endif /* LLDP_CONFIGURE_H */
