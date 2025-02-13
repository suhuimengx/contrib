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

#include "addressing_rule.h"

namespace ns3 {
    NS_OBJECT_ENSURE_REGISTERED (AddressingRule);

    TypeId
	AddressingRule::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::AddressingRule")
                .SetParent<Object>()
                .SetGroupName("SatelliteNetwork")
                .AddConstructor<AddressingRule>()
				;
        return tid;
    }

    AddressingRule::AddressingRule (){

    }

    AddressingRule::~AddressingRule (){

	}

    void
	AddressingRule::AssignInterSatelliteInterfaceAddress(NetDeviceContainer netDevices){

    }

    void
	AddressingRule::AssignSatelliteToGroundInterfaceAddress(NetDeviceContainer netDevices){

    }

    void
	AddressingRule::AssignGroundToSatelliteInterfaceAddress(uint32_t satId, NetDeviceContainer netDevices){

    }

    void
	AddressingRule::SetTopologyHandle(std::vector<Ptr<Constellation>> constellations, NodeContainer& groundStations, Ptr<SwitchStrategyGSL> switchStrategy){

    	m_constellations = constellations;
    	m_groundStations = groundStations;
    	m_switchStrategy = switchStrategy;

    }

    void
	AddressingRule::SetBasicSimHandle(Ptr<BasicSimulation> basicSimulation){

    }

    //-----------------------------------------------------------------------------
    // Addressing In IPv4
    //-----------------------------------------------------------------------------

    NS_OBJECT_ENSURE_REGISTERED (AddressingInIPv4);

    TypeId
	AddressingInIPv4::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::AddressingInIPv4")
                .SetParent<Object>()
                .SetGroupName("SatelliteNetwork")
                .AddConstructor<AddressingInIPv4>()
				;
        return tid;
    }

    AddressingInIPv4::AddressingInIPv4 (){

    }

    AddressingInIPv4::~AddressingInIPv4 (){

	}

    void
	AddressingInIPv4::AssignInterSatelliteInterfaceAddress(NetDeviceContainer netDevices){

    }

    void
	AddressingInIPv4::AssignSatelliteToGroundInterfaceAddress(NetDeviceContainer netDevices){

    }

    void
	AddressingInIPv4::AssignGroundToSatelliteInterfaceAddress(uint32_t satId, NetDeviceContainer netDevices){

    }

    void
	AddressingInIPv4::AssignGroundToSatelliteInterfaceAddress(uint32_t satId, NetDeviceContainer netDevices, Ipv4InterfaceAddress& adr){

    }

    void
	AddressingInIPv4::SetBasicSimHandle(Ptr<BasicSimulation> basicSimulation){
    	m_basicSimulation = basicSimulation;
    	//<! ip_global_attribute.json
		std::string filename = m_basicSimulation->GetRunDir() + "/config_protocol/ip_global_attribute.json";

		// Check that the file exists
		if (!file_exists(filename)) {
			throw std::runtime_error(format_string("File %s does not exist.", filename.c_str()));
		}
		else{
			ifstream jfile(filename);
			if (jfile) {
				json j;
				jfile >> j;
				std::string network = remove_start_end_double_quote_if_present(trim(j["addressing"]["network_part"]));
				m_network = Ipv4Address(network.c_str());
				std::string mask = remove_start_end_double_quote_if_present(trim(j["addressing"]["address_mask"]));
				m_mask = Ipv4Mask(mask.c_str());
				std::string base = remove_start_end_double_quote_if_present(trim(j["addressing"]["host_part_to_start_from"]));
				m_base = Ipv4Address(base.c_str());

			}
			else{
				throw std::runtime_error(format_string("File %s could not be read.", filename.c_str()));
			}

			jfile.close();
		}
        // IP helper
        m_ipv4_helper.SetBase (m_network, m_mask);
    }

    //-----------------------------------------------------------------------------
    // Addressing In IPv4
    //-----------------------------------------------------------------------------


    //-----------------------------------------------------------------------------
    // Addressing In IPv6
    //-----------------------------------------------------------------------------

    NS_OBJECT_ENSURE_REGISTERED (AddressingInIPv6);

    TypeId
	AddressingInIPv6::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::AddressingInIPv6")
                .SetParent<Object>()
                .SetGroupName("SatelliteNetwork")
                .AddConstructor<AddressingInIPv6>()
				;
        return tid;
    }

    AddressingInIPv6::AddressingInIPv6 (){

    }

    AddressingInIPv6::~AddressingInIPv6 (){

	}

    void
	AddressingInIPv6::AssignInterSatelliteInterfaceAddress(NetDeviceContainer netDevices){

    }

    void
	AddressingInIPv6::AssignSatelliteToGroundInterfaceAddress(NetDeviceContainer netDevices){

    }

    void
	AddressingInIPv6::AssignGroundToSatelliteInterfaceAddress(uint32_t satId, NetDeviceContainer netDevices){

    }

    void
	AddressingInIPv6::AssignGroundToSatelliteInterfaceAddress(uint32_t satId, NetDeviceContainer netDevices, Ipv6InterfaceAddress& adr){

    }

	void
	AddressingInIPv6::SetBasicSimHandle(Ptr<BasicSimulation> basicSimulation){

	}

    //-----------------------------------------------------------------------------
    // Addressing In IPv6
    //-----------------------------------------------------------------------------

    //-----------------------------------------------------------------------------
    // TraditionalAddressing In IPv4
    //-----------------------------------------------------------------------------

    NS_OBJECT_ENSURE_REGISTERED (TraditionalAddressing);

    TypeId
	TraditionalAddressing::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::TraditionalAddressing")
                .SetParent<Object>()
                .SetGroupName("SatelliteNetwork")
                .AddConstructor<TraditionalAddressing>()
				;
        return tid;
    }

    TraditionalAddressing::TraditionalAddressing ()
    :AddressingInIPv4::AddressingInIPv4 (){

    }

    TraditionalAddressing::~TraditionalAddressing (){

	}

    void
	TraditionalAddressing::AssignInterSatelliteInterfaceAddress(NetDeviceContainer netDevices){

        // Assign some IP address (nothing smart, no aggregation, just some IP address)
        m_ipv4_helper.Assign(netDevices);
        m_ipv4_helper.NewNetwork();
    }

    void
	TraditionalAddressing::AssignSatelliteToGroundInterfaceAddress(NetDeviceContainer netDevices){

    	m_ipv4_helper.Assign(netDevices);
		Ipv4Address networkNumber = m_ipv4_helper.GetNetwork();
		m_ipv4_helper.NewNetwork();
		Ipv4AddressHelper curAdrHelper(networkNumber, m_mask, "0.0.0.2"); // "0.0.0.1" is allocated to satellite
		m_gsl_ipv4_addressing_helper[netDevices.Get(0)->GetNode()->GetId()] = curAdrHelper;

    }

    void
	TraditionalAddressing::AssignGroundToSatelliteInterfaceAddress(uint32_t satId, NetDeviceContainer netDevices){

    	m_gsl_ipv4_addressing_helper[satId].Assign(netDevices);

    }

    void
	TraditionalAddressing::AssignGroundToSatelliteInterfaceAddress(uint32_t satId, NetDeviceContainer netDevices, Ipv4InterfaceAddress& adr){

    	adr = Ipv4InterfaceAddress (m_gsl_ipv4_addressing_helper[satId].NewAddress(), m_mask);

    }

    //-----------------------------------------------------------------------------
    // TraditionalAddressing In IPv4
    //-----------------------------------------------------------------------------

    //-----------------------------------------------------------------------------
    // TraditionalAddressingGroundSameNetworkSegment In IPv4
    //-----------------------------------------------------------------------------

    NS_OBJECT_ENSURE_REGISTERED (TraditionalAddressingGroundSameNetworkSegment);

    TypeId
	TraditionalAddressingGroundSameNetworkSegment::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::TraditionalAddressingGroundSameNetworkSegment")
                .SetParent<Object>()
                .SetGroupName("SatelliteNetwork")
                .AddConstructor<TraditionalAddressingGroundSameNetworkSegment>()
				;
        return tid;
    }

    TraditionalAddressingGroundSameNetworkSegment::TraditionalAddressingGroundSameNetworkSegment ()
    :AddressingInIPv4::AddressingInIPv4 (){

    }

    TraditionalAddressingGroundSameNetworkSegment::~TraditionalAddressingGroundSameNetworkSegment (){

	}

    void
	TraditionalAddressingGroundSameNetworkSegment::AssignInterSatelliteInterfaceAddress(NetDeviceContainer netDevices){

        // Assign some IP address (nothing smart, no aggregation, just some IP address)
        m_ipv4_helper.Assign(netDevices);
        m_ipv4_helper.NewNetwork();
    }

    void
	TraditionalAddressingGroundSameNetworkSegment::AssignSatelliteToGroundInterfaceAddress(NetDeviceContainer netDevices){
    	m_ipv4_helper.Assign(netDevices);
    }

    void
	TraditionalAddressingGroundSameNetworkSegment::AssignGroundToSatelliteInterfaceAddress(uint32_t satId, NetDeviceContainer netDevices){
    	m_ipv4_helper.Assign(netDevices);

    	Ptr<NetDevice> gslGsNetDevice = netDevices.Get(0);
    	Ptr<Node> gs = gslGsNetDevice->GetNode();
		Ptr<Ipv4L3Protocol> ipv4 = gs->GetObject<Ipv4L3Protocol>();
		int32_t itf = ipv4->GetInterfaceForDevice(gslGsNetDevice);
		NS_ASSERT_MSG (itf != -1, "No device");
		uint32_t nadr = ipv4->GetInterface(itf)->GetNAddresses();
		NS_ASSERT_MSG (nadr == 1, "One interface one address permitted");
		m_adr[gs->GetId()] = ipv4->GetInterface(itf)->GetAddress(0);
    }

    void
	TraditionalAddressingGroundSameNetworkSegment::AssignGroundToSatelliteInterfaceAddress(uint32_t satId, NetDeviceContainer netDevices, Ipv4InterfaceAddress& adr){

    	Ptr<NetDevice> gslGsNetDevice = netDevices.Get(0);
    	Ptr<Node> gs = gslGsNetDevice->GetNode();
    	auto iter = m_adr.find(gs->GetId());
		NS_ASSERT_MSG (iter != m_adr.end(), "No address");
    	adr = m_adr[gs->GetId()];
    }

    //-----------------------------------------------------------------------------
    // TraditionalAddressing In IPv4
    //-----------------------------------------------------------------------------




    //-----------------------------------------------------------------------------
    // TraditionalAddressing In IPv6
    //-----------------------------------------------------------------------------

    NS_OBJECT_ENSURE_REGISTERED (TraditionalAddressingIPv6);

    TypeId
	TraditionalAddressingIPv6::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::TraditionalAddressingIPv6")
                .SetParent<Object>()
                .SetGroupName("SatelliteNetwork")
                .AddConstructor<TraditionalAddressingIPv6>()
				;
        return tid;
    }

    TraditionalAddressingIPv6::TraditionalAddressingIPv6 (){
        // IP helper
        m_ipv6_helper.SetBase (m_network, m_prefix);
    }

    TraditionalAddressingIPv6::~TraditionalAddressingIPv6 (){

	}

    void
	TraditionalAddressingIPv6::AssignInterSatelliteInterfaceAddress(NetDeviceContainer netDevices){

        // Assign some IP address (nothing smart, no aggregation, just some IP address)
        m_ipv6_helper.Assign(netDevices);
        m_ipv6_helper.NewNetwork();
    }

    void
	TraditionalAddressingIPv6::AssignSatelliteToGroundInterfaceAddress(NetDeviceContainer netDevices){

    	m_ipv6_helper.Assign(netDevices);
		Ipv6Address networkNumber = m_ipv6_helper.GetNetwork();
		m_ipv6_helper.NewNetwork();
		Ipv6AddressHelper curAdrHelper(networkNumber, m_prefix, "::2"); // "::1" is allocated to satellite
		m_gsl_ipv6_addressing_helper[netDevices.Get(0)->GetNode()->GetId()] = curAdrHelper;

    }

    void
	TraditionalAddressingIPv6::AssignGroundToSatelliteInterfaceAddress(uint32_t satId, NetDeviceContainer netDevices){

    	m_gsl_ipv6_addressing_helper[satId].Assign(netDevices);

    }

    void
	TraditionalAddressingIPv6::AssignGroundToSatelliteInterfaceAddress(uint32_t satId, NetDeviceContainer netDevices, Ipv6InterfaceAddress& adr){

    	adr = Ipv6InterfaceAddress (m_gsl_ipv6_addressing_helper[satId].NewAddress(), m_prefix);

    }

    //-----------------------------------------------------------------------------
    // TraditionalAddressing In IPv6
    //-----------------------------------------------------------------------------


    //-----------------------------------------------------------------------------
    // GeographicAddressing In IPv6
    //-----------------------------------------------------------------------------

    NS_OBJECT_ENSURE_REGISTERED (GeographicAddressing);

    TypeId
	GeographicAddressing::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::GeographicAddressing")
                .SetParent<Object>()
                .SetGroupName("SatelliteNetwork")
                .AddConstructor<GeographicAddressing>()
				;
        return tid;
    }

    GeographicAddressing::GeographicAddressing (){

    }

    GeographicAddressing::~GeographicAddressing (){

	}

    void
	GeographicAddressing::AssignInterSatelliteInterfaceAddress(NetDeviceContainer netDevices){

    }

    void
	GeographicAddressing::AssignSatelliteToGroundInterfaceAddress(NetDeviceContainer netDevices){

    }

    void
	GeographicAddressing::AssignGroundToSatelliteInterfaceAddress(uint32_t satId, NetDeviceContainer netDevices){

    }

    //-----------------------------------------------------------------------------
    // GeographicAddressing In IPv6
    //-----------------------------------------------------------------------------




}




