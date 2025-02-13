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

#ifndef SRC_BGP_CONFIGURE_BGP_CONFIGURE_FULL_MESH_H_
#define SRC_BGP_CONFIGURE_BGP_CONFIGURE_FULL_MESH_H_

#include <arpa/inet.h>
#include <vector>
#include <cstdint>
#include "ns3/bgp.h"
#include "ns3/log.h"
#include "ns3/enum.h"
#include "ns3/ipv4-list-routing.h"
#include "ns3/ipv4-static-routing.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/simulator.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/topology-satellite-network.h"
#include "bgp-configure.h"
#include "ns3/basic-simulation.h"

namespace ns3 {
/**
 * \ingroup BgpConfigure
 *
 * \brief Bgp configure full mesh module
 */
class BgpConfigureFullMesh : public BgpConfigure
{
public:
	static TypeId GetTypeId (void);
	BgpConfigureFullMesh();
	/**
	 * \brief BgpConfigureFullMesh Constructor
	 *
	 * \param basicSimulation		BasicSimulation handler
	 * \param topology		        TopologySatelliteNetwork pointer
	 *
	 */
	BgpConfigureFullMesh(Ptr<BasicSimulation> basicSimulation, Ptr<TopologySatelliteNetwork> topology);
	/**
	 * \brief Initialize iBGP
	 *
	 */
	void InstallIbgpPeer();
	/**
	 * \brief Initialize eBGP and maintain eBGP switch
	 *
	 * \param time Trigger interval
	 */
	void InstallEbgpPeer(double time);
	/**
	 * \brief Add eBGP by groundStation Id and satellite Id.
	 *
	 * \param gnd		GroundStation Id
	 * \param sat		Satellite Id
	 *
	 */
	void AddEbgpByGndAndSat(Ptr<Node> gnd, Ptr<Node> sat);
	/**
	 * \brief Disable eBGP by groundStation Id and satellite Id.
	 *
	 * \param gnd		GroundStation Id
	 * \param sat		Satellite Id
	 *
	 */
	void DisableEbgpByGndAndSat(Ptr<Node> gnd, Ptr<Node> sat);
	/**
	 * \brief Add route by groundStation Id.
	 *
	 * \param gnd		GroundStation Id
	 *
	 */
	void MakeGndAddRoute(Ptr<Node> gnd);
	/**
	 * \brief Get bgp application by node Id.
	 *
	 * \param node		Node Id
	 *
	 */
	Ptr<Bgp> GetBgpApplication(Ptr<Node> node);

private:
	// Input
	bool m_enable_bgp;                           //<! True to enable bgp
	Ptr<TopologySatelliteNetwork> m_topology;    //<! Satellite networking instance
	Ptr<BasicSimulation> m_basicSimulation;      //<! Basic simulation instance
	// Values
	int64_t m_simulation_end_time_ns;
	double m_dynamicStateUpdateIntervalNs;
};

}
#endif /* SRC_BGP_CONFIGURE_BGP_CONFIGURE_FULL_MESH_H_ */
