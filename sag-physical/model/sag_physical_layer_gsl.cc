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


#include "sag_physical_layer_gsl.h"
#include "ns3/core-module.h"
#include "ns3/abort.h"
#include "ns3/mpi-interface.h"
//#include "ns3/gsl-net-device.h"
#include "ns3/sag_link_layer_gsl.h"
#include <vector>
#include "ns3/route_trace_tag.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SAGPhysicalLayerGSL");

NS_OBJECT_ENSURE_REGISTERED (SAGPhysicalLayerGSL);

TypeId 
SAGPhysicalLayerGSL::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SAGPhysicalLayerGSL")
    .SetParent<Channel> ()
    .SetGroupName ("GSL")
    .AddConstructor<SAGPhysicalLayerGSL> ()
    .AddAttribute ("Delay",
                   "The lower-bound propagation delay through the channel (it is accessed by the distributed simulator to determine lookahead time)",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&SAGPhysicalLayerGSL::m_lowerBoundDelay),
                   MakeTimeChecker ())
    .AddAttribute ("PropagationSpeed",
                   "Propagation speed through the channel in m/s (default is the speed of light)",
                   DoubleValue (299792458.0), // Default is speed of light
                   MakeDoubleAccessor (&SAGPhysicalLayerGSL::m_propagationSpeedMetersPerSecond),
                   MakeDoubleChecker<double> ())
	.AddAttribute ("Frequency",
				   "Propagation center frequency through the channel in Hz ",
				   DoubleValue (20.0e9),
				   MakeDoubleAccessor (&SAGPhysicalLayerGSL::m_propagationFrequency),
				   MakeDoubleChecker<double> ())
   .AddAttribute ("NoiseDbm",
					"Transmission antenna gain in dBm/Hz.",
					DoubleValue (-174),
					MakeDoubleAccessor (&SAGPhysicalLayerGSL::m_NoiseDbm),
					MakeDoubleChecker<double> ())
	.AddAttribute ("Bandwidth",
					"Channel bandwidth in Hz.",
					DoubleValue (2e7),
					MakeDoubleAccessor (&SAGPhysicalLayerGSL::m_bandwidth),
					MakeDoubleChecker<double> ())
	.AddAttribute ("Scenario",
				   "Propagation scenario",
				   StringValue ("DENSE_URBAN"),
				   MakeStringAccessor (&SAGPhysicalLayerGSL::m_propScenString),
				   MakeStringChecker ())
	.AddAttribute ("Enable_ber",
				   "if allow ber for packet receive or not",
				   BooleanValue (true),
				   MakeBooleanAccessor (&SAGPhysicalLayerGSL::m_enableBER),
				   MakeBooleanChecker ())

  ;
  return tid;
}

SAGPhysicalLayerGSL::SAGPhysicalLayerGSL()
  :
    Channel ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

//xhqin
Ptr<ThreeGppPropagationLossModel>
SAGPhysicalLayerGSL::GetPropagationScenario(){
	 m_propScenario= stringToEnum_scen(m_propScenString);
	 switch (m_propScenario)
	 {
	   case DENSE_URBAN:
		 m_loss_DU->SetAttribute("Frequency", DoubleValue(m_propagationFrequency));
		 m_loss_DU->SetAttribute("ShadowingEnabled", BooleanValue(false));
	     return m_loss_DU;
	   case URBAN:
		 m_loss_DU->SetAttribute("Frequency", DoubleValue(m_propagationFrequency));
	     return m_loss_U;
	   case SUBURBAN_AND_RURAL:
		 m_loss_DU->SetAttribute("Frequency", DoubleValue(m_propagationFrequency));
	     return m_loss_SU;
	   default:
	     std::cout<<"The propagation scenario is not supported!!!!!!"<<std::endl;
		 return m_loss_DU;
	  }
}
//xhqin
PropScenario
SAGPhysicalLayerGSL::stringToEnum_scen(std::string str){
	 if (str == "DENSE_URBAN") {
		return DENSE_URBAN;
	} else if (str == "URBAN") {
		return URBAN;
	} else if (str == "SUBURBAN_AND_RURAL") {
		return SUBURBAN_AND_RURAL;
	} else {
		return UNKNOWN;
	}
}

