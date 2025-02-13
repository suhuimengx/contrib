/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007, 2008 University of Washington
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
 * (Based on point-to-point-laser network device)
 * Author: Xu    June 2022
 * 
 */

#include "sag_link_layer.h"

#include "ns3/log.h"
#include "ns3/queue.h"
#include "ns3/simulator.h"
#include "ns3/mac48-address.h"
#include "ns3/llc-snap-header.h"
#include "ns3/error-model.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"
#include "ns3/ppp-header.h"
#include "ns3/sag_physical_layer.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SAGLinkLayer");

NS_OBJECT_ENSURE_REGISTERED (SAGLinkLayer);

TypeId 
SAGLinkLayer::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::SAGLinkLayer")
       .SetParent<NetDevice> ()
       .SetGroupName ("PointToPoint")
       .AddConstructor<SAGLinkLayer> ()
    ;
    return tid;
}

SAGLinkLayer::SAGLinkLayer ()
{
    NS_LOG_FUNCTION (this);
}

SAGLinkLayer::~SAGLinkLayer ()
{
    NS_LOG_FUNCTION (this);
}

void
SAGLinkLayer::SetLinkState (P2PLinkState p2pLinkState)
{

}


void
SAGLinkLayer::SetDataRate (DataRate bps)
{

}

uint64_t
SAGLinkLayer::GetDataRate (){
	return 0;
}

void
SAGLinkLayer::SetInterframeGap (Time t)
{

}


bool
SAGLinkLayer::Attach (Ptr<SAGPhysicalLayer> ch)
{
	std::cout<<1111111111111<<std::endl;
  return false;
}

void
SAGLinkLayer::SetQueue (Ptr<Queue<Packet> > q)
{

}

void
SAGLinkLayer::SetReceiveErrorModel (Ptr<ErrorModel> em)
{

}

void
SAGLinkLayer::Receive (Ptr<Packet> packet)
{
	std::cout<<"@@@@"<<std::endl;

}

Ptr<Queue<Packet> >
SAGLinkLayer::GetQueue (void) const
{
	Ptr<Queue<Packet>> queue = 0;
  return queue;
}

void
SAGLinkLayer::SetIfIndex (const uint32_t index)
{

}

uint32_t
SAGLinkLayer::GetIfIndex (void) const
{
  return 0;
}

Ptr<Channel>
SAGLinkLayer::GetChannel (void) const
{
  return nullptr;
}

//
// This is a point-to-point device, so we really don't need any kind of address
// information.  However, the base class NetDevice wants us to define the
// methods to get and set the address.  Rather than be rude and assert, we let
// clients get and set the address, but simply ignore them.

void
SAGLinkLayer::SetAddress (Address address)
{

}

Address
SAGLinkLayer::GetAddress (void) const
{
  return Mac48Address();
}

void
SAGLinkLayer::SetDestinationNode (Ptr<Node> node)
{

}

Ptr<Node>
SAGLinkLayer::GetDestinationNode (void) const
{
  return nullptr;
}

bool
SAGLinkLayer::IsLinkUp (void) const
{
  return false;
}

void
SAGLinkLayer::AddLinkChangeCallback (Callback<void> callback)
{

}

//
// This is a point-to-point device, so every transmission is a broadcast to
// all of the devices on the network.
//
bool
SAGLinkLayer::IsBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

//
// We don't really need any addressing information since this is a
// point-to-point device.  The base class NetDevice wants us to return a
// broadcast address, so we make up something reasonable.
//
Address
SAGLinkLayer::GetBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  return Mac48Address ("ff:ff:ff:ff:ff:ff");
}

