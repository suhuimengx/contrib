/*
 * open_shortest_path_first.h
 *
 *  Created on: 2023年1月5日
 *      Author: root
 */




#ifndef FYBBR_ROUT_H
#define FYBBR_ROUT_H

#include <list>
#include <utility>
#include <stdint.h>

#include "ns3/fybbr-neighbor.h"
#include "ns3/fybbr-packet.h"

//#include "ns3/centralized_packet.h"

#include "ns3/ipv4-address.h"
#include "ns3/ipv4-header.h"
#include "ns3/socket.h"
#include "ns3/ptr.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/arbiter.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/point-to-point-channel.h"
//#include "ns3/sag_routing_protocal_header.h"
#include "ns3/sag_routing_protocal.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-l3-protocol.h"
#include <map>
#include <unordered_map>
#include "ns3/fybbr-routing-table.h"
#include "ns3/fybbr-build-routing.h"
#include "ns3/sag_routing_table.h"
#include "ns3/sag_routing_table_entry.h"
#include "fybbr-queue.h"

namespace ns3 {
namespace fybbr {

class Fybbr_Rout: public SAGRoutingProtocal {
public:
	static TypeId GetTypeId(void);
	static const uint8_t FYBBR_PROTOCOL;

	/// constructor
    Fybbr_Rout ();
	virtual ~Fybbr_Rout ();

	// Inherited from SAGRoutingProtocal
	Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr);
	bool RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
				   UnicastForwardCallback ucb, MulticastForwardCallback mcb,
				   LocalDeliverCallback lcb, ErrorCallback ecb);
	virtual void NotifyInterfaceUp (uint32_t i);
	virtual void NotifyInterfaceDown (uint32_t i);
	virtual void NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address);
	virtual void NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address);

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

protected:
    virtual void DoInitialize (void);

private:
    bool m_enableHello;                  ///< Indicates whether a hello messages enable


    /// Raw unicast socket per each IP interface, map socket -> iface address (IP + mask)
    ///std::map< Ptr<Socket>, Ipv4InterfaceAddress > m_socketAddresses;

    /// Handle neighbors
    Neighbors m_nb;
    /// routingtable
    Ptr<SAGRoutingTable> m_routingTable;
    /// Routing algorithm
    Ptr<FybbrBuildRouting> m_routeBuild;

    Time m_routcalInterval;

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
	uint32_t sendnum=0;
	//The controller seq
	uint16_t m_ContrllerNo;
	uint16_t m_serverComputingPower;
	//record the convergence time or not
	//std::vector<int> convergence_record;

	//convergence_record.resize(m_satellitesNumber + m_groundStationNumber,1);

private:
    ///\name Receive control packets
	//\{
    /**
	* Receive fybbr packet
	* \param p the packet
	* \param receiver the address of receiving interface
	* \param src the address of sending interface
	*/
    void RecvFYBBR (Ptr<Packet> p, Ipv4Address receiver, Ipv4Address src);
    /// Receive hello packet
    void RecvHELLO (Ptr<Packet> p, Ipv4Address receiver, Ipv4Address src);
    /// Receive database description packet
    void RecvDD (Ptr<Packet> p, Ipv4Address my,Ipv4Address src);
    /// Receive RTS packet
    void RecvRTS (Ptr<Packet> p, Ipv4Address my, Ipv4Address src);
	/// Send hello packet
	void SendHello (Ipv4Address ad);
	/// Send database description packet
	void SendDD (Ipv4Address ad, Ipv4Address nb, uint8_t flags, uint32_t seqNum, std::vector<std::pair<LSAHeader,LSAPacket>> lsas);
	/// Send RTS packet
	void SendRTS (Ipv4Address ad, Ipv4Address nb, std::pair<RTSHeader,RTSPacket> rts);
	void SendInitialRTS (std::pair<RTSHeader,RTSPacket> rts);

	/// Hello timer
	Timer m_htimer;
	/// Schedule next send event of hello message
    void HelloTimerExpire ();
    /// 1-way received event triggering
    void HelloTriggeringBy1WayReceived (Ipv4Address ad);
    /// 2-Way received event triggering
    void DDTriggeringBy2WayReceived (Ipv4Address ad, Ipv4Address nb, uint8_t flags, uint32_t seqNum, std::vector<std::pair<LSAHeader,LSAPacket>> lsas);
    ///rts triggering
    void RTSTriggeringAftRouterCalculator (Ipv4Address ad, Ipv4Address nb, std::pair<RTSHeader,RTSPacket> rts);
   /* void
    InitialRTSTriggeringAftRouterCalculator (Ipv4Address ad, Ipv4Address nb,std::vector<std::pair<RTSHeader,RTSPacket>>);*/
    void InitialRTSTriggeringAftRouterCalculator (std::pair<RTSHeader,RTSPacket> rts);
    void RTSTimerExpire();

    /// Provides uniform random variables.
    Ptr<UniformRandomVariable> m_uniformRandomVariable;


    // FOR CONTROLLER
	FYBBRServerQueue m_queue;
	EventId m_rtsSendFromQueueEvent;
};

} // namespace ospf
} // namespace ns3


#endif /* OPEN_SHORTEST_PATH_FIRST_H */
