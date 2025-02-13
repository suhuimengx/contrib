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

#define NS_LOG_APPEND_CONTEXT  \
  if (m_ipv6 && m_ipv6->GetObject<Node> ()) { \
      std::clog << Simulator::Now ().GetSeconds () \
                << " [node " << m_ipv6->GetObject<Node> ()->GetId () << "] "; }

#include <iomanip>
#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/ipv6-route.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/sag_routing_protocal_ipv6.h"
#include "ns3/arbiter-single-forward.h"
#include "ns3/mpi-interface.h"

namespace ns3 {

    NS_LOG_COMPONENT_DEFINE ("SAGRoutingProtocalIPv6");

    NS_OBJECT_ENSURE_REGISTERED (SAGRoutingProtocalIPv6);

    TypeId
    SAGRoutingProtocalIPv6::GetTypeId(void) 
    {
        static TypeId tid = TypeId("ns3::SAGRoutingProtocalIPv6")
                .SetParent<Ipv6RoutingProtocol>()
                .SetGroupName("Internet")
                .AddConstructor<SAGRoutingProtocalIPv6>()
        .AddAttribute ("ControlPacketProcessDelay", "The processing delay of the control packet",
				TimeValue (MilliSeconds (5)),
				MakeTimeAccessor (&SAGRoutingProtocalIPv6::m_processDelay),
				MakeTimeChecker ())
		.AddAttribute ("EnableRtrCalculateTime", "Indicates whether router's CPU run time considered when calculating routing algorithm.",
				 BooleanValue (true),
				 MakeBooleanAccessor (&SAGRoutingProtocalIPv6::m_rtrCalTimeConsidered),
				 MakeBooleanChecker ())
		.AddAttribute("SatellitesNumber", "Satellites number in the simulation topology.",
				UintegerValue(0),
				MakeUintegerAccessor(&SAGRoutingProtocalIPv6::m_satellitesNumber),
				MakeUintegerChecker<uint32_t>())
		.AddAttribute("GroundStationNumber", "Ground station number in the simulation topology.",
				UintegerValue(0),
				MakeUintegerAccessor(&SAGRoutingProtocalIPv6::m_groundStationNumber),
				MakeUintegerChecker<uint32_t>())
		.AddAttribute("ConstellationTopology", "Topology information of constellation.",
				PointerValue(0),
				MakePointerAccessor (&SAGRoutingProtocalIPv6::m_walkerConstellation),
				MakePointerChecker<Constellation>())
				;
        return tid;
    }

    SAGRoutingProtocalIPv6::SAGRoutingProtocalIPv6() : m_ipv6(0) 
    {
        NS_LOG_FUNCTION(this);
    }

    SAGRoutingProtocalIPv6::~SAGRoutingProtocalIPv6() 
    {
        NS_LOG_FUNCTION(this);
    }

    void
    SAGRoutingProtocalIPv6::NotifyInterfaceUp(uint32_t i) 
    {

        // One IP address per interface
        // if (m_ipv6->GetNAddresses(i) != 1) {
        //     throw std::runtime_error("Each interface is permitted exactly one IPv6 address."); //todo: IPv6 can support more than one address 
        // }

        // Get interface single IP's address and mask
        // Ipv6Address if_addr = m_ipv6->GetAddress(i, 0).GetAddress();
        // Ipv6Prefix if_prefix = m_ipv6->GetAddress(i, 0).GetPrefix();

        // Loopback interface must be 0
        // if (i == 0) {
        //     if (if_addr != Ipv6Address("::1") || if_prefix != Ipv6Prefix(128)) {
        //         throw std::runtime_error("Loopback interface 0 must have IP ::1 and mask FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF");
        //     }

        // } else { // Towards another interface

            // Check that the subnet mask is maintained
            // uint8_t if_prefix_buf[16] = {0};
            // uint8_t prefix_buf[16] = {0};
            // if_prefix.GetBytes(if_prefix_buf);
            // Ipv6Prefix(64).GetBytes(prefix_buf);
            // if (if_prefix_buf != prefix_buf) {
            //     throw std::runtime_error("Each interface must have a subnet mask of FF:FF:FF:FF::");
            // }

        // }

    }

    void
    SAGRoutingProtocalIPv6::NotifyInterfaceDown(uint32_t i) 
    {
//        throw std::runtime_error("Interfaces are not permitted to go down.");
    }

