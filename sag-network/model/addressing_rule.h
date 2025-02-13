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

#ifndef ADDRESSING_RULE_H
#define ADDRESSING_RULE_H

#include <vector>
#include <cfloat>
#include "ns3/node-container.h"
#include "ns3/mobility-model.h"
#include "ns3/basic-simulation.h"
#include "ns3/walker-constellation-structure.h"
#include "ns3/satellite-position-mobility-model.h"
#include "ns3/net-device-container.h"
#include "ns3/gsl_switch_strategy.h"

namespace ns3 {
/**
 * \ingroup SatelliteNetwork
 *
 * \brief Rule of network addressing
 */
class AddressingRule : public Object
{
public:
	static TypeId GetTypeId (void);

	// constructor
	AddressingRule ();
	virtual ~AddressingRule ();

	virtual void AssignInterSatelliteInterfaceAddress(NetDeviceContainer netDevices);
	virtual void AssignSatelliteToGroundInterfaceAddress(NetDeviceContainer netDevices);
	virtual void AssignGroundToSatelliteInterfaceAddress(uint32_t satId, NetDeviceContainer netDevices);

	void SetTopologyHandle(std::vector<Ptr<Constellation>> constellations, NodeContainer& groundStations, Ptr<SwitchStrategyGSL> switchStrategy);
	virtual void SetBasicSimHandle(Ptr<BasicSimulation> basicSimulation);

protected:
	std::vector<Ptr<Constellation>> m_constellations;		//<! Constellation structure pointers
	NodeContainer m_groundStations;		//<! Ground stations
	Ptr<SwitchStrategyGSL> m_switchStrategy;		//<! Strategy of GSL switching

};

/**
 * \ingroup SatelliteNetwork
 *
 * \brief Rule of network addressing in IPv4
 */
class AddressingInIPv4 : public AddressingRule
{
public:
	static TypeId GetTypeId (void);

	// constructor
	AddressingInIPv4 ();
	virtual ~AddressingInIPv4 ();

	virtual void AssignInterSatelliteInterfaceAddress(NetDeviceContainer netDevices);
	virtual void AssignSatelliteToGroundInterfaceAddress(NetDeviceContainer netDevices);
	virtual void AssignGroundToSatelliteInterfaceAddress(uint32_t satId, NetDeviceContainer netDevices);
	virtual void AssignGroundToSatelliteInterfaceAddress(uint32_t satId, NetDeviceContainer netDevices, Ipv4InterfaceAddress& adr);

	void SetBasicSimHandle(Ptr<BasicSimulation> basicSimulation);

protected:
//	Ipv4Address m_network = "10.0.0.0";		//<! Network Number
//	Ipv4Mask m_mask = "255.255.255.0";		//<! Mask

	Ipv4Address m_network = "10.0.0.0";		//<! Network Number
	Ipv4Mask m_mask = "/20";		//<! Mask  "255.255.255.192" = "/26"
	Ipv4Address m_base = "0.0.0.1";

	Ipv4AddressHelper m_ipv4_helper;		//<! Ipv4AddressHelper for addressing at the satellite side

	Ptr<BasicSimulation> m_basicSimulation;



};

/**
 * \ingroup SatelliteNetwork
 *
 * \brief Rule of network addressing in IPv6
 */
class AddressingInIPv6 : public AddressingRule
{
public:
	static TypeId GetTypeId (void);

	// constructor
	AddressingInIPv6 ();
	virtual ~AddressingInIPv6 ();

	virtual void AssignInterSatelliteInterfaceAddress(NetDeviceContainer netDevices);
	virtual void AssignSatelliteToGroundInterfaceAddress(NetDeviceContainer netDevices);
	virtual void AssignGroundToSatelliteInterfaceAddress(uint32_t satId, NetDeviceContainer netDevices);
	virtual void AssignGroundToSatelliteInterfaceAddress(uint32_t satId, NetDeviceContainer netDevices, Ipv6InterfaceAddress& adr);

	void SetBasicSimHandle(Ptr<BasicSimulation> basicSimulation);

protected:
	Ptr<BasicSimulation> m_basicSimulation;


};

/**
 * \ingroup SatelliteNetwork
 *
 * \brief The traditional IP addressing method.
 *
 * The traditional IP addressing method for low-orbit satellite network is to set a fixed IP address
 * for each satellite node's port to the ground. When the mobile user on the ground is transferred from the
 * coverage area of the previous satellite node to the coverage area of another new satellite node, the IP
 * address of the mobile user needs to be updated. The new IP address of the mobile user and the IP address
 * of the new satellite node satisfy the address aggregation relationship, so that the mobile user can access
 * the new satellite node.
 */
class TraditionalAddressing: public AddressingInIPv4
{
public:
	static TypeId GetTypeId (void);

