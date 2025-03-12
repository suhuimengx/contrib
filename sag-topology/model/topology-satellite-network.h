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

#ifndef TOPOLOGY_SATELLITE_NETWORK_H
#define TOPOLOGY_SATELLITE_NETWORK_H

#include <utility>
#include "ns3/core-module.h"
#include "ns3/node.h"
#include "ns3/node-container.h"
#include "ns3/topology.h"
#include "ns3/exp-util.h"
#include "ns3/basic-simulation.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/random-variable-stream.h"
#include "ns3/command-line.h"
#include "ns3/traffic-control-helper.h"
#include "ns3/ground-station.h"
#include "ns3/satellite-position-helper.h"
#include "ns3/satellite-position-mobility-model.h"
#include "ns3/mobility-helper.h"
#include "ns3/string.h"
#include "ns3/type-id.h"
#include "ns3/satellite-position-helper.h"
#include "ns3/point-to-point-laser-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-static-routing.h"
#include "ns3/ipv4-routing-table-entry.h"
#include "ns3/wifi-net-device.h"
#include "ns3/point-to-point-laser-net-device.h"
#include "ns3/ipv4.h"
#include "ns3/ipv6.h"
#include <unordered_map>
#include <set>
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv6-l3-protocol.h"
#include "walker-constellation-structure.h"
#include "ns3/sag_application_layer.h"
#include "ns3/sag_physical_gsl_helper.h"
#include "gsl_switch_strategy.h"
#include "isl_establish_rule.h"
#include "ns3/exp-util.h"
#include "ns3/sag_routing_helper.h"
#include "ns3/sag_routing_helper_ipv6.h"
#include "ns3/addressing_rule.h"
#include "ns3/sag_aodv_helper.h"
#include "ns3/sag_open_shortest_path_first_helper.h"
#include "ns3/sag_gs_static_routing_helper.h"
#include "ns3/bgp-routing-helper.h"
#include "ns3/point-to-point-laser-channel.h"
#include "ns3/distributed_node_system_id_assignment.h"
//#include "ns3/sag_rtp_constants.h"
#include "ns3/earth.h"
#include "ns3/earth-position-mobility-model.h"
#include "ns3/earth-position-helper.h"

namespace ns3 {

// Topology Change
// ISL state
enum Oper{
	OPER_NULL,
	OPER_ADD,		/// Link establishment
	OPER_DEL,		/// Link interruption
};

struct OutageLink{
	OutageLink(uint32_t node1, uint32_t node2){
		nodeid1 = node1;
		nodeid2 = node2;
	}
	uint32_t nodeid1;
	uint32_t nodeid2;
	// outage start time, outage stop time(seconds)
	std::vector<std::pair<double, double>> outageTimeInterval;

	bool operator == (const OutageLink& a){
		return (a.nodeid1 == nodeid1) && (a.nodeid2 == nodeid2);
	}
};

struct ConnectionLink{
	ConnectionLink(uint32_t node1, uint32_t interface){
		nodeid1 = node1;
		interfaceid = interface;
	}
	uint32_t nodeid1;
	uint32_t interfaceid;
	// connection start time, connection stop time(seconds)
	std::vector<std::vector<std::pair<double, double>>> connectionTimeInterval;
	std::vector<uint32_t> satnodes;

	bool operator == (const ConnectionLink& a){
		return (a.nodeid1 == nodeid1) &&  (a.interfaceid == interfaceid);
	}
};

/**
 * \ingroup SatelliteNetwork
 *
 * \brief Satellite networking module
 */

class TopologySatelliteNetwork : public Topology
{
public:

	static TypeId GetTypeId (void);
	/**
	 * \brief TopologySatelliteNetwork Constructor
	 *
	 * \param basicSimulation		BasicSimulation handler
	 * \param ipv4RoutingHelper		Ipv4RoutingHelper instance reference for constructing routing protocol instance
	 *
	 */
	TopologySatelliteNetwork(Ptr<BasicSimulation> basicSimulation,
		std::vector<std::pair<SAGRoutingHelper, int16_t>>& ipv4RoutingHelper,
		std::vector<std::pair<SAGRoutingHelperIPv6, int16_t>>& ipv6RoutingHelper);