bool
SAGPhysicalLayerGSL::TransmitStart (
  Ptr<const Packet> p,
  Ptr<SAGLinkLayerGSL> src,
  Address dst_address,
  Time txTime)
{
  NS_LOG_FUNCTION (this << p << src);
  NS_LOG_LOGIC ("UID is " << p->GetUid () << ")");

  Mac48Address dmac = Mac48Address::ConvertFrom (dst_address);

  if(src == m_sat_net_device)
  {
    if(dmac.IsBroadcast())//or multicast
    {
      NS_LOG_LOGIC ("CASE 1: send broadcast from sat to gs(" << dmac << ")");
      for(MacToNetDeviceI it=m_link.begin();it!=m_link.end();it++)
      {
        Ptr<SAGLinkLayerGSL> dst = it->second;
        bool sameSystem = (src->GetNode()->GetSystemId() == dst->GetNode()->GetSystemId());
        TransmitTo(p, src, dst, txTime, sameSystem);
      }
      return true;
    }
    else
    {
      MacToNetDeviceI it = m_link.find (dmac);
      if (it != m_link.end ())
      {
        NS_LOG_LOGIC ("CASE 2: send unicast from sat to gs(" << dmac << ")");
        Ptr<SAGLinkLayerGSL> dst = it->second;
        bool sameSystem = (src->GetNode()->GetSystemId() == dst->GetNode()->GetSystemId());
        return TransmitTo(p, src, dst, txTime, sameSystem);
      }
    }
  }
  else if(find(m_ground_net_devices.begin(), m_ground_net_devices.end(), src) != m_ground_net_devices.end())
  {
    NS_LOG_LOGIC ("CASE 3: send unicast from gs to sat(" << dmac << ")");
    Ptr<SAGLinkLayerGSL> dst = m_sat_net_device;
    bool sameSystem = (src->GetNode()->GetSystemId() == dst->GetNode()->GetSystemId());
    return TransmitTo(p, src, dst, txTime, sameSystem);
  }


  //NS_ABORT_MSG("MAC address could not be mapped to a network device.");
  return false;
}

bool
SAGPhysicalLayerGSL::TransmitTo(Ptr<const Packet> p, Ptr<SAGLinkLayerGSL> srcNetDevice, Ptr<SAGLinkLayerGSL> destNetDevice, Time txTime, bool isSameSystem) {

  // Mobility models for source and destination
  Ptr<MobilityModel> senderMobility = srcNetDevice->GetNode()->GetObject<MobilityModel>();
  Ptr<Node> receiverNode = destNetDevice->GetNode();
  Ptr<MobilityModel> receiverMobility = receiverNode->GetObject<MobilityModel>();

  Ptr<Packet> p_copy = p->Copy();
  RouteHopCountTrace(srcNetDevice->GetNode()->GetId(), destNetDevice->GetNode()->GetId(), p_copy);

  // Calculate delay
  //Time delay = this->GetDelay(senderMobility, receiverMobility);
  Time delay = this->GetDelay(srcNetDevice, destNetDevice);
  NS_LOG_DEBUG(
          "Sending packet " << p << " from node " << srcNetDevice->GetNode()->GetId()
          << " to " << destNetDevice->GetNode()->GetId() << " with delay " << delay
  );

  // Distributed mode is not enabled
//  if(!isSameSystem){
//	  std::cout<<srcNetDevice->GetNode()->GetId()<<","<<srcNetDevice->GetNode()->GetSystemId()<<std::endl;
//	  std::cout<<destNetDevice->GetNode()->GetId()<<","<<destNetDevice->GetNode()->GetSystemId()<<std::endl;
//  }
//  NS_ABORT_MSG_UNLESS(isSameSystem, "MPI distributed mode is currently not supported by the GSL channel.");

  // Re-enabled below code if distributed is again enabled:

	//xyliu
	if(m_enableBER){
	  double per= this->GetPER(srcNetDevice, destNetDevice);
	  PerTag pertag;
	  pertag.SetPer(per);
	  p_copy->AddPacketTag(pertag);
	  //std::cout<<"PER is,"<<per<<std::endl;
	}
    if (isSameSystem) {
		// Schedule arrival of packet at destination network device
		Simulator::ScheduleWithContext(
			  receiverNode->GetId(),
			  txTime + delay,
			  &SAGLinkLayerGSL::Receive,
			  destNetDevice,
			  p_copy
		);
    } else {
	  #ifdef NS3_MPI
		  Time rxTime = Simulator::Now () + txTime + delay;
		  MpiInterface::SendPacket (p_copy, rxTime, destNetDevice->GetNode()->GetId (), destNetDevice->GetIfIndex());
	  #else
		  NS_FATAL_ERROR ("Can't use distributed simulator without MPI compiled in");
	  #endif
    }

  return true;
}