	// constructor
	TraditionalAddressing ();
	virtual ~TraditionalAddressing ();

	virtual void AssignInterSatelliteInterfaceAddress(NetDeviceContainer netDevices);
	virtual void AssignSatelliteToGroundInterfaceAddress(NetDeviceContainer netDevices);
	virtual void AssignGroundToSatelliteInterfaceAddress(uint32_t satId, NetDeviceContainer netDevices);
	virtual void AssignGroundToSatelliteInterfaceAddress(uint32_t satId, NetDeviceContainer netDevices, Ipv4InterfaceAddress& adr);

private:

	std::map<uint32_t, Ipv4AddressHelper> m_gsl_ipv4_addressing_helper;		//<! Ipv4AddressHelper for addressing of gsl interface at the ground station side


};

// All ground equipment and satellite-to-ground interfaces belong to the same network segment
class TraditionalAddressingGroundSameNetworkSegment: public AddressingInIPv4
{
public:
	static TypeId GetTypeId (void);

	// constructor
	TraditionalAddressingGroundSameNetworkSegment ();
	virtual ~TraditionalAddressingGroundSameNetworkSegment ();

	virtual void AssignInterSatelliteInterfaceAddress(NetDeviceContainer netDevices);
	virtual void AssignSatelliteToGroundInterfaceAddress(NetDeviceContainer netDevices);
	virtual void AssignGroundToSatelliteInterfaceAddress(uint32_t satId, NetDeviceContainer netDevices);
	virtual void AssignGroundToSatelliteInterfaceAddress(uint32_t satId, NetDeviceContainer netDevices, Ipv4InterfaceAddress& adr);

private:

	std::unordered_map<uint32_t, Ipv4InterfaceAddress> m_adr;

};

/**
 * \ingroup SatelliteNetwork
 *
 * \brief The traditional IPv6 addressing method.
 *
 * The traditional IPv6 addressing method for low-orbit satellite network is to set a fixed IPv6 address
 * for each satellite node's port to the ground. When the mobile user on the ground is transferred from the
 * coverage area of the previous satellite node to the coverage area of another new satellite node, the IPv6
 * address of the mobile user needs to be updated. The new IPv6 address of the mobile user and the IPv6 address
 * of the new satellite node satisfy the address aggregation relationship, so that the mobile user can access
 * the new satellite node.
 */
class TraditionalAddressingIPv6: public AddressingInIPv6
{
public:
	static TypeId GetTypeId (void);

	// constructor
	TraditionalAddressingIPv6 ();
	virtual ~TraditionalAddressingIPv6 ();

	virtual void AssignInterSatelliteInterfaceAddress(NetDeviceContainer netDevices);
	virtual void AssignSatelliteToGroundInterfaceAddress(NetDeviceContainer netDevices);
	virtual void AssignGroundToSatelliteInterfaceAddress(uint32_t satId, NetDeviceContainer netDevices);
	virtual void AssignGroundToSatelliteInterfaceAddress(uint32_t satId, NetDeviceContainer netDevices, Ipv6InterfaceAddress& adr);

private:

	Ipv6Address m_network="20::";		//<! Network Number
	Ipv6Prefix m_prefix="FFFF:FFFF:FFFF:FFFF::";		//<! Mask
	Ipv6AddressHelper m_ipv6_helper;		//<! Ipv6AddressHelper for addressing at the satellite side
	std::map<uint32_t, Ipv6AddressHelper> m_gsl_ipv6_addressing_helper;		//<! Ipv6AddressHelper for addressing of gsl interface at the ground station side


};

/**
 * \ingroup SatelliteNetwork
 *
 * \brief Geographic addressing in LEO satellite network.
 *
 */
class GeographicAddressing : public AddressingInIPv6
{
public:
	static TypeId GetTypeId (void);

	// constructor
	GeographicAddressing ();
	virtual ~GeographicAddressing ();

	virtual void AssignInterSatelliteInterfaceAddress(NetDeviceContainer netDevices);
	virtual void AssignSatelliteToGroundInterfaceAddress(NetDeviceContainer netDevices);
	virtual void AssignGroundToSatelliteInterfaceAddress(uint32_t satId, NetDeviceContainer netDevices);

	static void GetPartitionIdOfGroundStation(uint32_t groundStationId);
	static void GetPartitionIdOfSatellite(uint32_t satelliteId);

private:
	std::map<uint32_t,std::vector<float>> m_ascendingPartition;		//<! Partitions: key (partition id), values (partition extent coordinate)
	std::map<uint32_t,std::vector<float>> m_descendingPartition;		//<! Partitions: key (partition id), values (partition extent coordinate)

};







}
#endif /* ADDRESSING_RULE_H */
