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

#ifndef SAG_ROUTING_H
#define SAG_ROUTING_H

#include <list>
#include <utility>
#include <stdint.h>

#include "ns3/ipv4-address.h"
#include "ns3/ipv4-header.h"
#include "ns3/socket.h"
#include "ns3/ptr.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/arbiter.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/point-to-point-channel.h"
#include "ns3/route_trace_tag.h"
#include "ns3/walker-constellation-structure.h"

namespace ns3 {

class Packet;
class NetDevice;
class Ipv4Interface;
class Ipv4Address;
class Ipv4Header;
class Node;

class SAGRoutingProtocal : public Ipv4RoutingProtocol
{
public:
    static TypeId GetTypeId (void);

    // constructor
    SAGRoutingProtocal ();
    virtual ~SAGRoutingProtocal ();

    // Inherited from Ipv4RoutingProtocol
    virtual void NotifyInterfaceUp (uint32_t interface);
    virtual void NotifyInterfaceDown (uint32_t interface);
    virtual void NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address);
    virtual void NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address);
    virtual void PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit = Time::S) const;
    virtual void SetIpv4 (Ptr<Ipv4> ipv4);

    /**
     * Used by the transport-layer to output.
     *
     * /param p       Pointer to packet
     * /param header      IPv4 header of the packet
     * /param oif     Output interface
     * /param sockerr     Socket error (out)
     *
     * /return IPv4 route entry
     */
    virtual Ptr<Ipv4Route> RouteOutput(
        Ptr<Packet> p,
        const Ipv4Header &header,
        Ptr<NetDevice> oif,
        Socket::SocketErrno &sockerr
    );

    /**
     * A packet arrived at an interface, where to deliver next.
     *
     * /param p       Packet
     * /param header      IPv4 header of the packet
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
        const Ipv4Header &header,
        Ptr<const NetDevice> idev,
        UnicastForwardCallback ucb,
        MulticastForwardCallback mcb,
        LocalDeliverCallback lcb,
        ErrorCallback ecb
        );

    typedef Callback<void, Ptr<Packet>, Ipv4Address, Ipv4Address, uint8_t, Ptr<Ipv4Route>> IPv4SendCallback;


    /**
     * \brief Set IPv4 send callback
     *
     * \param ipv4SendCallback Callback for the event that there are control messages to be sent.
     * 		  This callback is passed a pointer to the packet, source address of packet, destination address of packet,
     * 		  protocol number of packet, and route route entry.
     */
    void SetIPv4SendCallback(IPv4SendCallback ipv4SendCallback);

    /**
     * \brief Set control messages send callback
     *
     * \param ipv4SendCallback Callback for the event that the overhead of control messages are needed to be counted.
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
     * \param header      IPv4 header of the packet
     *
     */
    virtual void RouteProtocal (Ptr<Packet> p, const Ipv4Header &header);

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
    virtual void SendRoutingProtocalPacket (Ptr<Packet> packet, Ipv4Address source,
    		Ipv4Address destination, uint8_t protocol, Ptr<Ipv4Route> route);

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
    virtual void DoSendRoutingProtocalPacket (Ptr<Packet> packet, Ipv4Address source,
    	    		Ipv4Address destination, uint8_t protocol, Ptr<Ipv4Route> route);

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
     * \param ipv4SendCallback Callback for the event that the route table calculation time is needed to be recorded.
     *
     */
    void SetRoutingCalculationCallback(Callback<void, uint32_t, Time, double> rtrCalCb);

    void CalculationTimeLog(uint32_t nodeId, Time curTime, double calculationTime);




protected:
    //virtual SAGRoutingProtocalHeader BuildRoutingHeader(uint16_t parameter1);

    Ptr<Ipv4> m_ipv4;
    /**
     * Lookup the route towards the destination.
     * Does not handle local delivery.
     *
     * \param dest      Destination IP address
     * \param header    IPv4 header
     * \param p         Packet
     * \param oif       Requested output interface
     *
     * \return Valid Ipv4 route
     */
    Ptr<Ipv4Route> LookupArbiter (const Ipv4Address& dest, const Ipv4Header &header, Ptr<const Packet> p, Ptr<NetDevice> oif = 0);
    Ptr<Arbiter> m_arbiter = 0;
    Ipv4Address m_nodeSingleIpAddress;
    Ipv4Mask loopbackMask = Ipv4Mask("255.0.0.0");
    Ipv4Address loopbackIp = Ipv4Address("127.0.0.1");
    uint32_t m_nodeId;
    uint32_t m_systemId;
    IPv4SendCallback m_ipv4SendPkt;
    Callback<void, Time, uint32_t, uint32_t> m_sendCb;
    Callback<void, uint32_t, Time, double> m_rtrCalCb;
    Time m_processDelay;
    bool m_rtrCalTimeConsidered;
    uint32_t m_satellitesNumber;
    uint32_t m_groundStationNumber;
    Ptr<Constellation> m_walkerConstellation;
    std::string m_baseDir;

};

} // Namespace ns3

#endif /* SAG_ROUTING_H */
