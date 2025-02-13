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
#define NS_LOG_APPEND_CONTEXT                                   \
   std::clog << "[node " << m_nodeId << "] ";
//#define NS_LOG_CONDITION    if (m_nodeId==9)

#include "mplb-routing.h"
#include "ns3/log.h"
#include "ns3/route_trace_tag.h"
#include "ns3/sag_physical_layer_gsl.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("MultiPathLoadBalance");

namespace mplb {
NS_OBJECT_ENSURE_REGISTERED (MultiPathLoadBalance);
/// IP Protocol Number for MPLB
const uint8_t MultiPathLoadBalance::MPLB_PROTOCOL = 100;

TypeId
MultiPathLoadBalance::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::mplb::MultiPathLoadBalance")
		  .SetParent<SAGRoutingProtocal>()
		  .SetGroupName("MPLB")
		  .AddConstructor<MultiPathLoadBalance>()
		  ;
	return tid;
}

MultiPathLoadBalance::MultiPathLoadBalance()
{
	m_routingTable = CreateObject<SAGRoutingTable>();
	m_routeBuild = CreateObject<MplbBuildRouting>();
	m_routeBuild->SetRouter(m_routingTable);
	m_routeBuild->SetIpv4(m_ipv4);
	m_routeBuild->SetRoutingCalculationCallback(MakeCallback(&SAGRoutingProtocal::CalculationTimeLog, this));
}

MultiPathLoadBalance::~MultiPathLoadBalance()
{
	NS_LOG_FUNCTION(this);
}




void
MultiPathLoadBalance::NotifyInterfaceUp(uint32_t i)
{
	NS_LOG_FUNCTION(this);
	Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
	// One IP address per interface
	if (m_ipv4->GetNAddresses(i) != 1) {
		throw std::runtime_error("Each interface is permitted exactly one IP address.");
	}

	// Get interface single IP's address and mask
	Ipv4Address if_addr = m_ipv4->GetAddress(i, 0).GetLocal();
	Ipv4Mask if_mask = m_ipv4->GetAddress(i, 0).GetMask();

	// Loopback interface must be 0
	if (i == 0) {
		if (if_addr != Ipv4Address("127.0.0.1") || if_mask != Ipv4Mask("255.0.0.0")) {
			throw std::runtime_error("Loopback interface 0 must have IP 127.0.0.1 and mask 255.0.0.0");
		}

	} else { // Towards another interface

		// Check that the subnet mask is maintained
		if (if_mask.Get() != Ipv4Mask("255.255.255.0").Get()) {
			throw std::runtime_error("Each interface must have a subnet mask of 255.255.255.0");
		}

	}


}

void
MultiPathLoadBalance::NotifyInterfaceDown (uint32_t i){

	NS_LOG_FUNCTION(this);
	Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
	// One IP address per interface
	if (m_ipv4->GetNAddresses(i) != 1) {
		throw std::runtime_error("Each interface is permitted exactly one IP address.");
	}

	// Get interface single IP's address and mask
	Ipv4Address if_addr = m_ipv4->GetAddress(i, 0).GetLocal();
	Ipv4Mask if_mask = m_ipv4->GetAddress(i, 0).GetMask();
	// Loopback interface must be 0
	if (i == 0) {
		if (if_addr != Ipv4Address("127.0.0.1") || if_mask != Ipv4Mask("255.0.0.0")) {
			throw std::runtime_error("Loopback interface 0 must have IP 127.0.0.1 and mask 255.0.0.0");
		}

	} else { // Towards another interface

		// Check that the subnet mask is maintained
		if (if_mask.Get() != Ipv4Mask("255.255.255.0").Get()) {
			throw std::runtime_error("Each interface must have a subnet mask of 255.255.255.0");
		}

	}

}

void
MultiPathLoadBalance::NotifyAddAddress(uint32_t interface, Ipv4InterfaceAddress address)
{
	NS_LOG_FUNCTION(this);
	Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
	if (!l3->IsUp (interface)){
		return;
	}
	if(interface == 0){
		return;
		throw std::runtime_error("Not permitted to add IP addresses after the loopback interface has gone up.");
	}
	// One IP address per interface
	if (m_ipv4->GetNAddresses(interface) != 1) {
		throw std::runtime_error("Each interface is permitted exactly one IP address.");
	}

	// Get interface single IP's address and mask
//	Ipv4Address if_addr = address.GetLocal();
	Ipv4Mask if_mask = address.GetMask();

	// Check that the subnet mask is maintained
	if (if_mask.Get() != Ipv4Mask("255.255.255.0").Get()) {
		throw std::runtime_error("Each interface must have a subnet mask of 255.255.255.0");
	}


}

