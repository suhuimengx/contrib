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

#ifndef GSL_SWITCH_STRATEGY_H
#define GSL_SWITCH_STRATEGY_H


#include <vector>
#include <cfloat>
#include "ns3/node-container.h"
#include "ns3/mobility-model.h"
#include "ns3/basic-simulation.h"
#include "ns3/walker-constellation-structure.h"
#include "ns3/satellite-position-mobility-model.h"
#include "ns3/ground-station.h"
//#include "ns3/sag_rtp_constants.h"

namespace ns3 {

//
//#define G 6.67259e-11   // 引力常量   单位 N*m^2/s^2
//#define M 5.965e24  // 地球质量   单位 kg
//#define R 6378.1350  // 地球半径  单位 km
#define SearchOrbitBond 3 //在上次连接卫星的附近几个轨道寻找新的卫星
#define SearchSateBond 3 //在上次连接的卫星


typedef enum {connedted, sleep, waitConnect} SatelliteConnectState;
struct SatelliteConnectEntry {
	uint32_t m_layer;
	double m_updateTime;
	uint32_t m_satellite;
	SatelliteConnectEntry(uint32_t layer, double updateTime, uint32_t satellite):m_layer(layer),m_updateTime(updateTime), m_satellite(satellite){};
};

/**
 * \ingroup SatelliteNetwork
 *
 * \brief Strategy of GSL switching
 */
class SwitchStrategyGSL : public Object
{
public:
	static TypeId GetTypeId (void);

	// constructor
	SwitchStrategyGSL ();
	virtual ~SwitchStrategyGSL ();

	//void SetTopologyHandle(std::vector<Ptr<Constellation>> constellations, NodeContainer& groundStations);
	void SetTopologyHandle(std::vector<Ptr<Constellation>> constellations, NodeContainer& groundStations, std::vector<Ptr<GroundStation>> &groundStationsModel);
	void SetBasicSimHandle(Ptr<BasicSimulation> basicSimulation);

	/**
	 * \brief Update all ground stations' switching state
	 * \param gslRecord		Adjacency between satellite and ground station
	 * \param gslRecordCopy		Adjacency between satellite and ground station before switching
	 */
	virtual void UpdateSwitch(std::vector<std::pair<Ptr<Node>, std::vector<std::pair<uint32_t, Ptr<Node>>>>>& gslRecord, std::vector<std::pair<Ptr<Node>, std::vector<std::pair<uint32_t, Ptr<Node>>>>>& gslRecordCopy);

	/**
	 * \brief Get max visible distance at a certain ground minimum elevation and orbit height
	 * \param orbitHeight		Orbit height
	 *
	 * \return distance in km
	 */
	double GetMaxVisibleDistance(double orbitHeight);

	bool RecordFeederLinkForSatellites(Ptr<Node> sat);
	bool CheckFeederLinkNumber(Ptr<Node> sat);
	void RecordFeederLinkUnique(Ptr<Node> sat, Ptr<Node> gs);
	bool CheckFeederLinkUnique(Ptr<Node> sat, Ptr<Node> gs);

	void InitializeSatelliteConnectEntry();
protected:
	std::vector<Ptr<Constellation>> m_constellations;		//<! Constellation structure pointers
	NodeContainer m_groundStations;		//<! Ground stations
	std::vector<Ptr<GroundStation> > m_groundStationsModel;  //!< Ground station models
	Ptr<BasicSimulation> m_basicSimulation;
	double m_elevation;		//<! Minimum elevation angle of ground station
	std::map<uint32_t, uint32_t> m_feederLink;		//<! FeederLink number of each satellite
	uint32_t m_feederLinkNum;		//<! Max number of FeederLinks of each satellite
	std::unordered_map<uint32_t, std::vector<uint32_t>> m_feederLinkUnique;		//<! FeederLink number of each ground station
	std::string m_baseLogsDir;

	//<! ground-to-satellite connection state todo
	std::vector<std::vector<SatelliteConnectState>> m_satelliteConnectionState;

	//<! Record the maximum visible distance of satellites at different orbital heights
	std::unordered_map<double, double> m_heightToDistTable;
	//<! Record the recently connected satellite information
	std::vector<std::pair<std::pair<Vector, uint32_t>, std::vector<SatelliteConnectEntry>>>  m_positionToSatelliteRecord;

