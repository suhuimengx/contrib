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

#include "topology-satellite-network.h"
#include "ns3/sag_rtp_constants.h"
#include "ns3/cppmap3d.hh"
#include "ns3/cppjson2structure.hh"
#include "ns3/sgp4coord.h"
#include "ns3/satellite.h"
#include <random>
#include "ns3/quic-helper.h"
#include "ns3/scpstp-helper.h"
#include "ns3/traffic-control-layer.h"
#include "ns3/red-queue-disc.h"

#define pi 3.14159265358979311599796346854

namespace ns3 {

	NS_LOG_COMPONENT_DEFINE ("TopologySatelliteNetwork");

    NS_OBJECT_ENSURE_REGISTERED (TopologySatelliteNetwork);

    TypeId TopologySatelliteNetwork::GetTypeId (void)
    {
        static TypeId tid = TypeId ("ns3::TopologySatelliteNetwork")
                .SetParent<Object> ()
                .SetGroupName("SatelliteNetwork")
        ;
        return tid;
    }

    TopologySatelliteNetwork::TopologySatelliteNetwork(Ptr<BasicSimulation> basicSimulation
		, std::vector<std::pair<SAGRoutingHelper, int16_t>>& ipv4RoutingHelper
		, std::vector<std::pair<SAGRoutingHelperIPv6, int16_t>>& ipv6RoutingHelper) {
		m_basicSimulation = basicSimulation;
        m_system_id = m_basicSimulation->GetSystemId();
        m_systems_count = m_basicSimulation->GetSystemsCount();
        m_enable_distributed = m_basicSimulation->IsDistributedEnabled();
		m_ipv4RoutingHelpers = &ipv4RoutingHelper;
		m_ipv6RoutingHelpers = &ipv6RoutingHelper;

        ReadConfig();
        if(parse_boolean(m_basicSimulation->GetConfigParamOrDefault("enable_distributed_pre_process", "false"))){
        	BuildTopologyOnly();  // for distributed pre process
        }
        else{
            Build();  // for normal simulation
        }
    }

    void TopologySatelliteNetwork::ReadConfig() {
        m_satellite_network_dir = m_basicSimulation->GetRunDir() + "/config_topology";
        m_dynamicStateUpdateIntervalNs = parse_positive_int64(m_basicSimulation->GetConfigParamOrFail("dynamic_state_update_interval_ns"));
        //m_satellite_network_routes_dir =  m_basicSimulation->GetRunDir() + "/" + m_basicSimulation->GetConfigParamOrFail("satellite_network_routes_dir");
        m_satellite_network_force_static = parse_boolean(m_basicSimulation->GetConfigParamOrDefault("satellite_network_force_static", "false"));
		m_time_end = parse_positive_int64(m_basicSimulation->GetConfigParamOrFail("simulation_end_time_ns"));

        if(parse_boolean(m_basicSimulation->GetConfigParamOrDefault("enable_distance_nearest_first", "false"))){
        	ObjectFactory switchStrategyFactory("ns3::SwitchTriggeredByInvisible");
        	m_switchStrategy = switchStrategyFactory.Create<SwitchStrategyGSL> ();
        }
        NS_ASSERT_MSG(m_switchStrategy != 0, "No Set Ground-to-Satellite Switching Strategy");


        if(parse_boolean(m_basicSimulation->GetConfigParamOrDefault("enable_grid_type_isl_establish", "false"))){
            ObjectFactory islEstablishStrategyFactory("ns3::GridTypeBuilding");
            m_islEstablishRule = islEstablishStrategyFactory.Create<ISLEstablishRule> ();
        }
        NS_ASSERT_MSG(m_switchStrategy != 0, "No Set Inter-Satellite Link Establishing Rule");

        m_isIPv4Networking = parse_boolean(m_basicSimulation->GetConfigParamOrDefault("enable_ipv4_addressing_protocol", "false"));
        m_isIPv6Networking = parse_boolean(m_basicSimulation->GetConfigParamOrDefault("enable_ipv6_addressing_protocol", "false"));
        NS_ASSERT_MSG(m_isIPv4Networking != m_isIPv6Networking, "Currently only supports one address protocol enabled, IPv4 or IPv6");

        if(m_basicSimulation->GetConfigParamOrDefault("network_addressing_method", "default") == "default"){
        	ObjectFactory addressingMethodFactory("ns3::TraditionalAddressingGroundSameNetworkSegment");
        	m_networkAddressingMethod = addressingMethodFactory.Create<AddressingRule> ();
        }
        else if(m_basicSimulation->GetConfigParamOrDefault("network_addressing_method", "default") == "default(Sat2GndwithDiffNetSegment"){
        	ObjectFactory addressingMethodFactory("ns3::TraditionalAddressing");
        	m_networkAddressingMethod = addressingMethodFactory.Create<AddressingRule> ();
        }

        if(m_basicSimulation->GetConfigParamOrDefault("network_addressing_method_ipv6", "default") == "default"){
    		ObjectFactory addressingMethodIPv6Factory("ns3::TraditionalAddressingIPv6");
            m_networkAddressingMethodIPv6 = addressingMethodIPv6Factory.Create<AddressingRule> ();
        }

    }

    void
    TopologySatelliteNetwork::Build() {
        std::cout << "SATELLITE NETWORK" << std::endl;

        // Generate TLEs
        GenerateTLE();

        // Initialize satellites
        ReadSatellites();
        std::cout << "  > Number of satellites........ " << m_satelliteNodes.GetN() << std::endl;

        // Initialize ground stations
		ReadGroundStations();
		std::cout << "  > Number of ground stations... " << m_groundStationNodes.GetN() << std::endl;



        // Only ground stations are valid endpoints
        for (uint32_t i = 0; i < m_groundStations.size(); i++) {
            m_endpoints.insert(m_satelliteNodes.GetN() + i);
        }

        // All nodes
        m_allNodes.Add(m_satelliteNodes);
        m_allNodes.Add(m_groundStationNodes);
        std::cout << "  > Number of nodes............. " << m_allNodes.GetN() << std::endl;


//		// All are valid endpoints
//		for (uint32_t i = 0; i < m_allNodes.GetN(); i++) {
//			m_endpoints.insert(i);
//		}


        // Install internet stacks on all nodes
        InstallInternetStacks();
        std::cout << "  > Installed Internet stacks" << std::endl;


      	// Add unique loop_back address for each node
    	Ipv4AddressHelper ipv4AddrHelper_lp;
    	ipv4AddrHelper_lp.SetBase ("128.0.0.0", "255.0.0.0");
    	for(uint32_t i = 0; i < this->GetNodes().GetN(); i++)
    	{
    		Ipv4Address new_ads = ipv4AddrHelper_lp.NewAddress();
    		this->GetNodes().Get(i)->GetObject<Ipv4L3Protocol>()->AddAddress(0, Ipv4InterfaceAddress(new_ads, Ipv4Mask("255.0.0.0")));
    	}

        // Link settings
        m_isl_data_rate_megabit_per_s = parse_positive_double(m_basicSimulation->GetConfigParamOrFail("isl_data_rate_megabit_per_s"));
        m_gsl_data_rate_megabit_per_s = parse_positive_double(m_basicSimulation->GetConfigParamOrFail("gsl_data_rate_megabit_per_s"));
        m_isl_max_queue_size_pkts = parse_positive_int64(m_basicSimulation->GetConfigParamOrFail("isl_max_queue_size_pkts"));
        m_gsl_max_queue_size_pkts = parse_positive_int64(m_basicSimulation->GetConfigParamOrFail("gsl_max_queue_size_pkts"));

        // Utilization tracking settings
        m_enable_link_utilization_tracking = parse_boolean(m_basicSimulation->GetConfigParamOrFail("enable_link_utilization_tracing"));
        if (m_enable_link_utilization_tracking) {
            m_link_utilization_tracking_interval_ns = parse_positive_int64(m_basicSimulation->GetConfigParamOrFail("link_utilization_tracking_interval_ns"));
        }

        m_networkAddressingMethod->SetBasicSimHandle(m_basicSimulation);
		m_networkAddressingMethod->SetTopologyHandle(m_constellations, m_groundStationNodes, m_switchStrategy);
		m_networkAddressingMethodIPv6->SetBasicSimHandle(m_basicSimulation);
		m_networkAddressingMethodIPv6->SetTopologyHandle(m_constellations, m_groundStationNodes, m_switchStrategy);

        // Create ISLs
        std::cout << "  > Reading and creating ISLs" << std::endl;
        m_islEstablishRule->SetBasicSimulationHandle(m_basicSimulation);
        m_islEstablishRule->SetTopologyHandle(m_constellations);
        m_islEstablishRule->SetSatelliteNetworkDir(m_satellite_network_dir);
        m_islEstablishRule->ISLEstablish(m_islInterOrbit);
        remove_file_if_exists(m_satellite_network_dir + "/system_" + std::to_string(m_system_id)+"_topology_change_message.txt");
        remove_file_if_exists(m_satellite_network_dir + "/system_" + std::to_string(m_system_id)+"_topology_change_message_isl.txt");
        ReadISLs();

        // Maintain GSLs (link switch)
		std::cout << "  > Creating GSLs" << std::endl;
		m_switchStrategy->SetTopologyHandle(m_constellations, m_groundStationNodes, m_groundStations);
		m_switchStrategy->SetBasicSimHandle(m_basicSimulation);
		InstallGSLInterfaceForAllSatellites();  // just install net devices, and initialize address
		InstallGSLInterfaceForAllGroundStations();  // just install net devices
		MakeGSLChangeEvent(0);
        m_basicSimulation->RegisterTimestamp("Initialize satellite-to-ground links");

        // Wireshark
        SAGPhysicalGSLHelper gsl_helper(m_basicSimulation);
        EnablePcap(gsl_helper);
        EnableUtilization();

        // Update link delay by time slot (optimize simulation efficiency in large traffic scenarios)
        if(parse_boolean(m_basicSimulation->GetConfigParamOrDefault("enable_trajectory_tracing", "false"))){
        	remove_dir_and_subfile_if_exists(m_satellite_network_dir + "/system_"+ to_string(m_system_id) + "_coordinates");
        	mkdir_force_if_not_exists(m_satellite_network_dir + "/system_"+ to_string(m_system_id) + "_coordinates");
        	remove_dir_and_subfile_if_exists(m_satellite_network_dir + "/system_"+ to_string(m_system_id) + "_orbital_elements");
        	mkdir_force_if_not_exists(m_satellite_network_dir + "/system_"+ to_string(m_system_id) + "_orbital_elements");
        }
        MakeLinkDelayUpdateEvent(0);
        m_basicSimulation->RegisterTimestamp("Initialize link propagation delay");

   		Ptr<SAGPhysicalLayerGSL> gsl_channel = m_gslSatNetDevices.Get(0)->GetChannel()->GetObject<SAGPhysicalLayerGSL>();
   		if (gsl_channel-> GetEnableBER()){
   			MakeLinkSINRUpdateEvent(0);
   			std::cout <<"Enable reading SINR-BER from link system simulatior" <<std::endl;
   		}

        // Sun Outage
        if(parse_boolean(m_basicSimulation->GetConfigParamOrDefault("enable_sun_outage", "false"))){
        	ReadSunTrajectoryEciFromCspice();
            MakeSunOutageEvent(0);
            m_basicSimulation->RegisterTimestamp("Enable sun outage simulation");
        }

        //PacketLossTrace();

        // Store IP address to node id (each interface has an IP address, so multiple IPs per node)
        // for (uint32_t i = 0; i < m_satelliteNodes.GetN(); i++) {
        //     for (uint32_t j = 1; j < m_satelliteNodes.Get(i)->GetObject<Ipv4>()->GetNInterfaces(); j++) {
        //         m_ip_to_node_id.insert({m_satelliteNodes.Get(i)->GetObject<Ipv4>()->GetAddress(j, 0).GetLocal().Get(), i});
        //     }
        // }

        std::cout << std::endl;

				
				// for(uint32_t i = 0; i < m_satelliteNodes.GetN(); i++){
				//     if( m_satelliteNodes.Get(i)->GetObject<TrafficControlLayer>() != 0){
				//         for(uint32_t j = 0; j < m_satelliteNodes.Get(i)->GetNDevices(); j++)
				//         {
				//             Ptr<TrafficControlLayer> trafficControlLayer = m_satelliteNodes.Get(i)->GetObject<TrafficControlLayer>();
				//             // 获取RedQueueDisc 的tid
				//             TypeId tid = ns3::RedQueueDisc::GetTypeId();
				//             std::cout << tid << std::endl;
				//         }
				//         //std::cout << "Satellite [" << i << "] has " <<  "TrafficControlLayer: " << m_satelliteNodes.Get(i)->GetObject<TrafficControlLayer>()-> GetRootQueueDiscOnDevice(m_satelliteNodes.Get(i)->GetDevice(j))->GetInstanceTypeId().GetName() << std::endl;
				//     }
				//     else{
				//         std::cout << "Satellite [" << i << "] hasn't " <<  "TrafficControlLayer" << std::endl;
				//     }
				// }
				// for(uint32_t i = 0; i < m_groundStationNodes.GetN(); i++){
				//     if( m_groundStationNodes.Get(i)->GetObject<TrafficControlLayer>() != 0){
				//         for(uint32_t j = 0; j < m_groundStationNodes.Get(i)->GetNDevices(); j++)
				//         {
				//             Ptr<TrafficControlLayer> trafficControlLayer = m_groundStationNodes.Get(i)->GetObject<TrafficControlLayer>();
				//         }
				//         //std::cout << "GroundStation [" << i << "] has " <<  "TrafficControlLayer: " << m_groundStationNodes.Get(i)->GetObject<TrafficControlLayer>()-> GetRootQueueDiscOnDevice(m_groundStationNodes.Get(i)->GetDevice(j))->GetInstanceTypeId().GetName() << std::endl;
				//     }
				//     else{
				//         std::cout << "GroundStation [" << i << "] hasn't " <<  "TrafficControlLayer" << std::endl;
				//     }
				// }
		}

    void
    TopologySatelliteNetwork::BuildTopologyOnly() {
        std::cout << "SATELLITE NETWORK" << std::endl;

        // Generate TLEs
        GenerateTLE();

        // Initialize satellites
        ReadSatellites();
        std::cout << "  > Number of satellites........ " << m_satelliteNodes.GetN() << std::endl;

        // Initialize ground stations
		ReadGroundStations();
		std::cout << "  > Number of ground stations... " << m_groundStationNodes.GetN() << std::endl;

        // All nodes
        m_allNodes.Add(m_satelliteNodes);
        m_allNodes.Add(m_groundStationNodes);
        std::cout << "  > Number of nodes............. " << m_allNodes.GetN() << std::endl;

        // Install internet stacks on all nodes
		InternetStackHelper internet1;
		internet1.Install(m_allNodes);
		/*
				// Install ScpsTp
		ScpsTpHelper scpstp;
		scpstp.Install(m_allNodes);*/
    	m_basicSimulation->RegisterTimestamp("Install Internet stacks");
        std::cout << "  > Installed Internet stacks" << std::endl;

	


      	// Add unique loop_back address for each node
    	Ipv4AddressHelper ipv4AddrHelper_lp;
    	ipv4AddrHelper_lp.SetBase ("128.0.0.0", "255.0.0.0");
    	for(uint32_t i = 0; i < this->GetNodes().GetN(); i++)
    	{
    		Ipv4Address new_ads = ipv4AddrHelper_lp.NewAddress();
    		this->GetNodes().Get(i)->GetObject<Ipv4L3Protocol>()->AddAddress(0, Ipv4InterfaceAddress(new_ads, Ipv4Mask("255.0.0.0")));
    	}

        // Link settings
        m_isl_data_rate_megabit_per_s = parse_positive_double(m_basicSimulation->GetConfigParamOrFail("isl_data_rate_megabit_per_s"));
        m_gsl_data_rate_megabit_per_s = parse_positive_double(m_basicSimulation->GetConfigParamOrFail("gsl_data_rate_megabit_per_s"));
        m_isl_max_queue_size_pkts = parse_positive_int64(m_basicSimulation->GetConfigParamOrFail("isl_max_queue_size_pkts"));
        m_gsl_max_queue_size_pkts = parse_positive_int64(m_basicSimulation->GetConfigParamOrFail("gsl_max_queue_size_pkts"));

        m_networkAddressingMethod->SetBasicSimHandle(m_basicSimulation);
		m_networkAddressingMethod->SetTopologyHandle(m_constellations, m_groundStationNodes, m_switchStrategy);
		m_networkAddressingMethodIPv6->SetBasicSimHandle(m_basicSimulation);
		m_networkAddressingMethodIPv6->SetTopologyHandle(m_constellations, m_groundStationNodes, m_switchStrategy);

        // Create ISLs
        std::cout << "  > Reading and creating ISLs" << std::endl;
        m_islEstablishRule->SetBasicSimulationHandle(m_basicSimulation);
        m_islEstablishRule->SetTopologyHandle(m_constellations);
        m_islEstablishRule->SetSatelliteNetworkDir(m_satellite_network_dir);
        m_islEstablishRule->ISLEstablish(m_islInterOrbit);
        remove_file_if_exists(m_satellite_network_dir + "/system_" + std::to_string(m_system_id)+"_topology_change_message.txt");
        remove_file_if_exists(m_satellite_network_dir + "/system_" + std::to_string(m_system_id)+"_topology_change_message_isl.txt");
        ReadISLs();

        // Maintain GSLs (link switch)
		std::cout << "  > Creating GSLs" << std::endl;
		m_switchStrategy->SetTopologyHandle(m_constellations, m_groundStationNodes, m_groundStations);
		m_switchStrategy->SetBasicSimHandle(m_basicSimulation);
		InstallGSLInterfaceForAllSatellites();  // just install net devices, and initialize address
		InstallGSLInterfaceForAllGroundStations();  // just install net devices
		MakeGSLChangeEvent(0);
        m_basicSimulation->RegisterTimestamp("Initialize satellite-to-ground links");

        // Sun Outage
        if(parse_boolean(m_basicSimulation->GetConfigParamOrDefault("enable_sun_outage", "false"))){
        	ReadSunTrajectoryEciFromCspice();
            MakeSunOutageEvent(0);
            m_basicSimulation->RegisterTimestamp("Enable sun outage simulation");
        }

        std::cout << std::endl;

    }


    void
	TopologySatelliteNetwork::PacketLossTrace(){
    	for(uint32_t i = 0; i < m_allNodes.GetN(); i++){
    		m_allNodes.Get(i)->GetObject<Ipv4L3Protocol>()->TraceConnectWithoutContext("Drop", MakeCallback (&TopologySatelliteNetwork::InsertDropLog, this));
    	}
    }

    void
    TopologySatelliteNetwork::InsertDropLog(const Ipv4Header & hd, Ptr<const Packet> p, Ipv4L3Protocol::DropReason r, Ptr<Ipv4> i, uint32_t j){

    	std::cout<<Simulator::Now()<<"   PacketDropReason: "<< r <<std::endl;
    }

    void
	TopologySatelliteNetwork::GenerateTLE(){
		std::string inputEpoch = m_basicSimulation->GetConfigParamOrDefault("epoch", "2000-01-01 00:00:00");
		double seconds;
		DateTime dt;
		// YYYY-MM-DD HH:MM:SS(.MMM)
		std::sscanf(
			inputEpoch.c_str(), "%04d%*c%02d%*c%02d %02d%*c%02d%*c%lf",
			&dt.year, &dt.month, &dt.day, &dt.hours, &dt.minutes, &seconds
		);

		NS_ASSERT_MSG (
			dt.year >= JulianDate::MinYear, "Complete EOP data is not available before that date!"
		);

		NS_ASSERT_MSG (
			dt.year <= JulianDate::MaxYear, "Dates beyond 2099 are not supported!"
		);

		dt.seconds = static_cast<uint32_t> (seconds);
		dt.millisecs = static_cast<uint32_t> ((seconds - dt.seconds)*1000 + 0.5);

		int monthDays[13] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};
		int nday = monthDays[dt.month-1] + dt.day;
		if(dt.month > 2 && ((dt.year % 4 == 0 && dt.year % 100 != 0) || dt.year % 400 == 0))
		{
			nday += 1;
		}

		// days
		std::stringstream ss;
		ss << setw(3) << setfill('0') << nday ;
		std::string ndaystr;
		ss >> ndaystr;         //将字符流传给 str

		// dayin
		uint32_t ms = dt.hours*JulianDate::HourToMs + dt.minutes*JulianDate::HourToMs/60 + dt.seconds*1000 + dt.millisecs;
		uint32_t ms_oneday = 24*JulianDate::HourToMs;
		double timeinday = 1.0*ms / ms_oneday;
		std::stringstream ss1;
		ss1 << std::fixed << std::setprecision(8) << timeinday;
		std::string result = ss1.str();
		std::string timeindaystr = result.substr(2,9);


		// year
		std::string yearstr = std::to_string(dt.year);
		yearstr = yearstr.substr(2,3);

