/*
 * tlr-neighbor.h
 *
 *  Created on: 2023年
 *      Author: 
 */

#ifndef TLRNEIGHBOR_H
#define TLRNEIGHBOR_H

#include <vector>
//#include "ns3/queue.h"
#include "ns3/tlr_link_state_packet.h"
#include "ns3/tlr-link-state-database.h"
#include "ns3/point-to-point-laser-net-device.h"
//#include "ns3/sag_link_layer_gsl.h"
#include "ns3/simulator.h"
#include "ns3/timer.h"
#include "ns3/ipv4-address.h"
#include "ns3/callback.h"
#include "ns3/arp-cache.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/mobility-model.h"
#include "tlr-build-routing.h"
namespace ns3 {

class WifiMacHeader;

namespace tlr {

class Traffic_Light_Based_Routing;

/**
 * Neighbor state
 */
enum NeighborState
{
	NeighborState_Down  = 1,
	NeighborState_Init  = 2,
	NeighborState_2Way  = 3,
	NeighborState_ExStart = 4,
	NeighborState_Exchange = 5,
	NeighborState_Loading = 6,
	NeighborState_Full = 7
};

/**
 * \ingroup TLR
 * \brief maintain list of active neighbors
 */
class Neighbors
{
public:
	Neighbors ();
	/**
	 * constructor
	 * \param delay the delay time for purging the list of neighbors
	 */
	Neighbors (Time helloInterval, Time rtrDeadInterval, Time rxmtInterval, Time LSRefreshInterval);
	/**
	 * Neighbor Data Structure
	 */
	struct Neighbor
	{
		/// Area ID
		Ipv4Address m_areaID;
		/// Local Interface
		Ipv4Address m_localAddress;

		/// Neighbor's Router ID
		Ipv4Address m_routerID;
		/// my Router ID
		Ipv4Address m_myRouterID;
		/// Neighbor Interface's Address
		Ipv4Address m_neighborAddress;
		/// Neighbor State
		NeighborState m_state;
		/// Mode: Myself is Master or Slave in link state information exchange process
		uint8_t m_mode;
		/// Router Priority: to identify the priority of neighboring routers (for subsequent DR role elections)
		uint8_t m_rtrPri;
		/// DR
		Ipv4Address m_dr;
		/// BDR
		Ipv4Address m_bdr;
		/// Neighbor expire time
		Time m_expireTime;
		/// Authentication Sequence
		uint8_t m_authSeq;
		/// DD Sequence Number
		uint32_t m_ddSeqNum;

		/// Last received Database Description packet's flag
		uint8_t m_flagsLast;
		/// Last received Database Description packet's option
		uint8_t m_optionLast;
		/// Last received Database Description packet's DD sequence
		uint32_t m_ddSeqNumLast;

		/// Link state retransmission list
		std::vector<std::pair<LSAHeader,LSAPacket>> m_lsRetransList;
		/// Database summary list
		std::vector<LSAHeader> m_dbSummaryList;
		/// Link state request list
		std::vector<LSAHeader> m_lsRequestList;


		/// The future DD retransmit event scheduled for master.
		EventId m_DDRetransmitEvent;

		/// The last Database Description packet freeing event scheduled for slave
		EventId m_lastDDFreeingEvent;
		/// The last sent Database Description packet freeing or not
		bool m_lastSentDDFreeingOrNot;
		/// Flags in last sent Database Description packet
		uint8_t m_flagsInLastSentDD;
		/// LSAHeaders in last sent Database Description packet
		std::vector<LSAHeader> m_lsInLastSentDD;

		/// The future request retransmit event scheduled
		EventId m_requestRetransmitEvent;
		/// Requests transmitted
		std::vector<LSAHeader> m_requestTransmitted;
		/// The future request retransmit event scheduled
		EventId m_lsRetransmitEvent;
		/// LSAs transmitted
		std::vector<LSAHeader> m_LSATransmitted;