void
SAGPhysicalLayerGSL::Attach (Ptr<SAGLinkLayerGSL> device, bool isSat)
{
    NS_LOG_FUNCTION (this << device);
    NS_ABORT_MSG_IF (device == 0, "Cannot add zero pointer network device.");

    Mac48Address address48 = Mac48Address::ConvertFrom (device->GetAddress());
    if(!isSat)
    {
    	m_link[address48] = device;
    	m_ground_net_devices.push_back(device);
    }
    else
    {
        NS_ABORT_MSG_IF (m_sat_net_device != 0, "Cannot add over one sat network device.");
        m_sat_net_device = device;
    }
}

void
SAGPhysicalLayerGSL::UnAttach (Ptr<SAGLinkLayerGSL> device, bool isSat)
{
    NS_LOG_FUNCTION (this << device);
    NS_ABORT_MSG_IF (device == 0, "Cannot add zero pointer network device.");

    Mac48Address address48 = Mac48Address::ConvertFrom (device->GetAddress());
    NS_ABORT_MSG_IF (isSat == 1, "Cannot unattach satellite gsl device.");
    auto itr = m_link.find(address48);
    if(itr != m_link.end()){
    	m_link.erase(itr);
    }
    else{
    	throw std::runtime_error("No address48");
    }
    auto itrDevice = find(m_ground_net_devices.begin(), m_ground_net_devices.end(), device);
    if(itrDevice != m_ground_net_devices.end()){
    	m_ground_net_devices.erase(itrDevice);
    }
    else{
    	throw std::runtime_error("No device");
    }



}

Time
SAGPhysicalLayerGSL::GetDelay (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
  double distance_m = a->GetDistanceFrom (b);
  double seconds = distance_m / m_propagationSpeedMetersPerSecond;
  return Seconds (seconds);
}

Time
SAGPhysicalLayerGSL::GetDelay (Ptr<SAGLinkLayerGSL> srcNetDevice, Ptr<SAGLinkLayerGSL> destNetDevice) const
{
  if(srcNetDevice == m_sat_net_device){
	  auto iter = find(m_ground_net_devices.begin(), m_ground_net_devices.end(), destNetDevice);
	  if(iter == m_ground_net_devices.end()){
		  throw std::runtime_error("SAGPhysicalLayerGSL::GetDelay.");
	  }
	  size_t s = iter - m_ground_net_devices.begin();
	  NS_ASSERT(s < m_delays.size());
	  return m_delays[s];
  }
  else{
	  auto iter = find(m_ground_net_devices.begin(), m_ground_net_devices.end(), srcNetDevice);
	  if(iter == m_ground_net_devices.end()){
		  throw std::runtime_error("SAGPhysicalLayerGSL::GetDelay.");
	  }
	  size_t s = iter - m_ground_net_devices.begin();
	  NS_ASSERT(s < m_delays.size());
	  return m_delays[s];
  }
}

size_t Mac48AddressHash::operator() (Mac48Address const &x) const
{
    uint8_t address[6]; //!< address value
    x.CopyTo(address);

    uint32_t host = 0;
    uint8_t byte = 0;
    for (size_t i = 0; i < 6; i++) {
        byte = address[i];
        host <<= 8;
        host |= byte;
    }

    return host;
}

std::size_t
SAGPhysicalLayerGSL::GetNDevices (void) const
{
    NS_LOG_FUNCTION_NOARGS ();
    return m_ground_net_devices.size() + 1;
}

Ptr<NetDevice>
SAGPhysicalLayerGSL::GetDevice (std::size_t i) const
{
    NS_LOG_FUNCTION (this << i); 
    if(i == 0)
    {
        return m_sat_net_device;
    }
    else
    {
        return m_ground_net_devices.at(i-1);//todo
    }
}

WireState
SAGPhysicalLayerGSL::GetState (){
    return m_state;
}

