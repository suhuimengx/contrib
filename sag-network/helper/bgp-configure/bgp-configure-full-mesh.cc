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

#include "bgp-configure-full-mesh.h"

namespace ns3
{
  TypeId BgpConfigureFullMesh::GetTypeId (void) {
      static TypeId tid = TypeId("ns3::BgpConfigureFullMesh")
          .SetParent<BgpConfigure>()
          .SetGroupName("BgpConfigure")
          .AddConstructor<BgpConfigureFullMesh>()
   	  ;
      return tid;
  }

  BgpConfigureFullMesh::BgpConfigureFullMesh(){

  }

  BgpConfigureFullMesh::BgpConfigureFullMesh(Ptr<BasicSimulation> basicSimulation, Ptr<TopologySatelliteNetwork> topology)
  {
      m_enable_bgp = parse_boolean(basicSimulation->GetConfigParamOrFail("enable_border_gateway_protocol"));

      if (m_enable_bgp) {
		  std::cout << "Bgp Configure" << std::endl;
		  std::cout << "  > iBGP : Full Mesh" << std::endl;
		  m_topology = topology;
		  m_basicSimulation = basicSimulation;
		  m_simulation_end_time_ns = m_basicSimulation->GetSimulationEndTimeNs();
		  m_dynamicStateUpdateIntervalNs = parse_positive_int64(m_basicSimulation->GetConfigParamOrFail("dynamic_state_update_interval_ns"));
		  //std::cout << "  > IBGP Installing" << std::endl;
		  InstallIbgpPeer();
		  //std::cout << "  > EBGP Installing" << std::endl;
		  InstallEbgpPeer(0);
      }
  }

  void BgpConfigureFullMesh::InstallIbgpPeer()
  {

	for (uint32_t i = 0; i < m_topology->GetNumSatellites(); i++)	{

	  Ptr<Bgp> bgp_app_sat = CreateObject<Bgp>();
	  bgp_app_sat->SetAttribute("RouterID", Ipv4AddressValue(m_topology->GetSatelliteNodes().Get(i)->GetObject<Ipv4L3Protocol>()->GetAddress(0,1).GetLocal()));
	  bgp_app_sat->SetAttribute("LibbgpLogLevel", EnumValue(libbgp::DEBUG));
	  bgp_app_sat->SetStartTime(NanoSeconds(0));
	  bgp_app_sat->SetStopTime(NanoSeconds(m_simulation_end_time_ns));
	  bgp_app_sat->SetSimulation(m_simulation_end_time_ns,m_dynamicStateUpdateIntervalNs);

	  m_topology->GetSatelliteNodes().Get(i)->AddApplication(bgp_app_sat);

	  for (uint32_t j = 0; j < m_topology->GetNumSatellites(); j++)	{

		  if(j==i) {
			  continue;
		  }

		  if(j<i) {
		  Peer bgp_app_peer;
		  bgp_app_peer.peer_address = m_topology->GetSatelliteNodes().Get(j)->GetObject<Ipv4L3Protocol>()->GetAddress(0,1).GetLocal();
		  bgp_app_peer.local_address = m_topology->GetSatelliteNodes().Get(i)->GetObject<Ipv4L3Protocol>()->GetAddress(0,1).GetLocal();
		  bgp_app_peer.peer_asn = 1;
		  bgp_app_peer.local_asn = 1; // All satellites are in AS1
		  bgp_app_peer.passive = true;
		  bgp_app_peer.ibgp_alter_nexthop = true; // Every satellite can be an AS border router
		  bgp_app_sat->AddPeer(bgp_app_peer);
		  }

		  if(j>i) {
		  Peer bgp_app_peer;
		  bgp_app_peer.peer_address = m_topology->GetSatelliteNodes().Get(j)->GetObject<Ipv4L3Protocol>()->GetAddress(0,1).GetLocal();
		  bgp_app_peer.local_address = m_topology->GetSatelliteNodes().Get(i)->GetObject<Ipv4L3Protocol>()->GetAddress(0,1).GetLocal();
		  bgp_app_peer.peer_asn = 1;
		  bgp_app_peer.local_asn = 1; // All satellites are in AS1
		  bgp_app_peer.passive = false;  // Every satellite can send OPEN
		  bgp_app_peer.ibgp_alter_nexthop = true; // Every satellite can be an AS border router
		  bgp_app_sat->AddPeer(bgp_app_peer);
		  }
	  }
	}

	for (uint32_t i = 0; i < m_topology->GetNumGroundStations(); i++)	{

	  Ptr<Bgp> bgp_app_gs = CreateObject<Bgp>();
	  bgp_app_gs->SetAttribute("RouterID", Ipv4AddressValue(m_topology->GetGroundStationNodes().Get(i)->GetObject<Ipv4L3Protocol>()->GetAddress(0,1).GetLocal()));
	  bgp_app_gs->SetAttribute("LibbgpLogLevel", EnumValue(libbgp::DEBUG));
	  bgp_app_gs->SetStartTime(NanoSeconds(0));
	  bgp_app_gs->SetStopTime(NanoSeconds(m_simulation_end_time_ns));
	  bgp_app_gs->SetSimulation(m_simulation_end_time_ns,m_dynamicStateUpdateIntervalNs);
	  m_topology->GetGroundStationNodes().Get(i)->AddApplication(bgp_app_gs);

	  //MakeGndAddRoute(m_topology->GetGroundStationNodes().Get(i));
	}

  }