		std::string tle1_temp_str = "1 %05dU 00000ABC "+yearstr+ndaystr+"."+timeindaystr+"  .00000000  00000-0  00000+0 0    0";
		const char* tle1_temp_char = tle1_temp_str.c_str();





    	// Open file
    	json j;
		std::ifstream config_file;
		config_file.open(m_satellite_network_dir + "/config_constellation.json");
		NS_ABORT_MSG_UNLESS(config_file.is_open(), "File config_constellation.json could not be opened");
		config_file >> j;
		int tilength = j["constellations"].size();
		jsonns::constellationinfo ti[tilength];
		for (int i = 0; i < tilength; i++) {
			ti[i] = j["constellations"][i];
		}

		std::string tle_file_name;
		tle_file_name = m_satellite_network_dir + "/system_" + std::to_string(m_system_id)+"_TLE.txt";
		remove_file_if_exists(tle_file_name);

		int index = 0;
		for (int i = 0; i < tilength; i++) {
			jsonns::constellationinfo consinfo = ti[i];
			std::string name = consinfo.name;
			std::string type = consinfo.walker_type;
			int orbits = consinfo.number_of_planes;
			int satsNum = consinfo.number_of_sats_per_plane;
			int phaseFactor = consinfo.phase_factor;
			double altitude = consinfo.altitude_km;
			double inclination = consinfo.inclination_deg;
			std::string color = consinfo.color;

			Ptr<Constellation> cons = CreateObject<Constellation>(name, index, orbits, satsNum, phaseFactor, altitude, inclination, type, color);
			m_constellations.push_back(cons);
			index++;
		}

		std::ofstream fileTLE(tle_file_name);
		int satellite_counter = 0;
		double G = 6.67259e-11;   // 引力常量   单位 N*m^2/s^2
		double M = 5.965e24;  // 地球质量   单位 kg
		double R = 6378.1350;  // 地球半径  单位 km
		for(auto cons : m_constellations){

			double eccentricity = 0.0;        //!< 圆形轨道 偏心率为0
			double arg_of_perigee_degree = 0;

			std::string name = cons->GetName();
			int orbitNumber = cons->GetOrbitNum();
			int satPerOrbit = cons->GetSatNum();
			int phaseFactor = cons->GetPhase();
			double altitude = cons->GetAltitude();
			double inclination = cons->GetInclination();
			std::string type = cons->GetType();

			fileTLE<<orbitNumber<<" "<<satPerOrbit<<std::endl;

			//!< 每日绕地球飞行圈数
			double r = R * 1000 + altitude * 1000;

			double mean_motion_rev_per_day=(24.0 / (2 * pi *  pow(r, 1.5) / pow(G * M, 0.5) / 60.0 / 60.0));

			for (int i = 0; i < orbitNumber; i++) {
				// phase offset of satellites with the same satellite number at adjacent orbits
				double phase_shift = phaseFactor * 360.0 / (satPerOrbit * orbitNumber);
				double raan_degree;

				if(type.compare("delta") == 0 || type.compare("Delta") == 0){
					raan_degree = i * 360.0 / orbitNumber * 1.0;        //!< raan initialization
				}
				else if(type.compare("star") == 0 || type.compare("Star") == 0){
					raan_degree = i * 360.0 / (2 * orbitNumber * 1.0);
				}
				else{
					throw std::runtime_error("Wrong input of constellation type.");
				}

				for (int j = 0; j < satPerOrbit; j++) {
					double mean_anomaly_degree = i * phase_shift * 1.0 + (j * 360.0 / satPerOrbit) * 1.0;        //!< 升交点角距
					//!< Epoch is 2000-01-01 00:00:00, which is 00001 in ddyyy format
					//!< See also: https://www.celestrak.com/columns/v04n03/#FAQ04


					char* tle_line1= static_cast<char *>(malloc(sizeof(char) * 100));
					//sprintf(tle_line1,"1 %05dU 00000ABC 00001.00000000  .00000000  00000-0  00000+0 0    0",satellite_counter + 1);
					sprintf(tle_line1,tle1_temp_char,satellite_counter + 1);

					char* strN = static_cast<char *>(malloc(sizeof(char) * 100));
					sprintf(strN, "%.7f", eccentricity);
					std::string eccentricity_st=strN;
					auto eccentricity_str=eccentricity_st.substr(2,7).c_str();
					char* tle_line2 = static_cast<char *>(malloc(sizeof(char) * 100));
					sprintf(tle_line2,"2 %05d %8.4f %8.4f %s %8.4f %8.4f %11.8f    0",
							satellite_counter+1,
							inclination,
							raan_degree,
							eccentricity_str,
							arg_of_perigee_degree,
							mean_anomaly_degree,
							mean_motion_rev_per_day
					);


					//!< Append checksums
					std::string tle_line_1 = std::string(tle_line1) + std::to_string(CalculateTleLineChecksum(tle_line1));
					std::string tle_line_2 = std::string(tle_line2) + std::to_string(CalculateTleLineChecksum(tle_line2));

					//!< Write TLE to file
					std::string satTleName = name + " " + std::to_string(satellite_counter);
					fileTLE << satTleName << std::endl;
					fileTLE << tle_line_1 << std::endl;
					fileTLE << tle_line_2 << std::endl;
					cons->SetTle({satTleName, tle_line_1, tle_line_2, std::to_string(i), std::to_string(j)});

					double AL = mean_anomaly_degree * pi / 180;
					double omega = raan_degree * pi / 180;
					double mu = inclination * pi / 180;
					double x = r * (cos(AL) * cos(omega) - sin(AL) * cos(mu) * sin(omega));
					double y = r * (cos(AL) * sin(omega) + sin(AL) * cos(mu) * cos(omega));
					double z = r * sin(AL) * sin(mu);
					double latitudeSatId0 = atan2(z, sqrt(pow(x, 2) + pow(y, 2))) * 180 / pi;
					double longitudeSatId0 = atan2(y, x) * 180 / pi;
					cons->SetInitialSatelliteCoordinates(std::make_pair(latitudeSatId0, longitudeSatId0));

					satellite_counter += 1;

				}
			}
		}
		fileTLE.close();
		config_file.close();


		m_basicSimulation->RegisterTimestamp("Create initial two line element");

		// Here, we calculate the number of ground stations in advance for the allocation of distributed system Ids.
		json gnds;
		std::ifstream fs;
		fs.open(m_satellite_network_dir + "/ground_stations.json");
		NS_ABORT_MSG_UNLESS(fs.is_open(), "File ground_stations.json could not be opened");
		fs >> gnds;
		uint32_t groundStation_counter = gnds["earth_stations"].size();
		fs.close();
		