void
MultiPathLoadBalance::NotifyRemoveAddress(uint32_t interface, Ipv4InterfaceAddress address)
{
	NS_LOG_FUNCTION(this);
	if(interface == 0){
		throw std::runtime_error("Not permitted to remove IP addresses after the loopback interface has gone up.");
	}
	Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
	// One IP address per interface
	if (m_ipv4->GetNAddresses(interface) != 0) {
		throw std::runtime_error("Each interface is permitted exactly one IP address.");
	}

	// Get interface single IP's address and mask
//	Ipv4Address if_addr = address.GetLocal();
	Ipv4Mask if_mask = address.GetMask();

	// Check that the subnet mask is maintained
	if (if_mask.Get() != Ipv4Mask("255.255.255.0").Get()) {
		throw std::runtime_error("Each interface must have a subnet mask of 255.255.255.0");
	}



}

Ptr<Ipv4Route>
MultiPathLoadBalance::RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr){
	NS_LOG_FUNCTION (this << header << (oif ? oif->GetIfIndex () : 0));

	sockerr = Socket::ERROR_NOTERROR;
	Ptr<Ipv4Route> route;
	Ipv4Address dst = header.GetDestination ();
	SAGRoutingTableEntry rtEntry;
	bool found = m_routingTable->LookupRoute(dst, rtEntry);
	if (found)
	{
		route = rtEntry.GetRoute();
		NS_ASSERT (route != 0);
		NS_LOG_DEBUG ("Exist route to " << route->GetDestination () << " from interface " << route->GetSource ());
		if (oif != 0 && route->GetOutputDevice () != oif)
		{
		  NS_LOG_DEBUG ("Output device doesn't match. Dropped.");
		  sockerr = Socket::ERROR_NOROUTETOHOST;
		  return Ptr<Ipv4Route> ();
		}

		//RouteTrace(p);

		return route;
	}
	return Ptr<Ipv4Route> ();
}

bool
MultiPathLoadBalance::RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
			   	   	   	   	   	   	  UnicastForwardCallback ucb, MulticastForwardCallback mcb,
									  LocalDeliverCallback lcb, ErrorCallback ecb)
{
	NS_LOG_FUNCTION (this << p->GetUid () << header.GetDestination () << idev->GetAddress ());
	NS_ASSERT(m_ipv4 != 0);
	NS_ASSERT (p != 0);

	// Check if input device supports IP
	NS_ASSERT (m_ipv4->GetInterfaceForDevice (idev) >= 0);
	int32_t iif = m_ipv4->GetInterfaceForDevice (idev);

	Ipv4Address dst = header.GetDestination ();
//	Ipv4Address origin = header.GetSource ();
//	Ipv4Address localInterfaceAdr = m_ipv4->GetAddress(iif, 0).GetLocal();

	// Multi-cast, waiting to be optimised
	// we use directed-broadcast currently
	if (dst.IsMulticast()) {
		throw std::runtime_error("Multi-cast not supported.");
	}

	// Local delivery
	if (m_ipv4->IsDestinationAddress(dst, iif)) {

		// Determine whether a protocol packet or a data packet
		if(header.GetProtocol() == MPLB_PROTOCOL)
		{
			Ptr<Packet> packet = p->Copy ();
			//RecvOSPF(packet, localInterfaceAdr, origin); todo
			return true;
		}
		else
		{
			if (lcb.IsNull()) {
				throw std::runtime_error("Local callback cannot be null");
			}
			// Info: If you want to decide that a packet should not be delivered (dropped),
			//       you can decide that here by not calling lcb(), but still returning true.

			// tag route
			Ptr<Packet> packet = p->Copy ();// Because the tag needs to be modified, we need a non-const packet
			//RouteTrace(packet);
		    lcb(packet, header, iif);

			return true;
		}
	}

	// Check if input device supports IP forwarding
	if (m_ipv4->IsForwarding(iif) == false) {
		throw std::runtime_error("Forwarding must be enabled for every interface");
	}

	// Uni-cast delivery
	SAGRoutingTableEntry rtEntry;
	bool found = m_routingTable->LookupRoute(dst, rtEntry);
	if (!found) {
		// Lookup failed, so we did not find a route
		// If there are no other routing protocols, this will lead to a drop
		return false;
	} else {
		// Lookup succeeded in producing a route
		// So we perform the unicast callback to forward there
		Ptr<Ipv4Route> route = rtEntry.GetRoute();
		NS_ASSERT (route != 0);
		Ptr<Packet> packet = p->Copy ();// Because the tag needs to be modified, we need a non-const packet
		//RouteTrace(packet);
		ucb(route, packet, header);

		return true;
	}

}






void
MultiPathLoadBalance::DoInitialize (void)
{
	NS_LOG_FUNCTION (this);

	m_routeBuild->SetTopology(m_walkerConstellation);
	m_routeBuild->RouterCalculate();
	Ipv4RoutingProtocol::DoInitialize ();
}




}
}