	// Based on the ground position and equipment interface number, return the recently connected satellite layer and satellite number
	std::unordered_map<uint32_t, uint32_t> GetRecentSatelliteByPosition(Vector gsPosition, uint32_t interface);
	void SetRecentSatelliteByPosition(Vector gsPosition, uint32_t layer, uint32_t satelliteNum, uint32_t interface);

};

/**
 * \ingroup SatelliteNetwork
 *
 * \brief Distance nearest first strategy
 */
class DistanceNearestFirst : public SwitchStrategyGSL
{
public:
	static TypeId GetTypeId (void);

	// constructor
	DistanceNearestFirst ();
	virtual ~DistanceNearestFirst ();

	virtual void UpdateSwitch(std::vector<std::pair<Ptr<Node>, std::vector<std::pair<uint32_t, Ptr<Node>>>>>& gslRecord, std::vector<std::pair<Ptr<Node>, std::vector<std::pair<uint32_t, Ptr<Node>>>>>& gslRecordCopy);

};

/**
 * \ingroup SatelliteNetwork
 *
 * \brief When the satellite is no longer visible to the ground, the ground station switches to the new satellite
 */
class SwitchTriggeredByInvisible : public SwitchStrategyGSL
{
public:
	static TypeId GetTypeId (void);

	// constructor
	SwitchTriggeredByInvisible ();
	virtual ~SwitchTriggeredByInvisible ();

	virtual void UpdateSwitch(std::vector<std::pair<Ptr<Node>, std::vector<std::pair<uint32_t, Ptr<Node>>>>>& gslRecord, std::vector<std::pair<Ptr<Node>, std::vector<std::pair<uint32_t, Ptr<Node>>>>>& gslRecordCopy);

};

/**
 * \ingroup SatelliteNetwork
 *
 * \brief Geographic information aware switching
 */
class GeographicInformationAwareSwitching : public SwitchStrategyGSL
{
public:
	static TypeId GetTypeId (void);

	// constructor
	GeographicInformationAwareSwitching ();
	virtual ~GeographicInformationAwareSwitching ();

	void UpdateSwitch(std::vector<std::pair<Ptr<Node>, std::vector<std::pair<uint32_t, Ptr<Node>>>>>& gslRecord, std::vector<std::pair<Ptr<Node>, std::vector<std::pair<uint32_t, Ptr<Node>>>>>& gslRecordCopy);
	std::pair<uint32_t, bool> GetPartitionIdOfGroundStation(Ptr<Node> groundStation);
	uint32_t GetSatelliteCorrespondingToPartition(uint32_t partitionId, bool flyingDirection);

private:
	void InitializeGeographicPartition();
	void UpdateMappingOfPartitionAndSatellite();
	void SupplementMappingForSomePartitions();
	uint32_t DetermineWhichAscendingPartitionSatelliteBelongTo(Ptr<Node> satellite);
	uint32_t DetermineWhichDescendingPartitionSatelliteBelongTo(Ptr<Node> satellite);

	/// Map: key (partition id), value (partition extent LLA coordinates)
	std::map<uint32_t, std::vector<std::pair<double, double>>> m_ascendingPartitionLLA;
	/// Map: key (partition id), value (partition extent LLA coordinates)
	std::map<uint32_t, std::vector<std::pair<double, double>>> m_descendingPartitionLLA;

	/// Map: key(ascending partition id), value (satellite node), ideally with one satellite in one partition
	std::map<uint32_t, std::vector<Ptr<Node>>> m_mapOfAscendingPartitionAndSatellite;
	/// Map: key(satellite node), value (ascending partition id), ideally with one satellite in one partition
	std::map<Ptr<Node>, uint32_t> m_mapOfSatelliteAndAscendingPartition;

	/// Map: key(descending partition id), value (satellite node), ideally with one satellite in one partition
	std::map<uint32_t, std::vector<Ptr<Node>>> m_mapOfDescendingPartitionAndSatellite;
	/// Map: key(satellite node), value (descending partition id), ideally with one satellite in one partition
	std::map<Ptr<Node>, uint32_t> m_mapOfSatelliteAndDescendingPartition;

	/// Map: key (ground station node), value (flying direction: 1 means ascending direction, 0 means descending direction)
	std::map<Ptr<Node>, bool> m_mapOfGroundStationAndSatelliteFlyingDirection;
	/// Map: key (ground station node), value (the ascending partition it belongs to)
	std::map<Ptr<Node>, uint32_t> m_mapOfGroundStationAndAscendingPartition;
	/// Map: key (ground station node), value (the descending partition it belongs to)
	std::map<Ptr<Node>, uint32_t> m_mapOfGroundStationAndDescendingPartition;
};

}

#endif /* GSL_SWITCH_STRATEGY_H */