  void BgpConfigureFullMesh::InstallEbgpPeer(double time)
  {
		// for initialization
		if(m_topology->GetPastGSLInformation().empty()){
			for(uint32_t p = 0; p < m_topology->GetGSLInformation().size(); p++){
				if(m_topology->GetGSLInformation().at(p).second[0].second != nullptr){ //todo
					auto gnd = m_topology->GetGSLInformation().at(p).first;
					auto sat = m_topology->GetGSLInformation().at(p).second[0].second; //todo

					AddEbgpByGndAndSat(gnd, sat);
				}
			}
		}
		else
		{
			// for eBGP switching
			for(uint32_t p = 0; p < m_topology->GetGSLInformation().size(); p++){

				auto gnd = m_topology->GetGSLInformation().at(p).first;
				auto sat = m_topology->GetGSLInformation().at(p).second[0].second; //todo
				auto gndPast = m_topology->GetPastGSLInformation().at(p).first;
				auto satPast = m_topology->GetPastGSLInformation().at(p).second[0].second; //todo

				if(gnd != nullptr && gndPast != nullptr && gnd->GetId() !=  gndPast->GetId()){
					throw std::runtime_error("Process Wrong: BgpConfigureFullMesh::InstallEbgpPeer");
				}
				if(sat == nullptr && satPast != nullptr){

					DisableEbgpByGndAndSat(gndPast, satPast);
				}
				else if(sat == nullptr && satPast == nullptr){

				}
				else if(sat != nullptr && satPast == nullptr){

					AddEbgpByGndAndSat(gnd, sat);
				}
				else{
					if(sat->GetId() == satPast->GetId()){

						}
						else if(sat->GetId() != satPast->GetId()){

//							GetBgpApplication(m_topology->GetGroundStationNodes().Get(0))->PrintNumRib4(GetBgpApplication(m_topology->GetGroundStationNodes().Get(0))->GetBgpId());
//							GetBgpApplication(m_topology->GetGroundStationNodes().Get(1))->PrintNumRib4(GetBgpApplication(m_topology->GetGroundStationNodes().Get(1))->GetBgpId());

							DisableEbgpByGndAndSat(gndPast, satPast);

//							GetBgpApplication(m_topology->GetGroundStationNodes().Get(0))->PrintNumRib4(GetBgpApplication(m_topology->GetGroundStationNodes().Get(0))->GetBgpId());
//							GetBgpApplication(m_topology->GetGroundStationNodes().Get(1))->PrintNumRib4(GetBgpApplication(m_topology->GetGroundStationNodes().Get(1))->GetBgpId());

							AddEbgpByGndAndSat(gnd, sat);

//							GetBgpApplication(m_topology->GetGroundStationNodes().Get(0))->PrintNumRib4(GetBgpApplication(m_topology->GetGroundStationNodes().Get(0))->GetBgpId());
//							GetBgpApplication(m_topology->GetGroundStationNodes().Get(1))->PrintNumRib4(GetBgpApplication(m_topology->GetGroundStationNodes().Get(1))->GetBgpId());
						}
					}
			 }
		}

		//GetBgpApplication(m_topology->GetGroundStationNodes().Get(0))->PrintNumRib4(GetBgpApplication(m_topology->GetGroundStationNodes().Get(0))->GetBgpId());
		//GetBgpApplication(m_topology->GetGroundStationNodes().Get(1))->PrintNumRib4(GetBgpApplication(m_topology->GetGroundStationNodes().Get(1))->GetBgpId());

		// Plan the next update
		int64_t next_update_ns = time + m_dynamicStateUpdateIntervalNs + 1;

		if (next_update_ns < m_simulation_end_time_ns){
				Simulator::Schedule(NanoSeconds(m_dynamicStateUpdateIntervalNs),
						&BgpConfigureFullMesh::InstallEbgpPeer, this, next_update_ns);

		}
  }

