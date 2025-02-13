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

#ifndef WALKER_CONSTELLATION_STRUCTURE_H
#define WALKER_CONSTELLATION_STRUCTURE_H


#include "ns3/node-container.h"
#include <tuple>
#include <unordered_map>
#include "ns3/point-to-point-laser-net-device.h"
#include "ns3/node-container.h"
#include "ns3/net-device-container.h"
#include "ns3/ipv4.h"

namespace ns3 {

class Constellation : public Object
{
public:
    static TypeId GetTypeId (void);

	/**
	 * \brief Constellation Constructor
	 */
    Constellation();
    Constellation(std::string name, int index, int orbitNum, int satNum, int phase, double altitude, double inclination, std::string type, std::string color);

    void SetTle(std::vector<std::string> tle);
    void SetNodes(NodeContainer curCon);
    NodeContainer GetNodes();
    void SetGroundStationNodes(NodeContainer gs);
    NodeContainer GetGroundStationNodes();
    std::string GetName();
    int GetIndex();
    std::string GetType();
    std::string GetColor();
    std::vector<double> GetColorNumbers();
    int GetOrbitNum();
    int GetSatNum();
    int GetPhase();
    double GetAltitude();
    double GetInclination();
    std::vector<std::vector<std::string>> GetTle();
    void SetInitialSatelliteCoordinates(std::pair<double, double> coordinates);
    std::vector<std::pair<double, double>>& GetInitialSatelliteCoordinates();
    void SetAdjacency(std::unordered_map<uint32_t, std::vector<uint32_t>> adjacency);
    void AddAdjacencySatellite(std::pair<uint32_t, uint32_t> newLink);
    std::unordered_map<uint32_t, std::vector<uint32_t>> GetAdjacency();
    std::vector<uint32_t> GetAdjacency(uint32_t cur_sat);

	/**
	* \brief Set/Obtain gsl information in all constellation.
	*/
    void SetGSLInformation(std::vector<std::pair<Ptr<Node>, std::vector<std::pair<uint32_t, Ptr<Node>>>>> gslInfo);
    std::vector<std::pair<Ptr<Node>, std::vector<std::pair<uint32_t, Ptr<Node>>>>>  GetGSLInformation();

	/**
	* \brief Get ISL interface number.
	* \param satId0		The satellite ID of one end.
	* \param satId1		The satellite ID of the other end.
	* \return ISL interface number.
	*/
    int32_t GetISLInterfaceNumber(uint32_t satId0, uint32_t satId1);

	/**
	* \brief Set/Obtain p2p netdevice in this constellation.
	*/
    void SetIslNetDevicesInfo(NetDeviceContainer& netdevices);
    NetDeviceContainer& GetIslNetDevicesInfo();

	/**
	* \brief Calculate link string key.
	*/
    std::string CalStringKey(uint32_t satId0, int32_t satId1);

	/**
	* \brief Set/Obtain link information.
	*/
    void SetIslFromTo(std::vector<std::pair<uint32_t, uint32_t>> islFromTo);
    std::vector<std::pair<uint32_t, uint32_t>>& GetIslFromTo();

	/**
	* \brief Set/Obtain unique link information, with the smaller satellite ID first.
	*/
	void SetIslFromToUnique(std::vector<std::pair<uint32_t, uint32_t>> islFromToUnique);
	std::vector<std::pair<uint32_t, uint32_t>>& GetIslFromToUnique();

	/**
	* \brief Set the range of satellite IDs included in this constellation.
	* \param satIdRange		A pair composed of the start number and the end number.
	*/
	void SetSatIdRange(std::pair<uint32_t, uint32_t> satIdRange);

	/**
	* \brief Determine whether a satellite belongs to this constellation.
	* \param satId		A satellite ID starting from 0.
	* \return whether the satellite belongs to this constellation.
	*/
	bool SatelliteBelongTo(uint32_t satId);

	/**
	* \brief Get the color value represented by rgba.
	* \param str	String in rgba format, such as "rgba(249, 252, 48, 1)".
	* \return the color value represented by rgba.
	*/
	std::vector<double> ExtractNumbers(const std::string& str);

	/**
	* \brief Get the radius in meters of the coverage area, approximately as a circle.
	* \return the radius in meters of the coverage area.
	*/
	double GetArcLength();

private:
    std::string m_name;
    int m_index;
	int m_orbitNum;
	int m_satNum;
	int m_phase;
	double m_altitude;
	double m_inclination;
	std::string m_type;
	std::string m_color;
	std::vector<double> m_color_numbers;									//<! The color value represented by rgba.
	double m_arclen;														//<! The radius in meters of a satellite coverage area.

	std::pair<uint32_t, uint32_t> m_satIdRange;								//<! A pair composed of the start number and the end number.
	std::vector<std::vector<std::string>> m_tles;							//<! Satellite initial TLEs
	NodeContainer m_nodes;  												//<! All satellites of this constellation
	NodeContainer m_groundStationNodes;  									//<! All ground stations including those who do not connect this constellation todo: to be optimized

	std::vector<std::pair<double, double>> m_initialSatelliteCoordinates;	//<! Initial constellation coordinates, pair(latitude, longitude)
	std::unordered_map<uint32_t, std::vector<uint32_t>> m_adjacency;  		//<! Just for satellites of this constellation, no ground station

	std::vector<std::pair<Ptr<Node>, std::vector<std::pair<uint32_t, Ptr<Node>>>>> m_gslLinks;  //<! All gsl relationships including those who do not connect this constellation todo: to be optimized

	NetDeviceContainer m_islNetDevices;  									//<! isl net device Container
	std::vector<std::pair<uint32_t, uint32_t>> m_islFromTo;  				//<! from node A to node B
	std::vector<std::pair<uint32_t, uint32_t>> m_islFromToUnique;  			//<! from node A to node B (unique)

};

}




#endif /* WALKER_CONSTELLATION_STRUCTURE_H */