        if(m_enable_distributed && m_basicSimulation->GetNodeAssignmentAlogirthm() != "manual"){
        	Ptr<PartitioningTopology> ParTop = CreateObject<PartitioningTopology>();
        	ParTop->SetBasicSim(m_basicSimulation);
        	//std::cout<<m_basicSimulation->GetNodeAssignmentAlogirthm()<<"  "<<satellite_counter<<"  "<<m_systems_count<<std::endl;
        	distributed_node_system_id_assignment = ParTop->GetDistributedNodeSystemIdAssignment
        			(m_basicSimulation->GetNodeAssignmentAlogirthm(),
        			m_satellite_network_dir,
        			satellite_counter,
					groundStation_counter,
					m_systems_count);
//        	std::vector<int64_t> distributed_node_system_id_assignment_sat = ParTop->GetDistributedNodeSystemIdAssignment
//					(m_basicSimulation->GetNodeAssignmentAlogirthm(),
//					m_satellite_network_dir,
//					orbit_one,
//					satsPerOrbit_one,
//					groundStation_counter,
//					m_systems_count);
        	m_basicSimulation->SetDistributedNodeSystemIdAssignment(distributed_node_system_id_assignment);
//        	for(auto x : distributed_node_system_id_assignment_sat){
//        		std::cout<<m_system_id<<"  "<<x<<std::endl;
//        	}
    		m_basicSimulation->RegisterTimestamp("Set distributed node system id assignment");
        }



    }

    int
	TopologySatelliteNetwork::CalculateTleLineChecksum(std::string tle_line_without_checksum){
        if(tle_line_without_checksum.size() != 68){
            throw std::runtime_error("Must have exactly 68 characters");
        }
        int s = 0;
        for(unsigned int i=0;i<tle_line_without_checksum.size();i++){
            if((int)tle_line_without_checksum[i]>=48 && (int)tle_line_without_checksum[i]<=57){
                s += int(tle_line_without_checksum[i])-48;
            }

            if(tle_line_without_checksum[i] == '-'){
                s += 1;
            }

        }

        return s % 10;

    }

    void
    TopologySatelliteNetwork::ReadSatellites()
    {
//    	std::vector<int64_t> distributed_node_system_id_assignment = m_basicSimulation->GetDistributedNodeSystemIdAssignment();
//    	remove_file_if_exists(m_satellite_network_dir + "/satellite_coordinates.txt");
//    	std::ofstream fileInitialCoordiantes(m_satellite_network_dir + "/satellite_coordinates.txt");

    	int64_t counter = 0;
    	int64_t totalSatellites = 0;
    	for(auto cons : m_constellations){
    		// Create the nodes
    		NodeContainer curCon;
    		//curCon.Create(cons->GetOrbitNum() * cons->GetSatNum());
    		uint32_t idStart = totalSatellites;
    		totalSatellites += cons->GetOrbitNum() * cons->GetSatNum();
    		// Associate satellite mobility model with each node
    		int64_t counterTemp = 0;
    		for(auto tle : cons->GetTle()){
				std::string name, tle1, tle2, orbitNumber, satNumber;
				name = tle[0];
				tle1 = tle[1];
				tle2 = tle[2];
				orbitNumber = tle[3];
				satNumber = tle[4];


				// enable distributd simulation need extra system id
				if (m_enable_distributed == false){
					curCon.Add(CreateObject<Node>());
				}
				else{
					Ptr<Node> node = CreateObject<Node>(distributed_node_system_id_assignment[counter]);
					curCon.Add(node);
					if (m_system_id == distributed_node_system_id_assignment[counter]){
						m_nodesCurSystem.Add(node);
						//std::cout<<"System Id "<<m_system_id<<" create Satellite Node "<<counter<<std::endl;
					}
					else{
						m_nodesCurSystemVirtual.Add(node);
					}
				}

				// Format:
				// <name>
				// <TLE line 1>
				// <TLE line 2>

				// Create satellite
				Ptr<Satellite> satellite = CreateObject<Satellite>();
				satellite->SetName(name);
				satellite->SetTleInfo(tle1, tle2);
				satellite->SetCons(cons);

				// Decide the mobility model of the satellite
				MobilityHelper mobility;
				if (m_satellite_network_force_static) {

					// Static at the start of the epoch
					mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
					mobility.Install(curCon.Get(counterTemp));
					Ptr<MobilityModel> mobModel = curCon.Get(counterTemp)->GetObject<MobilityModel>();
					mobModel->SetPosition(satellite->GetPosition(satellite->GetTleEpoch()));

				} else {

					// Dynamic
					mobility.SetMobilityModel(
							"ns3::SatellitePositionMobilityModel",
							"SatellitePositionHelper",
							SatellitePositionHelperValue(SatellitePositionHelper(satellite))
					);
					mobility.Install(curCon.Get(counterTemp));

				}

//				Vector satId0Position = curCon.Get(counterTemp)->GetObject<MobilityModel>()->GetPosition ();
//				double latitudeSatId0 = atan2(satId0Position.z, sqrt(pow(satId0Position.x, 2) + pow(satId0Position.y, 2))) * 180 / pi;
//				double longitudeSatId0 = atan2(satId0Position.y, satId0Position.x) * 180 / pi;
//				fileInitialCoordiantes << latitudeSatId0 << "," << longitudeSatId0 << std::endl;

				// Add to all satellites present
				m_satellites.push_back(satellite);

				counterTemp++;
				counter++;

    		}
    		m_satelliteNodes.Add(curCon);
    		cons->SetNodes(curCon);
    		cons->SetSatIdRange(std::make_pair(idStart, totalSatellites - 1));
    	}
    	if(m_enable_distributed == false){
    		m_nodesCurSystem = m_satelliteNodes;
    	}

		// Check that exactly that number of satellites has been read in
		if (totalSatellites != counter) {
			throw std::runtime_error("Number of satellites defined in the TLEs does not match");
		}
		m_basicSimulation->RegisterTimestamp("Create satellite objects");

		MakeSatelliteCoordinateUpdateEvent(0);

    }

    void
    TopologySatelliteNetwork::MakeSatelliteCoordinateUpdateEvent(double time){

    	JulianDate start = m_satelliteNodes.Get(0)->GetObject<SatellitePositionMobilityModel>()->GetStartTime();
		JulianDate cur = start + Simulator::Now ();
    	for(Ptr<Satellite> sat : m_satellites){
    		sat->SetPosition(cur);
    		sat->SetVelocity(cur);
    	}

    	int64_t next_update_ns = time + m_dynamicStateUpdateIntervalNs;
		if (next_update_ns < m_basicSimulation->GetSimulationEndTimeNs()) {
			Simulator::Schedule(NanoSeconds(m_dynamicStateUpdateIntervalNs),
					&TopologySatelliteNetwork::MakeSatelliteCoordinateUpdateEvent, this, next_update_ns);
		}

    }

    void
    TopologySatelliteNetwork::ReadGroundStations()
    {
//    	std::vector<int64_t> distributed_node_system_id_assignment = m_basicSimulation->GetDistributedNodeSystemIdAssignment();

    	uint32_t gsSleepTime = 0;
    	// Open file
		json j;
		std::ifstream config_file;
		config_file.open(m_satellite_network_dir + "/ground_stations.json");
		NS_ABORT_MSG_UNLESS(config_file.is_open(), "File ground_stations.json could not be opened");
		config_file >> j;
		int tilength = j["earth_stations"].size();
		jsonns::earthstationinfo ti[tilength];
		for (int i = 0; i < tilength; i++) {
			jsonns::earthstationinfo curgnd = j["earth_stations"][i];
			ti[i] = curgnd;

            // All eight values
            uint32_t gid = parse_positive_int64(curgnd.id);
            std::string name = curgnd.name;
            double latitude = parse_double(curgnd.latitude);
            double longitude = parse_double(curgnd.longitude);
            double elevation = parse_double(curgnd.altitude);

            double cartesian_x;
            double cartesian_y;
            double cartesian_z;
            cppmap3d::geodetic2ecef(latitude*pi/180,longitude*pi/180,elevation,cartesian_x,cartesian_y,cartesian_z,cppmap3d::Ellipsoid::WGS72);
            Vector cartesian_position(cartesian_x, cartesian_y, cartesian_z);

            // Create ground station data holder
    	    Ptr<GroundStation> gs = CreateObject<GroundStation>(
    			gid, name, gsSleepTime, latitude, longitude, elevation, cartesian_position);
            m_groundStations.push_back(gs);
            m_groundStationsReal.push_back(gs);

            // Create the node
            // enable distributd simulation need extra system id
			if (m_enable_distributed == false){
				m_groundStationNodes.Add(CreateObject<Node>());
			}
			else{
				uint32_t sid = distributed_node_system_id_assignment[gid+m_satelliteNodes.GetN()];
				Ptr<Node> node = CreateObject<Node>(sid);
				//Ptr<Node> node = CreateObject<Node>(0);
				//curCon.Add(node);
				m_groundStationNodes.Add(node);
				if (m_system_id == sid){
					m_nodesGsCurSystem.Add(node);
					//std::cout<<"System Id "<<m_system_id<<" create Satellite Node "<<counter<<std::endl;
				}
				else{
					m_nodesGsCurSystemVirtual.Add(node);
				}
			}


            if (m_groundStationNodes.GetN() != gid + 1) {
                throw std::runtime_error("GID is not incremented each line");
            }
            gs->m_groundStationNodeP = m_groundStationNodes.Get(gid);

            // Install the constant mobility model on the node
            MobilityHelper mobility;
            mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
            mobility.Install(m_groundStationNodes.Get(gid));
            Ptr<MobilityModel> mobilityModel = m_groundStationNodes.Get(gid)->GetObject<MobilityModel>();
            mobilityModel->SetPosition(cartesian_position);

        }

		config_file.close();


		if (m_enable_distributed == false){
			m_nodesGsCurSystem = m_groundStationNodes;
		}

        // todo: whether to set all gnds, or set accessed gnds
        for(auto cons: m_constellations){
        	cons->SetGroundStationNodes(m_groundStationNodes);
        }
        m_basicSimulation->RegisterTimestamp("Create ground station objects");
    }

    void TopologySatelliteNetwork::ReadAirCrafts(){

		uint32_t gsSleepTime = 0;
		// Open file
		json j;
		std::ifstream config_file;
		config_file.open(m_satellite_network_dir + "/air_crafts.json");
		NS_ABORT_MSG_UNLESS(config_file.is_open(), "File air_crafts.json could not be opened");
		config_file >> j;
		int tilength = j["air_crafts"].size();
		jsonns::earthstationinfo ti[tilength];
		int64_t totalGroundStations = int64_t(m_groundStations.size());
		for (int i = 0; i < tilength; i++) {
			jsonns::earthstationinfo curgnd = j["air_crafts"][i];
			ti[i] = curgnd;

			// All eight values
			uint32_t gid = totalGroundStations + parse_positive_int64(curgnd.id);
			std::string name = curgnd.name;
			double latitude = parse_double(curgnd.latitude);
			double longitude = parse_double(curgnd.longitude);
			double elevation = parse_double(curgnd.altitude);

			double cartesian_x;
			double cartesian_y;
			double cartesian_z;
			cppmap3d::geodetic2ecef(latitude*pi/180,longitude*pi/180,elevation,cartesian_x,cartesian_y,cartesian_z,cppmap3d::Ellipsoid::WGS72);
			Vector cartesian_position(cartesian_x, cartesian_y, cartesian_z);

			// Create air craft data holder
			Ptr<GroundStation> gs = CreateObject<GroundStation>(
				gid, name, gsSleepTime, latitude, longitude, elevation, cartesian_position);
			m_groundStations.push_back(gs);
			m_airCraftsReal.push_back(gs);

			// Create the node
			// enable distributd simulation need extra system id
			if (m_enable_distributed == false){
				m_groundStationNodes.Add(CreateObject<Node>());
			}
			else{
				uint32_t sid = distributed_node_system_id_assignment[gid+m_satelliteNodes.GetN()];
				Ptr<Node> node = CreateObject<Node>(sid);
				//Ptr<Node> node = CreateObject<Node>(0);
				//curCon.Add(node);
				m_groundStationNodes.Add(node);
				if (m_system_id == sid){
					m_nodesGsCurSystem.Add(node);
					//std::cout<<"System Id "<<m_system_id<<" create Satellite Node "<<counter<<std::endl;
				}
				else{
					m_nodesGsCurSystemVirtual.Add(node);
				}
			}


			if (m_groundStationNodes.GetN() != gid + 1) {
				throw std::runtime_error("GID is not incremented each line");
			}
			gs->m_groundStationNodeP = m_groundStationNodes.Get(gid);

			// Install the constant mobility model on the node
			MobilityHelper mobility;
			mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
			mobility.Install(m_groundStationNodes.Get(gid));
			Ptr<MobilityModel> mobilityModel = m_groundStationNodes.Get(gid)->GetObject<MobilityModel>();
			mobilityModel->SetPosition(cartesian_position);

		}

		config_file.close();

		std::vector<Vector> temptemp = {};
		std::vector<std::vector<Vector>> temp(tilength, temptemp);
		m_airCraftsPositions = temp;

		if (m_enable_distributed == false){
			m_nodesGsCurSystem = m_groundStationNodes;
		}

		// todo: whether to set all gnds, or set accessed gnds
		for(auto cons: m_constellations){
			cons->SetGroundStationNodes(m_groundStationNodes);
		}
		m_basicSimulation->RegisterTimestamp("Create air craft objects");

		MakeAirCraftFlyingEvent(0, 20);


    }

    void
	TopologySatelliteNetwork::MakeAirCraftFlyingEvent(double time,  double speed){
    	std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<double> directionDist(0, 2 * M_PI);

		for(uint32_t i = 0; i < m_airCraftsReal.size(); i++){
			auto ac = m_airCraftsReal[i];
			double latitude = ac->GetLatitude();
			double longitude = ac->GetLongitude();
			double elevation = ac->GetElevation();

			// 生成随机方向（角度）
			double direction = directionDist(gen);
			// 计算经纬度的增量
			double distance = speed * m_dynamicStateUpdateIntervalNs/1e9;
			double latDelta = (distance / getLatitudeDistance()) * sin(direction);
			double lonDelta = (distance / getLongitudeDistance(latitude)) * cos(direction);
			// 更新经纬度
			// 更新经度
			longitude += lonDelta;
			if (longitude > 180.0) {
			    longitude -= 360.0;
			} else if (longitude < -180.0) {
			    longitude += 360.0;
			}

			// 更新纬度
			latitude += latDelta;
			if (latitude > 90.0) {
			    latitude = 90.0 - (latitude - 90.0);
			} else if (latitude < -90.0) {
			    latitude = -90.0 - (latitude + 90.0);
			}

			ac->SetLatitude(latitude);
			ac->SetLongitude(longitude);

			double cartesian_x;
			double cartesian_y;
			double cartesian_z;
			cppmap3d::geodetic2ecef(latitude*pi/180,longitude*pi/180,elevation,cartesian_x,cartesian_y,cartesian_z,cppmap3d::Ellipsoid::WGS72);
			Vector cartesian_position(cartesian_x, cartesian_y, cartesian_z);
			Vector lla_position(longitude, latitude, elevation);

			Ptr<MobilityModel> mobilityModel = ac->m_groundStationNodeP->GetObject<MobilityModel>();
			mobilityModel->SetPosition(cartesian_position);
			ac->SetCartesianPosition(cartesian_position);

			m_airCraftsPositions[i].push_back(lla_position);
		}


    	// schedule next event
    	int64_t next_update_ns = time + m_dynamicStateUpdateIntervalNs;
		if (next_update_ns < m_basicSimulation->GetSimulationEndTimeNs()) {
			Simulator::Schedule(NanoSeconds(m_dynamicStateUpdateIntervalNs),
					&TopologySatelliteNetwork::MakeAirCraftFlyingEvent, this, next_update_ns, speed);
		}

    }

    void
	TopologySatelliteNetwork::ReadTerminals(){

		uint32_t gsSleepTime = 0;
		// Open file
		json j;
		std::ifstream config_file;
		config_file.open(m_satellite_network_dir + "/terminals.json");
		NS_ABORT_MSG_UNLESS(config_file.is_open(), "File terminals.json could not be opened");
		config_file >> j;
		int tilength = j["terminals"].size();
		jsonns::earthstationinfo ti[tilength];
		int64_t totalGroundStations = int64_t(m_groundStations.size());
		for (int i = 0; i < tilength; i++) {
			jsonns::earthstationinfo curgnd = j["terminals"][i];
			ti[i] = curgnd;

			// All eight values
			uint32_t gid = totalGroundStations + parse_positive_int64(curgnd.id);
			std::string name = curgnd.name;
			double latitude = parse_double(curgnd.latitude);
			double longitude = parse_double(curgnd.longitude);
			double elevation = parse_double(curgnd.altitude);

			double cartesian_x;
			double cartesian_y;
			double cartesian_z;
			cppmap3d::geodetic2ecef(latitude*pi/180,longitude*pi/180,elevation,cartesian_x,cartesian_y,cartesian_z,cppmap3d::Ellipsoid::WGS72);
			Vector cartesian_position(cartesian_x, cartesian_y, cartesian_z);

			// Create air craft data holder
			Ptr<GroundStation> gs = CreateObject<GroundStation>(
				gid, name, gsSleepTime, latitude, longitude, elevation, cartesian_position);
			m_groundStations.push_back(gs);
			m_terminalsReal.push_back(gs);

			// Create the node
			// enable distributd simulation need extra system id
			if (m_enable_distributed == false){
				m_groundStationNodes.Add(CreateObject<Node>());
			}
			else{
				uint32_t sid = distributed_node_system_id_assignment[gid+m_satelliteNodes.GetN()];
				Ptr<Node> node = CreateObject<Node>(sid);
				//Ptr<Node> node = CreateObject<Node>(0);
				//curCon.Add(node);
				m_groundStationNodes.Add(node);
				if (m_system_id == sid){
					m_nodesGsCurSystem.Add(node);
					//std::cout<<"System Id "<<m_system_id<<" create Satellite Node "<<counter<<std::endl;
				}
				else{
					m_nodesGsCurSystemVirtual.Add(node);
				}
			}


			if (m_groundStationNodes.GetN() != gid + 1) {
				throw std::runtime_error("GID is not incremented each line");
			}
			gs->m_groundStationNodeP = m_groundStationNodes.Get(gid);

			// Install the constant mobility model on the node
			MobilityHelper mobility;
			mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
			mobility.Install(m_groundStationNodes.Get(gid));
			Ptr<MobilityModel> mobilityModel = m_groundStationNodes.Get(gid)->GetObject<MobilityModel>();
			mobilityModel->SetPosition(cartesian_position);

		}

		config_file.close();

		if (m_enable_distributed == false){
			m_nodesGsCurSystem = m_groundStationNodes;
		}

		// todo: whether to set all gnds, or set accessed gnds
		for(auto cons: m_constellations){
			cons->SetGroundStationNodes(m_groundStationNodes);
		}
		m_basicSimulation->RegisterTimestamp("Create terminal objects");

    }

    void
    TopologySatelliteNetwork::InstallInternetStacks() {

//    	uint64_t controlPacketProcessDelay_ms = parse_positive_int64(m_basicSimulation->GetConfigParamOrFail("control_packet_process_delay_ms"));
//    	ipv4RoutingHelper.Set("ControlPacketProcessDelay", TimeValue (MilliSeconds (controlPacketProcessDelay_ms)));
//    	bool enableRouterCalculateTime = parse_boolean(m_basicSimulation->GetConfigParamOrDefault("enable_router_calculate_time", "true"));
//    	ipv4RoutingHelper.Set("EnableRtrCalculateTime", BooleanValue (enableRouterCalculateTime));
//    	uint32_t numSats = this->GetNumSatellites();
//    	ipv4RoutingHelper.Set("SatellitesNumber", UintegerValue(numSats));
//    	uint32_t numGnds = this->GetNumGroundStations();
//    	ipv4RoutingHelper.Set("GroundStationNumber", UintegerValue(numGnds));
//    	ipv4RoutingHelper.Set("ConstellationTopology", PointerValue(m_constellations[0]));
//
//    	m_ipv4RoutingHelper = ipv4RoutingHelper;
//
//    	InternetStackHelper internet;
//        internet.SetRoutingHelper(ipv4RoutingHelper);
////        internet.SetSAGTransportLayerType(m_basicSimulation->GetConfigParamOrDefault("transport_layer_protocal", "ns3::SAGTransportLayer"));
//        internet.Install(m_allNodes);


		Ipv4ListRoutingHelper list;  // satellite
		Ipv4ListRoutingHelper list_gs;  // ground station

		if(m_isIPv4Networking){
			for(auto ipv4RoutingHelper : *m_ipv4RoutingHelpers){

				//std::cout<<ipv4RoutingHelper.first.GetObjectFactoryTypeId()<<std::endl;
				if(ipv4RoutingHelper.first.GetObjectFactoryTypeId() != TypeId::LookupByName ("ns3::aodv::RoutingProtocol")
						 && ipv4RoutingHelper.first.GetObjectFactoryTypeId() != TypeId::LookupByName ("ns3::BgpRouting")){
					uint64_t controlPacketProcessDelay_ms = parse_positive_int64(m_basicSimulation->GetConfigParamOrDefault("control_packet_process_delay_ms", "10"));
					ipv4RoutingHelper.first.Set("ControlPacketProcessDelay", TimeValue (MilliSeconds (controlPacketProcessDelay_ms)));
					bool enableRouterCalculateTime = parse_boolean(m_basicSimulation->GetConfigParamOrDefault("enable_router_calculate_time", "false"));
					ipv4RoutingHelper.first.Set("EnableRtrCalculateTime", BooleanValue (enableRouterCalculateTime));
					uint32_t numSats = this->GetNumSatellites();
					ipv4RoutingHelper.first.Set("SatellitesNumber", UintegerValue(numSats));
					uint32_t numGnds = this->GetNumGroundStations();
					ipv4RoutingHelper.first.Set("GroundStationNumber", UintegerValue(numGnds));
					// todo: change m_constellations[0]
					ipv4RoutingHelper.first.Set("ConstellationTopology", PointerValue(m_constellations[0]));
					ipv4RoutingHelper.first.Set("BaseDir", StringValue (m_basicSimulation->GetRunDir()));
				}


				if(ipv4RoutingHelper.first.GetObjectNameString() == "satellite"){
					list.Add(ipv4RoutingHelper.first, ipv4RoutingHelper.second);
				}
				else if(ipv4RoutingHelper.first.GetObjectNameString() == "ground"){
					list_gs.Add(ipv4RoutingHelper.first, ipv4RoutingHelper.second);
				}
				else if(ipv4RoutingHelper.first.GetObjectNameString() == "all"){
					list.Add(ipv4RoutingHelper.first, ipv4RoutingHelper.second);
					list_gs.Add(ipv4RoutingHelper.first, ipv4RoutingHelper.second);
				}
				else{
					throw std::runtime_error ("Wrong object name for installation: "+ipv4RoutingHelper.first.GetObjectNameString()+".");
				}

			}
		}

		//ipv6 router
		Ipv6ListRoutingHelper v6list;  // satellite
		Ipv6ListRoutingHelper v6list_gs;  // ground station

		if(m_isIPv6Networking){
			for(auto ipv6RoutingHelper : *m_ipv6RoutingHelpers){

				//std::cout<<ipv6RoutingHelper.first.GetObjectFactoryTypeId()<<std::endl;
				// if(ipv6RoutingHelper.first.GetObjectFactoryTypeId() != TypeId::LookupByName ("ns3::aodv::RoutingProtocol")){
				// 		// && ipv6RoutingHelper.first.GetObjectFactoryTypeId() != TypeId::LookupByName ("ns3::BgpRouting")){
				// 	uint64_t controlPacketProcessDelay_ms = parse_positive_int64(m_basicSimulation->GetConfigParamOrFail("control_packet_process_delay_ms"));
				// 	ipv6RoutingHelper.first.Set("ControlPacketProcessDelay", TimeValue (MilliSeconds (controlPacketProcessDelay_ms)));
				// 	bool enableRouterCalculateTime = parse_boolean(m_basicSimulation->GetConfigParamOrDefault("enable_router_calculate_time", "true"));
				// 	ipv6RoutingHelper.first.Set("EnableRtrCalculateTime", BooleanValue (enableRouterCalculateTime));
				// 	uint32_t numSats = this->GetNumSatellites();
				// 	ipv6RoutingHelper.first.Set("SatellitesNumber", UintegerValue(numSats));
				// 	uint32_t numGnds = this->GetNumGroundStations();
				// 	ipv6RoutingHelper.first.Set("GroundStationNumber", UintegerValue(numGnds));
				// 	// todo: change m_constellations[0]
				// 	ipv6RoutingHelper.first.Set("ConstellationTopology", PointerValue(m_constellations[0]));
				// }


				if(ipv6RoutingHelper.first.GetObjectNameString() == "satellite"){
					v6list.Add(ipv6RoutingHelper.first, ipv6RoutingHelper.second);
				}
				else if(ipv6RoutingHelper.first.GetObjectNameString() == "ground"){
					v6list_gs.Add(ipv6RoutingHelper.first, ipv6RoutingHelper.second);
				}
				else if(ipv6RoutingHelper.first.GetObjectNameString() == "all"){
					v6list.Add(ipv6RoutingHelper.first, ipv6RoutingHelper.second);
					v6list_gs.Add(ipv6RoutingHelper.first, ipv6RoutingHelper.second);
				}
				else{
					throw std::runtime_error ("Wrong object name for installation: "+ipv6RoutingHelper.first.GetObjectNameString()+".");
				}

			}
		}

    	if(m_enable_distributed){
/*
				//ScpsTpHelper
				ScpsTpHelper scpstp;*/
    		// InternetStackHelper
    		QuicHelper internet1;
			internet1.SetRoutingHelper(list);
			internet1.SetRoutingHelper(v6list);
			//internet1.SetSAGTransportLayerType(m_basicSimulation->GetConfigParamOrDefault("transport_layer_protocal", "ns3::SAGTransportLayer"));
			internet1.InstallQuic(m_nodesCurSystem);
			/*
			//Install ScpsTp
			scpstp.InstallScpsTp(m_nodesCurSystem);*/

			QuicHelper internet2;
			internet2.InstallQuic(m_nodesCurSystemVirtual);
			//scpstp.InstallScpsTp(m_nodesCurSystemVirtual);

			if(m_groundStationNodes.GetN () == 0){
				return;
			}

			QuicHelper internet3;
			internet3.SetRoutingHelper(list_gs);
			internet3.SetRoutingHelper(v6list_gs);
			//internet2.SetRoutingHelper(list);
			//internet2.SetSAGTransportLayerType(m_basicSimulation->GetConfigParamOrDefault("transport_layer_protocal", "ns3::SAGTransportLayer"));
			internet3.InstallQuic(m_nodesGsCurSystem);
			internet2.InstallQuic(m_nodesGsCurSystemVirtual);
			/*
			scpstp.InstallScpsTp(m_nodesGsCurSystem);
			scpstp.InstallScpsTp(m_nodesGsCurSystemVirtual);*/

//		  if(m_system_id == 0){
//			std::stringstream stream;
//			Ptr<OutputStreamWrapper> routingstream = Create<OutputStreamWrapper> (&std::cout);
//			//	x.PrintRoutingTableAt (Seconds(0.0), node1, routingstream);
//			//	x.PrintRoutingTableAt (Seconds(0.04), node1, routingstream);
//			//	x.PrintRoutingTableAt (Seconds(0.05), node1, routingstream);
//			//	x.PrintRoutingTableAt (Seconds(0.06), node1, routingstream);
//			list.PrintRoutingTableAt (Seconds(4.0), m_nodesCurSystem.Get(1), routingstream);
//		  }
    	}
    	else{
    		QuicHelper internet1;
				/*
				//ScpsTpHelper
				ScpsTpHelper scpstp;*/
			internet1.SetRoutingHelper(list);
			internet1.SetRoutingHelper(v6list);
			//internet1.SetSAGTransportLayerType(m_basicSimulation->GetConfigParamOrDefault("transport_layer_protocal", "ns3::SAGTransportLayer"));
			internet1.InstallQuic(m_satelliteNodes);
			//scpstp.InstallScpsTp(m_satelliteNodes);


			if(m_groundStationNodes.GetN () == 0){
				return;
			}

			QuicHelper internet2;
			internet2.SetRoutingHelper(list_gs);
			internet2.SetRoutingHelper(v6list_gs);
			//internet2.SetRoutingHelper(list);
			//internet2.SetSAGTransportLayerType(m_basicSimulation->GetConfigParamOrDefault("transport_layer_protocal", "ns3::SAGTransportLayer"));
			internet2.InstallQuic(m_groundStationNodes);
			//scpstp.InstallScpsTp(m_groundStationNodes);
    	}

    	m_basicSimulation->RegisterTimestamp("Install Internet stacks");
    }

    void
    TopologySatelliteNetwork::ReadISLs()
    {


        // Link helper
        PointToPointLaserHelper p2p_laser_helper(m_basicSimulation);
        std::string max_queue_size_str = format_string("%" PRId64 "p", m_isl_max_queue_size_pkts);
        p2p_laser_helper.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", QueueSizeValue(QueueSize(max_queue_size_str)));
        p2p_laser_helper.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (std::to_string(m_isl_data_rate_megabit_per_s) + "Mbps")));
        std::cout << "    >> ISL data rate........ " << m_isl_data_rate_megabit_per_s << " Mbit/s" << std::endl;
        std::cout << "    >> ISL max queue size... " << m_isl_max_queue_size_pkts << " packets" << std::endl;

        // Traffic control helper
        TrafficControlHelper tch_isl;
        tch_isl.SetRootQueueDisc("ns3::FifoQueueDisc", "MaxSize", QueueSizeValue(QueueSize("1p"))); // Will be removed later any case



        //GenerateISLs();
        // Open file
		int counter = 0;
        for(auto cons : m_constellations){
			std::string name = cons->GetName();
			std::ifstream fs;
			//fs.open(m_satellite_network_dir + "/" + name + "_adjacency.txt");
			fs.open(m_satellite_network_dir + "/system_" + std::to_string(m_system_id) + "_" + name +"_adjacency.txt");
			NS_ABORT_MSG_UNLESS(fs.is_open(), name + "_adjacency.txt could not be opened");

			std::unordered_map<uint32_t, std::vector<uint32_t>> adjacency;
			NetDeviceContainer islNetDevices;
			std::vector<std::pair<uint32_t, uint32_t>> islFromTo;
			std::vector<std::pair<uint32_t, uint32_t>> islFromToUnique;



			// Read ISL pair from each line
			std::string line;
			while (std::getline(fs, line)) {

				std::vector<std::string> res = split_string(line, " ", 2);

				// Retrieve satellite identifiers
				uint32_t sat0_id = parse_positive_int64(res.at(0));
				uint32_t sat1_id = parse_positive_int64(res.at(1));
				Ptr<Satellite> sat0 = m_satellites.at(sat0_id);
				Ptr<Satellite> sat1 = m_satellites.at(sat1_id);

				// Create Netdevice and others
				DoCreateNetDevice(sat0_id, sat1_id, p2p_laser_helper, tch_isl, adjacency, islNetDevices, islFromTo, islFromToUnique);

				counter += 1;
			}
			fs.close();

			cons->SetAdjacency(adjacency);
			cons->SetIslNetDevicesInfo(islNetDevices);
			cons->SetIslFromTo(islFromTo);
			cons->SetIslFromToUnique(islFromToUnique);
        }

        // just for Star type walker constellation
        MakeRegularISLChangeEvent(0);

        //EnablePcapAll(p2p_laser_helper);
        // Completed
        std::cout << "    >> Created " << std::to_string(counter) << " ISL(s)" << std::endl;

        m_basicSimulation->RegisterTimestamp("Create inter-satellite links");

    }

