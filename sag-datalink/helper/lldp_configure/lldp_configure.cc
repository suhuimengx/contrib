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

#include "lldp_configure.h"
#include "ns3/sag_lldp.h"


namespace ns3
{

NS_LOG_COMPONENT_DEFINE("LLDPConfigure");
NS_OBJECT_ENSURE_REGISTERED(LLDPConfigure);

TypeId LLDPConfigure::GetTypeId (void)
{
	static TypeId tid = TypeId("ns3::LLDPConfigure")
		.SetParent<Object>()
		.SetGroupName("LLDPConfigure")
		.AddConstructor<LLDPConfigure>()
	;
	return tid;
}

LLDPConfigure::LLDPConfigure() {

}

LLDPConfigure::~LLDPConfigure() {

}

LLDPConfigure::LLDPConfigure(Ptr<BasicSimulation> basicSimulation, Ptr<TopologySatelliteNetwork> topology) {

    printf("PROTOCOL CONFIGURATION LLDP\n");
    m_basicSimulation = basicSimulation;
    // Check if it is enabled explicitly
    m_enabled = parse_boolean(basicSimulation->GetConfigParamOrDefault("enable_lldp", "false"));
	if (!m_enabled) {
		std::cout << "  > Not enabled explicitly for lldp, so disabled" << std::endl;
	}
	else{
		m_satelliteNodes = topology->GetSatelliteNodes();
		/*			for(uint32_t i = 0; i < m_allNodes.GetN(); i++)
		{
			Ptr<Node> p = m_allNodes.Get(i);
			uint32_t if_no = p->GetObject<Ipv4>()->GetNInterfaces();//每个卫星4个接口,1个回环接口
			Ptr<SAGLLDP> lldp_protocol = CreateObject<SAGLLDP>(p,if_no);
			p->AggregateObject(lldp_protocol);
		}*/

		//Only for Test
		for(uint32_t i = 0; i < 2; i++)
		{
			Ptr<Node> p = m_satelliteNodes.Get(i);
			Ptr<SAGLLDP> lldp_protocol = CreateObject<SAGLLDP>(p, 1);
			p->AggregateObject(lldp_protocol);

		}

	}

    std::cout << std::endl;





}

}