	// Inherited accessors
	const NodeContainer& GetNodes();
	int64_t GetNumNodes();
	bool IsValidEndpoint(int64_t node_id);
	const std::set<int64_t>& GetEndpoints();

	// Additional accessors
	std::vector<Ptr<Constellation>> GetConstellations();
	uint32_t GetNumSatellites();
	uint32_t GetNumGroundStations();
	const NodeContainer& GetSatelliteNodes();
	const NodeContainer& GetGroundStationNodes();
	const std::vector<Ptr<GroundStation>>& GetGroundStations();
	const std::vector<Ptr<GroundStation>>& GetGroundStationsReal();
	const std::vector<Ptr<GroundStation>>& GetAirCraftsReal();
	const std::vector<std::vector<Vector>>& GetAirCraftsPositions();
	const std::vector<Ptr<Satellite>>& GetSatellites();
	const Ptr<Satellite> GetSatellite(uint32_t sat_id);
	uint32_t NodeToGroundStationId(uint32_t node_id);
	bool IsSatelliteId(uint32_t node_id);
	bool IsGroundStationId(uint32_t node_id);

	const std::vector<std::pair<Ptr<Node>, std::vector<std::pair<uint32_t, Ptr<Node>>>>> GetGSLInformation();
	std::vector<uint32_t> GetAdjacency(uint32_t cur_sat);
	int32_t GetISLInterfaceNumber(uint32_t satId0, uint32_t satId1);
	int32_t GetISLInterfaceNumberIpv6(uint32_t satId0, uint32_t satId1);


	/**
	 * \brief Make Unpredictable ISL Change Event
	 *
	 * \param time		Trigger time
	 * \param oper		Operation: add or delete
	 * \param satId0		Satellite Id
	 * \param satId1		Satellite Id
	 *
	 */
	void MakeUnpredictableISLChangeEvent(double time ,Oper oper,uint32_t satId0, uint32_t satId1);
	/**
	 * \brief Make unpredictable ISL change event randomly according to the freqInterval and recoverInterval set
	 * \param freqInterval		Frequency interval for generating one random interrupt
	 * \param recoverInterval		Recovery time for ISL outages
	 */
	void MakeUnpredictableISLChangeEvent(double freqInterval, double recoverInterval);

	// among satellites
	double GetDistanceFromTo(uint32_t sat1, uint32_t sat2);
	uint32_t ResolveNodeIdFromIp(uint32_t ip);
	/**
	* \param position a reference to another mobility model
	* \return the A(Azimuth)E(Elevation)R(Elevation) between the two objects.
	*/
	Vector GetAERFromTo (Ptr<const MobilityModel> cur_position, Ptr<const MobilityModel> position) const;
	Vector CalculateAER(double rsat[3], double vsat[3], double rsat2[3]) const;
	double DotProduct(double a[3], double b[3]) const;
	double Norm2(double a[3]) const;
	void CrossProduct(double a[3], double b[3], double* res) const;

	// Post-processing
	void CollectUtilizationStatistics();

	void PacketLossTrace();
	void InsertDropLog(const Ipv4Header & hd, Ptr<const Packet> p, Ipv4L3Protocol::DropReason r, Ptr<Ipv4> i, uint32_t j);

	void MakeLinkDelayUpdateEvent(double time);
	void MakeLinkSINRUpdateEvent(double time);
	void ReadSunTrajectoryEciFromCspice();
	void MakeSunOutageEvent(double time);
	void MakeSatelliteCoordinateUpdateEvent(double time);

	const std::vector<std::pair<Ptr<Node>, std::vector<std::pair<uint32_t, Ptr<Node>>>>> GetPastGSLInformation();

	Ptr<SwitchStrategyGSL> GetGSLSwitch();

	std::vector<OutageLink> const& GetInterSatelliteOutageLinkDetails();
	std::vector<ConnectionLink> const& GetSatellite2GroundConnectionLinkDetails();
	NodeContainer GetCurrentSystemNodes();
	NodeContainer GetCurrentSystemGsNodes();
	Ptr<Constellation> FindConstellationBySatId(uint32_t satId);


private:

	// Build functions
	void ReadConfig();
	/**
	 * \brief Construct the whole satellite networking scenario
	 */
	void Build();
	void BuildTopologyOnly();