//    void
//	TopologySatelliteNetwork::EnablePcapAll(PointToPointLaserHelper p2p_laser_helper){
//    	remove_dir_and_subfile_if_exists(m_basicSimulation->GetRunDir() + "/pcap/system_" + std::to_string(m_system_id)+"_pcap");
//    	if(!parse_boolean(m_basicSimulation->GetConfigParamOrDefault("enable_pcap_tracing", "false"))){
//    		return;
//    	}
//    	mkdir_force_if_not_exists(m_basicSimulation->GetRunDir() + "/pcap/system_" + std::to_string(m_system_id)+"_pcap");
//    	if(parse_boolean(m_basicSimulation->GetConfigParamOrDefault("wireshark_target_device_all", "false"))){
//    		p2p_laser_helper.EnablePcapAll(m_basicSimulation->GetRunDir() + "/pcap/system_" + std::to_string(m_system_id)+"_pcap/sag");
//    	}
//    	else{
//    		//<! tracing.json
//			std::string filename = m_basicSimulation->GetRunDir() + "/config_analysis/tracing.json";
//
//			ifstream jfile(filename);
//			if (jfile) {
//				json j;
//				jfile >> j;
//				//jsonns::wireshark vj = j.at("wireshark");
//				int tilength = j["wireshark"]["target_device"].size();
//				jsonns::target_device_info target_device[tilength];
//
//			    for (int i = 0; i < tilength; i++) {
//			    	target_device[i] = j["wireshark"]["target_device"][i];
//			    	std::string pair = target_device[i].pairs;
//			    	std::vector<std::string> res = split_string(pair, ",", 2);
//
//					int nodeid = stoi(res[0]);
//					int netdeviceid = stoi(res[1]);
//
////					if(nodeid >= m_satelliteNodes.GetN()){
////						continue;
////					}
//
//		    		p2p_laser_helper.EnablePcap(
//		    				m_basicSimulation->GetRunDir() + "/pcap/system_" + std::to_string(m_system_id)+"_pcap/sag",
//		    				nodeid,
//							netdeviceid);
//			    }
//
//				jfile.close();
//
//			}
//			else{
//				throw std::runtime_error(format_string("File %s could not be read.", filename.c_str()));
//			}
//
//		}
//
//    }

    void
	TopologySatelliteNetwork::EnablePcap(SAGPhysicalGSLHelper gsl_helper){

    	//remove_dir_and_subfile_if_exists(m_basicSimulation->GetRunDir() + "/pcap/system_" + std::to_string(m_system_id)+"_pcap");
		if(!parse_boolean(m_basicSimulation->GetConfigParamOrDefault("enable_pcap_tracing", "false"))){
			return;
		}
		//mkdir_force_if_not_exists(m_basicSimulation->GetRunDir() + "/pcap/system_" + std::to_string(m_system_id)+"_pcap");
		if(parse_boolean(m_basicSimulation->GetConfigParamOrDefault("wireshark_target_device_all", "false"))){
			if(!m_enable_distributed){
				gsl_helper.EnablePcapAll(m_basicSimulation->GetRunDir() + "/pcap/system_" + std::to_string(m_system_id)+"_pcap/sag");
			}
			else{
				gsl_helper.EnablePcap(
						m_basicSimulation->GetRunDir() + "/results/network_results/global_statistics/pcap_for_wireshark/sag",
						m_nodesCurSystem);
				gsl_helper.EnablePcap(
						m_basicSimulation->GetRunDir() + "/results/network_results/global_statistics/pcap_for_wireshark/sag",
						m_nodesGsCurSystem);
			}
		}
		else{
			//<! tracing.json
			std::string filename = m_basicSimulation->GetRunDir() + "/config_analysis/tracing.json";

			ifstream jfile(filename);
			if (jfile) {
				json j;
				jfile >> j;
				//jsonns::wireshark vj = j.at("wireshark");
				int tilength = j["wireshark"]["wireshark_target_device"].size();
				jsonns::target_device_info target_device[tilength];

				for (int i = 0; i < tilength; i++) {
					target_device[i] = j["wireshark"]["wireshark_target_device"][i];
					std::string pair = target_device[i].pairs;
					std::vector<std::string> res = split_string(pair, ",", 2);

					int nodeid = stoi(res[0]);
					int netdeviceid = stoi(res[1]);

					if(m_enable_distributed && m_system_id != distributed_node_system_id_assignment[nodeid]){
						continue;
					}
					gsl_helper.EnablePcap(
							m_basicSimulation->GetRunDir() + "/results/network_results/global_statistics/pcap_for_wireshark/sag",
							nodeid,
							netdeviceid);
				}

				jfile.close();

			}
			else{
				throw std::runtime_error(format_string("File %s could not be read.", filename.c_str()));
			}

		}






    }

    void
	TopologySatelliteNetwork::EnableUtilization(){
		if(!m_enable_link_utilization_tracking){
			return;
		}
		if(parse_boolean(m_basicSimulation->GetConfigParamOrDefault("target_device_all", "false"))){
			EnableUtilizationAll();
		}
		else{
			//<! tracing.json
			std::string filename = m_basicSimulation->GetRunDir() + "/config_analysis/tracing.json";

			ifstream jfile(filename);
			if (jfile) {
				json j;
				jfile >> j;
				//jsonns::wireshark vj = j.at("wireshark");
				int tilength = j["utilization"]["target_device"].size();
				jsonns::target_device_info target_device[tilength];

				for (int i = 0; i < tilength; i++) {
					target_device[i] = j["utilization"]["target_device"][i];
					std::string pair = target_device[i].pairs;
					std::vector<std::string> res = split_string(pair, ",", 2);

					int nodeid = stoi(res[0]);
					int netdeviceid = stoi(res[1]);

					EnableUtilization(nodeid, netdeviceid);
				}

				jfile.close();

			}
			else{
				throw std::runtime_error(format_string("File %s could not be read.", filename.c_str()));
			}

		}
    }

    void
	TopologySatelliteNetwork::EnableUtilizationAll()
    {
		NodeContainer n = NodeContainer::GetGlobal ();
		for (NodeContainer::Iterator i = n.Begin (); i != n.End (); ++i)
		{
			Ptr<Node> node = *i;
			if (m_enable_distributed && m_system_id != distributed_node_system_id_assignment[node->GetId()]){
				continue;
			}
			for (uint32_t j = 1; j < node->GetNDevices (); ++j)
			{
				Ptr<ns3::NetDevice> nd = node->GetDevice (j);
				if(nd->GetObject<SAGLinkLayer>() != nullptr){
					nd->GetObject<SAGLinkLayer>()->EnableUtilizationTracking(m_link_utilization_tracking_interval_ns);
				}
				else if(nd->GetObject<SAGLinkLayerGSL>() != nullptr){
					nd->GetObject<SAGLinkLayerGSL>()->EnableUtilizationTracking(m_link_utilization_tracking_interval_ns);
				}
				else{
					throw std::runtime_error ("Wrong network device set for utilization.");
				}
			}
		}
    }

    void
	TopologySatelliteNetwork::EnableUtilization(uint32_t nodeid, uint32_t deviceid)
	{
		NodeContainer n = NodeContainer::GetGlobal ();

		for (NodeContainer::Iterator i = n.Begin (); i != n.End (); ++i)
		{
			Ptr<Node> node = *i;
			if (node->GetId () != nodeid){
			  continue;
			}
			if (m_enable_distributed && m_system_id != distributed_node_system_id_assignment[node->GetId()]){
				continue;
			}

			NS_ABORT_MSG_IF (deviceid >= node->GetNDevices (), "PcapHelperForDevice::EnablePcap(): Unknown deviceid = "
						   << deviceid);
			Ptr<NetDevice> nd = node->GetDevice (deviceid);
			if(nd->GetObject<SAGLinkLayer>() != nullptr){
				nd->GetObject<SAGLinkLayer>()->EnableUtilizationTracking(m_link_utilization_tracking_interval_ns);
			}
			else if(nd->GetObject<SAGLinkLayerGSL>() != nullptr){
				nd->GetObject<SAGLinkLayerGSL>()->EnableUtilizationTracking(m_link_utilization_tracking_interval_ns);
			}
			else{
				throw std::runtime_error ("Wrong network device set for utilization.");
			}
			return;
		}
	}



    void 
    TopologySatelliteNetwork::DoCreateNetDevice(
    		uint32_t satId0,
    		uint32_t satId1,
    		PointToPointLaserHelper& p2pLinkHelper,
			TrafficControlHelper& tcHelper,
    		std::unordered_map<uint32_t, std::vector<uint32_t>>& adjacency,
			NetDeviceContainer& islNetDevices,
			std::vector<std::pair<uint32_t, uint32_t>>& islFromTo,
			std::vector<std::pair<uint32_t, uint32_t>>& islFromToUnique){

    	adjacency[satId0].push_back(satId1);
    	adjacency[satId1].push_back(satId0);
    	// Install a p2p laser link between these two satellites
        NodeContainer c;
//        c.Add(m_satelliteNodes.Get(satId0));
//        c.Add(m_satelliteNodes.Get(satId1));
		c.Add(m_allNodes.Get(satId0));
		c.Add(m_allNodes.Get(satId1));
        NetDeviceContainer netDevices = p2pLinkHelper.Install(c);


        //std::cout<<c.Get(0)->GetId()<<"  "<<c.Get(0)->GetId()<<std::endl;

        // Install traffic control helper
        tcHelper.Install(netDevices.Get(0));
        tcHelper.Install(netDevices.Get(1));

        // Assign some IP address (nothing smart, no aggregation, just some IP address)
        m_networkAddressingMethod->AssignInterSatelliteInterfaceAddress(netDevices);
		m_networkAddressingMethodIPv6->AssignInterSatelliteInterfaceAddress(netDevices);

        // Remove the traffic control layer (must be done here, else the Ipv4 helper will assign a default one)
        TrafficControlHelper tch_uninstaller;
        tch_uninstaller.Uninstall(netDevices.Get(0));
        tch_uninstaller.Uninstall(netDevices.Get(1));

			// Install traffic control layer on islNetdevices with ECN enabled
			TrafficControlHelper tch_isl_ecn;
			Config::SetDefault ("ns3::RedQueueDisc::MaxSize",
				QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS, m_isl_max_queue_size_pkts)));
			tch_isl_ecn.SetRootQueueDisc("ns3::RedQueueDisc","UseEcn", BooleanValue(true));
      tch_isl_ecn.Install(netDevices.Get(0));
			tch_isl_ecn.Install(netDevices.Get(1));

//        // Utilization tracking
//        if (m_enable_link_utilization_tracking) {
//            netDevices.Get(0)->GetObject<SAGLinkLayer>()->EnableUtilizationTracking(m_link_utilization_tracking_interval_ns);
//            netDevices.Get(1)->GetObject<SAGLinkLayer>()->EnableUtilizationTracking(m_link_utilization_tracking_interval_ns);
//        }
		islNetDevices.AddWithKey(netDevices.Get(0), CalStringKey(satId0, satId1));
		islFromToUnique.push_back(std::make_pair(satId0, satId1));
		islFromTo.push_back(std::make_pair(satId0, satId1));
		islNetDevices.AddWithKey(netDevices.Get(1), CalStringKey(satId1, satId0));
		islFromTo.push_back(std::make_pair(satId1, satId0));

       
    }

    void
    TopologySatelliteNetwork::DoCreateNetDevice(
    		uint32_t satId0,
    		uint32_t satId1,
    		PointToPointLaserHelper& p2pLinkHelper,
			TrafficControlHelper& tcHelper){

    	Ptr<Constellation> cons1 = FindConstellationBySatId(satId0);
//		Ptr<Constellation> cons2 = FindConstellationBySatId(satId1);
//		if(cons1->GetName() != cons2->GetName()){
//			throw std::runtime_error ("No support to isl between different constellation now");
//		}
		NetDeviceContainer islNetDevices = cons1->GetIslNetDevicesInfo();
		std::vector<std::pair<uint32_t, uint32_t>> islFromTo = cons1->GetIslFromTo();
		std::vector<std::pair<uint32_t, uint32_t>> islFromToUnique = cons1->GetIslFromToUnique();

		cons1->AddAdjacencySatellite(std::make_pair(satId0, satId1));
//    	adjacency[satId0].push_back(satId1);
//    	adjacency[satId1].push_back(satId0);
    	// Install a p2p laser link between these two satellites
        NodeContainer c;
//        c.Add(m_satelliteNodes.Get(satId0));
//        c.Add(m_satelliteNodes.Get(satId1));
		c.Add(m_allNodes.Get(satId0));
		c.Add(m_allNodes.Get(satId1));
        NetDeviceContainer netDevices = p2pLinkHelper.Install(c);

        // Install traffic control helper
        tcHelper.Install(netDevices.Get(0));
        tcHelper.Install(netDevices.Get(1));

        // Assign some IP address (nothing smart, no aggregation, just some IP address)
        m_networkAddressingMethod->AssignInterSatelliteInterfaceAddress(netDevices);
		m_networkAddressingMethodIPv6->AssignInterSatelliteInterfaceAddress(netDevices);

        // Remove the traffic control layer (must be done here, else the Ipv4 helper will assign a default one)
        TrafficControlHelper tch_uninstaller;
        tch_uninstaller.Uninstall(netDevices.Get(0));
        tch_uninstaller.Uninstall(netDevices.Get(1));

			// Install traffic control layer on islNetdevices with ECN enabled
			TrafficControlHelper tch_isl_ecn;
			Config::SetDefault ("ns3::RedQueueDisc::MaxSize",
				QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS, m_isl_max_queue_size_pkts)));
				tch_isl_ecn.SetRootQueueDisc("ns3::RedQueueDisc","UseEcn", BooleanValue(true));
				tch_isl_ecn.Install(netDevices.Get(0));
				tch_isl_ecn.Install(netDevices.Get(1));
