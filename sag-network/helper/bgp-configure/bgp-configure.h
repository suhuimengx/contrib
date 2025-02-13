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

#ifndef SRC_BGP_CONFIGURE_BGP_CONFIGURE_H_
#define SRC_BGP_CONFIGURE_BGP_CONFIGURE_H_

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

class BgpConfigure : public Object
{
public:
    static TypeId GetTypeId (void);
    BgpConfigure();
	virtual ~BgpConfigure();
	BgpConfigure(Ptr<BasicSimulation> basicSimulation, Ptr<TopologySatelliteNetwork> topology);
	virtual void InstallIbgpPeer();
	virtual void InstallEbgpPeer();

protected:
	// Input
	bool m_enable_bgp;                           //<! True to enable bgp
	Ptr<TopologySatelliteNetwork> m_topology;    //<! Satellite networking instance
	Ptr<BasicSimulation> m_basicSimulation;      //<! Basic simulation instance
	// Values
	int64_t m_simulation_end_time_ns;
	double m_dynamicStateUpdateIntervalNs;
};

}
#endif /* SRC_BGP_CONFIGURE_BGP_CONFIGURE_H_ */