void
SAGPhysicalLayerGSL::RouteHopCountTrace(uint32_t nodeid1, uint32_t nodeid2, Ptr<Packet> p){
	// Here, we must determine whether p is null, because TCP connections will bring null pointers to RouteOutput.
	if(!p){
		return;
	}

	// peek tag
	RouteTraceTag rtTrTagOld;
	if(p->PeekPacketTag(rtTrTagOld)){

		std::vector<uint32_t> nodesPassed = rtTrTagOld.GetRouteTrace();
		if(nodesPassed.size()==0){
			nodesPassed.push_back(nodeid1);
		}
		else{
			nodesPassed[nodesPassed.size()-1] = nodeid1;
		}
		nodesPassed.push_back(nodeid2);
		RouteTraceTag rtTrTagNew((uint8_t)(nodesPassed.size()), nodesPassed);
		p->RemovePacketTag(rtTrTagOld);
		p->AddPacketTag(rtTrTagNew);

	}
}

//xhqin
double
SAGPhysicalLayerGSL::CalcSinrDb (double rxPowerDb, double NoiseDb) const
{
  double totalIntDb = KpToDb(DbToKp(m_NoiseDbm) * m_bandwidth);
  NS_LOG_DEBUG ("Calculating SINR:  RxPower = " << rxPowerDb << " dB" << "  Interference + noise power = " << totalIntDb << " dB.  SINR = " << rxPowerDb - totalIntDb << " dB.");
  return rxPowerDb - totalIntDb;
}

//xhqin
double
SAGPhysicalLayerGSL::GetSINR (Ptr<SAGLinkLayerGSL> srcNetDevice, Ptr<SAGLinkLayerGSL> destNetDevice) const
{

  if(srcNetDevice == m_sat_net_device){
	  auto iter = find(m_ground_net_devices.begin(), m_ground_net_devices.end(), destNetDevice);
	  if(iter == m_ground_net_devices.end()){
		  throw std::runtime_error("SAGPhysicalLayerGSL::GetPER.");
	  }
	  size_t s = iter - m_ground_net_devices.begin();
	  NS_ASSERT(s < m_SINRs.size());
	  return m_SINRs[s];
  }
  else{
	  auto iter = find(m_ground_net_devices.begin(), m_ground_net_devices.end(), srcNetDevice);
	  if(iter == m_ground_net_devices.end()){
		  throw std::runtime_error("SAGPhysicalLayerGSL::GetPER.");
	  }
	  size_t s = iter - m_ground_net_devices.begin();
	  NS_ASSERT(s < m_SINRs.size());
	  return m_SINRs[s];
  }
}

//xhqin
double
SAGPhysicalLayerGSL::GetPER (Ptr<SAGLinkLayerGSL> srcNetDevice, Ptr<SAGLinkLayerGSL> destNetDevice) const
{

  if(srcNetDevice == m_sat_net_device){//satellite-->gs
	  auto iter = find(m_ground_net_devices.begin(), m_ground_net_devices.end(), destNetDevice);
	  if(iter == m_ground_net_devices.end()){
		  throw std::runtime_error("SAGPhysicalLayerGSL::GetPER.");
	  }
	  size_t s = iter - m_ground_net_devices.begin();
	  return m_PERRTN[s];
  }
  else{//gs-->satellite
	  auto iter = find(m_ground_net_devices.begin(), m_ground_net_devices.end(), srcNetDevice);
	  if(iter == m_ground_net_devices.end()){
		  throw std::runtime_error("SAGPhysicalLayerGSL::GetPER.");
	  }
	  size_t s = iter - m_ground_net_devices.begin();
	  return m_PERFWD[s];
  }
}
//xhqin
void
SAGPhysicalLayerGSL::SetNoiseDbm (double NoiseDbm)
{
	m_NoiseDbm=NoiseDbm;
}
double
SAGPhysicalLayerGSL::GetNoiseDbm (void)
{
	double Noise=DbToKp(m_NoiseDbm);
	double NoiseP = Noise * m_bandwidth;
	double NoiseDbm=KpToDb(NoiseP);
	return NoiseDbm;
}
void
SAGPhysicalLayerGSL::SetBandwidth (double Bandwidth)
{
	m_bandwidth=Bandwidth;
}
double
SAGPhysicalLayerGSL::GetBandwidth (void)
{
	return m_bandwidth;
}
void
SAGPhysicalLayerGSL::SetPropFrequency (double propagationFrequency)
{
	m_propagationFrequency = propagationFrequency;
}
double
SAGPhysicalLayerGSL::GetPropFrequency (void)
{
	return m_propagationFrequency;
}
bool
SAGPhysicalLayerGSL::GetEnableBER (void)
{
	return m_enableBER;
}

} // namespace ns3