//        // Utilization tracking
//        if (m_enable_link_utilization_tracking) {
//            netDevices.Get(0)->GetObject<SAGLinkLayer>()->EnableUtilizationTracking(m_link_utilization_tracking_interval_ns);
//            netDevices.Get(1)->GetObject<SAGLinkLayer>()->EnableUtilizationTracking(m_link_utilization_tracking_interval_ns);
//        }
		islNetDevices.AddWithKey(netDevices.Get(0), CalStringKey(satId0, satId1));
		islFromToUnique.push_back(std::make_pair(satId0, satId1));
		islFromTo.push_back(std::make_pair(satId0, satId1));
		islNetDevices.AddWithKey(netDevices.Get(1), CalStringKey(satId1, satId0));
		islFromTo.push_back(std::make_pair(satId1, satId0));


    }

    void
    TopologySatelliteNetwork::MakeUnpredictableISLChangeEvent(double time ,Oper oper,uint32_t satId0, uint32_t satId1)
    {
        Time delay = Seconds (time);
        if(oper == OPER_ADD)
        {
            Simulator::Schedule(delay, &TopologySatelliteNetwork::AddISLBySatId, this, satId0, satId1, false);
        }
        else if(oper == OPER_DEL)
        {
            Simulator::Schedule(delay, &TopologySatelliteNetwork::DisableISLBySatId, this, satId0, satId1, false);

        }
        
    }

    void
	TopologySatelliteNetwork::MakeUnpredictableISLChangeEvent(double freqInterval, double recoverInterval){

    	srand(10000);

    	// todo: to be optimized
    	Ptr<Constellation> cons = m_constellations[0];
    	std::vector<std::pair<uint32_t, uint32_t>> islFromTo = cons->GetIslFromToUnique();

    	int a = 0;
    	int b = islFromTo.size() - 1;

    	double time = 0; // s
    	std::map<std::string, double> islRecoverTime;

    	int r = (rand() % (b - a + 1))+ a;
    	std::pair<uint32_t, uint32_t> ISL = islFromTo[r];

    	while(time * 1e9 < m_basicSimulation->GetSimulationEndTimeNs()){

    		for(auto iter = islRecoverTime.begin(); iter != islRecoverTime.end();){
    			if(iter->second <= time){
    				iter = islRecoverTime.erase(iter);
    			}
    			else{
    				iter++;
    			}
    		}

    		do{
            	r = (rand() % (b - a + 1))+ a;
            	ISL = islFromTo[r];
    		}while(islRecoverTime.find(CalStringKey(ISL.first, ISL.second)) != islRecoverTime.end());

    		MakeUnpredictableISLChangeEvent(time, OPER_DEL, ISL.first, ISL.second);

			MakeUnpredictableISLChangeEvent(time + recoverInterval, OPER_ADD, ISL.first, ISL.second);
			islRecoverTime[CalStringKey(ISL.first, ISL.second)] = time + recoverInterval;


    		time += freqInterval;
    	}

    }


    void
	TopologySatelliteNetwork::MakeRegularISLChangeEvent(double time){

    	// just for Star type walker constellation
    	if(m_islInterOrbit.size() == 0){
    		return;
    	}
    	for(uint32_t i = 0; i < m_islInterOrbit.size(); i++){
    		uint32_t satId0 = m_islInterOrbit.at(i).first;
    		uint32_t satId1 = m_islInterOrbit.at(i).second;
    		Ptr<Node> satNodeId0 = m_satelliteNodes.Get(satId0);
    		Ptr<Node> satNodeId1 = m_satelliteNodes.Get(satId1);
    		// need ECEF -> LLA todo
    		Vector satId0Position = satNodeId0->GetObject<MobilityModel>()->GetPosition ();
    		Vector satId1Position = satNodeId1->GetObject<MobilityModel>()->GetPosition ();
    		double latitudeSatId0 = atan2(satId0Position.z, sqrt(pow(satId0Position.x, 2) + pow(satId0Position.y, 2))) * 180 / pi;
    		double latitudeSatId1 = atan2(satId1Position.z, sqrt(pow(satId1Position.x, 2) + pow(satId1Position.y, 2))) * 180 / pi;
            // here just 66.5, waiting for modification of switch latitude
            if(std::abs(latitudeSatId0) > 60 || std::abs(latitudeSatId1) > 60){
            	DisableISLBySatId(satId0, satId1, true);
            	//if(m_islDisableNetDevices.find(CalStringKey(satId0, satId1)) == m_islDisableNetDevices.end()){
            	//	DisableISLBySatId(satId0, satId1);
            	//}
            	//else, this link has been interrupted
            	//std::cout<<"!!!!!!!!!!!!!!!!!!!!!!!!!"<<std::endl;
    		}
            else{
            	AddISLBySatId(satId0, satId1, true);
            	//if(m_islDisableNetDevices.find(CalStringKey(satId0, satId1)) != m_islDisableNetDevices.end()){
            	//	AddISLBySatId(satId0, satId1);
				//}
            }
    	}


    	int64_t next_update_ns = time + m_dynamicStateUpdateIntervalNs;
		if (next_update_ns < m_basicSimulation->GetSimulationEndTimeNs()) {
			Simulator::Schedule(NanoSeconds(m_dynamicStateUpdateIntervalNs),
					&TopologySatelliteNetwork::MakeRegularISLChangeEvent, this, next_update_ns);
		}
    }

    void
    TopologySatelliteNetwork::MakeLinkDelayUpdateEvent(double time){

    	uint64_t la = 10000000000000;
    	for(Ptr<Constellation> cons : m_constellations){
    		std::vector<std::pair<uint32_t, uint32_t>> islFromToUnique = cons->GetIslFromToUnique();
    		NetDeviceContainer islNetDevices = cons->GetIslNetDevicesInfo();
    		for(uint32_t i = 0; i < islFromToUnique.size(); i++){
				uint32_t satId0 = islFromToUnique.at(i).first;
				uint32_t satId1 = islFromToUnique.at(i).second;
				Ptr<Node> satNodeId0 = m_satelliteNodes.Get(satId0);
				Ptr<Node> satNodeId1 = m_satelliteNodes.Get(satId1);

				Ptr<MobilityModel> senderMobility = satNodeId0->GetObject<MobilityModel>();
				Ptr<MobilityModel> receiverMobility = satNodeId1->GetObject<MobilityModel>();

				double propagationSpeedMetersPerSecond = 299792458.0;
				double distance_m = senderMobility->GetDistanceFrom (receiverMobility);
				double seconds = distance_m / propagationSpeedMetersPerSecond;
				Time delay = Seconds (seconds);
				if(la > (uint64_t)delay.GetNanoSeconds()){
					la = (uint64_t)delay.GetNanoSeconds();
				}
				Ptr<SAGLinkLayer> dev = islNetDevices.GetWithKey(CalStringKey(satId0,satId1))->GetObject<SAGLinkLayer>();
				if(dev == nullptr){
					//std::cout<<islNetDevices.GetWithKey(CalStringKey(satId0,satId1))->GetMtu ()<<"  "<<satId1<<std::endl;
					throw std::runtime_error("TopologySatelliteNetwork::MakeLinkDelayUpdateEvent.");
				}
				Ptr<SAGPhysicalLayer> channel = dev->GetChannel()->GetObject<SAGPhysicalLayer>();
				channel->SetChannelDelay(delay);
			}
    	}

    	for(uint32_t i = 0; i < m_gslSatNetDevices.GetN(); i++){
    		Ptr<Node> sat = m_gslSatNetDevices.Get(i)->GetNode();
    		Ptr<SAGPhysicalLayerGSL> gsl_channel = m_gslSatNetDevices.Get(i)->GetChannel()->GetObject<SAGPhysicalLayerGSL>();

    		std::vector<Time> delays = {};

    		for(uint32_t j = 1; j < gsl_channel->GetNDevices(); j++){
    			Ptr<Node> gnd = gsl_channel->GetDevice(j)->GetNode();
    			Ptr<MobilityModel> senderMobility = sat->GetObject<MobilityModel>();
    			Ptr<MobilityModel> receiverMobility = gnd->GetObject<MobilityModel>();
    			double propagationSpeedMetersPerSecond = 299792458.0;
				double distance_m = senderMobility->GetDistanceFrom (receiverMobility);
				double seconds = distance_m / propagationSpeedMetersPerSecond;
				Time delay = Seconds (seconds);
				delays.push_back(delay);
				if(m_basicSimulation->GetNodeAssignmentAlogirthm() == "algorithm1" || m_basicSimulation->GetNodeAssignmentAlogirthm() == "customize" ){
					if(la > (uint64_t)delay.GetNanoSeconds()){
						la = (uint64_t)delay.GetNanoSeconds();
					}
				}

    		}
    		gsl_channel->SetChannelDelay(delays);
    	}

        std::ofstream file(m_satellite_network_dir + "/system_" + std::to_string(m_system_id)+"_mpi_lookahead.txt", std::ofstream::out);
        file<<la<<std::endl;
        file.close();

//    	if(m_system_id == 0 && parse_boolean(m_basicSimulation->GetConfigParamOrDefault("enable_trajectory_tracing", "false"))){
//			uint32_t t = uint32_t(Simulator::Now().GetSeconds());
//			for(uint32_t sat = 0; sat < m_satelliteNodes.GetN(); sat++){
//				Ptr<Node> satNode = m_satelliteNodes.Get(sat);
//				std::ofstream file(m_satellite_network_dir + "/system_" + std::to_string(m_system_id)+"_coordinate" + "/satellite_" + std::to_string(satNode->GetId()) +".txt", std::ofstream::out | std::ofstream::app);
//				Ptr<MobilityModel> nodeMobility = satNode->GetObject<MobilityModel>();
//				Vector pos = nodeMobility->GetPosition();
//				Vector vect = nodeMobility->GetVelocity();
//				double x = pos.x;
//				double y = pos.y;
//				double z = pos.z;
////    			double latitudeSatId0 = atan2(z, sqrt(pow(x, 2) + pow(y, 2))) * 180 / pi;
////    			double longitudeSatId0 = atan2(y, x) * 180 / pi;
//				double out_latitude, out_longitude, out_altitude;
//				cppmap3d::ecef2geodetic(x,y,z,out_latitude,out_longitude,out_altitude,cppmap3d::Ellipsoid::WGS72);
//				file<<t<<","<<pos.x<<","<<pos.y<<","<<pos.z<<","
//						<<out_latitude * 180 / pi<<","<<out_longitude * 180 / pi<<","<<out_altitude<<","
//						<<vect.x<<","<<vect.y<<","<<vect.z<<std::endl;
////    			file<<latitudeSatId0<<","<<longitudeSatId0<<std::endl;
//				file.close();
//
//				Ptr<SatellitePositionMobilityModel> satMobility = nodeMobility->GetObject<SatellitePositionMobilityModel>();
//				Ptr<Satellite> satellite_p = satMobility->GetSatellite();
//				OrbitalElementsRecord rec = satellite_p->GetOrbitalElements();
//				std::ofstream file_oe(m_satellite_network_dir + "/system_" + std::to_string(m_system_id)+"_orbital_elements"+ "/satellite_" + std::to_string(satNode->GetId()) +".txt", std::ofstream::out | std::ofstream::app);
//				file_oe<<t<<","
//						<<std::get<0>(rec)<<","
//						<<std::get<1>(rec)<<","
//						<<std::get<2>(rec)<<","
//						<<std::get<3>(rec)<<","
//						<<std::get<4>(rec)<<","
//						<<std::get<5>(rec)<<std::endl;
//				file_oe.close();
//			}
//    	}


    	if(parse_boolean(m_basicSimulation->GetConfigParamOrDefault("enable_trajectory_tracing", "false"))){
			uint32_t t = uint32_t(Simulator::Now().GetSeconds());
			for(uint32_t sat = 0; sat < m_nodesCurSystem.GetN(); sat++){
				Ptr<Node> satNode = m_nodesCurSystem.Get(sat);
				std::ofstream file(m_satellite_network_dir + "/system_"+ to_string(m_system_id) + "_coordinates" + "/satellite_" + std::to_string(satNode->GetId()) +".txt", std::ofstream::out | std::ofstream::app);
				Ptr<MobilityModel> nodeMobility = satNode->GetObject<MobilityModel>();
				Vector pos = nodeMobility->GetPosition();
				Vector vect = nodeMobility->GetVelocity();
				double x = pos.x;
				double y = pos.y;
				double z = pos.z;
//    			double latitudeSatId0 = atan2(z, sqrt(pow(x, 2) + pow(y, 2))) * 180 / pi;
//    			double longitudeSatId0 = atan2(y, x) * 180 / pi;
				double out_latitude, out_longitude, out_altitude;
				cppmap3d::ecef2geodetic(x,y,z,out_latitude,out_longitude,out_altitude,cppmap3d::Ellipsoid::WGS72);
				file<<t<<","<<pos.x<<","<<pos.y<<","<<pos.z<<","
						<<out_latitude * 180 / pi<<","<<out_longitude * 180 / pi<<","<<out_altitude<<","
						<<vect.x<<","<<vect.y<<","<<vect.z<<std::endl;
				file.close();


				Ptr<SatellitePositionMobilityModel> satMobility = nodeMobility->GetObject<SatellitePositionMobilityModel>();
				Ptr<Satellite> satellite_p = satMobility->GetSatellite();
				OrbitalElementsRecord rec = satellite_p->GetOrbitalElements();
//				std::ofstream file_oe(m_satellite_network_dir + "/system_" + std::to_string(m_system_id)+"_orbital_elements"+ "/satellite_" + std::to_string(satNode->GetId()) +".txt", std::ofstream::out | std::ofstream::app);
//				file_oe<<t<<","
//						<<std::get<0>(rec)<<","
//						<<std::get<1>(rec)<<","
//						<<std::get<2>(rec)<<","
//						<<std::get<3>(rec)<<","
//						<<std::get<4>(rec)<<","
//						<<std::get<5>(rec)<<std::endl;
//				file_oe.close();


				json jsonDatas;
				jsonDatas["Time(s)"] = t;
				jsonDatas["Satellite Name"] = satellite_p->GetName();
				jsonDatas["Satellite Number"] = satNode->GetId();
				jsonDatas["Latitude(deg)"] = out_latitude * 180 / pi;
				jsonDatas["Longitude(deg)"] = out_longitude * 180 / pi;
				jsonDatas["Altitude(m)"] = out_altitude;
				jsonDatas["Semi-Major Axis(m)"] = std::get<0>(rec);
				jsonDatas["Eccentricity"] = std::get<1>(rec);
				jsonDatas["Inclination(deg)"] = std::get<2>(rec);
				jsonDatas["RAAN(deg)"] = std::get<3>(rec);
				jsonDatas["Argument of Periapsis(deg)"] = std::get<4>(rec);
				jsonDatas["Mean Anomaly(deg)"] = std::get<5>(rec);

	    		if(m_satelliteElements.size() != m_nodesCurSystem.GetN()){
	    			json jsonDatasAll;
	    			jsonDatasAll.push_back(jsonDatas);
	    			m_satelliteElements.push_back(jsonDatasAll);
	    		}
	    		else{
	    			m_satelliteElements[sat].push_back(jsonDatas);
	    		}
			}
    	}



    	int64_t dynamicStateUpdateIntervalNsTemp;
    	if(time == 0) dynamicStateUpdateIntervalNsTemp = m_dynamicStateUpdateIntervalNs + 1;
    	else dynamicStateUpdateIntervalNsTemp = m_dynamicStateUpdateIntervalNs;
    	int64_t next_update_ns = time + dynamicStateUpdateIntervalNsTemp;
		if (next_update_ns < m_basicSimulation->GetSimulationEndTimeNs()) {
			Simulator::Schedule(NanoSeconds(dynamicStateUpdateIntervalNsTemp),
					&TopologySatelliteNetwork::MakeLinkDelayUpdateEvent, this, next_update_ns);
		}
		else{
			if(parse_boolean(m_basicSimulation->GetConfigParamOrDefault("enable_trajectory_tracing", "false"))){
				for(uint32_t sat = 0; sat < m_nodesCurSystem.GetN(); sat++){
					std::string jsonString = m_satelliteElements[sat].dump(4);
					std::ofstream fileTopologyChange(m_satellite_network_dir + "/system_"+ to_string(m_system_id) + "_orbital_elements"+ "/satellite_" + std::to_string(m_nodesCurSystem.Get(sat)->GetId()) +".json", std::ofstream::out);
					fileTopologyChange<<jsonString;
					fileTopologyChange.close();
				}
			}
		}

    }

    void
       TopologySatelliteNetwork::MakeLinkSINRUpdateEvent(double time){
    	std::vector<double> AveSINRs = {};
    	std::vector<double> AvePERs_fwd = {};
    	std::vector<double> AvePERs_rtn = {};
       	for(uint32_t i = 0; i < m_gslSatNetDevices.GetN(); i++){
       		Ptr<Node> sat = m_gslSatNetDevices.Get(i)->GetNode();
       		Ptr<SAGPhysicalLayerGSL> gsl_channel = m_gslSatNetDevices.Get(i)->GetChannel()->GetObject<SAGPhysicalLayerGSL>();
       		Ptr<SAGLinkLayerGSL> sat1 =  sat->GetDevice(5)->GetObject<SAGLinkLayerGSL>();
       		std::vector<double> SINRs = {};
       		std::vector<double> PERs_fwd = {};
       		std::vector<double> PERs_rtn = {};
       		double aveSINR; //均值
			double avePER_fwd; //均值
			double avePER_rtn;
       		for(uint32_t j = 1; j < gsl_channel->GetNDevices(); j++){
       			Ptr<Node> gnd = gsl_channel->GetDevice(j)->GetNode();
       			Ptr<MobilityModel> senderMobility = sat->GetObject<MobilityModel>();
       			Ptr<MobilityModel> receiverMobility = gnd->GetObject<MobilityModel>();
       			Ptr<SAGLinkLayerGSL> gs =  gnd->GetDevice(1)->GetObject<SAGLinkLayerGSL>();

       			//double AntennaGain = gs-> GetAntennaModel()-> GetGainDb(senderMobility->GetPosition(), receiverMobility->GetPosition());
       			double AntennaGain = gs->GetTxAntennaGain();
       			//double AntennaGain=40;
       			//std::cout<<"AntennaGain is,"<<AntennaGain<<std::endl;
       			//gsl_channel->GetTxPowerDb() +
       			double TxGain=gs->GetTxPowerDb() + AntennaGain;
       			double rvPower = gsl_channel->GetPropagationScenario()-> CalcRxPower (TxGain, senderMobility, receiverMobility);
       			double NoiseDbm = gsl_channel-> GetNoiseDbm() ;
       			double SINRDb = rvPower-NoiseDbm;
       			//SINRDb=17.74564926372155;
       			//std::cout<<"FrameType is,"<<gs->GetFrameType()<<std::endl;
       			//satellite-->gs
       			double actualBlerRTN= sat1->GetLinkResultsRTN()-> GetBler(sat1->GetWaveformId(), SINRDb);
       			//gs-->satellite
       			double actualBlerFWD= gs->GetLinkResultsFWD()-> GetBler(gs->GetTxMCS(), gs->GetFrameType(), SINRDb);

       			//std::cout<<"SINR is,"<<SINRDb<<std::endl;
   				SINRs.push_back(SINRDb);
   				//std::cout<<"PERs_rtn is,"<<actualBlerRTN<<std::endl;
   				PERs_rtn.push_back(actualBlerRTN);
   				//std::cout<<"PERs_fwd is,"<<actualBlerFWD<<std::endl;
   				PERs_fwd.push_back(actualBlerFWD);
       		}
       		if (!SINRs.empty()){
				gsl_channel-> SetChannelSINR(SINRs);
				gsl_channel-> SetChannelPERFWD(PERs_fwd);
				gsl_channel-> SetChannelPERRTN(PERs_rtn);
				double SumSINR = std::accumulate(std::begin(SINRs), std::end(SINRs), 0.0);
				aveSINR =  SumSINR / SINRs.size(); //均值
				//std::cout<<"aveSINR is,"<<aveSINR<<std::endl;
				double SumPER_fwd = std::accumulate(std::begin(PERs_fwd), std::end(PERs_fwd), 0.0);
				avePER_fwd =  SumPER_fwd / PERs_fwd.size(); //均值
				double SumPER_rtn= std::accumulate(std::begin(PERs_rtn), std::end(PERs_rtn), 0.0);
				avePER_rtn =  SumPER_rtn / PERs_rtn.size(); //均值
				AveSINRs.push_back(aveSINR);
				AvePERs_fwd.push_back(avePER_fwd);
				AvePERs_rtn.push_back(avePER_rtn);
       		}

       	}
    	int64_t dynamicStateUpdateIntervalNsTemp;
    	if(time == 0) dynamicStateUpdateIntervalNsTemp = m_dynamicStateUpdateIntervalNs + 2;
    	else dynamicStateUpdateIntervalNsTemp = m_dynamicStateUpdateIntervalNs;
    	int64_t next_update_ns = time + dynamicStateUpdateIntervalNsTemp;
		if (next_update_ns < m_basicSimulation->GetSimulationEndTimeNs()) {
			Simulator::Schedule(NanoSeconds(dynamicStateUpdateIntervalNsTemp),
					&TopologySatelliteNetwork::MakeLinkSINRUpdateEvent, this, next_update_ns);
		}
		// write sinr-ber.json
		nlohmann::ordered_json jsonObject2;
		jsonObject2["sinr"] = AveSINRs;
		jsonObject2["ber_rtn"] = AvePERs_fwd;
		jsonObject2["ber_fwd"] = AvePERs_rtn;
		jsonObject2["time_stamp_ns"] = time;
		std::ofstream sinr(m_basicSimulation->GetRunDir() + "/results/network_results/global_statistics/network_wide_sinr.json", std::ofstream::app);
		if (sinr.is_open()) {
			sinr << jsonObject2.dump(4);  // 使用缩进格式将 JSON 内容写入文件
			sinr.close();
			//std::cout << "JSON file created successfully." << std::endl;
		} else {
			std::cout << "Failed to create JSON file." << std::endl;
		}

    }


