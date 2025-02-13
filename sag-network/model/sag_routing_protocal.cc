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
  if (m_ipv4 && m_ipv4->GetObject<Node> ()) { \
      std::clog << Simulator::Now ().GetSeconds () \
                << " [node " << m_ipv4->GetObject<Node> ()->GetId () << "] "; }

#include <iomanip>
#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/ipv4-route.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/sag_routing_protocal.h"
#include "ns3/arbiter-single-forward.h"
#include "ns3/mpi-interface.h"

namespace ns3 {

    NS_LOG_COMPONENT_DEFINE ("SAGRoutingProtocal");

    NS_OBJECT_ENSURE_REGISTERED (SAGRoutingProtocal);

    TypeId
    SAGRoutingProtocal::GetTypeId(void) 
    {
        static TypeId tid = TypeId("ns3::SAGRoutingProtocal")
                .SetParent<Ipv4RoutingProtocol>()
                .SetGroupName("Internet")
                .AddConstructor<SAGRoutingProtocal>()
        .AddAttribute ("ControlPacketProcessDelay", "The processing delay of the control packet",
				TimeValue (MilliSeconds (5)),
				MakeTimeAccessor (&SAGRoutingProtocal::m_processDelay),
				MakeTimeChecker ())
		.AddAttribute ("EnableRtrCalculateTime", "Indicates whether router's CPU run time considered when calculating routing algorithm.",
				 BooleanValue (true),
				 MakeBooleanAccessor (&SAGRoutingProtocal::m_rtrCalTimeConsidered),
				 MakeBooleanChecker ())
		.AddAttribute("SatellitesNumber", "Satellites number in the simulation topology.",
				UintegerValue(0),
				MakeUintegerAccessor(&SAGRoutingProtocal::m_satellitesNumber),
				MakeUintegerChecker<uint32_t>())
		.AddAttribute("GroundStationNumber", "Ground station number in the simulation topology.",
				UintegerValue(0),
				MakeUintegerAccessor(&SAGRoutingProtocal::m_groundStationNumber),
				MakeUintegerChecker<uint32_t>())
		.AddAttribute("ConstellationTopology", "Topology information of constellation.",
				PointerValue(0),
				MakePointerAccessor (&SAGRoutingProtocal::m_walkerConstellation),
				MakePointerChecker<Constellation>())
		.AddAttribute ("BaseDir",
				"Directory (trace file will be placed here",
				StringValue (""),
				MakeStringAccessor (&SAGRoutingProtocal::m_baseDir),
				MakeStringChecker ())
				;
        return tid;
    }

    SAGRoutingProtocal::SAGRoutingProtocal() : m_ipv4(0) 
    {
        NS_LOG_FUNCTION(this);
    }

    SAGRoutingProtocal::~SAGRoutingProtocal() 
    {
        NS_LOG_FUNCTION(this);
    }

