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
 * (Based on point-to-point channel)
 * Author: Andre Aguas    March 2020
 * 
 */


#include "point-to-point-laser-channel.h"
#include "ns3/core-module.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PointToPointLaserChannel");

NS_OBJECT_ENSURE_REGISTERED (PointToPointLaserChannel);

TypeId 
PointToPointLaserChannel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PointToPointLaserChannel")
    .SetParent<SAGPhysicalLayer> ()
    .SetGroupName ("PointToPointLaser")
    .AddConstructor<PointToPointLaserChannel> ()
    .AddAttribute ("Delay", "Initial propagation delay through the channel",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&PointToPointLaserChannel::m_delay),
                   MakeTimeChecker ())
    .AddAttribute ("PropagationSpeed", "Propagation speed through the channel",
                   DoubleValue (299792458.0),
                   MakeDoubleAccessor (&PointToPointLaserChannel::m_propagationSpeed),
                   MakeDoubleChecker<double> ())
    .AddTraceSource ("TxRxPointToPoint",
                     "Trace source indicating transmission of packet "
                     "from the PointToPointLaserChannel, used by the Animation "
                     "interface.",
                     MakeTraceSourceAccessor (&PointToPointLaserChannel::m_txrxPointToPoint),
                     "ns3::PointToPointLaserChannel::TxRxAnimationCallback")
  ;
  return tid;
}

PointToPointLaserChannel::PointToPointLaserChannel()
  :
	SAGPhysicalLayer (),
    m_nDevices (0)
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
PointToPointLaserChannel::Attach (Ptr<SAGLinkLayer> device)
{
  NS_LOG_FUNCTION (this << device);
  NS_ASSERT_MSG (m_nDevices < N_DEVICES, "Only two devices permitted");
  NS_ASSERT (device != 0);


  m_link[m_nDevices++].m_src = device->GetObject<PointToPointLaserNetDevice>();
//
// If we have both devices connected to the channel, then finish introducing
// the two halves and set the links to IDLE.
//
  if (m_nDevices == N_DEVICES)
    {
      m_link[0].m_dst = m_link[1].m_src;
      m_link[1].m_dst = m_link[0].m_src;
      m_link[0].m_state = IDLE;
      m_link[1].m_state = IDLE;
    }
}

bool
PointToPointLaserChannel::TransmitStart (
  Ptr<const Packet> p,
  Ptr<SAGLinkLayer> src,
  Ptr<Node> node_other_end,
  Time txTime)
{
  NS_LOG_FUNCTION (this << p << src);
  NS_LOG_LOGIC ("UID is " << p->GetUid () << ")");

  NS_ASSERT (m_link[0].m_state != INITIALIZING);
  NS_ASSERT (m_link[1].m_state != INITIALIZING);

  Ptr<Packet> p_copy = p->Copy();
  RouteHopCountTrace(src->GetNode()->GetId(), node_other_end->GetId(), p_copy);

//  Ptr<MobilityModel> senderMobility = src->GetNode()->GetObject<MobilityModel>();
//  Ptr<MobilityModel> receiverMobility = node_other_end->GetObject<MobilityModel>();
//  Time delay = this->GetDelay(senderMobility, receiverMobility);
  Time delay = GetDelay ();

  uint32_t wire = src->GetObject<PointToPointLaserNetDevice>() == m_link[0].m_src ? 0 : 1;

  Simulator::ScheduleWithContext (m_link[wire].m_dst->GetNode()->GetId (),
                                  txTime + delay, &PointToPointLaserNetDevice::Receive,
                                  m_link[wire].m_dst, p_copy);

  // Call the tx anim callback on the net device
  m_txrxPointToPoint (p, src->GetObject<PointToPointLaserNetDevice>(), m_link[wire].m_dst, txTime, txTime + delay);
  return true;
}

void
PointToPointLaserChannel::SAGPhysicalDoSomeThing (Ptr<const Packet> p)
{
  //For vatural
}

std::size_t
PointToPointLaserChannel::GetNDevices (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_nDevices;
}

Ptr<SAGLinkLayer>
PointToPointLaserChannel::GetPointToPointLaserDevice (std::size_t i) const
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (i < 2);
  return m_link[i].m_src;
}

Ptr<NetDevice>
PointToPointLaserChannel::GetDevice (std::size_t i) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return GetPointToPointLaserDevice (i);
}

//Time
//PointToPointLaserChannel::GetDelay (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
//{
//  double distance = a->GetDistanceFrom (b);
//  double seconds = distance / m_propagationSpeed;
//  return Seconds (seconds);
//}

Time
PointToPointLaserChannel::GetDelay () const
{
  return m_delay;
}

Ptr<PointToPointLaserNetDevice>
PointToPointLaserChannel::GetSource (uint32_t i) const
{
  return m_link[i].m_src;
}

Ptr<PointToPointLaserNetDevice>
PointToPointLaserChannel::GetDestination (uint32_t i) const
{
  return m_link[i].m_dst;
}

bool
PointToPointLaserChannel::IsInitialized (void) const
{
  NS_ASSERT (m_link[0].m_state != INITIALIZING);
  NS_ASSERT (m_link[1].m_state != INITIALIZING);
  return true;
}

} // namespace ns3