//    void TopologySatelliteNetwork::MaketUnpredictableGSLChangeEvent(double time ,Oper oper,int32_t gndId, int32_t satId){
//
//		Time delay = Seconds (time);
//		if(oper == OPER_ADD)
//		{
//
//			Simulator::Schedule(delay, &TopologySatelliteNetwork::AddGSLByGndAndSatId, this, gndId, satId);
//		}
//		else if(oper == OPER_DEL)
//		{
//			Simulator::Schedule(delay, &TopologySatelliteNetwork::DisableGSLByGndAndSatId, this, gndId, satId);
//
//		}
//    }

    void TopologySatelliteNetwork::ReadSunTrajectoryEciFromCspice(){
    	// Open file
    	json j;
		std::ifstream config_file;
		config_file.open(m_satellite_network_dir + "/system_"+std::to_string(m_system_id)+"_sun_trajectory (eci_cspice).json");
		NS_ABORT_MSG_UNLESS(config_file.is_open(), "File sun_trajectory (eci_cspice).json could not be opened");
		config_file >> j;
		int tilength = j["cspice"].size();
		for (int i = 0; i < tilength; i++) {
			m_sun_trajectory.push_back(j["cspice"][i]);
		}


    }

    void TopologySatelliteNetwork::MakeSunOutageEvent(double time){

    	std::vector<std::pair<uint32_t,uint32_t>> sunOutageLinks = m_sunOutageLinks;
    	m_sunOutageLinks.clear();

    	if(m_sun == 0){
    		m_sun = CreateObject<Node>();

			// Create sun
			Ptr<Earth> sun = CreateObject<Earth>();
			sun->SetEpoch("2000-01-01 00:00:00");
			m_sun_model = sun;


			// Decide the mobility model of the satellite
			MobilityHelper mobility;
			// Dynamic
			mobility.SetMobilityModel(
					"ns3::EarthPositionMobilityModel",
					"EarthPositionHelper",
					EarthPositionHelperValue(EarthPositionHelper(sun))
			);
			mobility.Install(m_sun);
    	}
//    	if(Simulator::Now().GetNanoSeconds() == 0){
//        	jsonns::sun_trajectory_content cont = m_sun_trajectory[floor(Simulator::Now().GetNanoSeconds()/m_dynamicStateUpdateIntervalNs)];
//        	//NS_ASSERT(cont.time_stamp_ns == uint32_t(Simulator::Now().GetNanoSeconds()));
//        	m_sun_model->SetPosition(cont.position_km);
//        	m_sun_model->SetVelocity(cont.velocity_kmps);
//    	}
    	NS_ASSERT(m_sun_trajectory.size() == floor(m_time_end/m_dynamicStateUpdateIntervalNs + 1));
    	jsonns::sun_trajectory_content cont = m_sun_trajectory[floor(Simulator::Now().GetNanoSeconds()/m_dynamicStateUpdateIntervalNs)];
    	NS_ASSERT_MSG(cont.time_stamp_ns == int64_t(Simulator::Now().GetNanoSeconds()), to_string(cont.time_stamp_ns)+"_"+to_string(int64_t(Simulator::Now().GetNanoSeconds())));
    	m_sun_model->SetPosition(cont.position_km);
    	m_sun_model->SetVelocity(cont.velocity_kmps);

    	//<! AER Calculation
    	for(Ptr<Constellation> cons: m_constellations){
    		auto adjs = cons->GetAdjacency();
    		for(auto adj: adjs){
    			// current satellite
    			Ptr<Node> sat = this->GetSatelliteNodes().Get(adj.first);
    			Ptr<MobilityModel> satMobility = sat->GetObject<MobilityModel>();
    			Vector oPosition = satMobility->GetPosition ();
    			double rs[3] = {oPosition.x/1e3, oPosition.y/1e3, oPosition.z/1e3};
    			Vector oVelocity = satMobility->GetVelocity();
    			double vs[3] = {oVelocity.x/1e3, oVelocity.y/1e3, oVelocity.z/1e3};

    			// sun
    			double razel_sun[3];
				double razelrates_sun[3];
				Ptr<MobilityModel> sunMobility = m_sun->GetObject<MobilityModel>();
				Vector nPosition_sun = sunMobility->GetPosition ();
				double recef_sun[3] = {nPosition_sun.x/1e3, nPosition_sun.y/1e3, nPosition_sun.z/1e3};
				Vector nVelocity_sun = sunMobility->GetVelocity();
				double vecef_sun[3] = {nVelocity_sun.x/1e3, nVelocity_sun.y/1e3, nVelocity_sun.z/1e3};

				rv2azel(recef_sun, vecef_sun, rs, vs, razel_sun, razelrates_sun);
				//std::cout<<aer.x<<","<<aer.y<<","<<aer.z<<std::endl;
				//std::cout<<std::to_string(Simulator::Now().GetSeconds())<<","<<razel[0]<<","<<razel[1]*180.0/pi<<","<<razel[2]*180.0/pi<<std::endl;
				//std::cout<<""<<std::endl;
				Vector aer_sun = {razel_sun[1], razel_sun[2], razel_sun[0]};


    			// neighbour
    			for(auto neighbor: adj.second){
    				double razel[3];
    				double razelrates[3];
        			Ptr<Node> neighborNode = this->GetSatelliteNodes().Get(neighbor);
        			Ptr<MobilityModel> neighborNodeMobility = neighborNode->GetObject<MobilityModel>();
        			Vector nPosition = neighborNodeMobility->GetPosition ();
        			double recef[3] = {nPosition.x/1e3, nPosition.y/1e3, nPosition.z/1e3};
        			Vector nVelocity = neighborNodeMobility->GetVelocity();
        			double vecef[3] = {nVelocity.x/1e3, nVelocity.y/1e3, nVelocity.z/1e3};

        			rv2azel(recef, vecef, rs, vs, razel, razelrates);
					//std::cout<<aer.x<<","<<aer.y<<","<<aer.z<<std::endl;
					//std::cout<<std::to_string(Simulator::Now().GetSeconds())<<","<<razel[0]<<","<<razel[1]*180.0/pi<<","<<razel[2]*180.0/pi<<std::endl;
					//std::cout<<""<<std::endl;
					Vector aer = {razel[1], razel[2], razel[0]};

			    	// calculate SunOutage
			    	double s1[3] = {cos(aer_sun.y)*cos(aer_sun.x),cos(aer_sun.y)*sin(aer_sun.x),sin(aer_sun.y)};
			    	double s2[3] = {cos(aer.y)*cos(aer.x),cos(aer.y)*sin(aer.x),sin(aer.y)};

			    	double ang = acos(DotProduct(s1, s2)) * 180.0 / pi;
//			    	if(sat->GetId()== 0){
//				    	std::cout<<time/1e9<<"  "<<neighbor<<"  "<<ang<<std::endl;
//			    	}
			    	if(ang < 0){
			    		std::cout<<"ang < 0"<<std::endl;
			    		exit(1);
			    	}
			    	// Sun Outage Happen!
			    	if(ang < 5){
			    		auto p = std::make_pair(std::min(adj.first,neighbor),std::max(adj.first,neighbor));
			    		auto iter = find(sunOutageLinks.begin(), sunOutageLinks.end(), p);
						if(iter == sunOutageLinks.end()){
							DisableISLBySatId(p.first, p.second, false);
						}
						else{
							sunOutageLinks.erase(iter);
						}
						m_sunOutageLinks.push_back(p);
			    		//std::cout<<adj.first<<"  "<<neighbor<<std::endl;
			    	}

    			}

    		}

    	}

    	// Link State Recover from Sun Outage
    	for(auto link: sunOutageLinks){
    		AddISLBySatId(link.first, link.second, false);
    	}


    	// schedule next event
    	int64_t next_update_ns = time + m_dynamicStateUpdateIntervalNs;
		if (next_update_ns < m_basicSimulation->GetSimulationEndTimeNs()) {
			Simulator::Schedule(NanoSeconds(m_dynamicStateUpdateIntervalNs),
					&TopologySatelliteNetwork::MakeSunOutageEvent, this, next_update_ns);
		}

    }

    void TopologySatelliteNetwork::InstallGSLInterfaceForAllSatellites(){

		// Link helper
    	SAGPhysicalGSLHelper gsl_helper(m_basicSimulation);
		std::string max_queue_size_str = format_string("%" PRId64 "p", m_gsl_max_queue_size_pkts);
		gsl_helper.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", QueueSizeValue(QueueSize(max_queue_size_str)));
		gsl_helper.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (std::to_string(m_gsl_data_rate_megabit_per_s) + "Mbps")));
		gsl_helper.SetDeviceAttribute ("MaxFeederLinkNumber", UintegerValue((uint16_t)std::stoi(m_basicSimulation->GetConfigParamOrFail("maximum_feeder_link_number"))));
		std::cout << "    >> GSL data rate........ " << m_gsl_data_rate_megabit_per_s << " Mbit/s" << std::endl;
		std::cout << "    >> GSL max queue size... " << m_gsl_max_queue_size_pkts << " packets" << std::endl;

		// Traffic control helper
		TrafficControlHelper tch_gsl;
		tch_gsl.SetRootQueueDisc("ns3::FifoQueueDisc", "MaxSize", QueueSizeValue(QueueSize("1p")));  // Will be removed later any case

		// Create and install GSL network devices
		NetDeviceContainer devices = gsl_helper.SatelliteInstall(m_satelliteNodes);
		m_gslSatNetDevices = devices;
		NS_ABORT_MSG_IF (m_gslSatNetDevices.GetN() != m_satelliteNodes.GetN(), "Each node one gsl interface permitted now");
		std::cout << "    >> Finished installing GSL interfaces for all satellites" << std::endl;

		// Install queueing disciplines
		tch_gsl.Install(devices);
		std::cout << "    >> Finished installing traffic control layer qdisc which will be removed later" << std::endl;

		// Assign IP addresses for all satellites' gsl interfaces
		// The ground station interface within the coverage is allocated to the same network segment
		for (uint32_t i = 0; i < devices.GetN(); i++) {

			// Assign IPv4 address
			// One satellite GSL interface -> one fixed network segment
			Ptr<NetDevice> netdev = devices.Get(i);
			if(m_isIPv4Networking){
				m_networkAddressingMethod->AssignSatelliteToGroundInterfaceAddress(NetDeviceContainer(netdev));
			}
			else if(m_isIPv6Networking){
				m_networkAddressingMethodIPv6->AssignSatelliteToGroundInterfaceAddress(NetDeviceContainer(netdev));
			}

		}
		std::cout << "    >> Finished assigning IPs for all satellites' GSL interfaces" << std::endl;

		// Remove the traffic control layer (must be done here, else the Ipv4 helper will assign a default one)
		TrafficControlHelper tch_uninstaller;
		std::cout << "    >> Removing traffic control layers (qdiscs)..." << std::endl;
		for (uint32_t i = 0; i < devices.GetN(); i++) {
			tch_uninstaller.Uninstall(devices.Get(i));
		}
		std::cout << "    >> Finished removing GSL queueing disciplines for all satellites" << std::endl;




		// Install traffic control layer on GSL with ECN
		TrafficControlHelper tch_gsl_ecn;
		Config::SetDefault ("ns3::RedQueueDisc::MaxSize",
			QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS, m_gsl_max_queue_size_pkts)));
		tch_gsl_ecn.SetRootQueueDisc("ns3::RedQueueDisc","UseEcn", BooleanValue(true));
		for (uint32_t i = 0; i < devices.GetN(); i++) {
			tch_gsl_ecn.Install(devices.Get(i));
		}
		std::cout << "    >> Finished installing traffic control layer qdisc with ECN for all satellites' GSL netdevices" << std::endl;


		if(m_isIPv4Networking){
			InitializeSatellitesArpCaches();
			std::cout << "    >> Finished initializing Arps for all satellites" << std::endl;
		}
		std::cout << "    >> GSL interfaces of satellites are setup" << std::endl;

		for(uint32_t k = 0; k < m_gslSatNetDevices.GetN(); k++){
			Ptr<NetDevice> nd = m_gslSatNetDevices.Get(k);
			Ptr<RateErrorModel> em = CreateObjectWithAttributes<RateErrorModel>(
					"RanVar", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1.0]"),
					"ErrorRate", DoubleValue (0.0),
					"ErrorUnit", EnumValue (RateErrorModel::ErrorUnit::ERROR_UNIT_PACKET));
			nd->SetAttribute("ReceiveErrorModel", PointerValue(em));
		}
		std::cout << "    >> Receive error model of GSL are setup (satellite end)" << std::endl;


    }

    void TopologySatelliteNetwork::InstallGSLInterfaceForAllGroundStations(){
    	// Link helper
		SAGPhysicalGSLHelper gsl_helper(m_basicSimulation);
		std::string max_queue_size_str = format_string("%" PRId64 "p", m_gsl_max_queue_size_pkts);
		gsl_helper.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", QueueSizeValue(QueueSize(max_queue_size_str)));
		gsl_helper.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (std::to_string(m_gsl_data_rate_megabit_per_s) + "Mbps")));
		std::cout << "    >> GSL data rate........ " << m_gsl_data_rate_megabit_per_s << " Mbit/s" << std::endl;
		std::cout << "    >> GSL max queue size... " << m_gsl_max_queue_size_pkts << " packets" << std::endl;

		// Create and install GSL network devices
		//NetDeviceContainer llldevices;
    	//uint32_t ground_antenna_number = parse_positive_int64(m_basicSimulation->GetConfigParamOrDefault("ground_antenna_number", "1"));
		uint32_t ground_antenna_number = 1;
		for(uint32_t n = 0; n < ground_antenna_number; n++){
    		NetDeviceContainer devices = gsl_helper.GroundStationInstall(m_groundStationNodes);
    		m_gslGsNetDevices.Add(devices);
    		m_gslGsNetDevicesInterface.push_back(devices);
    	}

		NS_ABORT_MSG_IF (m_gslGsNetDevices.GetN() != m_groundStationNodes.GetN()*ground_antenna_number, "Each node one gsl interface permitted now");
		std::cout << "    >> Finished installing GSL interfaces for all ground stations" << std::endl;

		for(uint32_t k = 0; k < m_gslGsNetDevices.GetN(); k++){
			Ptr<NetDevice> nd = m_gslGsNetDevices.Get(k);
			Ptr<RateErrorModel> em = CreateObjectWithAttributes<RateErrorModel>(
					"RanVar", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1.0]"),
					"ErrorRate", DoubleValue (0.0),
					"ErrorUnit", EnumValue (RateErrorModel::ErrorUnit::ERROR_UNIT_PACKET));
			nd->SetAttribute("ReceiveErrorModel", PointerValue(em));
		}
		std::cout << "    >> Receive error model of GSL are setup (ground station end)" << std::endl;
    }

    void TopologySatelliteNetwork::SwitchGSLForAllGroundStations(){

		// for initialization
		if(m_gslRecordCopy.empty()){
			// just for infrastructure such as earth stations, air crafts,etc.
			for(uint32_t p = 0; p < (m_gslRecord.size() - m_terminalsReal.size()); p++){
				for(std::pair<uint32_t, Ptr<Node>> connectionEntry: m_gslRecord.at(p).second){
					if(connectionEntry.second != nullptr){
						auto gnd = m_gslRecord.at(p).first;
						auto sat = connectionEntry.second;

						AddGSLByGndAndSat(gnd, sat, connectionEntry.first);
					}
				}

			}
			return;
		}
		// for gsls switching
		// just for infrastructure such as earth stations, air crafts,etc.
		for(uint32_t p = 0; p < (m_gslRecord.size() - m_terminalsReal.size()); p++){
			auto gnd = m_gslRecord.at(p).first;
			uint32_t interface = 0;
			for(std::pair<uint32_t, Ptr<Node>> connectionEntry: m_gslRecord.at(p).second){
				auto sat = connectionEntry.second;
				auto gndPast = m_gslRecordCopy.at(p).first;
				std::pair<uint32_t, Ptr<Node>> connectionEntryPast = m_gslRecordCopy.at(p).second.at(interface);
				if(interface + 1 != connectionEntryPast.first){
					throw std::runtime_error("GSL interface and record do not correspond");
				}
				auto satPast = connectionEntryPast.second;
				if(gnd != nullptr && gndPast != nullptr && gnd->GetId() !=  gndPast->GetId()){
					throw std::runtime_error("GND node records are out of order");
				}
				if(sat == nullptr && satPast != nullptr){
					DisableGSLByGndAndSat(gndPast, satPast, connectionEntry.first);
				}
				else if(sat == nullptr && satPast == nullptr){

				}
				else if(sat != nullptr && satPast == nullptr){
					AddGSLByGndAndSat(gnd, sat, connectionEntry.first);
				}
				else{
					if(sat->GetId() == satPast->GetId()){

					}
					else if(sat->GetId() != satPast->GetId()){
						DisableGSLByGndAndSat(gndPast, satPast, connectionEntry.first);
						AddGSLByGndAndSat(gnd, sat, connectionEntry.first);
					}
				}
				interface++;
			}

		}

		//NotifyInterfaceAddressChangeToApp();

    }

    void TopologySatelliteNetwork::AddGSLByGndAndSat(Ptr<Node> gs, Ptr<Node> sat, uint32_t interface){

    	std::ofstream fileTopologyChange(m_satellite_network_dir + "/system_" + std::to_string(m_system_id)+"_topology_change_message.txt", std::ofstream::out | std::ofstream::app);
    	fileTopologyChange <<Simulator::Now().GetMilliSeconds()<<","<<gs->GetId()<<","<<sat->GetId()<<std::endl;
    	fileTopologyChange.close();
    	ConnectionLink addLink(gs->GetId(), interface);
        auto iter = find(m_gsLinkDetails.begin(), m_gsLinkDetails.end(), addLink);
        if(iter == m_gsLinkDetails.end()){
        	addLink.satnodes.push_back(sat->GetId());
        	std::vector<std::pair<double, double>> cont;
        	cont.push_back(std::make_pair(Simulator::Now().GetSeconds(), m_basicSimulation->GetSimulationEndTimeNs()/1e9));
        	addLink.connectionTimeInterval.push_back(cont);
        	m_gsLinkDetails.push_back(addLink);
        }
        else{
        	std::vector<uint32_t> sn = iter->satnodes;
        	auto iter_satnode = find(sn.begin(), sn.end(), sat->GetId());
        	if(iter_satnode == sn.end()){
        		iter->satnodes.push_back(sat->GetId());
				std::vector<std::pair<double, double>> cont;
				cont.push_back(std::make_pair(Simulator::Now().GetSeconds(), m_basicSimulation->GetSimulationEndTimeNs()/1e9));
				iter->connectionTimeInterval.push_back(cont);
        	}
        	else{
        		auto delta = iter_satnode - sn.begin();
        		iter->connectionTimeInterval[delta].push_back(std::make_pair(Simulator::Now().GetSeconds(), m_basicSimulation->GetSimulationEndTimeNs()/1e9));
        	}
        }



    	// todo modify the NetDevice obtaining?
		Ptr<NetDevice> gslSatNetDevice = GetSatGSLNetDevice(sat);
		Ptr<NetDevice> gslGsNetDevice = GetGsGSLNetDevice(gs, interface-1);
		NS_ASSERT_MSG (gslSatNetDevice != 0 && gslGsNetDevice != 0, "No device");

		// first attach channel
		gslGsNetDevice->GetObject<SAGLinkLayerGSL>()->Attach(gslSatNetDevice->GetChannel()->GetObject<SAGPhysicalLayerGSL>(), 0);

		// then allocate address for ground station gsl interface
		if(m_isIPv4Networking){
			Ptr<Ipv4L3Protocol> ipv4 = gs->GetObject<Ipv4L3Protocol>();
			int32_t itf = ipv4->GetInterfaceForDevice(gslGsNetDevice);
			if(itf == -1){
				// initialize
				// Traffic control helper
				TrafficControlHelper tch_gsl;
				tch_gsl.SetRootQueueDisc("ns3::FifoQueueDisc", "MaxSize", QueueSizeValue(QueueSize("1p")));  // Will be removed later any case
				tch_gsl.Install(gslGsNetDevice);

				m_networkAddressingMethod->GetObject<AddressingInIPv4>()->AssignGroundToSatelliteInterfaceAddress(sat->GetId(), gslGsNetDevice);

				// std::cout<<m_gsl_ipv4_addressing_helper[sat->GetId()].GetNetwork()<<std::endl;
				// std::cout<<ipv4->GetInterface(ipv4->GetInterfaceForDevice(gslGsNetDevice))->GetAddress(0).GetLocal()<<std::endl;
				TrafficControlHelper tch_uninstaller;
				tch_uninstaller.Uninstall(gslGsNetDevice);

				// after allocate address, initialize interface
				InitializeGroundStationsArpCaches(gs, gslGsNetDevice);
			}
			else{
				// add address
				Ipv4InterfaceAddress ipv4Addr;
				m_networkAddressingMethod->GetObject<AddressingInIPv4>()
						->AssignGroundToSatelliteInterfaceAddress(sat->GetId(), gslGsNetDevice, ipv4Addr);

				ipv4->AddAddress(itf, ipv4Addr);

				// std::cout<<gs->GetId()<<"   "<<ipv4->GetNAddresses(itf)<<"  !!!"<<ipv4Addr.GetLocal()<<std::endl;
				// std::cout<<ipv4->GetInterface(itf)->GetAddress(0).GetLocal()<<std::endl;
			}
			int32_t itfce = ipv4->GetInterfaceForDevice(gslGsNetDevice);
			NS_ASSERT_MSG (itfce != -1, "No device");
			uint32_t nadr = ipv4->GetInterface(itfce)->GetNAddresses();
			NS_ASSERT_MSG (nadr == 1, "One interface one address permitted");
		}
		else if(m_isIPv6Networking){
			Ptr<Ipv6L3Protocol> ipv6 = gs->GetObject<Ipv6L3Protocol>();
			int32_t itf = ipv6->GetInterfaceForDevice(gslGsNetDevice);
			if(itf == -1){
				// initialize
				// Traffic control helper
				TrafficControlHelper tch_gsl;
				tch_gsl.SetRootQueueDisc("ns3::FifoQueueDisc", "MaxSize", QueueSizeValue(QueueSize("1p")));  // Will be removed later any case
				tch_gsl.Install(gslGsNetDevice);

				m_networkAddressingMethodIPv6->GetObject<AddressingInIPv6>()->AssignGroundToSatelliteInterfaceAddress(sat->GetId(), gslGsNetDevice);

				// std::cout<<m_gsl_ipv6_addressing_helper[sat->GetId()].GetNetwork()<<std::endl;
				// std::cout<<ipv6->GetInterface(ipv4->GetInterfaceForDevice(gslGsNetDevice))->GetAddress(0).GetAddress()<<std::endl;
				TrafficControlHelper tch_uninstaller;
				tch_uninstaller.Uninstall(gslGsNetDevice);

				// after allocate address, initialize interface todo
				// InitializeGroundStationsArpCaches(gs, gslGsNetDevice);
				// InitializeGroundStationsNdCaches(gs, gslGsNetDevice);
			}
			else{
				// add address
				Ipv6InterfaceAddress ipv6Addr;
				m_networkAddressingMethodIPv6->GetObject<AddressingInIPv6>()
						->AssignGroundToSatelliteInterfaceAddress(sat->GetId(), gslGsNetDevice, ipv6Addr);
				ipv6->AddAddress(itf, ipv6Addr);

				// std::cout<<gs->GetId()<<"   "<<ipv4->GetNAddresses(itf)<<"  !!!"<<ipv4Addr.GetLocal()<<std::endl;
				// std::cout<<ipv4->GetInterface(itf)->GetAddress(0).GetLocal()<<std::endl;
			}
			int32_t itfce = ipv6->GetInterfaceForDevice(gslGsNetDevice);
			NS_ASSERT_MSG (itfce != -1, "No device");
			// uint32_t nadr = ipv6->GetInterface(itfce)->GetNAddresses();
			// NS_ASSERT_MSG (nadr == 1, "One interface one address permitted");

		}

		//Install traffic control layer on GSL with ECN
		TrafficControlHelper tch_gsl_ecn;
		Config::SetDefault ("ns3::RedQueueDisc::MaxSize",
			QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS, m_gsl_max_queue_size_pkts)));
		tch_gsl_ecn.SetRootQueueDisc("ns3::RedQueueDisc","UseEcn", BooleanValue(true));
		tch_gsl_ecn.Install(gslGsNetDevice);
		//之前已经为gslSatNetDevice安装过TrafficControlLayer，所以这里不需要再次安装
		//tch_gsl_ecn.Install(gslSatNetDevice);


		// notify sat L3
		gslSatNetDevice->GetObject<SAGLinkLayerGSL>()->NotifyStateToL3Stack();

		// update arp
		if(m_isIPv4Networking){
			ArpCacheEntryAdd(gs, sat, gslGsNetDevice, gslSatNetDevice);
		}

    }

	void TopologySatelliteNetwork::DisableGSLByGndAndSat(Ptr<Node> gs, Ptr<Node> sat, uint32_t interface){

    	//std::ofstream fileTopologyChange(m_satellite_network_dir + "/system_" + std::to_string(m_system_id)+"_topology_change_message.txt", std::ofstream::out | std::ofstream::app);
    	//fileTopologyChange << "GSL Interruption Time: "<<Simulator::Now().GetMilliSeconds()<<" ms Disable: "<<gs->GetId()<<"  "<<sat->GetId()<<std::endl;
		//std::cout<<"GSL Interruption Time: "<<Simulator::Now().GetMilliSeconds()<<" ms Disable: "<<gs->GetId()<<"  "<<sat->GetId()<<std::endl;
    	ConnectionLink addLink(gs->GetId(), interface);
        auto iter = find(m_gsLinkDetails.begin(), m_gsLinkDetails.end(), addLink);
        NS_ASSERT_MSG (iter != m_gsLinkDetails.end(), "No Connection Link Details Record");
        //std::get<2>(iter->connectionTimeInterval[iter->connectionTimeInterval.size()-1]) = Simulator::Now().GetSeconds();
    	std::vector<uint32_t> sn = iter->satnodes;
    	auto iter_satnode = find(sn.begin(), sn.end(), sat->GetId());
        NS_ASSERT_MSG (iter_satnode != sn.end(), "No Connection Link Details Record!");
		auto delta = iter_satnode - sn.begin();
		iter->connectionTimeInterval[delta][iter->connectionTimeInterval[delta].size()-1].second = Simulator::Now().GetSeconds();


		// todo modify the NetDevice obtaining?
		Ptr<NetDevice> gslSatNetDevice = GetSatGSLNetDevice(sat);
		Ptr<NetDevice> gslGsNetDevice = GetGsGSLNetDevice(gs, interface-1);
		NS_ASSERT_MSG (gslSatNetDevice != 0 && gslGsNetDevice != 0, "No device");

		// remove address for ground station gsl interface
		if(m_isIPv4Networking){
			Ptr<Ipv4L3Protocol> ipv4 = gs->GetObject<Ipv4L3Protocol>();
			int32_t itf = ipv4->GetInterfaceForDevice(gslGsNetDevice);
			NS_ASSERT_MSG (itf != -1, "No device");
			uint32_t nadr = ipv4->GetInterface(itf)->GetNAddresses();
			NS_ASSERT_MSG (nadr == 1, "One interface one address permitted");

			// update arp
			ArpCacheEntryDelete(gs, sat, gslGsNetDevice, gslSatNetDevice);

			ipv4->RemoveAddress(itf, 0);
		}
		else if(m_isIPv6Networking){
			Ptr<Ipv6L3Protocol> ipv6 = gs->GetObject<Ipv6L3Protocol>();
			int32_t itf = ipv6->GetInterfaceForDevice(gslGsNetDevice);
			NS_ASSERT_MSG (itf != -1, "No device");
			uint32_t nadr = ipv6->GetInterface(itf)->GetNAddresses();
			NS_ASSERT_MSG (nadr == 2, "One interface Two address permitted for ipv6");

			// update arp
			// ArpCacheEntryDelete(gs, sat, gslGsNetDevice, gslSatNetDevice);

			ipv6->RemoveAddress(itf, 1);
		}

		// unattach channel
		gslGsNetDevice->GetObject<SAGLinkLayerGSL>()->UnAttach(0);
		gslSatNetDevice->GetObject<SAGLinkLayerGSL>()->NotifyStateToL3Stack();

	}


    void TopologySatelliteNetwork::MakeGSLChangeEvent(double time){

    	// Make GSL handover event

    	// A global variable that records whether GSL changes (infrastructure such as earth stations, air crafts,etc)
    	Topology_CHANGE_GSL = false;

    	// Data structure for recording connection information
    	m_gslRecordCopy = m_gslRecord;
    	m_gslRecord.clear();

    	// just for infrastructure such as earth stations, air crafts,etc.
    	m_switchStrategy->UpdateSwitch(m_gslRecord, m_gslRecordCopy);
    	// for terminals todo
    	// m_switchStrategyTerminal->UpdateSwitch(...);

		// If GSL changes
    	if(Topology_CHANGE_GSL){
        	// Perform GSL switching and update addressing and other policies
			// just for infrastructure such as earth stations, air crafts,etc.
        	SwitchGSLForAllGroundStations();

        	// for terminals todo
        	// SwitchAccessForAllTerminals();

        	// need to be modified: the satellite that the terminal is ultimately pointing to todo
        	// (a satellite that is directly connected, or a satellite that is accessed through a relay)
    		for(auto cons : m_constellations){
    			cons->SetGSLInformation(m_gslRecord);
    		}
    	}

    	// Update actual GSL rate immediately after GSL handovers todo
    	if(parse_boolean(m_basicSimulation->GetConfigParamOrFail("enable_gsl_data_rate_fixed")) && time == 0){
    		// for gsl data rate fixed mode
        	for(uint32_t i = 0; i < m_gslGsNetDevices.GetN(); i++){
        		Ptr<NetDevice> ntd = m_gslGsNetDevices.Get(i);
        		Ptr<SAGLinkLayerGSL> ntdGSL = ntd->GetObject<SAGLinkLayerGSL>();
    	        NS_ASSERT_MSG (ntdGSL != nullptr, "instance error");
    	        ntdGSL->UpdateDataRate();
        	}
        	for(uint32_t i = 0; i < m_gslSatNetDevices.GetN(); i++){
        		Ptr<NetDevice> ntd = m_gslSatNetDevices.Get(i);
        		Ptr<SAGLinkLayerGSL> ntdGSL = ntd->GetObject<SAGLinkLayerGSL>();
    	        NS_ASSERT_MSG (ntdGSL != nullptr, "instance error");
    	        ntdGSL->UpdateDataRate();
        	}
    	}
    	else if(!parse_boolean(m_basicSimulation->GetConfigParamOrFail("enable_gsl_data_rate_fixed"))){
        	for(uint32_t i = 0; i < m_gslGsNetDevices.GetN(); i++){
        		Ptr<NetDevice> ntd = m_gslGsNetDevices.Get(i);
        		Ptr<SAGLinkLayerGSL> ntdGSL = ntd->GetObject<SAGLinkLayerGSL>();
    	        NS_ASSERT_MSG (ntdGSL != nullptr, "instance error");
    	        ntdGSL->UpdateDataRate();
        	}
        	for(uint32_t i = 0; i < m_gslSatNetDevices.GetN(); i++){
        		Ptr<NetDevice> ntd = m_gslSatNetDevices.Get(i);
        		Ptr<SAGLinkLayerGSL> ntdGSL = ntd->GetObject<SAGLinkLayerGSL>();
    	        NS_ASSERT_MSG (ntdGSL != nullptr, "instance error");
    	        ntdGSL->UpdateDataRate();
        	}
    	}

		// Plan the next update
    	int64_t dynamicStateUpdateIntervalNsTemp;
    	if(time == 0) dynamicStateUpdateIntervalNsTemp = m_dynamicStateUpdateIntervalNs + 1;
    	else dynamicStateUpdateIntervalNsTemp = m_dynamicStateUpdateIntervalNs;
    	int64_t next_update_ns = time + dynamicStateUpdateIntervalNsTemp;
		if (next_update_ns < m_basicSimulation->GetSimulationEndTimeNs()) {
			Simulator::Schedule(NanoSeconds(dynamicStateUpdateIntervalNsTemp),
					&TopologySatelliteNetwork::MakeGSLChangeEvent, this, next_update_ns);
		}

    }



    void 
    TopologySatelliteNetwork::AddISLBySatId(uint32_t satId0, uint32_t satId1, bool regularOrNot)
    {
    	Ptr<Constellation> cons1 = FindConstellationBySatId(satId0);
    	Ptr<Constellation> cons2 = FindConstellationBySatId(satId1);
    	if(cons1->GetName() != cons2->GetName()){
    		throw std::runtime_error ("No support to isl between different constellation now");
    	}
    	NetDeviceContainer islNetDevices = cons1->GetIslNetDevicesInfo();
    	std::vector<std::pair<uint32_t, uint32_t>> islFromTo = cons1->GetIslFromTo();

        Ptr<SAGLinkLayer> dev0= islNetDevices.GetWithKey(CalStringKey(satId0,satId1))->GetObject<SAGLinkLayer>();
        Ptr<SAGLinkLayer> dev1= islNetDevices.GetWithKey(CalStringKey(satId1,satId0))->GetObject<SAGLinkLayer>();

    	std::map<std::string, std::pair<bool, bool>>::iterator iter = m_islDisableNetDevices.find(CalStringKey(satId0, satId1));
        if(iter != m_islDisableNetDevices.end())
        {
        	if((*iter).second.first == true && regularOrNot){
        		(*iter).second.first = false;
        		dev0->SetInterruptionInformation(P2PInterruptionType::Predictable, false);
        		dev1->SetInterruptionInformation(P2PInterruptionType::Predictable, false);
        	}
        	else if((*iter).second.second == true && !regularOrNot){
        		(*iter).second.second = false;
        		dev0->SetInterruptionInformation(P2PInterruptionType::Unpredictable, false);
        		dev1->SetInterruptionInformation(P2PInterruptionType::Unpredictable, false);
        	}

        	// if link interruption still exists
        	if((*iter).second.first || (*iter).second.second){
        		return;
        	}

        	// if link interruption recovers
        	if(dev0 != nullptr && dev1 != nullptr)
            {
        		// set interruption information
				dev0->SetInterruptionInformation(P2PInterruptionType::Predictable, false);
				dev1->SetInterruptionInformation(P2PInterruptionType::Predictable, false);
				dev0->SetInterruptionInformation(P2PInterruptionType::Unpredictable, false);
				dev1->SetInterruptionInformation(P2PInterruptionType::Unpredictable, false);

				// do enable
                dev0->SetLinkState(NORMAL);
                dev1->SetLinkState(NORMAL);


                //update m_disableNetDevices
                m_islDisableNetDevices.erase(CalStringKey(satId0,satId1));
                m_islDisableNetDevices.erase(CalStringKey(satId1,satId0));

                //std::cout <<"ISL Re-establishment Time: "<<Simulator::Now().GetMilliSeconds()<<" ms Add: "<<satId0<<"  "<<satId1<<std::endl;
                //std::cout <<"Total Interrupted ISL Number: "<< m_islDisableNetDevices.size()/2 << std::endl;
//                std::ofstream fileTopologyChange(m_satellite_network_dir + "/topology_change_message.txt", std::ofstream::out | std::ofstream::app);
//                fileTopologyChange <<"ISL Re-establishment Time: "<<Simulator::Now().GetMilliSeconds()<<" ms Add: "<<satId0<<"  "<<satId1<<std::endl;
//                fileTopologyChange <<"Total Interrupted ISL Number: "<< m_islDisableNetDevices.size()/2 << std::endl;

                std::ofstream fileTopologyChange(m_satellite_network_dir + "/system_" + std::to_string(m_system_id)+"_topology_change_message_isl.txt", std::ofstream::out | std::ofstream::app);
                fileTopologyChange <<Simulator::Now().GetMilliSeconds()<<","<<satId0<<","<<satId1<<","<<"recover"<<std::endl;
                fileTopologyChange.close();
    	        OutageLink failedLink(satId0, satId1);
    	        auto iter = find(m_sunOutageLinkDetails.begin(), m_sunOutageLinkDetails.end(), failedLink);
    	        NS_ASSERT_MSG (iter != m_sunOutageLinkDetails.end(), "No Sun Outage Link Details Record");
    	        iter->outageTimeInterval[iter->outageTimeInterval.size()-1].second = Simulator::Now().GetSeconds();
            }
            else{
            	throw std::runtime_error ("Wrong device.");
            }

        	return;

        }
        
        std::vector<std::pair<uint32_t, uint32_t>>::iterator iter1 = find(islFromTo.begin(), islFromTo.end(), std::make_pair(satId0, satId1));
        if(iter1 != islFromTo.end()){
        	return;
        }

        // if it is a new link, just create it
        // Link helper
        PointToPointLaserHelper p2p_laser_helper(m_basicSimulation);
        std::string max_queue_size_str = format_string("%" PRId64 "p", m_isl_max_queue_size_pkts);
        p2p_laser_helper.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", QueueSizeValue(QueueSize(max_queue_size_str)));
        p2p_laser_helper.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (std::to_string(m_isl_data_rate_megabit_per_s) + "Mbps")));