		/**
		* \brief Neighbor data structure constructor
		*
		* \param ...
		*/
		Neighbor (Ipv4Address areaID, Ipv4Address localAddress, Ipv4Address routerID, Ipv4Address myrouterID, Ipv4Address neighborAddress, NeighborState state,
				 uint8_t rtrPri, Ipv4Address dr, Ipv4Address bdr, Time expireTime)
		{
			m_areaID = areaID;
			m_localAddress = localAddress;
			m_routerID = routerID;
			m_myRouterID = myrouterID;
			m_neighborAddress = neighborAddress;
			m_state = state;
			m_mode = 0;
			m_rtrPri = rtrPri;
			m_dr = dr;
			m_bdr = bdr;
			m_expireTime = expireTime;
			m_authSeq = 0;

			m_ddSeqNum = 0;
			m_flagsLast = 0;
			m_optionLast = 0;
			m_ddSeqNumLast = 0;

			m_lsRetransList = {};
			m_lsRequestList = {};

			m_DDRetransmitEvent = EventId();
			m_lastDDFreeingEvent = EventId();
			m_lastSentDDFreeingOrNot = true;
			m_flagsInLastSentDD = 0;
			m_lsInLastSentDD = {};
			m_requestRetransmitEvent = EventId();
			m_lsRetransmitEvent = EventId();
		}
	};

  /**
   * Return expire time for neighbor node with address addr, if exists, else return 0.
   * \param addr the IP address of the neighbor node
   * \returns the expire time for the neighbor node
   */
  Time GetExpireTime (Ipv4Address addr);
  /**
   * Check that node with address addr is neighbor
   * \param addr the IP address to check
   * \returns true if the node with IP address is a neighbor
   */
  bool IsNeighbor (Ipv4Address addr);

  std::vector<Ipv4Address> GetNeighbors ();
  /**
   * Update expire time for entry with address addr, if it exists, else add new entry
   * \param addr the IP address to check
   * \param expire the expire time for the address
   */
  void HelloReceived (Ipv4Address murouterId,Ipv4Address areaID, Ipv4Address localAddress, Ipv4Address routerID, Ipv4Address neighborAddress,
		  uint8_t rtrPri, Ipv4Address dr, Ipv4Address bdr, Time expireTime, std::vector<Ipv4Address> neighors);
  /// 2-way received
  void TwoWayReceived (std::vector<Neighbor>::iterator i);
  /// 1-way received
  void OneWayReceived (std::vector<Neighbor>::iterator i);
  /// master/slave negotiation done
  void NegotiationDone (std::vector<Neighbor>::iterator i);
  /// Exchange Done
  void ExchangeDone (std::vector<Neighbor>::iterator i);
  /// The (possibly partially formed) adjacency is torn down, and then an attempt is made at reestablishment.
  void SeqNumberMismatch (std::vector<Neighbor>::iterator i);