    void
    SAGRoutingProtocalIPv6::NotifyAddAddress(uint32_t interface, Ipv6InterfaceAddress address) 
    {
//        if (m_ipv6->IsUp(interface)) {
//            throw std::runtime_error("Not permitted to add IP addresses after the interface has gone up.");
//        }
    }

    void
    SAGRoutingProtocalIPv6::NotifyRemoveAddress(uint32_t interface, Ipv6InterfaceAddress address) 
    {
//        if (m_ipv6->IsUp(interface)) {
//            throw std::runtime_error("Not permitted to remove IP addresses after the interface has gone up.");
//        }
    }

    void 
    SAGRoutingProtocalIPv6::NotifyAddRoute (Ipv6Address dst, Ipv6Prefix mask, Ipv6Address nextHop, uint32_t interface, Ipv6Address prefixToUse)
    {

    }

    void 
    SAGRoutingProtocalIPv6::NotifyRemoveRoute (Ipv6Address dst, Ipv6Prefix mask, Ipv6Address nextHop, uint32_t interface, Ipv6Address prefixToUse)
    {

    }


    void
    SAGRoutingProtocalIPv6::SetIpv6(Ptr <Ipv6> ipv6) 
    {
        NS_LOG_FUNCTION(this << ipv6);
        NS_ASSERT(m_ipv6 == 0 && ipv6 != 0);
        m_ipv6 = ipv6;
        for (uint32_t i = 0; i < m_ipv6->GetNInterfaces(); i++) {
            if (m_ipv6->IsUp(i)) {
                NotifyInterfaceUp(i);
            } else {
                NotifyInterfaceDown(i);
            }
        }
        m_nodeId = m_ipv6->GetObject<Node>()->GetId();
        m_systemId = m_ipv6->GetObject<Node>()->GetSystemId();
        SetIPv6SendCallback (MakeCallback (&Ipv6::Send, ipv6));
    }

    void 
    SAGRoutingProtocalIPv6::PrintRoutingTable(Ptr<OutputStreamWrapper> stream, Time::Unit unit) const 
    {
        std::ostream* os = stream->GetStream ();
        *os << m_arbiter->StringReprOfForwardingState();
    }

    void
    SAGRoutingProtocalIPv6::SetArbiter (Ptr<Arbiter> arbiter) 
    {
        m_arbiter = arbiter;
    }

    Ptr<Arbiter>
    SAGRoutingProtocalIPv6::GetArbiter () 
    {
        return m_arbiter;
    }

    ///////////////////////////////////////// Routing Protocal Forwarding surface

    Ptr<Ipv6Route>
    SAGRoutingProtocalIPv6::LookupArbiter (const Ipv6Address& dest, const Ipv6Header &header, Ptr<const Packet> p, Ptr<NetDevice> oif) 
    {
        // Arbiter must be set
        if (m_arbiter == 0) {
            throw std::runtime_error("Arbiter has not been set");
        }


        // Multi-cast not supported
        if (dest.IsLinkLocalMulticast()) {
            throw std::runtime_error("Multi-cast is not supported");
        }

        // No support for requested output interfaces
        if (oif != 0) {
            throw std::runtime_error("Requested output interfaces are not supported");
        }

        // Decide interface index
        uint32_t if_idx;
        uint8_t gateway_ip_address[16] = {0};
        if (loopbackMask.IsMatch(dest, loopbackIp)) { // Loop-back
            if_idx = 0;
            memset(gateway_ip_address, 0, 16);

        } else { // If not loop-back, it goes to the arbiter
                 // Local delivery has already been handled if it was input

            ArbiterResultIPv6 result = m_arbiter->BaseDecide(p, header);
            if (result.Failed()) {
                return 0;
            } else {
                if_idx = result.GetOutIfIdx();
                result.GetGatewayIpv6Address(gateway_ip_address);
            }

        }

        // Create routing entry
        Ptr<Ipv6Route> rtentry = Create<Ipv6Route>();
        rtentry->SetDestination(dest);
        rtentry->SetSource(m_ipv6->SourceAddressSelection(if_idx, dest)); // This is basically the IP of the interface
                                                                          // It is used by a transport layer to
                                                                          // determine its source IP address
        rtentry->SetGateway(Ipv6Address(gateway_ip_address)); // If the network device does not care about ARP resolution,
        //                                                       // this can be set to 0.0.0.0
        rtentry->SetOutputDevice(m_ipv6->GetNetDevice(if_idx));
        return rtentry;

    }

