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


#include "sag_physical_layer.h"

#include "ns3/core-module.h"

#include "ns3/route_trace_tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SAGPhysicalLayer");

NS_OBJECT_ENSURE_REGISTERED (SAGPhysicalLayer);

TypeId 
SAGPhysicalLayer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SAGPhysicalLayer")
    .SetParent<Channel> ()
    .SetGroupName ("PointToPoint")
    .AddConstructor<SAGPhysicalLayer> ()
  ;
  return tid;
}

SAGPhysicalLayer::SAGPhysicalLayer()
: Channel ()
{

}


void
SAGPhysicalLayer::Attach (Ptr<SAGLinkLayer> device)
{

}

bool
SAGPhysicalLayer::TransmitStart (
  Ptr<const Packet> p,
  Ptr<SAGLinkLayer> src,
  Ptr<Node> node_other_end,
  Time txTime)
{
  return true;
}

std::size_t
SAGPhysicalLayer::GetNDevices (void) const
{
  return 0;
}

Ptr<SAGLinkLayer>
SAGPhysicalLayer::GetPointToPointLaserDevice (std::size_t i) const
{
  return nullptr;
}

Ptr<NetDevice>
SAGPhysicalLayer::GetDevice (std::size_t i) const
{
	return nullptr;
}

void
SAGPhysicalLayer::SetChannelDelay(Time delay){

}

void
SAGPhysicalLayer::RouteHopCountTrace(uint32_t nodeid1, uint32_t nodeid2, Ptr<Packet> p){
	// Here, we must determine whether p is null, because TCP connections will bring null pointers to RouteOutput.
	if(!p){
		return;
	}

	// peek tag
	RouteTraceTag rtTrTagOld;
	if(p->PeekPacketTag(rtTrTagOld)){
		std::vector<uint32_t> nodesPassed = rtTrTagOld.GetRouteTrace();
		nodesPassed[nodesPassed.size()-1] = nodeid1;
		nodesPassed.push_back(nodeid2);
		RouteTraceTag rtTrTagNew((uint8_t)(nodesPassed.size()), nodesPassed);
		p->RemovePacketTag(rtTrTagOld);
		p->AddPacketTag(rtTrTagNew);

	}
}


} // namespace ns3