  /// Update neighbor state, include: 1-WayReceived & 2-WayReceived Event
  void UpdateState(std::vector<Neighbor>::iterator i, std::vector<Ipv4Address> neighors);
  /// DD Received
  void DDReceived (Ipv4Address routerID, Ipv4Address nb, uint8_t flags, uint32_t seqNum, std::vector<LSAHeader> lsas);
  /// DD Processed
  void DDProcessed (std::vector<Neighbor>::iterator i, uint8_t flags, uint32_t seqNum, std::vector<LSAHeader> lsas);
  /// DD Retransmit Schedule
  void DDRetransmit (Ipv4Address localAddress, Ipv4Address neighborAddress, uint8_t flags, uint32_t seqNum);
  /// Request Retransmit Schedule
  void RequestRetransmit (Ipv4Address localAddress, Ipv4Address neighborAddress);
  /// Update Retransmit Schedule
  void UpdateRetransmit (Ipv4Address localAddress, Ipv4Address neighborAddress);
  /// DD Freeing Schedule For Slave
  void DDFreeingForSlave (Ipv4Address localAddress, Ipv4Address neighborAddress, uint8_t flags, uint32_t seqNum);
  ///#Mengy:LSR Received
  void LSRReceived (Ipv4Address myOwnrouterID, Ipv4Address nbAddress, LSRHeader lsr);
  ///#Mengy:LSU Received
  void LSUReceived (Ipv4Address myOwnrouterID, Ipv4Address nbAddress, LSUHeader lsu);
  ///#Mengy:LSAack Received
  void LSAckReceived (Ipv4Address myOwnrouterID, Ipv4Address nbAddress, LSAackHeader lsack);
  ///#Mengy:Flod
  //void Flood (std::pair<LSAHeader,LSAPacket> lsa,Ipv4Address myOwnrouterID,Ipv4Address nbAddress);
  void Flood (std::vector<std::pair<LSAHeader,LSAPacket>> lsas, Ipv4Address myOwnrouterID, Ipv4Address nbAddress);
  /// Remove all expired entries
  void Purge ();
  /// Schedule m_ntimer.
  void ScheduleTimer ();
  /// Update LSDB when all UP interfaces' corresponding neighbors' states are 2-Way, it's just for speeding up the simulation.
  /// If you don't consider accelerating the simulation, change LSDBUpdateDecision() to UpdateLSDB() in HelloReceived!
  void LSDBUpdateDecision();
  /// Update LSDB
  void UpdateLSDB();
  /// LSA Periodic Origination
  void LSAPeriodicOrigination();
  /// Delete neighbor because of interface down
  void DeleteNeighbor(Ipv4Address murouterId, Ipv4Address myAddress);
  /// Delete neighbor because of interface down
  void DeleteNeighbor(std::vector<Neighbor>::iterator i);
  void DeleteNeighborsPMPPTriggering(Ipv4Address myPointAdr, std::vector<Ipv4Address> multiPointsAdrs);
  /// Remove all entries
  void Clear ()
  {
    m_nb.clear ();
  }
  /// Get DD Sequence Number
  uint32_t GetDDSequenceNumber(Ipv4Address ad);
  /// Add DD Sequence Number
  uint32_t AddDDSequenceNumberByOne(Ipv4Address ad);
  /// set RouterId
  void SetRouterId(Ipv4Address routerId);



  /// Describes the current top of the Database summary list
  std::vector<LSAHeader> GetTopOfDBSummaryList(std::vector<Neighbor>::iterator i);
  /// Describes the current top of the Database summary list
  void DeleteTopOfDBSummaryList(std::vector<Neighbor>::iterator i);
  /// Send From Request List
  void SendFromRequestList(std::vector<Neighbor>::iterator i);
  /// Send From Retransmission List
  void SendFromRetransmissionList(std::vector<Neighbor>::iterator i);
  /// Delete From Request List
  void DeleteFromRequestList(std::vector<Neighbor>::iterator i, std::vector<LSAHeader> lsaack);
  /// Delete From LSA Retransmission List
  void DeleteFromRetransmissionList(std::vector<Neighbor>::iterator i, std::vector<LSAHeader> lasack);
  /// Delete LSA In the LSRList
  //void DeleteLSAInLSRList(std::vector<Neighbor>::iterator i, LSAHeader lsaHeader);
  /// Delete LSA In the LSA Retransmission List
  //void DeleteLSAInRetransmissionList(std::vector<Neighbor>::iterator i, LSAHeader lsaHeader);


  void SetRouterBuildCallback (Callback<void, std::map<TLRLinkStateIdentifier, std::pair<LSAHeader,LSAPacket>>*, Ipv4Address, std::map<Ipv4Address, std::pair<Ipv4Address, Ipv4Address>>> cb)
    {
      m_lsdb.SetRouterBuildCallback(cb);
    }


  /**
   * Set link failure callback
   * \param cb the callback function
   */
  void SetLinkFailureCallback (Callback<void, Ipv4Address> cb)
  {
    m_handleLinkFailure = cb;
  }
  /**
   * Get link failure callback
   * \returns the link failure callback
   */
  Callback<void, Ipv4Address> GetLinkFailureCallback () const
  {
    return m_handleLinkFailure;
  }
	/**
	 * Set hello triggering callback
	 * \param cb the callback function
	 */
	void SetHelloTriggeringCallback (Callback<void, Ipv4Address> cb)
	{
		m_handleHelloTriggering = cb;
	}
	/**
	 * Get hello triggering callback
	 * \returns the hello callback
	 */
	Callback<void, Ipv4Address> GetHelloTriggeringCallback () const
	{
	  return m_handleHelloTriggering;
	}

