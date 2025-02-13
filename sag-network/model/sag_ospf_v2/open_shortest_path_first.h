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

#ifndef OPEN_SHORTEST_PATH_FIRST_H
#define OPEN_SHORTEST_PATH_FIRST_H

#include <list>
#include <utility>
#include <stdint.h>

#include "ns3/ospf-neighbor.h"
#include "ns3/ospf-packet.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-header.h"
#include "ns3/socket.h"
#include "ns3/ptr.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/arbiter.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/point-to-point-channel.h"
#include "ns3/sag_routing_protocal.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-l3-protocol.h"
#include <map>
#include <unordered_map>
#include "ns3/ospf-build-routing.h"
#include "ns3/sag_routing_table.h"
#include "ns3/sag_routing_table_entry.h"

namespace ns3 {
namespace ospf {
/**
 * \ingroup OSPF
 *
 * \brief OSPF routing protocol
 */
class Open_Shortest_Path_First: public SAGRoutingProtocal {
public:
	/**
	* \brief Get the type ID.
	* \return the object TypeId
	*/
	static TypeId GetTypeId(void);
	static const uint8_t OSPF_PROTOCOL;

	/// constructor
	Open_Shortest_Path_First ();
	virtual ~Open_Shortest_Path_First ();

	// Inherited from SAGRoutingProtocal
	Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr);
	bool RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
				   UnicastForwardCallback ucb, MulticastForwardCallback mcb,
				   LocalDeliverCallback lcb, ErrorCallback ecb);
	virtual void NotifyInterfaceUp (uint32_t i);
	virtual void NotifyInterfaceDown (uint32_t i);
	virtual void NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address);
	virtual void NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address);
	virtual void PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit = Time::S) const;

	// Handle protocol parameters
	/**
	* Set hello enable
	* \param f the hello enable flag
	*/
	void SetHelloEnable (bool f)
	{
		m_enableHello = f;
	}
	/**
	* Get hello enable flag
	* \returns the enable hello flag
	*/
	bool GetHelloEnable () const
	{
		return m_enableHello;
	}
	void SetArea (uint32_t areaId)
	{
		m_areaId = areaId;
	}

protected:
    virtual void DoInitialize (void);

private:
    /// Indicates whether a hello messages enable
    bool m_enableHello;

    /// Handle neighbors
    Neighbors m_nb;
    /// Routing table
    Ptr<SAGRoutingTable> m_routingTable;
    /// Routing algorithm
    Ptr<OspfBuildRouting> m_routeBuild;

    /// Interval of generating Hello message
    Time m_helloInterval;
    /// Router dead interval
    Time m_rtrDeadInterval;
    /// Router dead interval
    Time m_rxmtInterval;
    /// Router dead interval
    Time m_LSRefreshTime;
    /// Indicates each interface's hello expire time
    std::vector<std::pair<Ipv4Address,Time>> m_helloTimeExpireRecord;

private:
    ///\name Receive control packets
	//\{
    /**
	* Receive ospf packet
	* \param p the packet
	* \param receiver the address of receiving interface
	* \param src the address of sending interface
	*/
    void RecvOSPF (Ptr<Packet> p, Ipv4Address receiver, Ipv4Address src);
    /// Receive hello packet
    void RecvHELLO (Ptr<Packet> p, Ipv4Address receiver, Ipv4Address src, OspfHeader ospfHeader);
    /// Receive database description packet
    void RecvDD (Ptr<Packet> p, Ipv4Address my,Ipv4Address src, OspfHeader ospfHeader);
    /// Receive link state request packet
	void RecvLSR (Ptr<Packet> p, Ipv4Address my,Ipv4Address src, OspfHeader ospfHeader);
	/// Receive link state update packet
	void RecvLSU (Ptr<Packet> p, Ipv4Address my,Ipv4Address src, OspfHeader ospfHeader);
    /// Receive link state acknowledge packet
	void RecvLSAck (Ptr<Packet> p, Ipv4Address my,Ipv4Address src, OspfHeader ospfHeader);
	//\}

	///\name Send control packets
	//\{
	/// Send hello packet
	void SendHello (Ipv4Address ad);
	/// Send database description packet
	void SendDD (Ipv4Address ad, Ipv4Address nb, uint8_t flags, uint32_t seqNum, std::vector<LSAHeader> lsaHeaders);
	/// Send link state request packet
	void SendLSR(Ipv4Address ad, Ipv4Address nb,std::vector <LSAHeader> lsaHeaders);
	/// Send link state update packet
	void SendLSU(Ipv4Address ad, Ipv4Address nb,std::vector<std::pair<LSAHeader,LSAPacket>> lsas);
	/// Send link state acknowledge packet
	void SendLSAack(Ipv4Address ad, Ipv4Address nb,std::vector<LSAHeader> lsaheaders);
	//\}

	/// Hello timer
	Timer m_htimer;
	/// Schedule next send event of hello message
    void HelloTimerExpire ();
    /// 1-way received event triggering
    void HelloTriggeringBy1WayReceived (Ipv4Address ad);
    /// 2-Way received event triggering
    void DDTriggeringBy2WayReceived (Ipv4Address ad, Ipv4Address nb, uint8_t flags, uint32_t seqNum, std::vector<LSAHeader> lsa);
    /// LSU Triggering
    void LSUTriggeringByLSRReceived(Ipv4Address ad, Ipv4Address nb,std::vector<std::pair<LSAHeader,LSAPacket>> lsas);
    /// LSA Triggering
    void LSAaackTriggeringByLSUReceived(Ipv4Address ad, Ipv4Address nb,std::vector<LSAHeader> lsaheaders);
    /// LSR Triggering
    void LSRTriggering(Ipv4Address ad, Ipv4Address nb,std::vector <LSAHeader> lsaHeaders);


    /// Provides uniform random variables.
    Ptr<UniformRandomVariable> m_uniformRandomVariable;

    uint32_t m_areaId;
    bool m_promptMode;
};

} // namespace ospf
} // namespace ns3



#endif /* OPEN_SHORTEST_PATH_FIRST_H */
