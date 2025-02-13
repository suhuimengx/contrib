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

#ifndef SAG_ROUTING_IPV6_H
#define SAG_ROUTING_IPV6_H

#include <list>
#include <utility>
#include <stdint.h>

#include "ns3/ipv6-address.h"
#include "ns3/ipv6-header.h"
#include "ns3/socket.h"
#include "ns3/ptr.h"
#include "ns3/ipv6.h"
#include "ns3/ipv6-routing-protocol.h"
#include "ns3/arbiter.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/point-to-point-channel.h"
#include "ns3/route_trace_tag.h"
#include "ns3/walker-constellation-structure.h"

namespace ns3 {

class Packet;
class NetDevice;
class Ipv6Interface;
class Ipv6Address;
class Ipv6Header;
class Node;

class SAGRoutingProtocalIPv6 : public Ipv6RoutingProtocol
{
public:
    static TypeId GetTypeId (void);

    // constructor
    SAGRoutingProtocalIPv6 ();
    virtual ~SAGRoutingProtocalIPv6 ();

    // Inherited from Ipv6RoutingProtocol
    virtual void NotifyInterfaceUp (uint32_t interface);
    virtual void NotifyInterfaceDown (uint32_t interface);
    virtual void NotifyAddAddress (uint32_t interface, Ipv6InterfaceAddress address);
    virtual void NotifyRemoveAddress (uint32_t interface, Ipv6InterfaceAddress address);
    virtual void NotifyAddRoute (Ipv6Address dst, Ipv6Prefix mask, Ipv6Address nextHop, uint32_t interface, Ipv6Address prefixToUse = Ipv6Address::GetZero ());
    virtual void NotifyRemoveRoute (Ipv6Address dst, Ipv6Prefix mask, Ipv6Address nextHop, uint32_t interface, Ipv6Address prefixToUse = Ipv6Address::GetZero ());
    virtual void PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit = Time::S) const;
    virtual void SetIpv6 (Ptr<Ipv6> ipv6);

    /**
     * Used by the transport-layer to output.
     *
     * /param p       Pointer to packet
     * /param header      IPv6 header of the packet
     * /param oif     Output interface
     * /param sockerr     Socket error (out)
     *
     * /return IPv6 route entry
     */
    virtual Ptr<Ipv6Route> RouteOutput(
        Ptr<Packet> p,
        const Ipv6Header &header,
        Ptr<NetDevice> oif,
        Socket::SocketErrno &sockerr
    );

    /**
     * A packet arrived at an interface, where to deliver next.
     *
     * /param p       Packet
     * /param header      IPv6 header of the packet
     * /param idev    Input interface device
     * /param ucb     Uni-cast forward call-back
     * /param mcb     Multi-cast forward call-back
     * /param lcb     Local delivery call-back (it was destined for here)
     * /param ecb     Error call-back
     *
     * /return True if found an interface to forward to
     */
    virtual bool RouteInput(
        Ptr<const Packet> p,
        const Ipv6Header &header,
        Ptr<const NetDevice> idev,
        UnicastForwardCallback ucb,
        MulticastForwardCallback mcb,
        LocalDeliverCallback lcb,
        ErrorCallback ecb
        );

    typedef Callback<void, Ptr<Packet>, Ipv6Address, Ipv6Address, uint8_t, Ptr<Ipv6Route>> IPv6SendCallback;


    /**
     * \brief Set IPv6 send callback
     *
     * \param ipv6SendCallback Callback for the event that there are control messages to be sent.
     * 		  This callback is passed a pointer to the packet, source address of packet, destination address of packet,
     * 		  protocol number of packet, and route route entry.
     */
    void SetIPv6SendCallback(IPv6SendCallback ipv6SendCallback);

    /**
     * \brief Set control messages send callback
     *
     * \param ipv6SendCallback Callback for the event that the overhead of control messages are needed to be counted.
     * 		  This callback is passed the timestamp of the send event, packet bytes and current node id.
     */
    void SetSendCallback (Callback<void, Time, uint32_t, uint32_t> sendCb);

    /**
     * \brief Set the arbiter entry for the routing protocol
     *
     * \param arbiter the arbiter entry
     *
     */
    void SetArbiter (Ptr<Arbiter> arbiter);

    /**
     * \brief Get the arbiter entry of the routing protocol
     *
     * \return the arbiter entry
     */
    Ptr<Arbiter> GetArbiter ();

    /**
     * \brief Receive and process control packets
     *
     * \param p       A pointer to the packet
     * \param header      IPv6 header of the packet
     *
     */
    virtual void RouteProtocal (Ptr<Packet> p, const Ipv6Header &header);

    /**
     * \brief Schedule control packet sending events.
     *
     * \param packet packet to send
     * \param source source address of packet
     * \param destination destination address of packet
     * \param protocol number of packet
     * \param route route entry
     *
     */
    virtual void SendRoutingProtocalPacket (Ptr<Packet> packet, Ipv6Address source,
    		Ipv6Address destination, uint8_t protocol, Ptr<Ipv6Route> route);

    /**
     * \brief Execute control packet sending.
     *
     * \param packet packet to send
     * \param source source address of packet
     * \param destination destination address of packet
     * \param protocol number of packet
     * \param route route entry
     *
     */
    virtual void DoSendRoutingProtocalPacket (Ptr<Packet> packet, Ipv6Address source,
    	    		Ipv6Address destination, uint8_t protocol, Ptr<Ipv6Route> route);

    /**
     * \brief Record route tag.
     *
     * \param p A pointer to packet
     *
     */
    void RouteTrace(Ptr<Packet> p);

    /**
     * \brief Set routing calculation callback.
     *
     * \param ipv6SendCallback Callback for the event that the route table calculation time is needed to be recorded.
     *
     */
    void SetRoutingCalculationCallback(Callback<void, uint32_t, Time, double> rtrCalCb);

    void CalculationTimeLog(uint32_t nodeId, Time curTime, double calculationTime);




protected:
    //virtual SAGRoutingProtocalHeaderIPv6 BuildRoutingHeader(uint16_t parameter1);

    Ptr<Ipv6> m_ipv6;
    /**
     * Lookup the route towards the destination.
     * Does not handle local delivery.
     *
     * \param dest      Destination IP address
     * \param header    IPv6 header
     * \param p         Packet
     * \param oif       Requested output interface
     *
     * \return Valid Ipv6 route
     */
    Ptr<Ipv6Route> LookupArbiter (const Ipv6Address& dest, const Ipv6Header &header, Ptr<const Packet> p, Ptr<NetDevice> oif = 0);
    Ptr<Arbiter> m_arbiter = nullptr;
    Ipv6Address m_nodeSingleIpv6Address;
    Ipv6Prefix loopbackMask = Ipv6Prefix(128);
    Ipv6Address loopbackIp = Ipv6Address("::1");
    uint32_t m_nodeId;
    uint32_t m_systemId;
    IPv6SendCallback m_ipv6SendPkt;
    Callback<void, Time, uint32_t, uint32_t> m_sendCb;
    Callback<void, uint32_t, Time, double> m_rtrCalCb;
    Time m_processDelay;
    bool m_rtrCalTimeConsidered;
    uint32_t m_satellitesNumber;
    uint32_t m_groundStationNumber;
    Ptr<Constellation> m_walkerConstellation;

};

} // Namespace ns3

#endif /* SAG_ROUTING_IPV6_H */