	/**
	 * \brief Generate a complete TLE file according to the configuration file
	 */
	void GenerateTLE();
	int CalculateTleLineChecksum(std::string tle_line_without_checksum);

	/**
	 * \brief Create satellite node and install mobility model
	 */
	void ReadSatellites();
	/**
	 * \brief Create ground station node and install mobility model
	 */
	void ReadGroundStations();

	/**
	 * \brief Create aircraft node and install mobility model
	 */
	void ReadAirCrafts();
	// waiting to be modified todo
	void MakeAirCraftFlyingEvent(double time, double speed);
    double getLatitudeDistance() const {
        // 假设地球是一个球体，计算纬度上每度的距离
        double radius = 6371000; // 地球半径（米）
        return 2 * M_PI * radius / 360.0;
    }

    double getLongitudeDistance(double lat) const {
        // 假设地球是一个球体，计算经度上每度的距离，根据纬度进行修正
        double radius = 6371000; // 地球半径（米）
        double distance = 2 * M_PI * radius * cos(lat * M_PI / 180) / 360.0;
        return distance;
    }

	/**
	 * \brief Create terminal node and install mobility model
	 */
    void ReadTerminals();


	/**
	 * \brief Install Internet stacks on nodes
	 *
	 * \param ipv4RoutingHelper Ipv4RoutingHelper instance reference for constructing routing protocol instance
	 */
	void InstallInternetStacks();





	/**
	 * \brief Construct the ISL according to the generated adjacencies
	 */
	void ReadISLs();
	//void EnablePcapAll(PointToPointLaserHelper p2p_laser_helper);
	void EnablePcap(SAGPhysicalGSLHelper gsl_helper);
	void EnableUtilization();
	void EnableUtilizationAll();
	void EnableUtilization(uint32_t nodeid, uint32_t deviceid);
	/**
	 * \brief Maintain regular ISL change events, such as cross-orbit link interruptions over the polar region
	 *
	 * \param time Trigger interval
	 */
	//<<<<<<<<<<<<<<<< todo: optimize
	void MakeRegularISLChangeEvent(double time);
	/**
	 * \brief Add link by satellite Ids
	 *
	 * \param satId0		Satellite Id
	 * \param satId1		Another satellite Id
	 * \param regularOrNot		Is it caused by regular link switching
	 *
	 */
	void AddISLBySatId(uint32_t satId0, uint32_t satId1, bool regularOrNot);
	/**
	 * \brief Disable link by satellite Ids
	 *
	 * \param satId0		Satellite Id
	 * \param satId1		Another satellite Id
	 * \param regularOrNot		Is it caused by regular link switching
	 *
	 */
	void DisableISLBySatId(uint32_t satId0, uint32_t satId1, bool regularOrNot);
	/**
	 * \brief Create netdevice and allocate network address
	 *
	 * \param satId0		Satellite Id
	 * \param satId1		Another satellite Id
	 * \param p2pLinkHelper		PointToPointLaserHelper instance reference
	 * \param tcHelper		TrafficControlHelper instance reference
	 *
	 */
	void DoCreateNetDevice(
			uint32_t satId0,
			uint32_t satId1,
			PointToPointLaserHelper& p2pLinkHelper,
			TrafficControlHelper& tcHelper,
			std::unordered_map<uint32_t, std::vector<uint32_t>>& adjacency,
			NetDeviceContainer& islNetDevices,
			std::vector<std::pair<uint32_t, uint32_t>>& islFromTo,
			std::vector<std::pair<uint32_t, uint32_t>>& islFromToUnique
			);
    void DoCreateNetDevice(
    		uint32_t satId0,
    		uint32_t satId1,
    		PointToPointLaserHelper& p2pLinkHelper,
			TrafficControlHelper& tcHelper);