bool
SAGLinkLayer::IsMulticast (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

Address
SAGLinkLayer::GetMulticast (Ipv4Address multicastGroup) const
{
  NS_LOG_FUNCTION (this);
  return Mac48Address ("01:00:5e:00:00:00");
}

Address
SAGLinkLayer::GetMulticast (Ipv6Address addr) const
{
  NS_LOG_FUNCTION (this << addr);
  return Mac48Address ("33:33:00:00:00:00");
}

double
SAGLinkLayer::GetQueueOccupancyRate(){
	return 0;
}

bool
SAGLinkLayer::IsPointToPoint (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

bool
SAGLinkLayer::IsBridge (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

bool
SAGLinkLayer::Send (
  Ptr<Packet> packet,
  const Address &dest,
  uint16_t protocolNumber)
{
  return 0;

}

uint32_t
SAGLinkLayer::GetMaxsize(){
	return 0;
}


bool
SAGLinkLayer::SendFrom (Ptr<Packet> packet,
                                 const Address &source,
                                 const Address &dest,
                                 uint16_t protocolNumber)
{
  return false;
}

Ptr<Node>
SAGLinkLayer::GetNode (void) const
{
  return nullptr;
}

void
SAGLinkLayer::SetNode (Ptr<Node> node)
{

}

bool
SAGLinkLayer::NeedsArp (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

void
SAGLinkLayer::SetReceiveCallback (NetDevice::ReceiveCallback cb)
{

}

void
SAGLinkLayer::SetPromiscReceiveCallback (NetDevice::PromiscReceiveCallback cb)
{

}

bool
SAGLinkLayer::SupportsSendFrom (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

void
SAGLinkLayer::SetInterruptionInformation(P2PInterruptionType metaData, bool b){

}

bool
SAGLinkLayer::GetInterruptionInformation(P2PInterruptionType metaData){
	return false;
}

bool
SAGLinkLayer::SetMtu (uint16_t mtu)
{
  return true;
}

uint16_t
SAGLinkLayer::GetMtu (void) const
{
  return 0;
}


void
SAGLinkLayer::EnableUtilizationTracking(int64_t interval_ns) {
    m_utilization_tracking_enabled = true;
    m_interval_ns = interval_ns;
    m_prev_time_ns = 0;
    m_current_interval_start = 0;
    m_current_interval_end = m_interval_ns;
    m_idle_time_counter_ns = 0;
    m_busy_time_counter_ns = 0;
    m_current_state_is_on = false;
}

void
SAGLinkLayer::TrackUtilization(bool next_state_is_on) {
    if (m_utilization_tracking_enabled) {

        // Current time in nanoseconds
        int64_t now_ns = Simulator::Now().GetNanoSeconds();
        while (now_ns >= m_current_interval_end) {

            // Add everything until the end of the interval
            if (next_state_is_on) {
                m_idle_time_counter_ns += m_current_interval_end - m_prev_time_ns;
            } else {
                m_busy_time_counter_ns += m_current_interval_end - m_prev_time_ns;
            }

            // Save into the utilization array
            m_utilization.push_back(((double) m_busy_time_counter_ns) / ((double) m_interval_ns));

            // This must match up
            NS_ABORT_MSG_IF(m_idle_time_counter_ns + m_busy_time_counter_ns != m_interval_ns, "Not all time is accounted for");

            // Move to next interval
            m_idle_time_counter_ns = 0;
            m_busy_time_counter_ns = 0;
            m_prev_time_ns = m_current_interval_end;
            m_current_interval_start += m_interval_ns;
            m_current_interval_end += m_interval_ns;
        }

        // If not at the end of a new interval, just keep track of it all
        if (next_state_is_on) {
            m_idle_time_counter_ns += now_ns - m_prev_time_ns;
        } else {
            m_busy_time_counter_ns += now_ns - m_prev_time_ns;
        }

        // This has become the previous call
        m_current_state_is_on = next_state_is_on;
        m_prev_time_ns = now_ns;

    }
}



const std::vector<double>&
SAGLinkLayer::FinalizeUtilization() {
    TrackUtilization(!m_current_state_is_on);
    return m_utilization;
}



}// namespace ns3