//        std::cout << "    >> new ISL data rate........ " << m_isl_data_rate_megabit_per_s << " Mbit/s" << std::endl;
//        std::cout << "    >> new ISL max queue size... " << m_isl_max_queue_size_pkts << " packets" << std::endl;

        // Traffic control helper
        TrafficControlHelper tch_isl;
        tch_isl.SetRootQueueDisc("ns3::FifoQueueDisc", "MaxSize", QueueSizeValue(QueueSize("1p"))); // Will be removed later any case

        // if it is a new link, create it
        DoCreateNetDevice(satId0, satId1, p2p_laser_helper, tch_isl);

        //std::cout <<"New ISL Establishment Time: "<<Simulator::Now().GetMilliSeconds()<<" ms Add: "<<satId0<<"  "<<satId1<<std::endl;
        std::ofstream fileTopologyChange(m_satellite_network_dir + "/system_" + std::to_string(m_system_id)+"_topology_change_message_isl.txt", std::ofstream::out | std::ofstream::app);
        fileTopologyChange <<Simulator::Now().GetMilliSeconds()<<","<<satId0<<","<<satId1<<","<<"new_establish"<<std::endl;
        fileTopologyChange.close();
    }

    void 
    TopologySatelliteNetwork::DisableISLBySatId(uint32_t satId0, uint32_t satId1, bool regularOrNot)
    {
    	Ptr<Constellation> cons1 = FindConstellationBySatId(satId0);
    	Ptr<Constellation> cons2 = FindConstellationBySatId(satId1);
    	if(cons1->GetName() != cons2->GetName()){
    		throw std::runtime_error ("No support to isl between different constellation now");
    	}
    	NetDeviceContainer islNetDevices = FindConstellationBySatId(satId0)->GetIslNetDevicesInfo();


        Ptr<SAGLinkLayer> dev0= islNetDevices.GetWithKey(CalStringKey(satId0,satId1))->GetObject<SAGLinkLayer>();
        Ptr<SAGLinkLayer> dev1= islNetDevices.GetWithKey(CalStringKey(satId1,satId0))->GetObject<SAGLinkLayer>();

    	std::map<std::string, std::pair<bool, bool>>::iterator iter = m_islDisableNetDevices.find(CalStringKey(satId0, satId1));
    	if(regularOrNot){
        	if(iter != m_islDisableNetDevices.end()){
				if((*iter).second.first){

				}
				else{
					(*iter).second.first = true;
	                dev0->SetInterruptionInformation(P2PInterruptionType::Predictable, true);
	                dev1->SetInterruptionInformation(P2PInterruptionType::Predictable, true);
				}
				return;
			}
        }
        else{
        	if(iter != m_islDisableNetDevices.end()){
				if((*iter).second.second){

				}
				else{
					(*iter).second.second = true;
					dev0->SetInterruptionInformation(P2PInterruptionType::Unpredictable, true);
					dev1->SetInterruptionInformation(P2PInterruptionType::Unpredictable, true);
				}
				return;
			}
        }

    	if(dev0 != nullptr && dev1 != nullptr)
        {
            // set interruption type
            if(regularOrNot){
                dev0->SetInterruptionInformation(P2PInterruptionType::Predictable, true);
                dev1->SetInterruptionInformation(P2PInterruptionType::Predictable, true);
            }
            else{
            	dev0->SetInterruptionInformation(P2PInterruptionType::Unpredictable, true);
            	dev1->SetInterruptionInformation(P2PInterruptionType::Unpredictable, true);
            }

    		// do disable
            dev0->SetLinkState(DISABLE);
            dev1->SetLinkState(DISABLE);

//            m_islDisableNetDevices.insert(CalStringKey(satId0,satId1));
//            m_islDisableNetDevices.insert(CalStringKey(satId1,satId0));
			m_islDisableNetDevices[CalStringKey(satId0,satId1)] = std::pair<bool,bool>(regularOrNot, !regularOrNot);
			m_islDisableNetDevices[CalStringKey(satId1,satId0)] = std::pair<bool,bool>(regularOrNot, !regularOrNot);

	        //std::cout <<"ISL Interruption Time: "<<Simulator::Now().GetMilliSeconds()<<" ms Disable: "<<satId0<<"  "<<satId1<<std::endl;
	        //std::cout <<"Total Interrupted ISL Number: "<< m_islDisableNetDevices.size()/2 << std::endl;

	        std::ofstream fileTopologyChange(m_satellite_network_dir + "/system_" + std::to_string(m_system_id)+"_topology_change_message_isl.txt", std::ofstream::out | std::ofstream::app);
	        fileTopologyChange <<Simulator::Now().GetMilliSeconds()<<","<<satId0<<","<<satId1<<","<<"interruption"<<std::endl;
	        fileTopologyChange.close();
	        OutageLink failedLink(satId0, satId1);
	        auto iter = find(m_sunOutageLinkDetails.begin(), m_sunOutageLinkDetails.end(), failedLink);
	        if(iter == m_sunOutageLinkDetails.end()){
	        	failedLink.outageTimeInterval.push_back(std::make_pair(Simulator::Now().GetSeconds(), m_basicSimulation->GetSimulationEndTimeNs()/1e9));
	        	m_sunOutageLinkDetails.push_back(failedLink);
	        }
	        else{
	        	iter->outageTimeInterval.push_back(std::make_pair(Simulator::Now().GetSeconds(), m_basicSimulation->GetSimulationEndTimeNs()/1e9));
	        }
        }
    }

    void
	TopologySatelliteNetwork::NotifyInterfaceAddressChangeToApp(){

    	for(uint32_t i = 0; i < m_groundStationNodes.GetN(); i++){
    		uint32_t nApps = m_groundStationNodes.Get(i)->GetNApplications();
    		for(uint32_t j = 0; j < nApps; j++){
//    			if(m_groundStationNodes.Get(i)->GetApplication(j)->GetInstanceTypeId() == TypeId::LookupByName ("ns3::Bgp")){
//    				continue;
//    			}
//    			if(m_groundStationNodes.Get(i)->GetApplication(j)->GetInstanceTypeId() == TypeId::LookupByName ("ns3::SAGApplicationLayer")){
//        			Ptr<SAGApplicationLayer> app = m_groundStationNodes.Get(i)->GetApplication(j)->GetObject<SAGApplicationLayer>();
//        			app->NotifyAddressChange();
//    			}

    			Ptr<SAGApplicationLayer> app = m_groundStationNodes.Get(i)->GetApplication(j)->GetObject<SAGApplicationLayer>();
    			if(app == nullptr){
    				std::cout<<"TopologySatelliteNetwork::NotifyInterfaceAddressChangeToApp() !!!!!!!!!!!!!!!!!!!!!!"<<std::endl;
    				continue;
    			}
    			app->NotifyAddressChange();
    		}
    	}

    }

    Ptr<NetDevice>
	TopologySatelliteNetwork::GetSatGSLNetDevice(Ptr<Node> sat){
    	for(uint32_t i = 0; i < m_satelliteNodes.GetN(); i++){
			if(m_satelliteNodes.Get(i)->GetId() == sat->GetId()){
				return m_gslSatNetDevices.Get(i);
			}
		}
		return 0;



    }

    Ptr<NetDevice>
	TopologySatelliteNetwork::GetGsGSLNetDevice(Ptr<Node> gs, uint32_t interface){
    	NetDeviceContainer netDevCon = m_gslGsNetDevicesInterface[interface];
    	for(uint32_t i = 0; i < m_groundStationNodes.GetN(); i++){
    		if(m_groundStationNodes.Get(i)->GetId() == gs->GetId()){
    			return netDevCon.Get(i);
    		}
    	}
    	return 0;

    }

    void
    TopologySatelliteNetwork::InitializeGroundStationsArpCaches(Ptr<Node> gs, Ptr<NetDevice> ntDevice) {

		Ptr<ArpCache> arpAll = CreateObject<ArpCache>();
		arpAll->SetAliveTimeout (Seconds(3600 * 24 * 365)); // Valid one year

		Ptr<Ipv4L3Protocol> ipv4 = gs->GetObject<Ipv4L3Protocol>();
		int32_t itf = ipv4->GetInterfaceForDevice(ntDevice);
		NS_ASSERT_MSG (itf != -1, "No device");

//		//################
//		Ptr<NetDevice> dec = ipv4->GetNetDevice(itf);
//		arpAll->SetDevice(dec, ipv4->GetInterface(itf));
//		//################

		// Set a pointer to the ARP cache it should use (will be filled at the end of this function, it's only a pointer)
		ipv4->GetInterface(itf)->SetAttribute("ArpCache", PointerValue(arpAll));

    }

    void
	TopologySatelliteNetwork::InitializeSatellitesArpCaches() {

		// ARP lookups hinder performance, and actually won't succeed, so to prevent that from happening,
		// all GSL interfaces' IPs are added into an ARP cache

		for (uint32_t i = 0; i < m_satelliteNodes.GetN(); i++) {

			// Only needs to be GSL interfaces
			Ptr<ArpCache> arpAll = CreateObject<ArpCache>();
			arpAll->SetAliveTimeout (Seconds(3600 * 24 * 365)); // Valid one year

			Ptr<Ipv4L3Protocol> ipv4 = m_satelliteNodes.Get(i)->GetObject<Ipv4L3Protocol>();
			int32_t itf = ipv4->GetInterfaceForDevice(m_gslSatNetDevices.Get(i));
			NS_ASSERT_MSG (itf != -1, "No device");

//			//################
//			Ptr<NetDevice> dec = ipv4->GetNetDevice(itf);
//			arpAll->SetDevice(dec, ipv4->GetInterface(itf));
//			//################

			// Set a pointer to the ARP cache it should use (will be filled at the end of this function, it's only a pointer)
			ipv4->GetInterface(itf)->SetAttribute("ArpCache", PointerValue(arpAll));

		}

	}

    void
	TopologySatelliteNetwork::ArpCacheEntryAdd(Ptr<Node> gs, Ptr<Node> sat, Ptr<NetDevice> gslGsNetDevice, Ptr<NetDevice> gslSatNetDevice){

    	Ptr<Ipv4L3Protocol> gsL3 = gs->GetObject<Ipv4L3Protocol>();
		Ptr<Ipv4L3Protocol> satL3 = sat->GetObject<Ipv4L3Protocol>();

		int32_t gsIntf = gsL3->GetInterfaceForDevice(gslGsNetDevice);
		int32_t satIntf = satL3->GetInterfaceForDevice(gslSatNetDevice);
		NS_ASSERT_MSG (gsIntf != -1 && satIntf != -1, "No device");

		// One IP address per interface
		if (gsL3->GetNAddresses(gsIntf) != 1 || satL3->GetNAddresses(satIntf) != 1) {
			throw std::runtime_error("Each interface is permitted exactly one IP address.");
		}

    	Mac48Address mac48AddressGs = Mac48Address::ConvertFrom(gslGsNetDevice->GetAddress());
    	Mac48Address mac48AddressSat = Mac48Address::ConvertFrom(gslSatNetDevice->GetAddress());

		Ipv4Address ipv4AddressGs = gsL3->GetAddress(gsIntf, 0).GetLocal();
		Ipv4Address ipv4AddressSat = satL3->GetAddress(satIntf, 0).GetLocal();

		Ptr<ArpCache> arpGs = gsL3->GetInterface(gsIntf)->GetArpCache();
		Ptr<ArpCache> arpSat = satL3->GetInterface(satIntf)->GetArpCache();

		// Add the info of the GSL interface to the cache
		ArpCache::Entry * entryGs = arpGs->Add(ipv4AddressSat);
		entryGs->SetMacAddress(mac48AddressSat);

		ArpCache::Entry * entrySat = arpSat->Add(ipv4AddressGs);
		entrySat->SetMacAddress(mac48AddressGs);


    }

    void
	TopologySatelliteNetwork::ArpCacheEntryDelete(Ptr<Node> gs, Ptr<Node> sat, Ptr<NetDevice> gslGsNetDevice, Ptr<NetDevice> gslSatNetDevice){

    	Ptr<Ipv4L3Protocol> gsL3 = gs->GetObject<Ipv4L3Protocol>();
		Ptr<Ipv4L3Protocol> satL3 = sat->GetObject<Ipv4L3Protocol>();

		int32_t gsIntf = gsL3->GetInterfaceForDevice(gslGsNetDevice);
		int32_t satIntf = satL3->GetInterfaceForDevice(gslSatNetDevice);
		NS_ASSERT_MSG (gsIntf != -1 && satIntf != -1, "No device");

		uint32_t gsItfAdrNum = gsL3->GetNAddresses(gsIntf);
		uint32_t satItfAdrNum = satL3->GetNAddresses(satIntf);
		// One IP address per interface
		if (gsItfAdrNum != 1 || satItfAdrNum != 1) {
			throw std::runtime_error("Each interface is permitted exactly one IP address.");
		}

//		Mac48Address mac48AddressGs = Mac48Address::ConvertFrom(gslGsNetDevice->GetAddress());
//		Mac48Address mac48AddressSat = Mac48Address::ConvertFrom(gslSatNetDevice->GetAddress());

		Ipv4Address ipv4AddressGs = gsL3->GetAddress(gsIntf, 0).GetLocal();
		Ipv4Address ipv4AddressSat = satL3->GetAddress(satIntf, 0).GetLocal();

		Ptr<ArpCache> arpGs = gsL3->GetInterface(gsIntf)->GetArpCache();
		Ptr<ArpCache> arpSat = satL3->GetInterface(satIntf)->GetArpCache();

		// Add the info of the GSL interface to the cache
		ArpCache::Entry * entry1 = arpGs->Lookup(ipv4AddressSat);
		NS_ASSERT_MSG (entry1 != 0, "No entry");
		arpGs->Remove(entry1);

		ArpCache::Entry * entry2 = arpSat->Lookup(ipv4AddressGs);
		NS_ASSERT_MSG (entry2 != 0, "No entry");
		arpSat->Remove(entry2);

	}

    double TopologySatelliteNetwork::GetDistanceFromTo(uint32_t sat1, uint32_t sat2){

    	Ptr<Node> satNode1 = m_allNodes.Get(sat1);
    	Ptr<Node> satNode2 = m_allNodes.Get(sat2);
    	Ptr<MobilityModel> satMobility1 = satNode1->GetObject<MobilityModel>();
    	Ptr<MobilityModel> satMobility2 = satNode2->GetObject<MobilityModel>();
    	return satMobility1->GetDistanceFrom(satMobility2)/1000.0;


    }

    Vector
	TopologySatelliteNetwork::GetAERFromTo (Ptr<const MobilityModel> cur_position, Ptr<const MobilityModel> position) const{
    	Vector oPositionInECI = position->DoGetPositionInECI();
    	Vector positionInECI = cur_position->DoGetPositionInECI ();
    	Vector velocityInECI = cur_position->DoGetVelocityInECI();
    	double rsat[3] = {positionInECI.x, positionInECI.y, positionInECI.z};
    	double vsat[3] = {velocityInECI.x, velocityInECI.y, velocityInECI.z};
    	double rsat2[3] = {oPositionInECI.x, oPositionInECI.y, oPositionInECI.z};
    	Vector aer = CalculateAER(rsat, vsat, rsat2);
    	return aer;
    }

    Vector
	TopologySatelliteNetwork::CalculateAER(double rsat[3], double vsat[3], double rsat2[3]) const{
    	double v1[3] = {vsat[0],vsat[1],vsat[2]};
    	double v2[3] = {rsat[0],rsat[1],rsat[2]};
    	double v3[3] = {rsat2[0] - rsat[0],rsat2[1] - rsat[1],rsat2[2] - rsat[2]};
    	double ct = (-1) * DotProduct(v3, v2) / DotProduct(v2, v2);
    	double v3p[3] = {v3[0] + ct * v2[0],v3[1] + ct * v2[1],v3[2] + ct * v2[2]};
    	double t1 = DotProduct(v1, v3p);
    	double t2 = Norm2(v1) * Norm2(v3p);
    	double temp = t1 / t2;
    	double A = abs(std::acos(temp)*180/pi);
    	double crossp[3];
    	CrossProduct(v3p, v1, crossp);
    	double xres = (crossp[0]*v2[0]+crossp[1]*v2[1]+crossp[2]*v2[2]) / (v2[0]*v2[0]+v2[1]*v2[1]+v2[2]*v2[2]);
    	if(xres < 0){
    		A = 360 - A;
    	}
    	double Az = A;
    	double E = std::acos(DotProduct(v3, v3p)/(Norm2(v3)*Norm2(v3p)))*180/pi;
    	double E1;
    	if(ct > 0){
    		E1 = -E;
    	}
    	else{
    		E1 = E;
    	}
    	double Range = sqrt(pow(rsat[0]-rsat2[0],2)+pow(rsat[1]-rsat2[1],2)+pow(rsat[2]-rsat2[2],2));

    	return Vector(Az, E1, Range);
    }

    double
	TopologySatelliteNetwork::DotProduct(double a[3], double b[3]) const{
    	double res = 0;
    	res += a[0] * b[0];
    	res += a[1] * b[1];
    	res += a[2] * b[2];
    	return res;
    }


    double
	TopologySatelliteNetwork::Norm2(double a[3]) const{
    	double res = 0;
    	res += a[0] * a[0];
    	res += a[1] * a[1];
    	res += a[2] * a[2];
    	return sqrt(res);
    }

    void
	TopologySatelliteNetwork::CrossProduct(double a[3], double b[3], double* res) const{

    	res[0] = a[1]*b[2]-b[1]*a[2];
    	res[1] = -(a[0]*b[2]-b[0]*a[2]);
    	res[2] = a[0]*b[1]-b[0]*a[1];
    }


    void TopologySatelliteNetwork::CollectUtilizationStatistics() {

    	remove_file_if_exists(m_basicSimulation->GetLogsDir() + "/system_" + std::to_string(m_system_id)+ "_isl_utilization.json");
		remove_file_if_exists(m_basicSimulation->GetLogsDir() + "/system_" + std::to_string(m_system_id)+ "_gsl_utilization.json");

        std::vector<double> time_stamp;
		if (m_enable_link_utilization_tracking) {
	        for(uint32_t s = 0; s < ceil(m_time_end/m_link_utilization_tracking_interval_ns); s++){
	            time_stamp.push_back(s * m_link_utilization_tracking_interval_ns);
	        }
		}

		if (m_enable_link_utilization_tracking) {
			nlohmann::ordered_json json_v;
			nlohmann::ordered_json json_v_mlu;
			nlohmann::ordered_json json_v_mlu_cons;
        	for(Ptr<Constellation> cons: m_constellations){
                std::vector<double> mlu(ceil(m_time_end/m_link_utilization_tracking_interval_ns),0);
            	NetDeviceContainer islNetDevices = cons->GetIslNetDevicesInfo();
            	std::vector<std::pair<uint32_t, uint32_t>> islFromTo = cons->GetIslFromTo();
            	// Open CSV file
				//FILE* file_utilization_csv = fopen((m_basicSimulation->GetLogsDir() + "/system_" + std::to_string(m_system_id)+ "_isl_utilization.csv").c_str(), "w+");

				// Go over every ISL network device
				for (size_t i = 0; i < islNetDevices.GetN(); i++) {
					Ptr<SAGLinkLayer> dev = islNetDevices.Get(i)->GetObject<SAGLinkLayer>();
					const std::vector<double> utilization = dev->FinalizeUtilization();
					if(utilization.size() == 0) continue;
					std::pair<uint32_t, uint32_t> src_dst = islFromTo[i];
					int64_t interval_left_side_ns = 0;
					nlohmann::ordered_json jsonObject;
					jsonObject["src"] = src_dst.first;
					jsonObject["dst"] = src_dst.second;
					nlohmann::ordered_json jsonUtiliTotal;

					for (size_t j = 0; j < utilization.size(); j++) {

						if(mlu[j] < utilization[j]){
							mlu[j] = utilization[j];
						}

						if (j == utilization.size() - 1 || utilization[j] != utilization[j + 1]) {

							// Write plain to the CSV file:
							// <src>,<dst>,<interval start (ns)>,<interval end (ns)>,<utilization 0.0-1.0>
//							fprintf(file_utilization_csv,
//									"%d,%d,%" PRId64 ",%" PRId64 ",%f\n",
//									src_dst.first,
//									src_dst.second,
//									interval_left_side_ns,
//									(j + 1) * m_link_utilization_tracking_interval_ns,
//									utilization[j]
//							);
							nlohmann::ordered_json jsonUtili;
							jsonUtili["interval start (ns)"] = interval_left_side_ns;
							jsonUtili["interval end (ns)"] = (j + 1) * m_link_utilization_tracking_interval_ns;
							jsonUtili["utilization"] = utilization[j];
							jsonUtiliTotal.push_back(jsonUtili);

							interval_left_side_ns = (j + 1) * m_link_utilization_tracking_interval_ns;

						}
					}
					jsonObject["utilization_details"] = jsonUtiliTotal;
					json_v.push_back(jsonObject);

				}

				nlohmann::ordered_json cons_mlu;
				cons_mlu["constellation_name"] = cons->GetName();
				cons_mlu["maximum_link_utilization (MLU)"] = mlu;
				json_v_mlu_cons.push_back(cons_mlu);

				// Close CSV file
				//fclose(file_utilization_csv);
        	}
			std::ofstream results(m_basicSimulation->GetLogsDir() + "/system_" + std::to_string(m_system_id)+ "_isl_utilization.json", std::ofstream::out);
			if (results.is_open()) {
				results << json_v.dump(4);  // 使用缩进格式将 JSON 内容写入文件
				results.close();
				//std::cout << "JSON file created successfully." << std::endl;
			} else {
				std::cout << "Failed to create JSON file." << std::endl;
			}

			json_v_mlu["time_stamp (ns)"] = time_stamp;
			json_v_mlu["max_utilization_details"] = json_v_mlu_cons;
			std::ofstream mlu_results(m_basicSimulation->GetLogsDir() + "/system_" + std::to_string(m_system_id)+ "_max_link_utilization.json", std::ofstream::out);
			if (mlu_results.is_open()) {
				mlu_results << json_v_mlu.dump(4);  // 使用缩进格式将 JSON 内容写入文件
				mlu_results.close();
				//std::cout << "JSON file created successfully." << std::endl;
			} else {
				std::cout << "Failed to create JSON file." << std::endl;
			}

        }


        if (m_enable_link_utilization_tracking) {

            // Open CSV file
            //FILE* gsl_file_utilization_csv = fopen((m_basicSimulation->GetLogsDir() + "/system_" + std::to_string(m_system_id)+ "_gsl_utilization.csv").c_str(), "w+");
			nlohmann::ordered_json json_v;
            std::vector<double> throughput_sink(ceil(m_time_end/m_link_utilization_tracking_interval_ns),0);
            // Go over every ISL network device
            for (size_t i = 0; i < m_gslSatNetDevices.GetN(); i++) {
                Ptr<SAGLinkLayerGSL> dev = m_gslSatNetDevices.Get(i)->GetObject<SAGLinkLayerGSL>();
                const std::vector<double> utilization = dev->FinalizeUtilization();
				if(utilization.size() == 0) continue;
                //std::pair<int32_t, int32_t> src_dst = m_islFromTo[i];
                int64_t interval_left_side_ns = 0;
                nlohmann::ordered_json jsonObject;
				jsonObject["src"] = dev->GetNode()->GetId();
				nlohmann::ordered_json jsonUtiliTotal;
                for (size_t j = 0; j < utilization.size(); j++) {

					if (j == utilization.size() - 1 || utilization[j] != utilization[j + 1]) {

						// Write plain to the CSV file:
						// <src>,<dst>,<interval start (ns)>,<interval end (ns)>,<utilization 0.0-1.0>
//						fprintf(gsl_file_utilization_csv,
//								"%d,%" PRId64 ",%" PRId64 ",%f\n",
//								dev->GetNode()->GetId(),
//								interval_left_side_ns,
//								(j + 1) * m_link_utilization_tracking_interval_ns,
//								utilization[j]
//						);
						nlohmann::ordered_json jsonUtili;
						jsonUtili["interval start (ns)"] = interval_left_side_ns;
						jsonUtili["interval end (ns)"] = (j + 1) * m_link_utilization_tracking_interval_ns;
						jsonUtili["utilization"] = utilization[j];
						jsonUtiliTotal.push_back(jsonUtili);

						interval_left_side_ns = (j + 1) * m_link_utilization_tracking_interval_ns;
					}


                    throughput_sink[j] += utilization[j] * dev->GetDataRate();
                }
				jsonObject["utilization_details"] = jsonUtiliTotal;
				json_v.push_back(jsonObject);
            }


            std::vector<double> throughput_send(ceil(m_time_end/m_link_utilization_tracking_interval_ns),0);
            for (size_t i = 0; i < m_gslGsNetDevices.GetN(); i++) {
                Ptr<SAGLinkLayerGSL> dev = m_gslGsNetDevices.Get(i)->GetObject<SAGLinkLayerGSL>();
                const std::vector<double> utilization = dev->FinalizeUtilization();
				if(utilization.size() == 0) continue;
                //std::pair<int32_t, int32_t> src_dst = m_islFromTo[i];
                int64_t interval_left_side_ns = 0;
                nlohmann::ordered_json jsonObject;
				jsonObject["src"] = dev->GetNode()->GetId();
				nlohmann::ordered_json jsonUtiliTotal;
                for (size_t j = 0; j < utilization.size(); j++) {
                	if (j == utilization.size() - 1 || utilization[j] != utilization[j + 1]) {
						// Write plain to the CSV file:
						// <src>,<dst>,<interval start (ns)>,<interval end (ns)>,<utilization 0.0-1.0>
//						fprintf(gsl_file_utilization_csv,
//								"%d,%" PRId64 ",%" PRId64 ",%f\n",
//								dev->GetNode()->GetId(),
//								interval_left_side_ns,
//								(j + 1) * m_link_utilization_tracking_interval_ns,
//								utilization[j]
//						);
                		nlohmann::ordered_json jsonUtili;
						jsonUtili["interval start (ns)"] = interval_left_side_ns;
						jsonUtili["interval end (ns)"] = (j + 1) * m_link_utilization_tracking_interval_ns;
						jsonUtili["utilization"] = utilization[j];
						jsonUtiliTotal.push_back(jsonUtili);
						interval_left_side_ns = (j + 1) * m_link_utilization_tracking_interval_ns;

                	}
                    throughput_send[j] += utilization[j] * dev->GetDataRate();

                }
				jsonObject["utilization_details"] = jsonUtiliTotal;
				json_v.push_back(jsonObject);
            }
			std::ofstream results(m_basicSimulation->GetLogsDir() + "/system_" + std::to_string(m_system_id)+ "_gsl_utilization.json", std::ofstream::out);
			if (results.is_open()) {
				results << json_v.dump(4);  // 使用缩进格式将 JSON 内容写入文件
				results.close();
				//std::cout << "JSON file created successfully." << std::endl;
			} else {
				std::cout << "Failed to create JSON file." << std::endl;
			}

            // write throughput.json
			nlohmann::ordered_json jsonObject2;
			jsonObject2["throughput_send_bps"] = throughput_send;
			jsonObject2["throughput_sink_bps"] = throughput_sink;
			jsonObject2["time_stamp_ns"] = time_stamp;
			std::ofstream throughput(m_basicSimulation->GetRunDir() + "/logs_ns3/system_"+std::to_string(m_system_id)+"_network_wide_throughput.json", std::ofstream::out);
			if (throughput.is_open()) {
				throughput << jsonObject2.dump(4);  // 使用缩进格式将 JSON 内容写入文件
				throughput.close();
				//std::cout << "JSON file created successfully." << std::endl;
			} else {
				std::cout << "Failed to create JSON file." << std::endl;
			}

//            if(!m_enable_distributed){
//                // write throughput.json
//    			nlohmann::ordered_json jsonObject2;
//    			jsonObject2["throughput_send_bps"] = throughput_send;
//    			jsonObject2["throughput_sink_bps"] = throughput_sink;
//    			jsonObject2["time_stamp_ns"] = time_stamp;
//    			std::ofstream throughput(m_basicSimulation->GetRunDir() + "/results/network_results/global_statistics/network_wide_throughput.json", std::ofstream::out);
//    			if (throughput.is_open()) {
//    				throughput << jsonObject2.dump(4);  // 使用缩进格式将 JSON 内容写入文件
//    				throughput.close();
//    				//std::cout << "JSON file created successfully." << std::endl;
//    			} else {
//    				std::cout << "Failed to create JSON file." << std::endl;
//    			}
//            }
//            else{
//                // write throughput.json
//    			nlohmann::ordered_json jsonObject2;
//    			jsonObject2["throughput_send_bps"] = throughput_send;
//    			jsonObject2["throughput_sink_bps"] = throughput_sink;
//    			jsonObject2["time_stamp_ns"] = time_stamp;
//    			std::ofstream throughput(m_basicSimulation->GetRunDir() + "/logs_ns3/system_"+std::to_string(m_system_id)+"_network_wide_throughput.json", std::ofstream::out);
//    			if (throughput.is_open()) {
//    				throughput << jsonObject2.dump(4);  // 使用缩进格式将 JSON 内容写入文件
//    				throughput.close();
//    				//std::cout << "JSON file created successfully." << std::endl;
//    			} else {
//    				std::cout << "Failed to create JSON file." << std::endl;
//    			}
//            }



        }





    }

    std::vector<Ptr<Constellation>> TopologySatelliteNetwork::GetConstellations(){
    	return m_constellations;
    }

    uint32_t TopologySatelliteNetwork::GetNumSatellites() {
        return m_satelliteNodes.GetN();
    }

    uint32_t TopologySatelliteNetwork::GetNumGroundStations() {
        return m_groundStationNodes.GetN();
    }

    const NodeContainer& TopologySatelliteNetwork::GetNodes() {
        return m_allNodes;
    }

    int64_t TopologySatelliteNetwork::GetNumNodes() {
        return m_allNodes.GetN();
    }

    const NodeContainer& TopologySatelliteNetwork::GetSatelliteNodes() {
        return m_satelliteNodes;
    }

    const NodeContainer& TopologySatelliteNetwork::GetGroundStationNodes() {
        return m_groundStationNodes;
    }

    const std::vector<Ptr<GroundStation>>& TopologySatelliteNetwork::GetGroundStations() {
        return m_groundStations;
    }

    const std::vector<Ptr<GroundStation>>& TopologySatelliteNetwork::GetGroundStationsReal() {
        return m_groundStationsReal;
    }

    const std::vector<Ptr<GroundStation>>& TopologySatelliteNetwork::GetAirCraftsReal() {
        return m_airCraftsReal;
    }

    const std::vector<std::vector<Vector>>& TopologySatelliteNetwork::GetAirCraftsPositions(){
    	return m_airCraftsPositions;
    }

    const std::vector<Ptr<Satellite>>& TopologySatelliteNetwork::GetSatellites() {
        return m_satellites;
    }

    void TopologySatelliteNetwork::EnsureValidNodeId(uint32_t node_id) {
        if (node_id < 0 || node_id >= m_satellites.size() + m_groundStations.size()) {
            throw std::runtime_error("Invalid node identifier.");
        }
    }

    std::string TopologySatelliteNetwork::CalStringKey(uint32_t satId0, int32_t satId1){
        std::string key="";
        key = std::to_string(satId0) + "_to_" + std::to_string(satId1);
        return key;
    }

    bool TopologySatelliteNetwork::IsSatelliteId(uint32_t node_id) {
        EnsureValidNodeId(node_id);
        return node_id < m_satellites.size();
    }

    bool TopologySatelliteNetwork::IsGroundStationId(uint32_t node_id) {
        EnsureValidNodeId(node_id);
        return node_id >= m_satellites.size() && node_id ;
    }

    const std::vector<std::pair<Ptr<Node>, std::vector<std::pair<uint32_t, Ptr<Node>>>>> TopologySatelliteNetwork:: GetGSLInformation(){
    	return m_gslRecord;
    }

    std::vector<uint32_t> TopologySatelliteNetwork::GetAdjacency(uint32_t cur_sat){
    	Ptr<Constellation> cons = FindConstellationBySatId(cur_sat);
    	std::vector<uint32_t> adjs = cons->GetAdjacency(cur_sat);
    	return adjs;
//    	if(adjs.size() != 0){
//    		return adjs;
//    	}
//    	else{
//    		throw std::runtime_error("Wrong satellite ID for GetAdjacency");
//    	}
    }

    int32_t TopologySatelliteNetwork::GetISLInterfaceNumber(uint32_t satId0, uint32_t satId1){
    	Ptr<Constellation> cons1 = FindConstellationBySatId(satId0);
		Ptr<Constellation> cons2 = FindConstellationBySatId(satId1);
		if(cons1->GetName() != cons2->GetName()){
			throw std::runtime_error ("No support to isl between different constellation now");
		}
		NetDeviceContainer islNetDevices = FindConstellationBySatId(satId0)->GetIslNetDevicesInfo();

    	Ptr<SAGLinkLayer> dev0= islNetDevices.GetWithKey(CalStringKey(satId0,satId1))->GetObject<SAGLinkLayer>();
    	int32_t interfaceNumber = m_satelliteNodes.Get(satId0)->GetObject<Ipv4>()->GetInterfaceForDevice(dev0);
    	if(interfaceNumber != -1){
    		return interfaceNumber;
    	}
    	else{
    		throw std::runtime_error("TopologySatelliteNetwork::GetISLInterfaceNumber");
    	}
    }

    const Ptr<Satellite> TopologySatelliteNetwork::GetSatellite(uint32_t satellite_id) {
        if (satellite_id >= m_satellites.size()) {
            throw std::runtime_error("Cannot retrieve satellite with an invalid satellite ID");
        }
        Ptr<Satellite> satellite = m_satellites.at(satellite_id);
        return satellite;
    }

    uint32_t TopologySatelliteNetwork::NodeToGroundStationId(uint32_t node_id) {
        EnsureValidNodeId(node_id);
        return node_id - GetNumSatellites();
    }

    bool TopologySatelliteNetwork::IsValidEndpoint(int64_t node_id) {
        return m_endpoints.find(node_id) != m_endpoints.end();
    }

    const std::set<int64_t>& TopologySatelliteNetwork::GetEndpoints() {
        return m_endpoints;
    }

    Ptr<Constellation>
    TopologySatelliteNetwork::FindConstellationBySatId(uint32_t satId){
    	for(Ptr<Constellation> cons: m_constellations){
    		if(cons->SatelliteBelongTo(satId)){
    			return cons;
    		}
    	}
    	throw std::runtime_error("Wrong satellite ID for FindConstellationBySatId");
    	return nullptr;
    }

    NodeContainer
	TopologySatelliteNetwork::GetCurrentSystemNodes(){
    	return m_nodesCurSystem;
    }

    NodeContainer
	TopologySatelliteNetwork::GetCurrentSystemGsNodes(){
    	return m_nodesGsCurSystem;
    }

    const std::vector<std::pair<Ptr<Node>, std::vector<std::pair<uint32_t, Ptr<Node>>>>> TopologySatelliteNetwork:: GetPastGSLInformation(){
		return m_gslRecordCopy;
	}

    Ptr<SwitchStrategyGSL> TopologySatelliteNetwork::GetGSLSwitch(){
    	return m_switchStrategy;
    }

    std::vector<OutageLink> const& TopologySatelliteNetwork::GetInterSatelliteOutageLinkDetails(){
    	return m_sunOutageLinkDetails;
    }

    std::vector<ConnectionLink> const& TopologySatelliteNetwork::GetSatellite2GroundConnectionLinkDetails(){
    	return m_gsLinkDetails;
    }

}