	/**
	 * \brief Install satellite-to-ground netdevices for all satellites
	 * 		  and allocate different network segment and address for each
	 */
	void InstallGSLInterfaceForAllSatellites();
	/**
	 * \brief Install satellite-to-ground netdevices for all ground stations
	 */
	void InstallGSLInterfaceForAllGroundStations();
	/**
	 * \brief Maintain GSL switch event
	 *
	 * \param time Trigger interval
	 */
	void MakeGSLChangeEvent(double time);
	/**
	 * \brief Maintain GSL switch
	 */
	void SwitchGSLForAllGroundStations();
	/**
	 * \brief Add GSL by groundStation Id and satellite Id. Attach channel, allocate new interface address, notify L3 and update ARP.
	 *
	 * \param gs		GroundStation Id
	 * \param sat		Satellite Id
	 *
	 */
	void AddGSLByGndAndSat(Ptr<Node> gs, Ptr<Node> sat, uint32_t interface); // for ground stations owning only one interface
	/**
	 * \brief Disable GSL by groundStation Id and satellite Id. Remove interface address, update ARP, notify L3 and unattach channel.
	 *
	 * \param gs		GroundStation Id
	 * \param sat		Satellite Id
	 *
	 */
	void DisableGSLByGndAndSat(Ptr<Node> gs, Ptr<Node> sat, uint32_t interface); // for ground stations owning only one interface



	/**
	 * \brief Initialize GroundStation ArpCache
	 *
	 * \param gs		GroundStation
	 * \param ntDevice		GroundStation Netdevice
	 *
	 */
	void InitializeGroundStationsArpCaches(Ptr<Node> gs, Ptr<NetDevice> ntDevice);
	/**
	 * \brief Initialize Satellites' ArpCaches
	 */
	void InitializeSatellitesArpCaches();
	/**
	 * \brief Add ArpCache Entry
	 *
	 * \param gs		GroundStation
	 * \param sat		Satellite
	 * \param gslGsNetDevice		Netdevice on the ground station side
	 * \param gslSatNetDevice		Netdevice on the satellite side
	 */
	void ArpCacheEntryAdd(Ptr<Node> gs, Ptr<Node> sat, Ptr<NetDevice> gslGsNetDevice, Ptr<NetDevice> gslSatNetDevice);
	/**
	 * \brief Delete ArpCache Entry
	 *+-
	 * \param gs		GroundStation
	 * \param sat		Satellite
	 * \param gslGsNetDevice		Netdevice on the ground station side
	 * \param gslSatNetDevice		Netdevice on the satellite side
	 */
	void ArpCacheEntryDelete(Ptr<Node> gs, Ptr<Node> sat, Ptr<NetDevice> gslGsNetDevice, Ptr<NetDevice> gslSatNetDevice);


	/**
	 * In order to meet the end-to-end link requirements, the corresponding APP is notified
	 * when the interface address changes when the handover occurs.
	 */
	void NotifyInterfaceAddressChangeToApp();
	// For scenario of one network card of each ground station
	Ptr<NetDevice> GetSatGSLNetDevice(Ptr<Node> sat);
	Ptr<NetDevice> GetGsGSLNetDevice(Ptr<Node> gs, uint32_t interface);


	void EnsureValidNodeId(uint32_t node_id);
	std::string CalStringKey(uint32_t satId0, int32_t satId1);



	// Input
	Ptr<BasicSimulation> m_basicSimulation;       //<! Basic simulation instance
	std::string m_satellite_network_dir;          //<! Directory containing satellite network information
	std::string m_satellite_network_routes_dir;   //<! Directory containing the routes over time of the network
	bool m_satellite_network_force_static;        //<! True to disable satellite movement and basically run
												  //   it static at t=0 (like a static network)

	// Generated state
	NodeContainer m_allNodes;                           				//!< All nodes
	NodeContainer m_groundStationNodes;                 				//!< Ground station nodes
	NodeContainer m_satelliteNodes;                     				//!< Satellite nodes
	NodeContainer m_terminalNodes;										//!< Terminals, sensors, etc.
	Ptr<Node> m_sun;													//!< Sun node
	Ptr<Earth> m_sun_model;												//!< Sun instance
	std::vector<jsonns::sun_trajectory_content> m_sun_trajectory;		//!< The coordinate trajectory of the sun
	std::vector<Ptr<GroundStation> > m_groundStations;  				//!< Ground station instances
	std::vector<Ptr<GroundStation> > m_groundStationsReal;  			//!< Real Ground station instances
	std::vector<Ptr<GroundStation> > m_airCraftsReal;					//!< Real aircrafts instances
	std::vector<std::vector<Vector>> m_airCraftsPositions;				//!< Aircrafts' positions
	std::vector<Ptr<GroundStation> > m_terminalsReal;					//!< Real terminal instances
	std::vector<Ptr<Satellite>> m_satellites;           				//<! Satellite instances
	std::set<int64_t> m_endpoints;                      				//<! EndPoint ids = ground station ids / all node ids
	std::vector<Ptr<Constellation>> m_constellations;					//<! Constellation structure pointers
	std::vector<json> m_satelliteElements;