  void BgpConfigureFullMesh::AddEbgpByGndAndSat(Ptr<Node> gnd, Ptr<Node> sat)
  {
		Peer bgp_app_peer_sat;
		bgp_app_peer_sat.peer_address = gnd->GetObject<Ipv4L3Protocol>()->GetAddress(1,0).GetLocal();
		bgp_app_peer_sat.local_address = sat->GetObject<Ipv4L3Protocol>()->GetAddress(5,0).GetLocal();
		bgp_app_peer_sat.peer_asn = gnd->GetId()-m_topology->GetNumSatellites()+2;
		bgp_app_peer_sat.local_asn = 1;
		bgp_app_peer_sat.passive = false;
		GetBgpApplication(sat)->AddPeer(bgp_app_peer_sat);

		Peer bgp_app_peer_gnd;
		bgp_app_peer_gnd.peer_address = sat->GetObject<Ipv4L3Protocol>()->GetAddress(5,0).GetLocal();
		bgp_app_peer_gnd.local_address = gnd->GetObject<Ipv4L3Protocol>()->GetAddress(1,0).GetLocal();
		bgp_app_peer_gnd.peer_asn = 1;
		bgp_app_peer_gnd.local_asn = gnd->GetId()-m_topology->GetNumSatellites()+2;
		bgp_app_peer_gnd.passive = true;
		GetBgpApplication(gnd)->AddPeer(bgp_app_peer_gnd);

		//std::cout << " > Add eBGP between sat " << sat->GetId()<< " and gnd "<< gnd->GetId() << std::endl;

	    MakeGndAddRoute(gnd);
  }

  void BgpConfigureFullMesh::DisableEbgpByGndAndSat(Ptr<Node> gnd, Ptr<Node> sat)
  {
	  for(uint32_t i = 0; i < GetBgpApplication(sat)->GetNumPeer(); i++){
		  if(GetBgpApplication(sat)->GetPeer(i)->peer_address == gnd->GetObject<Ipv4L3Protocol>()->GetAddress(1,0).GetLocal()) {
			 GetBgpApplication(sat)->DeletePeer(i);
		  }
	  }

	  for(uint32_t i = 0; i < GetBgpApplication(gnd)->GetNumPeer(); i++){
		  if(GetBgpApplication(gnd)->GetPeer(i)->peer_address == sat->GetObject<Ipv4L3Protocol>()->GetAddress(5,0).GetLocal()) {
			 GetBgpApplication(gnd)->DeletePeer(i);
		  }
	  }

	  //std::cout << " > Disable eBGP between sat " << sat->GetId()<< " and gnd "<< gnd->GetId() << std::endl;

	  for (uint32_t i = 0; i < m_topology->GetNumGroundStations(); i++)	{
		//GetBgpApplication(m_topology->GetGroundStationNodes().Get(i))->ClearRib4(GetBgpApplication(gnd)->GetBgpId());
		MakeGndAddRoute(m_topology->GetGroundStationNodes().Get(i));
      }

	  GetBgpApplication(gnd)->ClearRib4(GetBgpApplication(gnd)->GetBgpId());


  }

  void BgpConfigureFullMesh::MakeGndAddRoute(Ptr<Node> gnd)
  {

	  Ipv4Mask mask("255.255.255.0");
	  auto gndAddress = gnd->GetObject<Ipv4L3Protocol>()->GetAddress(1,0).GetLocal();
	  GetBgpApplication(gnd)->AddRoute(gndAddress.CombineMask(mask), mask, gndAddress);

	  //std::cout << " > Add Route  Network Destination:"<< gndAddress.CombineMask(mask) << "  Nexthop:" << gndAddress << std::endl;
  }

  Ptr<Bgp> BgpConfigureFullMesh::GetBgpApplication(Ptr<Node> node)
  {
		uint32_t nApps = m_topology->GetNodes().Get(node->GetId())->GetNApplications();

		for(uint32_t j = 0; j < nApps; j++)	{
			auto instanceTypeId = m_topology->GetNodes().Get(node->GetId())->GetApplication(j)->GetInstanceTypeId();
			if(instanceTypeId == TypeId::LookupByName ("ns3::Bgp")) {
				return m_topology->GetNodes().Get(node->GetId())->GetApplication(j)->GetObject<Bgp>();
			}
		}
		return nullptr;
  }

}
