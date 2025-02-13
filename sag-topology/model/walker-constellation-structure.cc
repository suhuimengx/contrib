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

#include "ns3/walker-constellation-structure.h"
#include "ns3/cppmap3d.hh"

namespace ns3 {
	NS_OBJECT_ENSURE_REGISTERED (Constellation);

	TypeId
	Constellation::GetTypeId (void)
	{
		static TypeId tid = TypeId ("ns3::Constellation")
				.SetParent<Object> ()
				.SetGroupName("SatelliteNetwork")
		;
		return tid;
	}

	Constellation::Constellation(){

	}
	Constellation::Constellation(std::string name, int index, int orbitNum, int satNum, int phase, double altitude, double inclination, std::string type, std::string color){
		m_name = name;
		m_index = index;
		m_orbitNum = orbitNum;
		m_satNum = satNum;
		m_phase = phase;
		m_altitude = altitude;
		m_inclination = inclination;
		m_tles = {};
		m_type = type;
		m_color = color;
		m_color_numbers = ExtractNumbers(m_color);
	    const double pi = 3.14159265358979311599796346854;
		double R = cppmap3d::internal::getMajor(cppmap3d::Ellipsoid::WGS72);
		double H = R + GetAltitude()*1000;
		double alpha = 30 * pi / 180;
		double theta = pi/2 - alpha - acos(H*sin(alpha)/R);
		m_arclen = theta*R;
	}

	void
	Constellation::SetTle(std::vector<std::string> tle){

		m_tles.push_back(tle);
	}

	void
	Constellation::SetNodes(NodeContainer curCon){
		m_nodes = curCon;
	}

	std::string
	Constellation::GetName(){
		return m_name;
	}

 	int
	Constellation::GetIndex(){
    	return m_index;
    }

	std::string
	Constellation::GetType(){
		return m_type;
	}

	std::string
	Constellation::GetColor(){
		return m_color;
	}

    std::vector<double>
    Constellation::GetColorNumbers(){
    	return m_color_numbers;
    }

    double
	Constellation::GetArcLength(){
    	return m_arclen;
    }

	int
	Constellation::GetOrbitNum(){
		return m_orbitNum;
	}

	int
	Constellation::GetSatNum(){
		return m_satNum;
	}

	int
	Constellation::GetPhase(){
		return m_phase;
	}

	double
	Constellation::GetAltitude(){
		return m_altitude;
	}

	double
	Constellation::GetInclination(){
		return m_inclination;
	}

	std::vector<std::vector<std::string>>
	Constellation::GetTle(){
		return m_tles;
	}

	NodeContainer
	Constellation::GetNodes(){
		return m_nodes;
	}

    void
	Constellation::SetGroundStationNodes(NodeContainer gs){
    	m_groundStationNodes = gs;
    }

    NodeContainer
	Constellation::GetGroundStationNodes(){
    	return m_groundStationNodes;
    }

	void
	Constellation::SetInitialSatelliteCoordinates(std::pair<double, double> coordinates){
		m_initialSatelliteCoordinates.push_back(coordinates);
	}

	std::vector<std::pair<double, double>>&
	Constellation::GetInitialSatelliteCoordinates(){
		return m_initialSatelliteCoordinates;
	}

	void
	Constellation::SetAdjacency(std::unordered_map<uint32_t, std::vector<uint32_t>> adjacency){
		m_adjacency = adjacency;
	}

    void
	Constellation::AddAdjacencySatellite(std::pair<uint32_t, uint32_t> newLink){
    	m_adjacency[newLink.first].push_back(newLink.second);
    	m_adjacency[newLink.second].push_back(newLink.first);
    }

	std::unordered_map<uint32_t, std::vector<uint32_t>>
	Constellation::GetAdjacency(){
		return m_adjacency;
	}

    std::vector<uint32_t>
    Constellation::GetAdjacency(uint32_t cur_sat){
    	if(m_adjacency.find(cur_sat)!=m_adjacency.end()){
    		return m_adjacency[cur_sat];
    	}
    	else{
    		throw std::runtime_error("Wrong satellite ID for GetAdjacency");
    	}
    }

    void
	Constellation::SetGSLInformation(std::vector<std::pair<Ptr<Node>, std::vector<std::pair<uint32_t, Ptr<Node>>>>> gslInfo){
    	m_gslLinks = gslInfo;
    }

    std::vector<std::pair<Ptr<Node>, std::vector<std::pair<uint32_t, Ptr<Node>>>>>
	Constellation::GetGSLInformation(){
    	return m_gslLinks;
    }

    void
	Constellation::SetIslNetDevicesInfo(NetDeviceContainer& netdevices){
    	m_islNetDevices = netdevices;
    }

    NetDeviceContainer&
	Constellation::GetIslNetDevicesInfo(){
    	return m_islNetDevices;
    }


    int32_t
	Constellation::GetISLInterfaceNumber(uint32_t satId0, uint32_t satId1){
    	Ptr<SAGLinkLayer> dev0= m_islNetDevices.GetWithKey(CalStringKey(satId0,satId1))->GetObject<SAGLinkLayer>();
    	int32_t interfaceNumber = m_nodes.Get(satId0)->GetObject<Ipv4>()->GetInterfaceForDevice(dev0);
    	if(interfaceNumber != -1){
    		return interfaceNumber;
    	}
    	else{
    		throw std::runtime_error("Constellation::GetISLInterfaceNumber");
    	}
    }

    std::string
	Constellation::CalStringKey(uint32_t satId0, int32_t satId1){
        std::string key="";
        key = std::to_string(satId0) + "_to_" + std::to_string(satId1);
        return key;
    }


    void
	Constellation::SetIslFromTo(std::vector<std::pair<uint32_t, uint32_t>> islFromTo){
    	m_islFromTo = islFromTo;
    }

    std::vector<std::pair<uint32_t, uint32_t>>&
	Constellation::GetIslFromTo(){
    	return m_islFromTo;
    }

	void
	Constellation::SetIslFromToUnique(std::vector<std::pair<uint32_t, uint32_t>> islFromToUnique){
		m_islFromToUnique = islFromToUnique;
	}

	std::vector<std::pair<uint32_t, uint32_t>>&
	Constellation::GetIslFromToUnique(){
		return m_islFromToUnique;
	}

	void
	Constellation::SetSatIdRange(std::pair<uint32_t, uint32_t> satIdRange){
		m_satIdRange = satIdRange;
	}

	bool
	Constellation::SatelliteBelongTo(uint32_t satId){
		if(m_satIdRange.first <= satId && m_satIdRange.second >= satId){
			return true;
		}
		return false;
	}

	std::vector<double>
	Constellation::ExtractNumbers(const std::string& str) {
	    std::vector<double> numbers;
	    size_t startPos = str.find('(');
	    size_t endPos = str.find(')');
	    std::string numbersStr = str.substr(startPos + 1, endPos - startPos - 1);

	    std::stringstream ss(numbersStr);
	    std::string numStr;
	    while (std::getline(ss, numStr, ',')) {
	        double num = std::stod(numStr);
	        numbers.push_back(num);
	    }

	    return numbers;
	}


}