	// ISL devices
	std::vector<std::pair<uint32_t, uint32_t>> m_islInterOrbit;  			//<! for star type constellation ISL interruption
	std::map<std::string, std::pair<bool, bool>> m_islDisableNetDevices; 	//<! ISL interruption information: First element represents regular interruption or not; Second element represents unpredictable interruption or not


	// GSL devices
	NetDeviceContainer m_gslSatNetDevices;															//<! All satellite-to-ground NetDevices, one satellite has only one
	NetDeviceContainer m_gslGsNetDevices;															//<! All ground-to-satellite NetDevices, one ground station has only one
	std::vector<NetDeviceContainer> m_gslGsNetDevicesInterface;										//<! When the ground station has multiple interfaces, it is used for recording, reserved, and temporarily useless
	std::vector<std::pair<Ptr<Node>, std::vector<std::pair<uint32_t, Ptr<Node>>>>> m_gslRecord;		//<! Satellite-ground correspondence(key: ground station node, value: connection pairs), value is written in the form of multiple interfaces, reserved, and temporarily useless
	std::vector<std::pair<Ptr<Node>, std::vector<std::pair<uint32_t, Ptr<Node>>>>> m_gslRecordCopy;	//<! Satellite-ground correspondence copy(key: ground station node, value: connection pairs)
	std::vector<ConnectionLink> m_gsLinkDetails;													//<! Record duration details of GSL


	// Values
	double m_isl_data_rate_megabit_per_s;
	double m_gsl_data_rate_megabit_per_s;
	double m_isl_error_rate_per_pkt;
	double m_gsl_error_rate_per_pkt;
	int64_t m_isl_max_queue_size_pkts;
	int64_t m_gsl_max_queue_size_pkts;
	bool m_enable_red_queue_disc;
	bool m_enable_link_utilization_tracking = false;
	int64_t m_link_utilization_tracking_interval_ns;
	double m_time_end;
	double m_dynamicStateUpdateIntervalNs;

	// Strategy
	Ptr<SwitchStrategyGSL> m_switchStrategy;				//<! GSL switching strategy
	Ptr<ISLEstablishRule> m_islEstablishRule;				//<! ISL establish rule
	Ptr<AddressingRule> m_networkAddressingMethod;			//<! Network addressing method
	Ptr<AddressingRule> m_networkAddressingMethodIPv6;		//<! Network addressing method
	bool m_isIPv4Networking;								//<! ipv4
	bool m_isIPv6Networking;								//<! ipv6
	std::vector<std::pair<SAGRoutingHelper, int16_t>>* m_ipv4RoutingHelpers;
	std::vector<std::pair<SAGRoutingHelperIPv6, int16_t>>* m_ipv6RoutingHelpers;


	// Distributed simulation
	bool m_enable_distributed;										//<! Whether to enable distributed mode
	uint32_t m_system_id;											//<! Current system id
	uint32_t m_systems_count;										//<! Total number of systems
	NodeContainer m_nodesCurSystem;									//<! Satellites nodes belonging to the current system
	NodeContainer m_nodesCurSystemVirtual;							//<! Virtual satellites nodes in the current system
	NodeContainer m_nodesGsCurSystem;								//<! Ground station nodes belonging to the current system
	NodeContainer m_nodesGsCurSystemVirtual;						//<! Virtual ground station nodes in the current system
	std::vector<int64_t> distributed_node_system_id_assignment;		//<! Node partitioning algorithm


	// ISL Sun OutAge
	std::vector<std::pair<uint32_t,uint32_t>> m_sunOutageLinks;		//<! Record only sun outage links, updated every time slot, pair<minNodeId, maxNodeId>
	std::vector<OutageLink> m_sunOutageLinkDetails;					//<! Record link outage duration details of ISL, log outages in all cases, not just Sun outage


};

}

#endif //TOPOLOGY_SATELLITE_NETWORK_H