    /**
     * Get an output route.
     *
     * TCP:
     * (1) RouteOutput gets called once by TCP (tcp-socket-base.cc) with an header of which only the destination
     *     IP address is set in order to determine the source IP address for the socket (function: SetupEndpoint).
     * (2) Subsequently, RouteOutput is called by TCP (tcp-l4-protocol.cc) on the sender node with a proper IP
     *     AND TCP header.
     *
     * UDP:
     * (1) RouteOutput gets called once by UDP (udp-socket-impl.cc) with an header of which only the destination
     *     IP address is set in order to determine the source IP address for the socket (function: DoSendTo).
     * (2) It is **NOT** called subsequently in udp-l4-protocol.cc, as such the first decision which did not take
     *     into account the UDP header is final. This means ECMP load balancing does not work at a UDP source.
     *
     * @param p         Packet
     * @param header    Header
     * @param oif       Requested output interface
     * @param sockerr   (Output) socket error, set if cannot find route
     *
     * @return IPv6 route
     */
    Ptr <Ipv6Route>
    SAGRoutingProtocalIPv6::RouteOutput(Ptr <Packet> p, const Ipv6Header &header, Ptr <NetDevice> oif, Socket::SocketErrno &sockerr) 
    {
        NS_LOG_FUNCTION(this << p << header << oif << sockerr);
        Ipv6Address destination = header.GetDestination();
        Ptr<Ipv6Route> route = nullptr;

        // Multi-cast needs to be supported
        if (destination.IsMulticast()) {
            //send any gateway if multi-cast
            NS_LOG_LOGIC ("RouteOutput ()::Multicast destination");
            NS_ASSERT_MSG (oif, "Try to send on link-local multicast address, and no oif index is given!");
            route = Create<Ipv6Route> ();
            route->SetSource (m_ipv6->SourceAddressSelection (m_ipv6->GetInterfaceForDevice (oif), destination));
            route->SetDestination (destination);
            route->SetGateway (Ipv6Address::GetZero ());
            route->SetOutputDevice (oif);
        }
        else{

            // Perform lookup
            // Info: If no route is found for a packet with the header with source IP = 102.102.102.102,
            //       the TCP socket will conclude there is no route and not even send out SYNs (any real packet).
            //       If source IP is set already, it just gets dropped and the TCP socket sees it as a normal loss somewhere in the network.
            route = LookupArbiter(destination, header, p, oif);
        }


        //std::cout<<"SAG test:"+ std::to_string(m_nodeId) +" Routing Lookup"<<std::endl;


        if (route == 0) {
            sockerr = Socket::ERROR_NOROUTETOHOST;
        } else {
            sockerr = Socket::ERROR_NOTERROR;
        }
        return route;

    }

    bool
    SAGRoutingProtocalIPv6::RouteInput(Ptr<const Packet> p, const Ipv6Header &ipHeader, Ptr<const NetDevice> idev,
                                          UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                                          LocalDeliverCallback lcb, ErrorCallback ecb) 
    {
        NS_ASSERT(m_ipv6 != 0);

        //std::cout<<"SAG test:"+ std::to_string(m_nodeId) +" Routing Lookup"<<std::endl;

        // Check if input device supports IP
        NS_ASSERT(m_ipv6->GetInterfaceForDevice(idev) >= 0);
        Ipv6Address destination = ipHeader.GetDestination();
        // uint32_t iif = m_ipv6->GetInterfaceForDevice(idev);
        Ptr<Ipv6Route> route = NULL;

        // Multi-cast
        if (destination.IsMulticast()) {
            return false;
            // lcb(p, ipHeader, iif);
            // throw std::runtime_error("Multi-cast not supported.");
            // route = Create<Ipv6Route> ();
            // route->SetSource (m_ipv6->SourceAddressSelection (m_ipv6->GetInterfaceForDevice (iif), ipHeader.GetSource()));
            // route->SetDestination (destination);
            // route->SetGateway (Ipv6Address::GetZero ());
            // // route->SetOutputDevice (iif);
        }

        // Local delivery
        // if (m_ipv6->IsDestinationAddress(ipHeader.GetDestination(), iif)) { // WeakESModel is set by default to true,
        //                                                                     // as such it works for any IP address
        //                                                                     // on any interface of the node
        //     if (lcb.IsNull()) {
        //         throw std::runtime_error("Local callback cannot be null");
        //     }
        //     // Info: If you want to decide that a packet should not be delivered (dropped),
        //     //       you can decide that here by not calling lcb(), but still returning true.
        //     lcb(p, ipHeader, iif);
        //     return true;
        // }

        // Check if input device supports IP forwarding
        // if (m_ipv6->IsForwarding(iif) == false) {
        //     throw std::runtime_error("Forwarding must be enabled for every interface");
        // }

        // Uni-cast delivery
        route = LookupArbiter(ipHeader.GetDestination(), ipHeader, p);
        if (route == 0) {

            // Lookup failed, so we did not find a route
            // If there are no other routing protocols, this will lead to a drop
            return false;

        } else {

            // Lookup succeeded in producing a route
            // So we perform the unicast callback to forward there
            ucb(idev, route, p, ipHeader); //idev not be used
            return true;

        }

    }