    void
    SAGRoutingProtocal::NotifyInterfaceUp(uint32_t i) 
    {

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
                //throw std::runtime_error("Each interface must have a subnet mask of 255.255.255.0");
            }

        }

    }

    void
    SAGRoutingProtocal::NotifyInterfaceDown(uint32_t i) 
    {
//        throw std::runtime_error("Interfaces are not permitted to go down.");
    }

    void
    SAGRoutingProtocal::NotifyAddAddress(uint32_t interface, Ipv4InterfaceAddress address) 
    {
//        if (m_ipv4->IsUp(interface)) {
//            throw std::runtime_error("Not permitted to add IP addresses after the interface has gone up.");
//        }
    }

    void
    SAGRoutingProtocal::NotifyRemoveAddress(uint32_t interface, Ipv4InterfaceAddress address) 
    {
//        if (m_ipv4->IsUp(interface)) {
//            throw std::runtime_error("Not permitted to remove IP addresses after the interface has gone up.");
//        }
    }

    void
    SAGRoutingProtocal::SetIpv4(Ptr <Ipv4> ipv4) 
    {
        NS_LOG_FUNCTION(this << ipv4);
        NS_ASSERT(m_ipv4 == 0 && ipv4 != 0);
        m_ipv4 = ipv4;
        for (uint32_t i = 0; i < m_ipv4->GetNInterfaces(); i++) {
            if (m_ipv4->IsUp(i)) {
                NotifyInterfaceUp(i);
            } else {
                NotifyInterfaceDown(i);
            }
        }
        m_nodeId = m_ipv4->GetObject<Node>()->GetId();
        m_systemId = m_ipv4->GetObject<Node>()->GetSystemId();
        SetIPv4SendCallback (MakeCallback (&Ipv4::Send, ipv4));
    }

    void 
    SAGRoutingProtocal::PrintRoutingTable(Ptr<OutputStreamWrapper> stream, Time::Unit unit) const 
    {
        std::ostream* os = stream->GetStream ();
        *os << m_arbiter->StringReprOfForwardingState();
    }

    void
    SAGRoutingProtocal::SetArbiter (Ptr<Arbiter> arbiter) 
    {
        m_arbiter = arbiter;
    }

    Ptr<Arbiter>
    SAGRoutingProtocal::GetArbiter () 
    {
        return m_arbiter;
    }

    ///////////////////////////////////////// Routing Protocal Forwarding surface

    Ptr<Ipv4Route>
    SAGRoutingProtocal::LookupArbiter (const Ipv4Address& dest, const Ipv4Header &header, Ptr<const Packet> p, Ptr<NetDevice> oif) 
    {
        // Arbiter must be set
        if (m_arbiter == 0) {
            throw std::runtime_error("Arbiter has not been set");
        }


        // Multi-cast not supported
        if (dest.IsLocalMulticast()) {
            throw std::runtime_error("Multi-cast is not supported");
        }

        // No support for requested output interfaces
        if (oif != 0) {
            throw std::runtime_error("Requested output interfaces are not supported");
        }

        // Decide interface index
        uint32_t if_idx;
        uint32_t gateway_ip_address;
        if (loopbackMask.IsMatch(dest, loopbackIp)) { // Loop-back
            if_idx = 0;
            gateway_ip_address = 0;

        } else { // If not loop-back, it goes to the arbiter
                 // Local delivery has already been handled if it was input

            ArbiterResultIPv4 result = m_arbiter->BaseDecide(p, header);
//            for(auto x: m_arbiter->GetObject<ArbiterSingleForward>()->m_next_hop_list){
//            	std::tuple kk = x.second;
//            	std::cout<<MpiInterface::GetSystemId()<<":"<<m_nodeId<<","<<x.first<<","<<std::get<0>(kk)<<","<<std::to_string(std::get<1>(kk))<<","<<std::to_string(std::get<2>(kk))<<std::endl;
//            }
            if (result.Failed()) {
            	//std::cout<<"@@@@@@@@@@@"<<m_nodeId<<std::endl;
                return 0;
            } else {
            	//std::cout<<"@@@@@@@@@@@!!!!!"<<m_nodeId<<std::endl;
                if_idx = result.GetOutIfIdx();
                gateway_ip_address = result.GetGatewayIpAddress();
            }

        }

        // Create routing entry
        Ptr<Ipv4Route> rtentry = Create<Ipv4Route>();
        rtentry->SetDestination(dest);
        rtentry->SetSource(m_ipv4->SourceAddressSelection(if_idx, dest)); // This is basically the IP of the interface
                                                                          // It is used by a transport layer to
                                                                          // determine its source IP address
        rtentry->SetGateway(Ipv4Address(gateway_ip_address)); // If the network device does not care about ARP resolution,
                                                              // this can be set to 0.0.0.0
        rtentry->SetOutputDevice(m_ipv4->GetNetDevice(if_idx));
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
     * @return IPv4 route
     */
    Ptr <Ipv4Route>
    SAGRoutingProtocal::RouteOutput(Ptr <Packet> p, const Ipv4Header &header, Ptr <NetDevice> oif, Socket::SocketErrno &sockerr) 
    {
        NS_LOG_FUNCTION(this << p << header << oif << sockerr);
        Ipv4Address destination = header.GetDestination();

        // Multi-cast to multiple interfaces is not supported
        if (destination.IsMulticast()) {
            throw std::runtime_error("Multi-cast not supported");
        }

        // Perform lookup
        // Info: If no route is found for a packet with the header with source IP = 102.102.102.102,
        //       the TCP socket will conclude there is no route and not even send out SYNs (any real packet).
        //       If source IP is set already, it just gets dropped and the TCP socket sees it as a normal loss somewhere in the network.
        Ptr<Ipv4Route> route = LookupArbiter(destination, header, p, oif);


        //std::cout<<"SAG test:"+ std::to_string(m_nodeId) +" Routing Lookup"<<std::endl;


        if (route == 0) {
            sockerr = Socket::ERROR_NOROUTETOHOST;
        } else {
            sockerr = Socket::ERROR_NOTERROR;
        }
        return route;

    }

    bool
    SAGRoutingProtocal::RouteInput(Ptr<const Packet> p, const Ipv4Header &ipHeader, Ptr<const NetDevice> idev,
                                          UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                                          LocalDeliverCallback lcb, ErrorCallback ecb) 
    {
        NS_ASSERT(m_ipv4 != 0);

        //std::cout<<"SAG test:"+ std::to_string(m_nodeId) +" Routing Lookup"<<std::endl;

        // Check if input device supports IP
        NS_ASSERT(m_ipv4->GetInterfaceForDevice(idev) >= 0);
        uint32_t iif = m_ipv4->GetInterfaceForDevice(idev);

        // Multi-cast
        if (ipHeader.GetDestination().IsMulticast()) {
            throw std::runtime_error("Multi-cast not supported.");
        }

        // Local delivery
        if (m_ipv4->IsDestinationAddress(ipHeader.GetDestination(), iif)) { // WeakESModel is set by default to true,
                                                                            // as such it works for any IP address
                                                                            // on any interface of the node
            if (lcb.IsNull()) {
                throw std::runtime_error("Local callback cannot be null");
            }
            // Info: If you want to decide that a packet should not be delivered (dropped),
            //       you can decide that here by not calling lcb(), but still returning true.
            lcb(p, ipHeader, iif);
            return true;
        }

        // Check if input device supports IP forwarding
        if (m_ipv4->IsForwarding(iif) == false) {
            throw std::runtime_error("Forwarding must be enabled for every interface");
        }

        // Uni-cast delivery
        Ptr<Ipv4Route> route = LookupArbiter(ipHeader.GetDestination(), ipHeader, p);
        if (route == 0) {

            // Lookup failed, so we did not find a route
            // If there are no other routing protocols, this will lead to a drop
            return false;

        } else {

            // Lookup succeeded in producing a route
            // So we perform the unicast callback to forward there
            ucb(route, p, ipHeader);
            return true;

        }

    }

    /////////////////////////// Routing Protocal Control surface
    void 
    SAGRoutingProtocal::RouteProtocal (Ptr<Packet> p, const Ipv4Header &header)
    {
        std::cout << "SAG Test Destributed Router" << std::endl;
        Ptr<Packet> packet = p->Copy ();

//        SAGRoutingProtocalHeader routingHeader;
//        packet->RemoveHeader (routingHeader);

        //you can mantain your routing protocal using information from routingHeader
        // many things ...
    }  

    void 
	SAGRoutingProtocal::SendRoutingProtocalPacket (Ptr<Packet> packet, Ipv4Address source,
	    		Ipv4Address destination, uint8_t protocol, Ptr<Ipv4Route> route)
    {

        Simulator::Schedule(m_processDelay, &SAGRoutingProtocal::DoSendRoutingProtocalPacket, this,
        		packet, source, destination, protocol, route);
//    	DoSendRoutingProtocalPacket(packet, source, destination, protocol, route);

    }

    void
	SAGRoutingProtocal::DoSendRoutingProtocalPacket (Ptr<Packet> packet, Ipv4Address source,
        	    		Ipv4Address destination, uint8_t protocol, Ptr<Ipv4Route> route){

    	if(!m_sendCb.IsNull ()){
    		m_sendCb(Simulator::Now(), packet->GetSize(), m_nodeId);
    	}
        m_ipv4SendPkt(packet, source, destination, protocol, route);

    }



    void
    SAGRoutingProtocal::RouteTrace(Ptr<Packet> p){

    	// Here, we must determine whether p is null, because TCP connections will bring null pointers to RouteOutput.
    	if(!p){
    		return;
    	}

    	// peek tag
    	RouteTraceTag rtTrTagOld;
    	if(p->PeekPacketTag(rtTrTagOld)){

			std::vector<uint32_t> nodesPassed = rtTrTagOld.GetRouteTrace();
			nodesPassed.push_back((m_ipv4->GetObject<Node>())->GetId());
			RouteTraceTag rtTrTagNew((uint8_t)(nodesPassed.size()), nodesPassed);
			p->RemovePacketTag(rtTrTagOld);
			p->AddPacketTag(rtTrTagNew);

    	}

    }

    void
    SAGRoutingProtocal::CalculationTimeLog(uint32_t nodeId, Time curTime, double calculationTime){

    	if(!m_rtrCalCb.IsNull()){
    		m_rtrCalCb(nodeId, curTime, calculationTime);
    	}

    }

    void
	SAGRoutingProtocal::SetRoutingCalculationCallback(Callback<void, uint32_t, Time, double> rtrCalCb)
    {
    	m_rtrCalCb = rtrCalCb;
    }

    void 
    SAGRoutingProtocal::SetIPv4SendCallback(IPv4SendCallback ipv4SendCallback)
    {
        m_ipv4SendPkt = ipv4SendCallback;
    }

    void
	SAGRoutingProtocal::SetSendCallback (Callback<void, Time, uint32_t, uint32_t> sendCb)
    {
    	m_sendCb = sendCb;
    }

} // namespace ns3
