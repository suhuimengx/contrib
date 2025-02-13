/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 University of Washington
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
 */


#include "ns3/core-module.h"
#include "ns3/abort.h"
#include "ns3/mpi-interface.h"
#include <vector>
#include "sag_csma_channel.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SAGCsmaChannel");   //add state

NS_OBJECT_ENSURE_REGISTERED (SAGCsmaChannel);

TypeId 
SAGCsmaChannel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SAGCsmaChannel")
    .SetParent<SAGPhysicalLayerGSL> ()
    .SetGroupName ("GSL")
    .AddConstructor<SAGCsmaChannel> ()
  ;
  return tid;
}

SAGCsmaChannel::SAGCsmaChannel()
  :
	SAGPhysicalLayerGSL ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_state = IDLE;
}

bool
SAGCsmaChannel::TransmitStart (
			  Ptr<const Packet> p,
			  Ptr<SAGLinkLayerGSL> src,
			  Address dst_address,
			  Time txTime)
{
  NS_LOG_FUNCTION (this << p << src);
  NS_LOG_LOGIC ("UID is " << p->GetUid () << ")");

  Mac48Address dmac = Mac48Address::ConvertFrom (dst_address);

  //m_state = TRANSMITTING;

  if(src == m_sat_net_device)
  {
    if(dmac.IsBroadcast())//or multicast
    {
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
        Ptr<SAGLinkLayerGSL> dst = it->second;
        bool sameSystem = (src->GetNode()->GetSystemId() == dst->GetNode()->GetSystemId());
        return TransmitTo(p, src, dst, txTime, sameSystem);
      }
    }
  }
  else if(find(m_ground_net_devices.begin(), m_ground_net_devices.end(), src) != m_ground_net_devices.end())
  {
    m_state = TRANSMITTING;
    
    Ptr<SAGLinkLayerGSL> dst = m_sat_net_device;
    bool sameSystem = (src->GetNode()->GetSystemId() == dst->GetNode()->GetSystemId());
    return TransmitTo(p, src, dst, txTime, sameSystem);
  }


  //NS_ABORT_MSG("MAC address could not be mapped to a network device.");
  return false;
}

bool
SAGCsmaChannel::TransmitTo(Ptr<const Packet> p, Ptr<SAGLinkLayerGSL> srcNetDevice, Ptr<SAGLinkLayerGSL> destNetDevice, Time txTime, bool isSameSystem)
{
  // State converts
  //m_state = PROPAGATING;
  // Mobility models for source and destination
  Ptr<MobilityModel> senderMobility = srcNetDevice->GetNode()->GetObject<MobilityModel>();
  Ptr<Node> receiverNode = destNetDevice->GetNode();
  Ptr<MobilityModel> receiverMobility = receiverNode->GetObject<MobilityModel>();

  Ptr<Packet> pktCopy = p->Copy ();
  RouteHopCountTrace(srcNetDevice->GetNode()->GetId(), destNetDevice->GetNode()->GetId(), pktCopy);

  // Calculate delay
  Time delay = this->GetDelay(senderMobility, receiverMobility);
  NS_LOG_DEBUG(
          "Sending packet " << p << " from node " << srcNetDevice->GetNode()->GetId()
          << " to " << destNetDevice->GetNode()->GetId() << " with delay " << delay
  );

  // Distributed mode is not enabled
  NS_ABORT_MSG_UNLESS(isSameSystem, "MPI distributed mode is currently not supported by the GSL channel.");

  // Schedule arrival of packet at destination network device
  Simulator::Schedule(
				txTime + delay,
				&SAGCsmaChannel::PacketReceiving,
				this,
				pktCopy,
				destNetDevice
	  );

  return true;
}



void
SAGCsmaChannel::PacketReceiving (Ptr<Packet> packet,  Ptr<SAGLinkLayerGSL> destNetDevice)
{

		// the other end of the link receiving
		Simulator::ScheduleWithContext(
				destNetDevice->GetNode()->GetId(),
				Time(0),
				&SAGCsmaNetDevice::Receive,
				destNetDevice,
				packet
		);
		//state convert
		m_state = IDLE;



}




} // namespace ns3