    /////////////////////////// Routing Protocal Control surface
    void 
    SAGRoutingProtocalIPv6::RouteProtocal (Ptr<Packet> p, const Ipv6Header &header)
    {
        std::cout << "SAG Test Destributed Router" << std::endl;
        Ptr<Packet> packet = p->Copy ();

//        SAGRoutingProtocalHeaderIPv6 routingHeader;
//        packet->RemoveHeader (routingHeader);

        //you can mantain your routing protocal using information from routingHeader
        // many things ...
    }  

    void 
	SAGRoutingProtocalIPv6::SendRoutingProtocalPacket (Ptr<Packet> packet, Ipv6Address source,
	    		Ipv6Address destination, uint8_t protocol, Ptr<Ipv6Route> route)
    {

        Simulator::Schedule(m_processDelay, &SAGRoutingProtocalIPv6::DoSendRoutingProtocalPacket, this,
        		packet, source, destination, protocol, route);
//    	DoSendRoutingProtocalPacket(packet, source, destination, protocol, route);

    }

    void
	SAGRoutingProtocalIPv6::DoSendRoutingProtocalPacket (Ptr<Packet> packet, Ipv6Address source,
        	    		Ipv6Address destination, uint8_t protocol, Ptr<Ipv6Route> route){

    	if(!m_sendCb.IsNull ()){
    		m_sendCb(Simulator::Now(), packet->GetSize(), m_nodeId);
    	}
        m_ipv6SendPkt(packet, source, destination, protocol, route);

    }



    void
    SAGRoutingProtocalIPv6::RouteTrace(Ptr<Packet> p){

    	// Here, we must determine whether p is null, because TCP connections will bring null pointers to RouteOutput.
    	if(!p){
    		return;
    	}

    	// peek tag
    	RouteTraceTag rtTrTagOld;
    	if(p->PeekPacketTag(rtTrTagOld)){

			std::vector<uint32_t> nodesPassed = rtTrTagOld.GetRouteTrace();
			nodesPassed.push_back((m_ipv6->GetObject<Node>())->GetId());
			RouteTraceTag rtTrTagNew((uint8_t)(nodesPassed.size()), nodesPassed);
			p->RemovePacketTag(rtTrTagOld);
			p->AddPacketTag(rtTrTagNew);

    	}

    }

    void
    SAGRoutingProtocalIPv6::CalculationTimeLog(uint32_t nodeId, Time curTime, double calculationTime){

    	if(!m_rtrCalCb.IsNull()){
    		m_rtrCalCb(nodeId, curTime, calculationTime);
    	}

    }

    void
	SAGRoutingProtocalIPv6::SetRoutingCalculationCallback(Callback<void, uint32_t, Time, double> rtrCalCb)
    {
    	m_rtrCalCb = rtrCalCb;
    }

    void 
    SAGRoutingProtocalIPv6::SetIPv6SendCallback(IPv6SendCallback ipv6SendCallback)
    {
        m_ipv6SendPkt = ipv6SendCallback;
    }

    void
	SAGRoutingProtocalIPv6::SetSendCallback (Callback<void, Time, uint32_t, uint32_t> sendCb)
    {
    	m_sendCb = sendCb;
    }

} // namespace ns3