	/**
	 * Set database description triggering callback
	 * \param cb the callback function
	 */
	void SetDDTriggeringCallback (Callback<void, Ipv4Address, Ipv4Address, uint8_t, uint32_t, std::vector<LSAHeader>> cb)
	{
		m_handleDDTriggering = cb;
	}
	/**
	 * Get database description triggering callback
	 * \returns the database description callback
	 */
	Callback<void, Ipv4Address, Ipv4Address, uint8_t, uint32_t, std::vector<LSAHeader>> GetDDTriggeringCallback () const
	{
	  return m_handleDDTriggering;
	}


	void SetLSUTriggeringCallback (Callback<void,Ipv4Address, Ipv4Address, std::vector<std::pair<LSAHeader,LSAPacket>>> cb)
	{
		m_handleLSUTriggering = cb;
	}

	Callback<void,Ipv4Address, Ipv4Address, std::vector<std::pair<LSAHeader,LSAPacket>>> GetLSUTriggeringCallback () const
	{
	  return m_handleLSUTriggering;
	}

	void SetLSATriggeringCallback (Callback<void,Ipv4Address, Ipv4Address,std::vector<LSAHeader>> cb)
	{
		m_handleLSAackTriggering = cb;
	}

	Callback<void,Ipv4Address, Ipv4Address,std::vector<LSAHeader>>  GetLSATriggeringCallback () const
	{
	  return m_handleLSAackTriggering;
	}

	void SetLSRTriggeringCallback (Callback<void,Ipv4Address, Ipv4Address,std::vector<LSAHeader>> cb)
	{
		m_handleLSRTriggering = cb;
	}

	Callback<void,Ipv4Address, Ipv4Address,std::vector<LSAHeader>>  GetLSRTriggeringCallback () const
	{
	  return m_handleLSRTriggering;
	}

	void SetIpv4(Ptr<Ipv4> ipv4){
		m_ipv4 = ipv4;
	}

	uint16_t GetLinkCost(Ipv4Address ad);

    void operator=(const Neighbors &nb)
    {
    	m_helloInterval = nb.m_helloInterval;
    	m_ntimer.SetDelay (m_helloInterval);
    	m_ntimer.SetFunction (&Neighbors::Purge, this);
    	m_LSRefreshTime = nb.m_LSRefreshTime;
    	m_routerDeadInterval = nb.m_routerDeadInterval;
    	m_rxmtInterval = nb.m_rxmtInterval;
    }


private:
	/// link failure callback
	Callback<void, Ipv4Address> m_handleLinkFailure;
	/// hello triggering callback
	Callback<void, Ipv4Address> m_handleHelloTriggering;
	/// database description packet triggering callback
	Callback<void, Ipv4Address, Ipv4Address, uint8_t, uint32_t, std::vector<LSAHeader>> m_handleDDTriggering;
	/// LSU sending triggering callback
	Callback<void,Ipv4Address, Ipv4Address,std::vector<std::pair<LSAHeader,LSAPacket>>> m_handleLSUTriggering;
	/// LSA sending triggering callback
	Callback<void,Ipv4Address, Ipv4Address,std::vector<LSAHeader>> m_handleLSAackTriggering;
	/// LSR sending triggering callback
	Callback<void,Ipv4Address, Ipv4Address,std::vector<LSAHeader>> m_handleLSRTriggering;
	/// Timer for neighbor's list
	Timer m_ntimer;
	/// vector of entries
	std::vector<Neighbor> m_nb;
	/// vector of neighbors' router ID for facility
	std::vector<Ipv4Address> m_nbAdr;
	/// Hello interval
	Time m_helloInterval;
	/// LSA retransmit timer interval
	Time m_rxmtInterval;
	/// Router Dead Interval
	Time m_routerDeadInterval;
	/// LSDB
	TLRLSDB m_lsdb;
	/// LSRefreshTime
	Time m_LSRefreshTime;
	/// LS Originated Event
	EventId m_LSOriginatedEvent;
	/// myRouterId
	Ipv4Address m_myRouterId;
	/// ipv4
	Ptr<Ipv4> m_ipv4;



};

}  // namespace tlr
}  // namespace ns3

#endif /* TLRNEIGHBOR_H */
