/*
 * tlr-neighbor.cc
 *
 *  Created on: 2023年
 *      Author: root
 */

#define NS_LOG_APPEND_CONTEXT                                   \
   std::clog << "[node " << m_myRouterId.Get() << "] ";

#include "ns3/tlr-neighbor.h"

#include <algorithm>
#include "ns3/log.h"
#include "ns3/wifi-mac-header.h"
//#include "ns3/queue.h"
#include "ns3/point-to-point-laser-net-device.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TrafficLightBasedRoutingStateMachine");

namespace tlr {
Neighbors::Neighbors (Time helloInterval, Time rtrDeadInterval, Time rxmtInterval, Time LSRefreshInterval)
  : m_ntimer (Timer::CANCEL_ON_DESTROY)
{
	m_helloInterval = helloInterval;
	m_ntimer.SetDelay (helloInterval);
	m_ntimer.SetFunction (&Neighbors::Purge, this);
	m_rxmtInterval = rxmtInterval;
	m_routerDeadInterval = rtrDeadInterval;
	m_LSRefreshTime = LSRefreshInterval;
	m_lsdb = TLRLSDB();
	m_lsdb.SetFloodCallback(MakeCallback(&Neighbors::Flood,this));
}

Neighbors ::Neighbors()
  : m_ntimer (Timer::CANCEL_ON_DESTROY)
{
	m_lsdb = TLRLSDB();
	m_lsdb.SetFloodCallback(MakeCallback(&Neighbors::Flood,this));
}

bool
Neighbors::IsNeighbor (Ipv4Address addr)
{
  for (std::vector<Neighbor>::const_iterator i = m_nb.begin ();
       i != m_nb.end (); ++i)
    {
      if (i->m_neighborAddress == addr)
        {
          return true;
        }
    }
  return false;
}

std::vector<Ipv4Address>
Neighbors::GetNeighbors ()
{
	return m_nbAdr;
}

Time
Neighbors::GetExpireTime (Ipv4Address addr)
{
  Purge ();
  for (std::vector<Neighbor>::const_iterator i = m_nb.begin (); i
       != m_nb.end (); ++i)
    {
      if (i->m_neighborAddress == addr)
        {
          return (i->m_expireTime - Simulator::Now ());
        }
    }
  return Seconds (0);
}

void
Neighbors::HelloReceived (Ipv4Address murouterId, Ipv4Address areaID, Ipv4Address localAddress, Ipv4Address routerID, Ipv4Address neighborAddress,
		uint8_t rtrPri, Ipv4Address dr, Ipv4Address bdr, Time expireTime, std::vector<Ipv4Address> neighors)
{
	NS_LOG_FUNCTION (this);
  // State(s): Init or greater
//	if(m_myRouterId.Get()==24){
//		NS_LOG_FUNCTION (this);
//	}

  for (std::vector<Neighbor>::iterator i = m_nb.begin (); i != m_nb.end (); ++i)
    {
      if (i->m_neighborAddress == neighborAddress)
        {
    	  // update expireTime
          i->m_expireTime = std::max (expireTime + Simulator::Now (), i->m_expireTime);
          NS_LOG_LOGIC ("Neighbor " << neighborAddress << " exists, just update its expire time");
          // update state
          UpdateState(i, neighors);
          // update LSDB
          LSDBUpdateDecision();
          return;
        }
    }

  // State(s): Down
  NS_LOG_LOGIC ("Open link to " << neighborAddress << ", add new neighbor");
  // create new entry
  Neighbor neighbor (areaID, localAddress, routerID, murouterId, neighborAddress, NeighborState_Down,
		  rtrPri, dr, bdr, expireTime + Simulator::Now ());
  m_nb.push_back (neighbor);
  // update state
  UpdateState(m_nb.end() - 1, neighors);
  // update LSDB
  LSDBUpdateDecision();

//  if(murouterId == Ipv4Address(659)){
//  		std::cout<<Simulator::Now()<<std::endl;
//  		std::cout<<m_nb.size()<<std::endl;
//  		for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end();++i) {
//  			std::cout << i->m_neighborAddress <<"     "<< i->m_routerID.Get()<< std::endl;
//
//  		}
//
//  	}
}


void
Neighbors::TwoWayReceived (std::vector<Neighbor>::iterator i)
{

	NS_LOG_FUNCTION (this);
	// Event 2-WayReceived
	// Down, Init -> 2Way
	// Other conditions, no change
	if (i->m_state < NeighborState_2Way) {
		i->m_state = NeighborState_2Way;

		//<! Here should determine whether an adjacency should be established with the neighbor
		//<! p2p network, state must be exstart
		i->m_state = NeighborState_ExStart;
		NS_LOG_LOGIC ("Change to neighborState: " << "ExStart " << "myAdr: " << i->m_localAddress << " neighborAdr: " << i->m_neighborAddress);
//		// last received DD packet information
//		i->m_ddSeqNum = 0;
//		i->m_flagsLast = 0;
//		i->m_optionLast = 0;
//		i->m_ddSeqNumLast = 0;
		// send first DD sent
		NS_LOG_LOGIC("Send DD packet for master/slave selection");
		m_handleDDTriggering(i->m_localAddress, i->m_neighborAddress, 7, i->m_ddSeqNum,{});
		i->m_DDRetransmitEvent.Cancel();
		i->m_DDRetransmitEvent = Simulator::Schedule(m_rxmtInterval,
								&Neighbors::DDRetransmit, this, i->m_localAddress, i->m_neighborAddress, 7, i->m_ddSeqNum);
		NS_LOG_LOGIC ("Set DD retransmission event from "<<i->m_localAddress<<" to "<<i->m_neighborAddress<<" in "<<m_rxmtInterval.GetSeconds()<<" s");

	}
	// Update neighbors and expireTime
	Purge();

}

void
Neighbors::OneWayReceived (std::vector<Neighbor>::iterator i)
{
	NS_LOG_FUNCTION (this);
	// Event 1-WayReceived
	// All -> Init
	NS_LOG_LOGIC ("Change to neighborState: " << "Init " << "myAdr: " << i->m_localAddress << " neighborAdr: " << i->m_neighborAddress);
	if (i->m_state <= NeighborState_Init) {
		// no action
		i->m_state = NeighborState_Init;
	} else {
		i->m_state = NeighborState_Init;
		/*The Link state retransmission list, Database summary
		 list and Link state request list are cleared of LSAs*/
		NS_LOG_LOGIC ("Clear the link state retransmission list, database summary list and link state request list");
		i->m_lsRequestList.clear();
		i->m_lsRetransList.clear();
		i->m_dbSummaryList.clear();
	}
	// update neighbors and expireTime
	Purge();
	// Here need to reset hello mission for local interface
	NS_LOG_LOGIC ("Send Hello from "<<i->m_localAddress<<" to "<<i->m_neighborAddress);
	m_handleHelloTriggering(i->m_localAddress);

}

void
Neighbors::NegotiationDone (std::vector<Neighbor>::iterator i)
{
	NS_LOG_FUNCTION (this);

//	if(i->m_myRouterID.Get() == 6){
//
//	}


	i->m_state = NeighborState_Exchange;
	NS_LOG_LOGIC ("Change to neighborState: " << "Exchange " << "myAdr: " << i->m_localAddress << " neighborAdr: " << i->m_neighborAddress);

	// Each LSA in the area’s link-state database (at the time the neighbor transitions into Exchange state) is
	// listed in the neighbor Database summary list.
	//##########Database summary list maintain
	i->m_dbSummaryList = m_lsdb.GetDataBaseSummary();

	// cancel past retransmit event for first DD(just for master/slave selection), both master and slave
	i->m_DDRetransmitEvent.Cancel();
	if(i->m_mode == 1){
		// ######  Describes the current top of the Database summary list. (waiting change here when LSDB done)
		std::vector<LSAHeader> lsa = GetTopOfDBSummaryList(i);//{1} // must not be empty, router-lsa at least
		if(lsa.empty()){
			throw std::runtime_error("Process Wrong: must not be empty, router-lsa at least");
		}
		NS_LOG_LOGIC("Master send DD packet to slave");
		m_handleDDTriggering(i->m_localAddress, i->m_neighborAddress, 3, i->m_ddSeqNum, lsa); // 00000011
		// retransmit schedule
		i->m_DDRetransmitEvent = Simulator::Schedule(m_rxmtInterval,
				&Neighbors::DDRetransmit, this, i->m_localAddress, i->m_neighborAddress, 3, i->m_ddSeqNum);
		NS_LOG_LOGIC("Set DD packet retransmission event in "<<m_rxmtInterval.GetSeconds()<<" s");
	}
	else if(i->m_mode == 0){

		//lsa here must be empty
		std::vector<LSAHeader> lsa = {};
		NS_LOG_LOGIC("Slave send DD packet to master");
		m_handleDDTriggering(i->m_localAddress, i->m_neighborAddress, 2, i->m_ddSeqNum, lsa); // 00000010
		i->m_lastSentDDFreeingOrNot = false;
		i->m_flagsInLastSentDD = 2;
		i->m_lsInLastSentDD = lsa;
		i->m_lastDDFreeingEvent.Cancel();
		i->m_lastDDFreeingEvent = Simulator::Schedule(m_routerDeadInterval, &Neighbors::DDFreeingForSlave,
				this, i->m_localAddress, i->m_neighborAddress, 2, i->m_ddSeqNum);
		NS_LOG_LOGIC("Set last sent DD packet freeing event in "<<m_routerDeadInterval.GetSeconds()<<" s");


	}

}

void
Neighbors::ExchangeDone (std::vector<Neighbor>::iterator i)
{
	NS_LOG_FUNCTION (this);
	// If the neighbor Link state request list is empty, the new neighbor state is Full. No other action is
	// required. This is an adjacency’s final state. Otherwise, the new neighbor state is Loading. Start
	// (or continue) sending Link State Request packets to the neighbor (see Section 10.9). These are requests
	// for the neighbor’s more recent LSAs (which were discovered but not yet received in the Exchange
	// state). These LSAs are listed in the Link state request list associated with the neighbor.
	// todo
	if(i->m_lsRequestList.empty()){
		NS_LOG_LOGIC ("Change to neighborState: " << "Full " << "myAdr: " << i->m_localAddress << " neighborAdr: " << i->m_neighborAddress);
		i->m_state = NeighborState_Full;

		//std::cout<<"Time: "<<Simulator::Now().GetSeconds()<<", F: "<<i->m_routerID.Get()<<std::endl;
	}
	else{
		NS_LOG_LOGIC ("Change to neighborState: " << "Loading " << "myAdr: " << i->m_localAddress << " neighborAdr: " << i->m_neighborAddress);
		i->m_state = NeighborState_Loading;
//		if(i->m_routerID.Get() == 6){
//			std::cout<<i->m_routerID.Get()<<std::endl;
//		}
		SendFromRequestList(i);
		//std::cout<<"Time: "<<Simulator::Now().GetSeconds()<<", LL: "<<i->m_routerID.Get()<<std::endl;
	}
	//std::cout<<i->m_routerID.Get()<<std::endl;
}

void
Neighbors::SeqNumberMismatch (std::vector<Neighbor>::iterator i){

	NS_LOG_FUNCTION (this);
	// The neighbor state first transitions to ExStart.
	NS_LOG_LOGIC ("Change to neighborState: " << "ExStart " << "myAdr: " << i->m_localAddress << " neighborAdr: " << i->m_neighborAddress);
	i->m_state = NeighborState_ExStart;
	// The Link state retransmission list, Database summary list and Link state request list are cleared of LSAs.
	NS_LOG_LOGIC ("Clear the link state retransmission list, database summary list and link state request list");
	DeleteNeighbor(i);
    // The router increments the DD sequence number in the neighbor data structure,
	// declares itself master (sets the master/slave bit to master), and starts
    // sending Database Description Packets, with the initialize (I), more (M) and master (MS) bits set.
	i->m_ddSeqNum += 1;
	//i->m_mode = 1;
	NS_LOG_LOGIC("Send DD packet for master/slave selection");
	m_handleDDTriggering(i->m_localAddress, i->m_neighborAddress, 7, i->m_ddSeqNum,{});
	i->m_DDRetransmitEvent.Cancel();
	i->m_DDRetransmitEvent = Simulator::Schedule(m_rxmtInterval,
									&Neighbors::DDRetransmit, this, i->m_localAddress, i->m_neighborAddress, 7, i->m_ddSeqNum);
	NS_LOG_LOGIC ("Set DD retransmission event from "<<i->m_localAddress<<" to "<<i->m_neighborAddress<<" in "<<m_rxmtInterval.GetSeconds()<<" s");

}

void
Neighbors::UpdateState(std::vector<Neighbor>::iterator i, std::vector<Ipv4Address> neighors){

	if(find(neighors.begin(), neighors.end(), i->m_localAddress) != neighors.end()){
		TwoWayReceived(i);
	}
	else{
		OneWayReceived(i);
	}

}

void
Neighbors::DDReceived (Ipv4Address myOwnrouterID, Ipv4Address nbAddress, uint8_t flags, uint32_t seqNum, std::vector<LSAHeader> lsas)
{
	NS_LOG_FUNCTION (this);
	///////////////////////////////
	// these just for debug, finally will be deleted
	uint32_t rtrID = 100;
	bool debugForRtrID = false;
//	if(myOwnrouterID == Ipv4Address(5)){
//	std::cout<<1<<std::endl;
//	}
	///////////////////////////////////////////



	std::vector<Neighbor>::iterator it = m_nb.end();
	for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
		if (i->m_neighborAddress == nbAddress) {
			it = i;
			break;
		}
	}
	if(it == m_nb.end()){
		// The packet should be rejected.
		// we delete the neighbor entry when happening the interface down
		if(myOwnrouterID == Ipv4Address(rtrID) && debugForRtrID){
			std::cout<<Simulator::Now()<<"  "<<"no nb"<<std::endl;
			for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
				std::cout<<i->m_neighborAddress<<"  "<<i->m_state<<std::endl;
			}
		}
		NS_LOG_LOGIC ("Neighbor "<<nbAddress<<" not found");
		return;
	}


	if(it->m_state == NeighborState_Down){
		// The packet should be rejected.
		if(myOwnrouterID == Ipv4Address(rtrID) && debugForRtrID){
			std::cout<<Simulator::Now()<<"  "<<"down"<<std::endl;
			for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
				std::cout<<i->m_neighborAddress<<"  "<<i->m_state<<std::endl;
			}
		}
		NS_LOG_LOGIC ("Current NeighborState: Down, reject the DD packet");
	}
	else if(it->m_state == NeighborState_Init){
		NS_LOG_LOGIC ("Current neighborState: Init");
		TwoWayReceived(it);
		if(it->m_state == NeighborState_ExStart){
			DDReceived (myOwnrouterID, nbAddress, flags, seqNum, lsas);
		}

	}
	else if(it->m_state == NeighborState_2Way){
		// The packet should be ignored.
		// Actually, interfaces in p2p network enter exstart by default
		if(myOwnrouterID == Ipv4Address(rtrID) && debugForRtrID){
			std::cout<<Simulator::Now()<<"  "<<"2way"<<std::endl;
			for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
				std::cout<<i->m_neighborAddress<<"  "<<i->m_state<<std::endl;
			}
		}
		NS_LOG_LOGIC ("Current neighborState: 2Way");
	}
	else if(it->m_state == NeighborState_ExStart){
		// The initialize(I), more (M) and master(MS) bits are set,
		// the contents of the packet are empty, and the neighbor’s
		// Router ID is larger than the router’s own. In this case
		// the router is now Slave. Set the master/slave bit to
		// slave, and set the neighbor data structure’s DD sequence
		// number to that specified by the master.
		NS_LOG_LOGIC ("Current neighborState: ExStart");
		if(flags == 7  && myOwnrouterID.Get() < it->m_routerID.Get() && lsas.empty()){
			// Slave -> me
			NS_LOG_LOGIC("Slave -> me");
			it->m_mode = 0;
			// DD sequence
			it->m_ddSeqNum = seqNum;
			it->m_ddSeqNumLast = seqNum;
			it->m_flagsLast = flags;
			NegotiationDone(it);
		}
		// The initialize(I) and master(MS) bits are off, the
		// packet’s DD sequence number equals the neighbor data
		// structure’s DD sequence number (indicating
		// acknowledgment) and the neighbor’s Router ID is smaller
		// than the router’s own. In this case the router is Master.
		else if(flags == 2  && myOwnrouterID.Get() > it->m_routerID.Get() && seqNum == it->m_ddSeqNum){
			// Master -> me
			NS_LOG_LOGIC("Master -> me");
			it->m_mode = 1;
			it->m_ddSeqNum = seqNum + 1;
			it->m_ddSeqNumLast = seqNum;
			it->m_flagsLast = flags;
			NegotiationDone(it);
		}
		else{
			// The packet should be ignored.
		}
		if(myOwnrouterID == Ipv4Address(rtrID) && debugForRtrID){
			std::cout<<Simulator::Now()<<"  "<<"exstart"<<std::endl;
			for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
				std::cout<<i->m_neighborAddress<<"  "<<i->m_state<<std::endl;
			}
		}


	}
	else if(it->m_state == NeighborState_Exchange){
		NS_LOG_LOGIC ("Current neighborState: Exchange");
		bool duplicate = false;
		if(flags == it->m_flagsLast && seqNum == it->m_ddSeqNumLast){
			duplicate = true;
		}
		// Duplicate Database Description packets are discarded by the
		// master, and cause the slave to retransmit the last Database
		// Description packet that it had sent.
		if (duplicate) {
			NS_LOG_LOGIC ("A duplicate DD packet received");
			if (it->m_mode == 0) {
				// slave: retransmit the last DD packet
				if(it->m_lastSentDDFreeingOrNot == false){
					NS_LOG_LOGIC ("Slave retransmit the last DD packet due to a duplicate");
					m_handleDDTriggering(it->m_localAddress, it->m_neighborAddress, it->m_flagsInLastSentDD,
							it->m_ddSeqNum, it->m_lsInLastSentDD);
				}
				else{
					// router dead interval expire
					NS_LOG_LOGIC ("Router dead interval expire");
					SeqNumberMismatch(it);

				}


			}
			else{
				// master: discarded
			}
			if(myOwnrouterID == Ipv4Address(rtrID) && debugForRtrID){
				std::cout<<Simulator::Now()<<"  "<<"exchange1"<<std::endl;
				for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
					std::cout<<i->m_neighborAddress<<"  "<<i->m_state<<std::endl;
				}
			}
			return;
		}

		// Otherwise (the packet is not a duplicate)
		// (1) MS-bit inconsistent with the master/slave state of the connection
		// (2) initialize(I) bit is set
		// (3) different packet’s Options field (no use)
		uint8_t MS = flags & 1; // 1 = 00000001
		uint8_t I = flags & 4; // 4 = 00000100
		if(MS + it->m_mode != 1 || I == 4){
			NS_LOG_LOGIC ("The packet is not a duplicate, but MS/I-bit unsatisfied");
			SeqNumberMismatch(it);
			// stop processing the packet
			if(myOwnrouterID == Ipv4Address(rtrID) && debugForRtrID){
				std::cout<<Simulator::Now()<<"  "<<"exchange2"<<std::endl;
				for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
					std::cout<<i->m_neighborAddress<<"  "<<i->m_state<<std::endl;
				}
			}
			return;
		}


		// (4) Database Description packets must be processed in sequence, as indicated by the packets’ DD sequence numbers.
		if(it->m_mode == 1 && seqNum == it->m_ddSeqNum){
			// accept and processed
			it->m_ddSeqNumLast = seqNum;
			it->m_flagsLast = flags;
			DDProcessed(it, flags, seqNum, lsas);
			if(myOwnrouterID == Ipv4Address(rtrID) && debugForRtrID){
				std::cout<<Simulator::Now()<<"  "<<"sxhcange3"<<std::endl;
				for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
					std::cout<<i->m_neighborAddress<<"  "<<i->m_state<<std::endl;
				}
			}
			return;
		}
		else if(it->m_mode == 0 && seqNum == it->m_ddSeqNum + 1){
			// accept and processed
			it->m_ddSeqNumLast = seqNum;
			it->m_flagsLast = flags;
			DDProcessed(it, flags, seqNum, lsas);
			if(myOwnrouterID == Ipv4Address(rtrID) && debugForRtrID){
				std::cout<<Simulator::Now()<<"  "<<"exchange4"<<std::endl;
				for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
					std::cout<<i->m_neighborAddress<<"  "<<i->m_state<<std::endl;
				}
			}
			return;
		}



		// (5) Else, generate the neighbor event SeqNumberMismatch and stop processing the packet.
		SeqNumberMismatch(it);
		if(myOwnrouterID == Ipv4Address(rtrID) && debugForRtrID){
			std::cout<<Simulator::Now()<<"  "<<"exchange5"<<std::endl;
			for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
				std::cout<<i->m_neighborAddress<<"  "<<i->m_state<<std::endl;
			}
		}

	}
	else if(it->m_state > NeighborState_Exchange){
		NS_LOG_LOGIC ("Current neighborState: >Exchange");
		bool duplicate = false;
		if (flags == it->m_flagsLast && seqNum == it->m_ddSeqNumLast) {
			duplicate = true;
		}
		//<! The only packets received should be duplicates
		// Duplicates should be discarded by the master. The slave must respond to
		// duplicates by repeating the last Database Description packet that it had sent.

		if (duplicate) {
			NS_LOG_LOGIC ("A duplicate DD packet received");
			if (it->m_mode == 0) {
				// slave: retransmit the last DD packet
				if(it->m_lastSentDDFreeingOrNot == false){
					NS_LOG_LOGIC ("Slave retransmit the last DD packet due to a duplicate");
					m_handleDDTriggering(it->m_localAddress, it->m_neighborAddress, it->m_flagsInLastSentDD,
							it->m_ddSeqNum, it->m_lsInLastSentDD);
				}
				else{
					// router dead interval expire
					NS_LOG_LOGIC("Router dead interval expire");
					SeqNumberMismatch(it);
				}
			} else {
				// master: discarded
				NS_LOG_LOGIC("Master discards the duplicate DD packet");
			}
			if(myOwnrouterID == Ipv4Address(rtrID) && debugForRtrID){
				std::cout<<Simulator::Now()<<"  "<<"full462"<<std::endl;
				for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
					std::cout<<i->m_neighborAddress<<"  "<<i->m_state<<std::endl;
				}
			}
			return;
		}
		// Any other packets received, including the reception of a packet with the Initialize(I) bit set,
		// should generate the neighbor event SeqNumberMismatch

		SeqNumberMismatch(it);
		if(myOwnrouterID == Ipv4Address(rtrID) && debugForRtrID){
			std::cout<<Simulator::Now()<<"  "<<"full3"<<std::endl;
			for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
				std::cout<<i->m_neighborAddress<<"  "<<i->m_state<<std::endl;
			}
		}

	}


}

void
Neighbors::DDProcessed (std::vector<Neighbor>::iterator i, uint8_t flags, uint32_t seqNum, std::vector<LSAHeader> lsas)
{
	NS_LOG_FUNCTION (this);
	// When the router accepts a received Database Description Packet
	// as the next in sequence the packet contents are processed as follows.
//	if(0){
//		SeqNumberMismatch(i);
//		return;
//	}

	//###### look up & Link state request list todo
	// Otherwise, the router looks up the LSA in its database to see whether it also has an instance of
	// the LSA. If it does not, or if the database copy is less recent, the LSA is put on the Link state request
	// list so that it can be requested (immediately or at some later time) in Link State Request Packets.

//	if(i->m_myRouterID.Get() == 3 && i->m_routerID.Get() == 6){
//		std::cout<<lsas.size()<<std::endl;
//	}

	for(auto lsa : lsas){
		if(m_lsdb.Has(lsa) && !(m_lsdb.checknewer(lsa))){

		}
		else if(m_lsdb.Has(lsa) && m_lsdb.checknewer(lsa)){
			i->m_lsRequestList.push_back(lsa);
		}
		else if(!m_lsdb.Has(lsa)){
			i->m_lsRequestList.push_back(lsa);
		}
		else{
			throw std::runtime_error("Process Wrong: Neighbors::DDProcessed");
		}
	}
	// send request (immediately or at some later time todo)
	SendFromRequestList(i);


	if(i->m_mode == 1){
		// Increments the DD sequence number in the neighbor data structure. If the router has already sent its entire
		// sequence of Database Description Packets, and the just accepted packet has the more bit (M) set to 0, the neighbor
		// event ExchangeDone is generated. Otherwise, it should send a new Database Description to the slave.
		i->m_ddSeqNum = seqNum + 1;
		i->m_DDRetransmitEvent.Cancel();

		if(i->m_dbSummaryList.empty() && (flags & 2) == 0){ // 2 = 00000010
			// I: empty, nb: empty
			ExchangeDone(i);
		}
		else{
			// ###### First: Items are removed from the Database summary list when the previous packet is acknowledged.
			// ###### Then: Describes the current top of the Database summary list.
			DeleteTopOfDBSummaryList(i);
			std::vector<LSAHeader> lsa = GetTopOfDBSummaryList(i); // maybe empty
			// send new DD
			if(i->m_dbSummaryList.empty()){
				// I: empty, nb: not empty
				NS_LOG_LOGIC("Master send DD packet to slave");
				m_handleDDTriggering(i->m_localAddress, i->m_neighborAddress, 1, i->m_ddSeqNum, lsa); // 00000001
				// retransmit schedule
				i->m_DDRetransmitEvent = Simulator::Schedule(m_rxmtInterval,
						&Neighbors::DDRetransmit, this, i->m_localAddress, i->m_neighborAddress, 1, i->m_ddSeqNum);
				NS_LOG_LOGIC("Set DD packet retransmission event in "<<m_rxmtInterval.GetSeconds()<<" s");
			}
			else{
				NS_LOG_LOGIC("Master send DD packet to slave");
				// I: no empty, nb: empty/no empty
				m_handleDDTriggering(i->m_localAddress, i->m_neighborAddress, 3, i->m_ddSeqNum, lsa); // 00000011
				// retransmit schedule
				i->m_DDRetransmitEvent = Simulator::Schedule(m_rxmtInterval,
						&Neighbors::DDRetransmit, this, i->m_localAddress, i->m_neighborAddress, 3, i->m_ddSeqNum);
				NS_LOG_LOGIC("Set DD packet retransmission event in "<<m_rxmtInterval.GetSeconds()<<" s");
			}
		}

	}
	else if(i->m_mode == 0){
		// Sets the DD sequence number in the neighbor data structure to the DD sequence number appearing in the received packet.
		// The slave must send a Database Description Packet in reply. If the received packet has the more bit (M) set to 0, and
		// the packet to be sent by the slave will also have the M-bit set to 0, the neighbor event ExchangeDone is generated.
		// Note that the slave always generates this event before the master.
		i->m_ddSeqNum = seqNum;
		// we are going to freeing past information
		if(i->m_lastSentDDFreeingOrNot == false){
			i->m_lastDDFreeingEvent.Cancel();
			i->m_lastSentDDFreeingOrNot = true;
			i->m_flagsInLastSentDD = 0;
			i->m_lsInLastSentDD.clear();
		}
		// send new DD in reply
		// ###### Then: Describes the current top of the Database summary list.
		std::vector<LSAHeader> lsa = GetTopOfDBSummaryList(i); //{1}####### waiting change here when LSDB done
		DeleteTopOfDBSummaryList(i);
		uint8_t curFlags;
		if(i->m_dbSummaryList.empty() && (flags & 2) == 0){
			// I: empty, nb: empty
			ExchangeDone(i);
			NS_LOG_LOGIC("Slave send DD packet to master");
			m_handleDDTriggering(i->m_localAddress, i->m_neighborAddress, 0, i->m_ddSeqNum, lsa); // 00000000
			curFlags = 0;

		}
		else{
			if(i->m_dbSummaryList.empty()){
				// I: empty, nb: no empty
				NS_LOG_LOGIC("Slave send DD packet to master");
				m_handleDDTriggering(i->m_localAddress, i->m_neighborAddress, 0, i->m_ddSeqNum, lsa); // 00000000
				curFlags = 0;
			}
			else{
				// I: no empty, nb: no empty/empty
				NS_LOG_LOGIC("Slave send DD packet to master");
				m_handleDDTriggering(i->m_localAddress, i->m_neighborAddress, 2, i->m_ddSeqNum,lsa); // 00000010
				curFlags = 2;
			}
		}


		i->m_lastSentDDFreeingOrNot = false;
		i->m_flagsInLastSentDD = curFlags;
		i->m_lsInLastSentDD = lsa;
		i->m_lastDDFreeingEvent = Simulator::Schedule(m_routerDeadInterval, &Neighbors::DDFreeingForSlave,
				this, i->m_localAddress, i->m_neighborAddress, curFlags, i->m_ddSeqNum);
		NS_LOG_LOGIC("Set last sent DD packet freeing event in "<<m_routerDeadInterval.GetSeconds()<<" s");

	}

}

void
Neighbors::DDRetransmit (Ipv4Address localAddress, Ipv4Address neighborAddress, uint8_t flags, uint32_t seqNum)
{
	NS_LOG_FUNCTION (this);
	bool found = false;
	for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
		if (i->m_neighborAddress == neighborAddress) {
			if(i->m_localAddress == localAddress && i->m_ddSeqNum == seqNum){
				// ###### Describes the current top of the Database summary list. (waiting change here when LSDB done)
				std::vector<LSAHeader> lsa;
				if(flags == 7){
					lsa = {};
				}
				else{
					// Describes the current top of the Database summary list
					lsa = this->GetTopOfDBSummaryList(i);
				}
				m_handleDDTriggering(localAddress, neighborAddress, flags, seqNum, lsa);
				// retransmit schedule
				i->m_DDRetransmitEvent.Cancel();
				i->m_DDRetransmitEvent = Simulator::Schedule(m_rxmtInterval,
						&Neighbors::DDRetransmit, this, localAddress, neighborAddress, flags, seqNum);
				found = true;
				NS_LOG_LOGIC ("Set DD retransmission event from "<<localAddress<<" to "<<neighborAddress<<" in "<<m_rxmtInterval.GetSeconds()<<" s");
			}

			break;
		}
	}

	if (!found) {
		// when interface down, we have delete the neighbor and cancel the retrans event
		// if happened, have a bug
		throw std::runtime_error("Process Wrong: Neighbors::DDRetransmit");
	}


}

void
Neighbors::RequestRetransmit (Ipv4Address localAddress, Ipv4Address neighborAddress)
{
	NS_LOG_FUNCTION (this);
	bool found = false;
	for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
		if (i->m_neighborAddress == neighborAddress && i->m_localAddress == localAddress) {
			found = true;
			i->m_requestTransmitted.clear();
			i->m_requestRetransmitEvent.Cancel();
			SendFromRequestList(i);
			break;
		}
	}

	if (!found) {
		// when interface down, we have delete the neighbor and cancel the retrans event
		// if happened, have a bug
		throw std::runtime_error("Process Wrong: Neighbors::RequestRetransmit");
	}


}

void
Neighbors::UpdateRetransmit (Ipv4Address localAddress, Ipv4Address neighborAddress)
{
	NS_LOG_FUNCTION (this);
	bool found = false;
	for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
		if (i->m_neighborAddress == neighborAddress && i->m_localAddress == localAddress) {
			found = true;
			i->m_LSATransmitted.clear();
			i->m_lsRetransmitEvent.Cancel();
			SendFromRetransmissionList(i);
			break;
		}
	}

	if (!found) {
		// when interface down, we have delete the neighbor and cancel the retrans event
		// if happened, have a bug
		throw std::runtime_error("Process Wrong: Neighbors::UpdateRetransmit");
	}


}

void
Neighbors::DDFreeingForSlave (Ipv4Address localAddress, Ipv4Address neighborAddress, uint8_t flags, uint32_t seqNum)
{
	NS_LOG_FUNCTION (this);
	bool found = false;
	std::vector<Neighbor>::iterator it;
	for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
		if (i->m_localAddress == localAddress && i->m_neighborAddress == neighborAddress) {
			if(i->m_ddSeqNum == seqNum && i->m_flagsInLastSentDD == flags && !(i->m_lastSentDDFreeingOrNot)){
				// we are going to freeing these information
				i->m_lastSentDDFreeingOrNot = true;
				i->m_flagsInLastSentDD = 0;
				i->m_lsInLastSentDD.clear();
				found = true;
			}
			break;
		}
	}

	if (!found) {
		// when interface down, we have delete the neighbor and cancel the retrans event
		// if happened, have a bug
		throw std::runtime_error("Process Wrong: Neighbors::DDFreeingForSlave");
	}

}

void
Neighbors::LSRReceived (Ipv4Address myOwnrouterID, Ipv4Address nbAddress, LSRHeader lsr)
{
	///////////////////////////////
	// these just for debug, finally will be deleted
	uint32_t rtrID = 357;
	bool debugForRtrID = false;
//	if(myOwnrouterID == Ipv4Address(5)){
//	std::cout<<1<<std::endl;
//	}
	///////////////////////////////////////////

	std::vector<Neighbor>::iterator it = m_nb.end();
	for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
		if (i->m_neighborAddress == nbAddress) {
			it = i;
			break;
		}
	}
	if(it == m_nb.end()){
		// The packet should be rejected.
		// we delete the neighbor entry when happening the interface down
		if(myOwnrouterID == Ipv4Address(rtrID) && debugForRtrID){
			std::cout<<Simulator::Now()<<"  "<<"no nb"<<std::endl;
			for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
				std::cout<<i->m_neighborAddress<<"  "<<i->m_state<<std::endl;
			}
		}
		return;
	}

//	if(it->m_myRouterID.Get() == 6 && it->m_routerID.Get() == 3){
//		std::cout<<1<<std::endl;
//	}

	if(it->m_state == NeighborState_Down){
		// The packet should be rejected.
		if(myOwnrouterID == Ipv4Address(rtrID) && debugForRtrID){
			std::cout<<Simulator::Now()<<"  "<<"down"<<std::endl;
			for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
				std::cout<<i->m_neighborAddress<<"  "<<i->m_state<<std::endl;
			}
		}
	}
	else if(it->m_state == NeighborState_Init){
		if(myOwnrouterID == Ipv4Address(rtrID) && debugForRtrID){
			std::cout<<Simulator::Now()<<"  "<<"init"<<std::endl;
			for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
				std::cout<<i->m_neighborAddress<<"  "<<i->m_state<<std::endl;
			}
		}
	}
	else if(it->m_state == NeighborState_2Way){
		// The packet should be ignored.
		// Actually, interfaces in p2p network enter exstart by default
		if(myOwnrouterID == Ipv4Address(rtrID) && debugForRtrID){
			std::cout<<Simulator::Now()<<"  "<<"init"<<std::endl;
			for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
				std::cout<<i->m_neighborAddress<<"  "<<i->m_state<<std::endl;
			}
		}
	}
	else if(it->m_state == NeighborState_ExStart){
		if(myOwnrouterID == Ipv4Address(rtrID) && debugForRtrID){
			std::cout<<Simulator::Now()<<"  "<<"down"<<std::endl;
			for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
				std::cout<<i->m_neighborAddress<<"  "<<i->m_state<<std::endl;
			}
		}
	}
	else if(it->m_state == NeighborState_Exchange || it->m_state == NeighborState_Loading || it->m_state == NeighborState_Full){
		std::vector<LSRPacket> LSRPackets = lsr.GetLSRs();
		std::vector<std::pair<LSAHeader,LSAPacket>> lsas = {};
		for (std::vector<LSRPacket>::iterator i = LSRPackets.begin(); i != LSRPackets.end(); ++i)
		{
			uint32_t LinkStateType = i->GetLSType();
			Ipv4Address LinkStateID = i->GetLSID();
			Ipv4Address AdvertisingRouter = i->GetAdRouter();
			TLRLinkStateIdentifier lsaidenti(LinkStateType,LinkStateID,AdvertisingRouter);
			bool Has = m_lsdb.Has(lsaidenti);
			if (Has == true)
			{
				///#Mengy: add lsa to the lsu, send lsu
				std::pair<LSAHeader,LSAPacket> lsa = m_lsdb.Get(lsaidenti);
				lsas.push_back(lsa);
				it->m_lsRetransList.push_back(lsa);
			}
			else
			{
				///Mengy: return a false state
			}
		}
		///#Mengy:send lsu
		// there is a list
		SendFromRetransmissionList(it);
	}
}

void
Neighbors::LSUReceived (Ipv4Address myOwnrouterID, Ipv4Address nbAddress, LSUHeader lsu)
{
	///////////////////////////////
	// these just for debug, finally will be deleted
	uint32_t rtrID = 357;
	bool debugForRtrID = false;
//	if(myOwnrouterID == Ipv4Address(5)){
//	std::cout<<1<<std::endl;
//	}
	///////////////////////////////////////////


	std::vector<Neighbor>::iterator it = m_nb.end();
	for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
		if (i->m_neighborAddress == nbAddress) {
			it = i;
			break;
		}
	}

	if(it == m_nb.end()){
		// The packet should be rejected.
		// we delete the neighbor entry when happening the interface down
		if(myOwnrouterID == Ipv4Address(rtrID) && debugForRtrID){
			std::cout<<Simulator::Now()<<"  "<<"no nb"<<std::endl;
			for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
				std::cout<<i->m_neighborAddress<<"  "<<i->m_state<<std::endl;
			}
		}
		return;
	}

	if(it->m_state == NeighborState_Down){
		// The packet should be rejected.
		if(myOwnrouterID == Ipv4Address(rtrID) && debugForRtrID){
			std::cout<<Simulator::Now()<<"  "<<"down"<<std::endl;
			for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
				std::cout<<i->m_neighborAddress<<"  "<<i->m_state<<std::endl;
			}
		}
	}
	else if(it->m_state == NeighborState_Init){
		if(myOwnrouterID == Ipv4Address(rtrID) && debugForRtrID){
			std::cout<<Simulator::Now()<<"  "<<"init"<<std::endl;
			for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
				std::cout<<i->m_neighborAddress<<"  "<<i->m_state<<std::endl;
			}
		}
	}
	else if(it->m_state == NeighborState_2Way){
		// The packet should be ignored.
		// Actually, interfaces in p2p network enter exstart by default
		if(myOwnrouterID == Ipv4Address(rtrID) && debugForRtrID){
			std::cout<<Simulator::Now()<<"  "<<"init"<<std::endl;
			for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
				std::cout<<i->m_neighborAddress<<"  "<<i->m_state<<std::endl;
			}
		}
	}
	else if(it->m_state == NeighborState_ExStart){
		if(myOwnrouterID == Ipv4Address(rtrID) && debugForRtrID){
			std::cout<<Simulator::Now()<<"  "<<"down"<<std::endl;
			for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
				std::cout<<i->m_neighborAddress<<"  "<<i->m_state<<std::endl;
			}
		}
	}
	/// #Mengy:更新本地数据库，更新LSR列表，更新LSR重传列表
	else if(it->m_state == NeighborState_Exchange || it->m_state == NeighborState_Loading || it->m_state == NeighborState_Full ){

		std::vector<std::pair<LSAHeader,LSAPacket>> lsas = lsu.GetLSAs();
		std::vector<LSAHeader> lsaheaders = {};
		std::vector<std::pair<LSAHeader,LSAPacket>> NewFloodingLSAs = {};
		std::vector<LSAHeader> LsaHeaderAck = {};


		std::map<Ipv4Address, std::pair<Ipv4Address, Ipv4Address>> interfaces;
		for(std::vector<Neighbor>::iterator i = m_nb.begin (); i != m_nb.end (); ++i){
			if(i->m_state >= NeighborState_2Way){
				interfaces[i->m_routerID] = std::make_pair(i->m_localAddress, i->m_neighborAddress);
			}
		}
		m_lsdb.SetInterfaceAddress(interfaces);
		m_lsdb.UpdateAge();




		bool needUpdateRoute = false;
		for (std::vector<std::pair<LSAHeader,LSAPacket>>::iterator i = lsas.begin(); i != lsas.end(); ++i)
		{

			i->first.SetLSAge(m_lsdb.UpdateCurrentPacketAge(i->first.GetLSAge(), i->first.GetLSAddedTime()));

			// Simplification: Once an update is received, it is confirmed by ACK
			LsaHeaderAck.push_back(i->first);

//			if(m_myRouterId.Get() == 100 && i->first.GetLinkStateID().Get() == 100 && Simulator::Now() >= Seconds(100)){
//				std::cout<<1<<std::endl;
//			}

//			if(it->m_myRouterID.Get() == 10 && a.first.GetAdvertisiongRouter().Get() == 6 && a.first.GetLSSequence() == 4){
//				std::cout<<1<<std::endl;
//			}

			///check the lsatype, if type not equal to 1, discard it and process next lsa
			uint8_t LinkStateType = i->first.GetLSAType();
			if(LinkStateType != 1)
			{
				continue;
			}

			/// construct LSA Identifier
			Ipv4Address LinkStateID = i->first.GetLinkStateID();
			Ipv4Address AdvertisingRouter = i->first.GetAdvertisiongRouter();
			TLRLinkStateIdentifier lsaidenti(LinkStateType,LinkStateID,AdvertisingRouter);

			/// check if all the neighbor is in exchange or loading
//			bool state_ok = true;
//			for (std::vector<Neighbor>::iterator Nei = m_nb.begin(); Nei != m_nb.end(); ++Nei) {
//				if (Nei->m_state == NeighborState_Exchange || Nei->m_state == NeighborState_Loading) {
//					state_ok = false;
//					break;
//				}
//			}

//			if(myOwnrouterID == AdvertisingRouter)//todo
//			{
//				//throw std::runtime_error("Process Wrong: the LSA is originated by self and newer");
//				std::cout<<1<<std::endl;
//			}

			bool exist = m_lsdb.Has(lsaidenti);
			bool newer, older, same;
			if(exist == true){
				newer = m_lsdb.checknewer(*i);
				older = m_lsdb.checkolder(*i);
				same = !(newer || older);
			}

			// the LSA’s LS age is equal to MaxAge, and there is currently no instance of the LSA in the router’s link state
			// database, and none of router’s neighbors are in states Exchange or Loading todo
//			if(i->first.GetLSAge() == m_lsdb.GetMaxAge() && exist == false && state_ok == true)
//			{
//				lsaheaders.push_back(i->first);
//				m_handleLSAackTriggering(it->m_localAddress,nbAddress,lsaheaders);
//				lsaheaders.clear();
//				continue;
//			}


			/// If there is no database copy, or the received LSA is more recent than the database copy
			/// if the lsa is in the database and database's lsa is received by flooding and database's lsa's (now-added_time)<MinLSArrival,check next lsa
			if(exist == false || (exist == true && newer == true))
			{

				if(myOwnrouterID == AdvertisingRouter)//todo
				{
					throw std::runtime_error("Process Wrong: the LSA is originated by self and newer");
				}
//				if(exist == true)
//				{
//					// todo
//					std::pair<LSAHeader,LSAPacket> temp = m_lsdb.Get(lsaidenti);
//					bool flooded = temp.first.GetLSAFloodedFlag(); //todo
//					if(flooded == true)
//					{
//						if(m_lsdb.CalcExistDuration(lsaidenti) < m_lsdb.GetMinLSArrival())
//						{
//							continue;
//						}
//					}
//				}
				/// 如果没有数据库副本或者接受到的数据库副本更新, flooding first(actually flooding lsa after the end of for)

				NewFloodingLSAs.push_back(*i);
				//LsaHeaderAck.push_back(i->first);

				// Add the new LSA in the LSDB, this may result in routing table calculation
				m_lsdb.Add(*i);
				needUpdateRoute = true;

				// Remove the current database copy from all neighbors’ Link state retransmission lists.
				// We haven't deleted yet, avoid request-updates don't end. todo

				// If the LSA is originated by self
				// In most cases, the router must then advance the LSA’s LS sequence number one past the received LS sequence number, and
				// originate a new instance of the LSA.
				if(myOwnrouterID == AdvertisingRouter)//todo
				{
					throw std::runtime_error("Process Wrong: the LSA is originated by self and newer");
				}
			}

			///否则，如果发送邻居的链路状态请求列表中存在lsa实例，交换过程发生错误。此时生成事件BadLSReq,重新启动数据库交换过程，停止处理LSU数据包
			///todo

			///else. if the LSA and the instance in database is same
			else if(exist == true && same == true)
			{
				// "implied acknowledgment"
				// We have not considered this situation for now
				// todo
			}

			/// else the local lsa is newer
			else if (exist == true && older == true)
			{
				// Else, the database copy is more recent. If the database copy has LS age equal to MaxAge and LS sequence number equal to
				// MaxSequenceNumber, simply discard the received LSA without acknowledging it. (In this case, the LSA’s LS sequence number is
				// wrapping, and the MaxSequenceNumber LSA must be completely flushed before any new LSA instance can be introduced).
				// m_lsdb.updateAge();
				if(m_lsdb.DetectMaxAge(lsaidenti) && m_lsdb.DetectMaxSeq(lsaidenti))
				{
					continue;
				}

				/// else if the sendingtimeduration for the isa in databse is higher than MinArrivalTime,sendlsu back instantly
				else
				{
					// as long as the database copy has not been sent in a Link State Update within the last MinLSArrival seconds, send the
					// database copy back to the sending neighbor, encapsulated within a Link State Update Packet
					// do not put the database copy of the LSA on the neighbor’s link state retransmission list, and do not acknowledge the received (less
					// recent) LSA instance.
//					if(m_lsdb.HigherThanMinArrivalTimeForSendingLSU(lsaidenti))
//					{
//						std::vector<std::pair<LSAHeader,LSAPacket>> TempLSAs ={};
//						TempLSAs.push_back(*i);
//						m_handleLSUTriggering(it->m_localAddress, it->m_neighborAddress, TempLSAs);
//					}
				}

			}


		}

		if(needUpdateRoute){
			m_lsdb.UpdateRoute();
		}


		/// flooding
		Flood(NewFloodingLSAs,myOwnrouterID,nbAddress);

		/// send lsaack
		m_handleLSAackTriggering(it->m_localAddress, nbAddress, LsaHeaderAck);

		/// xyLiu: arrange new request if necessary
		DeleteFromRequestList(it, LsaHeaderAck);

	}


//	if(myOwnrouterID.Get() == 0){
//		std::cout<<"!!!!!!!!!!!!!!!!"<<std::endl;
//		std::cout<<Simulator::Now().GetSeconds()<<std::endl;
//		for(auto x : m_lsdb.GetDataBaseSummary()){
//			std::cout<<x.GetLSSequence()<<" "<<x.GetAdvertisiongRouter().Get()<<std::endl;
//		}
//	}

}


void
Neighbors::LSAckReceived (Ipv4Address myOwnrouterID, Ipv4Address nbAddress, LSAackHeader lsack)
{
///////////////////////////////
	// these just for debug, finally will be deleted
	uint32_t rtrID = 357;
	bool debugForRtrID = false;
//	if(myOwnrouterID == Ipv4Address(5)){
//	std::cout<<1<<std::endl;
//	}
	///////////////////////////////////////////


	std::vector<Neighbor>::iterator it = m_nb.end();
	for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
		if (i->m_neighborAddress == nbAddress) {
			it = i;
			break;
		}
	}
	if(it == m_nb.end()){
		// The packet should be rejected.
		// we delete the neighbor entry when happening the interface down
		if(myOwnrouterID == Ipv4Address(rtrID) && debugForRtrID){
			std::cout<<Simulator::Now()<<"  "<<"no nb"<<std::endl;
			for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
				std::cout<<i->m_neighborAddress<<"  "<<i->m_state<<std::endl;
			}
		}
		return;
	}

	if(it->m_state == NeighborState_Down){
		// The packet should be rejected.
		if(myOwnrouterID == Ipv4Address(rtrID) && debugForRtrID){
			std::cout<<Simulator::Now()<<"  "<<"down"<<std::endl;
			for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
				std::cout<<i->m_neighborAddress<<"  "<<i->m_state<<std::endl;
			}
		}
	}
	else if(it->m_state == NeighborState_Init){
		if(myOwnrouterID == Ipv4Address(rtrID) && debugForRtrID){
			std::cout<<Simulator::Now()<<"  "<<"init"<<std::endl;
			for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
				std::cout<<i->m_neighborAddress<<"  "<<i->m_state<<std::endl;
			}
		}
	}
	else if(it->m_state == NeighborState_2Way){
		// The packet should be ignored.
		// Actually, interfaces in p2p network enter exstart by default
		if(myOwnrouterID == Ipv4Address(rtrID) && debugForRtrID){
			std::cout<<Simulator::Now()<<"  "<<"init"<<std::endl;
			for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
				std::cout<<i->m_neighborAddress<<"  "<<i->m_state<<std::endl;
			}
		}
	}
	else if(it->m_state == NeighborState_ExStart){
		if(myOwnrouterID == Ipv4Address(rtrID) && debugForRtrID){
			std::cout<<Simulator::Now()<<"  "<<"down"<<std::endl;
			for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
				std::cout<<i->m_neighborAddress<<"  "<<i->m_state<<std::endl;
			}
		}
	}

	/// #Mengy:更新本地数据库，更新LSR列表，更新LSR重传列表
	else if(it->m_state == NeighborState_Exchange || it->m_state == NeighborState_Loading || it->m_state == NeighborState_Full ){

		DeleteFromRetransmissionList(it, lsack.GetLSAacks());



	}
}

//void
//Neighbors::Flood (std::pair<LSAHeader,LSAPacket> lsa, Ipv4Address myOwnrouterID, Ipv4Address nbAddress)
//{
////	if(m_nb.begin()->m_myRouterID.Get() == 6 && lsa.first.GetLSSequence() == 4){
////		std::cout<<"111111111"<<std::endl;
////	}
//
//	for(std::vector<Neighbor>::iterator iter = m_nb.begin();iter!=m_nb.end();++iter)
//	{
////		if(iter->m_myRouterID.Get() == 6 && nbAddress.IsAny()){
////
////				std::cout<<iter->m_routerID.Get()<<"  "<<iter->m_state<<std::endl;
////
////		}
//		if(iter->m_state < NeighborState_Exchange)
//		{
//			continue;
//		}
//		//else if(iter->m_state == NeighborState_Exchange || iter->m_state == NeighborState_Loading)
//		else
//		{
//			// If the new LSA was received from this neighbor, examine the next neighbor.
//			// And the LSA is not the router itself has just originated
//			if(!nbAddress.IsAny() && iter -> m_neighborAddress == nbAddress)
//			{
//				//DeleteLSAInLSRList(iter, lsa.first);
//				continue;
//			}
//
//			/// check Link state request list, if the Link state request list has the lsa instance of the flooded_lsa,compare newer?
//			std::vector<LSAHeader>::iterator lt = find(iter->m_lsRequestList.begin(), iter->m_lsRequestList.end(), lsa.first);
//			if(lt != iter->m_lsRequestList.end()){
//				if(m_lsdb.checknewer(*lt, lsa.first)){
//					continue;
//				}
//				else if(m_lsdb.checknewer(lsa.first, *lt)){
//					//DeleteLSAInLSRList(iter, *lt);
//				}
//				else{
//					//DeleteLSAInLSRList(iter, lsa.first);
//					continue;
//				}
//			}
//
//
//			/// add the lsa in the lsa retransmission list,flood it todo
//			/// add the lsage in the database
////			TLRLinkStateIdentifier identi(lsa.first.GetLSAType(),lsa.first.GetLinkStateID(),lsa.first.GetAdvertisiongRouter());
////			m_lsdb.AddAgeByFlooding(identi);
//
//			iter->m_lsRetransList.push_back(lsa);
//
//
////			std::vector<std::pair<LSAHeader,LSAPacket>> Floodlsas = {};
////			Floodlsas.push_back(lsa);
////			m_handleLSUTriggering(iter->m_localAddress,iter->m_neighborAddress,Floodlsas);
//		}
//	}
//}

void
Neighbors::Flood (std::vector<std::pair<LSAHeader,LSAPacket>> lsas, Ipv4Address myOwnrouterID, Ipv4Address nbAddress){

	for(auto lsa : lsas){
		for(std::vector<Neighbor>::iterator iter = m_nb.begin();iter!=m_nb.end();++iter)
		{
			if(iter->m_state < NeighborState_Exchange)
			{
				continue;
			}
			else
			{
				// If the new LSA was received from this neighbor, examine the next neighbor.
				// And the LSA is not the router itself has just originated
				if(!nbAddress.IsAny() && iter -> m_neighborAddress == nbAddress)
				{
					//DeleteLSAInLSRList(iter, lsa.first);
					continue;
				}

				/// check Link state request list, if the Link state request list has the lsa instance of the flooded_lsa,compare newer?
				std::vector<LSAHeader>::iterator lt = find(iter->m_lsRequestList.begin(), iter->m_lsRequestList.end(), lsa.first);
				if(lt != iter->m_lsRequestList.end()){
					if(m_lsdb.checknewer(*lt, lsa.first)){
						continue;
					}
					else if(m_lsdb.checknewer(lsa.first, *lt)){
						//DeleteLSAInLSRList(iter, *lt);
					}
					else{
						//DeleteLSAInLSRList(iter, lsa.first);
						continue;
					}
				}

				iter->m_lsRetransList.push_back(lsa);

			}
		}
	}


	for(std::vector<Neighbor>::iterator iterNb = m_nb.begin(); iterNb!=m_nb.end(); ++iterNb)
	{
		SendFromRetransmissionList(iterNb);
	}

}


/**
 * \brief CloseNeighbor structure
 */
struct CloseNeighbor
{
  /**
   * Check if the entry is expired
   *
   * \param nb Neighbors::Neighbor entry
   * \return true if expired, false otherwise
   */
  bool operator() (const Neighbors::Neighbor & nb) const
  {
    return ((nb.m_expireTime <= Simulator::Now ()) || nb.m_state == NeighborState_Down);
  }
};

void
Neighbors::Purge ()
{
	NS_LOG_FUNCTION (this);
  if (m_nb.empty ())
    {
      return;
    }

// if there is a link down, is there any triggering?
  CloseNeighbor pred;
  if (!m_handleLinkFailure.IsNull ())
    {
      for (std::vector<Neighbor>::iterator j = m_nb.begin (); j != m_nb.end (); ++j)
        {
          if (pred (*j))
            {
              NS_LOG_LOGIC ("Close link to " << j->m_neighborAddress<<" and delete its neighbor structure");
              m_handleLinkFailure (j->m_neighborAddress);
            }
        }
    }
  for (std::vector<Neighbor>::iterator j = m_nb.begin (); j != m_nb.end (); ++j)
  {
	if (pred (*j))
		DeleteNeighbor(j);
  }

  // If there is a link down, delete the attached neighbor
  m_nb.erase (std::remove_if (m_nb.begin (), m_nb.end (), pred), m_nb.end ());
  UpdateLSDB();
  //NS_LOG_LOGIC (Simulator::Now()<<"  "<<m_nb.size());

  // reset timer, triggering at the latest expire time
  Time latestExpireTime = (*m_nb.begin ()).m_expireTime;
  m_nbAdr.clear();
  for (std::vector<Neighbor>::iterator j = m_nb.begin (); j != m_nb.end (); ++j){
	  if((*j).m_expireTime < latestExpireTime){
		  latestExpireTime = (*j).m_expireTime;
	  }
	  // all neighbors'states are all >= init
	  m_nbAdr.push_back((*j).m_neighborAddress);
  }
  m_ntimer.Cancel ();
  m_ntimer.SetDelay (latestExpireTime - Simulator::Now ());
  m_ntimer.Schedule ();
}

void
Neighbors::ScheduleTimer ()
{
  m_ntimer.Cancel ();
  m_ntimer.Schedule ();
}

uint32_t
Neighbors::GetDDSequenceNumber(Ipv4Address ad)
{
	for (std::vector<Neighbor>::const_iterator i = m_nb.begin (); i != m_nb.end (); ++i)
	{
	  if (i->m_localAddress == ad)
		{
		  return i->m_ddSeqNum;
		}
	}
	throw std::runtime_error ("Wrong interface address: Neighbors::GetDDSequenceNumber");

}

uint32_t
Neighbors::AddDDSequenceNumberByOne(Ipv4Address ad)
{
	for (std::vector<Neighbor>::iterator i = m_nb.begin (); i != m_nb.end (); ++i)
	{
	  if (i->m_localAddress == ad)
		{
		   i->m_ddSeqNum = i->m_ddSeqNum + 1;
		   return i->m_ddSeqNum;
		}
	}
	throw std::runtime_error ("Wrong interface address: Neighbors::AddDDSequenceNumberByOne");

}

void
Neighbors::SetRouterId(Ipv4Address routerId){
	m_myRouterId = routerId;
	m_lsdb.SetRouterId(routerId);
}

std::vector<LSAHeader>
Neighbors::GetTopOfDBSummaryList(std::vector<Neighbor>::iterator i){
	// standard: tlr header 24 bytes + DD (8 bytes + ...)    simulation: tlr header 10 bytes + DD (9 bytes + ...)
	// standard: LSA heaeder 20 bytes      simulation: LSA heaeder 16 bytes
	uint32_t maxLSAHeaderNum = (1500 - 20 - 33) / 23; // 73 // need a interface for mtu 1500 bytes// todo
	if(maxLSAHeaderNum >= i->m_dbSummaryList.size()){
		return i->m_dbSummaryList;
	}
	else{
		return std::vector<LSAHeader>({i->m_dbSummaryList.begin(), i->m_dbSummaryList.begin() + maxLSAHeaderNum});
	}
}

void
Neighbors::DeleteTopOfDBSummaryList(std::vector<Neighbor>::iterator i){
	uint32_t maxLSAHeaderNum = (1500 - 20 - 33) / 23; // 73 // need a interface for mtu 1500 bytes
	if(maxLSAHeaderNum >= i->m_dbSummaryList.size()){
		i->m_dbSummaryList.clear();
	}
	else{
		i->m_dbSummaryList.erase(i->m_dbSummaryList.begin(), i->m_dbSummaryList.begin() + maxLSAHeaderNum);
	}
}

void
Neighbors::LSDBUpdateDecision(){
	NS_LOG_FUNCTION (this);
	// only for router-LSA
	std::vector<LSALinkData> lsaLinkData;
	std::map<Ipv4Address, std::pair<Ipv4Address, Ipv4Address>> interfaces;
	for(std::vector<Neighbor>::iterator i = m_nb.begin (); i != m_nb.end (); ++i){
		if(i->m_state >= NeighborState_2Way){
			lsaLinkData.push_back(LSALinkData(i->m_routerID, i->m_localAddress, GetLinkCost(i->m_localAddress))); // metric =  by default todo
			interfaces[i->m_routerID] = std::make_pair(i->m_localAddress, i->m_neighborAddress);

//			if(i->m_routerID.Get()==113 && m_myRouterId.Get()==200 && Simulator::Now().GetSeconds()>255){
//				std::cout<<"!!!!!!!!!!!!!!!!!!!!!!!!!"<<std::endl;
//				std::cout<<i->m_localAddress.Get()<<std::endl;
//				std::cout<<"!!!!!!!!!!!!!!!!!!!!!!!!!"<<std::endl;
//			}




		}
	}

	// It is optimized in the neighbor relationship establishment stage
	// to reduce the simulation time overhead caused by frequent LSA origination.
	// skip the look_back interface
	uint32_t counter = 0;
	for(uint32_t it = 1; it < m_ipv4->GetNInterfaces(); it++){
		bool interfaceUp = m_ipv4->GetNetDevice(it)->IsLinkUp();
		if(interfaceUp){
			counter++;
		}
	}
	if(lsaLinkData.size() < counter){
		return;
	}

	if(lsaLinkData.empty()){
		m_lsdb.Clear();
		return;
	}

	// router-lsa by default, because we just consider single domain in current version
	LSAHeader tempLsaHeader(/*LSage=*/ 0, /*options=*/0, /*LinkStateID=*/m_myRouterId, /*AdvertisingRouter=*/m_myRouterId,
				/*LSsequence=*/0, /*checkSum=*/0, /*PacketLength=*/0, /*addedTime=*/(uint32_t)Simulator::Now().GetSeconds());
	LSAHeader lsaHeader(/*LSage=*/ 0, /*options=*/0, /*LinkStateID=*/m_myRouterId, /*AdvertisingRouter=*/m_myRouterId,
						/*LSsequence=*/0, /*checkSum=*/0, /*PacketLength=*/0, /*addedTime=*/(uint32_t)Simulator::Now().GetSeconds());

	// just update when interface number changes (interface up or down)
	if(m_lsdb.Has(tempLsaHeader)){
		std::pair<LSAHeader,LSAPacket> lsa = m_lsdb.Get(tempLsaHeader);
		if(lsa.second.Getlinkn() != lsaLinkData.size()){

		}
		else{
			bool equalOrNot = true;
			std::vector<LSALinkData> dateLinksLast = lsa.second.GetLSALinkDatas();
			for(uint32_t i = 0; i < dateLinksLast.size(); i++){
				if(!(lsaLinkData.at(i) == dateLinksLast.at(i))){
					equalOrNot = false;
					break;
				}
			}
			if(equalOrNot){
				// If exactly the same, do not update
				// just waiting originating new lsa due to its age field exceeds
				return;
			}
		}
	}
    NS_LOG_LOGIC ("Originate new Router-LSA ");
    uint32_t seqLast = m_lsdb.AddSeqForSelfOriginatedLSA();
    lsaHeader.SetLSSequence(seqLast);
	LSAPacket lsaPacket(0, lsaLinkData.size(), lsaLinkData);
	m_lsdb.SetInterfaceAddress(interfaces);
	m_lsdb.Add(std::make_pair(lsaHeader,lsaPacket));
	m_lsdb.UpdateRoute();
	std::vector<std::pair<LSAHeader,LSAPacket>> lsaTemp = {std::make_pair(lsaHeader,lsaPacket)};
	Flood(lsaTemp,Ipv4Address::GetAny(),Ipv4Address::GetAny());
	// This guarantees periodic originations of all LSAs.
	m_LSOriginatedEvent.Cancel();
	m_LSOriginatedEvent = Simulator::Schedule(m_LSRefreshTime, &Neighbors::LSAPeriodicOrigination, this);
}

void
Neighbors::UpdateLSDB(){
	NS_LOG_FUNCTION (this);
	// only for router-LSA
	std::vector<LSALinkData> lsaLinkData;
	std::map<Ipv4Address, std::pair<Ipv4Address, Ipv4Address>> interfaces;
	for(std::vector<Neighbor>::iterator i = m_nb.begin (); i != m_nb.end (); ++i){
		if(i->m_state >= NeighborState_2Way){
			lsaLinkData.push_back(LSALinkData(i->m_routerID, i->m_localAddress, GetLinkCost(i->m_localAddress))); // metric = 1 by default todo
			interfaces[i->m_routerID] = std::make_pair(i->m_localAddress, i->m_neighborAddress);

//			if(i->m_routerID.Get()==113 && m_myRouterId.Get()==200 && Simulator::Now().GetSeconds()>255){
//				std::cout<<"!!!!!!!!!!!!!!!!!!!!!!!!!"<<std::endl;
//				std::cout<<i->m_localAddress.Get()<<std::endl;
//				std::cout<<"!!!!!!!!!!!!!!!!!!!!!!!!!"<<std::endl;
//			}



		}
	}

	// router-lsa by default, because we just consider single domain in current version
	LSAHeader tempLsaHeader(/*LSage=*/ 0, /*options=*/0, /*LinkStateID=*/m_myRouterId, /*AdvertisingRouter=*/m_myRouterId,
				/*LSsequence=*/0, /*checkSum=*/0, /*PacketLength=*/0, /*addedTime=*/(uint32_t)Simulator::Now().GetSeconds());
	LSAHeader lsaHeader(/*LSage=*/ 0, /*options=*/0, /*LinkStateID=*/m_myRouterId, /*AdvertisingRouter=*/m_myRouterId,
						/*LSsequence=*/0, /*checkSum=*/0, /*PacketLength=*/0, /*addedTime=*/(uint32_t)Simulator::Now().GetSeconds());


	if(lsaLinkData.empty()){
		m_lsdb.Clear();
		return;
	}

	// just update when interface number changes (interface up or down)
	if(m_lsdb.Has(tempLsaHeader)){
		std::pair<LSAHeader,LSAPacket> lsa = m_lsdb.Get(tempLsaHeader);
		if(lsa.second.Getlinkn() != lsaLinkData.size()){

		}
		else{
			bool equalOrNot = true;
			std::vector<LSALinkData> dateLinksLast = lsa.second.GetLSALinkDatas();
			for(uint32_t i = 0; i < dateLinksLast.size(); i++){
				if(!(lsaLinkData.at(i) == dateLinksLast.at(i))){
					equalOrNot = false;
					break;
				}
			}
			if(equalOrNot){
				// If exactly the same, do not update
				// just waiting originating new lsa due to its age field exceeds
				return;
			}
		}
	}
    NS_LOG_LOGIC ("Originate new Router-LSA ");
    uint32_t seqLast = m_lsdb.AddSeqForSelfOriginatedLSA();
    lsaHeader.SetLSSequence(seqLast);
	LSAPacket lsaPacket(0, lsaLinkData.size(), lsaLinkData);
	m_lsdb.SetInterfaceAddress(interfaces);
	m_lsdb.Add(std::make_pair(lsaHeader,lsaPacket));
	m_lsdb.UpdateRoute();
	std::vector<std::pair<LSAHeader,LSAPacket>> lsaTemp = {std::make_pair(lsaHeader,lsaPacket)};
	Flood(lsaTemp,Ipv4Address::GetAny(),Ipv4Address::GetAny());
	// This guarantees periodic originations of all LSAs.
	m_LSOriginatedEvent.Cancel();
	m_LSOriginatedEvent = Simulator::Schedule(m_LSRefreshTime, &Neighbors::LSAPeriodicOrigination, this);
}

void
Neighbors::LSAPeriodicOrigination(){
	NS_LOG_FUNCTION (this);
	// only for router-LSA
	std::vector<LSALinkData> lsaLinkData;
	std::map<Ipv4Address, std::pair<Ipv4Address, Ipv4Address>> interfaces;
	for(std::vector<Neighbor>::iterator i = m_nb.begin (); i != m_nb.end (); ++i){
		if(i->m_state >= NeighborState_2Way){
			lsaLinkData.push_back(LSALinkData(i->m_routerID, i->m_localAddress, GetLinkCost(i->m_localAddress))); // metric = 1 by default todo
			interfaces[i->m_routerID] = std::make_pair(i->m_localAddress, i->m_neighborAddress);
		}
	}

	// router-lsa by default, because we just consider single domain in current version
	LSAHeader tempLsaHeader(/*LSage=*/ 0, /*options=*/0, /*LinkStateID=*/m_myRouterId, /*AdvertisingRouter=*/m_myRouterId,
				/*LSsequence=*/0, /*checkSum=*/0, /*PacketLength=*/0, /*addedTime=*/(uint32_t)Simulator::Now().GetSeconds());
	LSAHeader lsaHeader(/*LSage=*/ 0, /*options=*/0, /*LinkStateID=*/m_myRouterId, /*AdvertisingRouter=*/m_myRouterId,
						/*LSsequence=*/0, /*checkSum=*/0, /*PacketLength=*/0, /*addedTime=*/(uint32_t)Simulator::Now().GetSeconds());


	if(lsaLinkData.empty()){
		m_lsdb.Clear();
		return;
	}

	// just update

	uint32_t seqLast = m_lsdb.AddSeqForSelfOriginatedLSA();
	lsaHeader.SetLSSequence(seqLast);

    NS_LOG_LOGIC ("Originate Router-LSA ");
	LSAPacket lsaPacket(0, lsaLinkData.size(), lsaLinkData);
	m_lsdb.SetInterfaceAddress(interfaces);
	m_lsdb.Add(std::make_pair(lsaHeader,lsaPacket));
	m_lsdb.UpdateRoute();
	std::vector<std::pair<LSAHeader,LSAPacket>> lsaTemp = {std::make_pair(lsaHeader,lsaPacket)};
	Flood(lsaTemp,Ipv4Address::GetAny(),Ipv4Address::GetAny());

	// This guarantees periodic originations of all LSAs.
	m_LSOriginatedEvent.Cancel();
	m_LSOriginatedEvent = Simulator::Schedule(m_LSRefreshTime, &Neighbors::LSAPeriodicOrigination, this);
}

void
Neighbors::DeleteNeighbor(Ipv4Address murouterId, Ipv4Address myAddress){

	// delete neighbor because of own interface down
	for(std::vector<Neighbor>::iterator i = m_nb.begin (); i != m_nb.end (); ++i){
		if(i->m_localAddress == myAddress){
			i->m_state = NeighborState_Down;
		}
	}
	Purge ();

}

void
Neighbors::DeleteNeighbor(std::vector<Neighbor>::iterator i){
	// cancel its events
	i->m_DDRetransmitEvent.Cancel();
	i->m_lastDDFreeingEvent.Cancel();
	i->m_requestRetransmitEvent.Cancel();
	i->m_lsRetransmitEvent.Cancel();
	i->m_requestTransmitted.clear();
	i->m_LSATransmitted.clear();
	i->m_lsRequestList.clear();
	i->m_lsRetransList.clear();
	i->m_dbSummaryList.clear();

}

void
Neighbors::DeleteNeighborsPMPPTriggering(Ipv4Address myPointAdr, std::vector<Ipv4Address> multiPointsAdrs){

	// A not exist in multiPointsAdrs, must be not in nbs

	for(std::vector<Neighbor>::iterator i = m_nb.begin (); i != m_nb.end (); ++i){
		if(i->m_localAddress == myPointAdr){
			if(find(multiPointsAdrs.begin(), multiPointsAdrs.end(), i->m_neighborAddress) == multiPointsAdrs.end()){
				i->m_state = NeighborState_Down;
			}
		}
	}
	Purge ();


}

void
Neighbors::SendFromRequestList(std::vector<Neighbor>::iterator i){
	if(i->m_requestRetransmitEvent.IsRunning() || i->m_lsRequestList.empty()){
		if(i->m_lsRequestList.empty()){
			i->m_requestRetransmitEvent.Cancel();
		}
		return;
	}
	if(!i->m_requestTransmitted.empty()){
		return;
	}

	// 20 bytes IP header
	// tlr header 10 bytes
	// typeID 1 byte
	// LSRHeader 4 bytes
	// LSRPacket 12 bytes
	uint32_t maxLSAHeaderNum = (1500 - 20 - 10 - 1 - 4) / 12; // 92 // need a interface for mtu 1500 bytes
	std::vector<LSAHeader> sentList;
	if(maxLSAHeaderNum >= i->m_lsRequestList.size()){
		sentList = i->m_lsRequestList;
		i->m_requestTransmitted = i->m_lsRequestList;
	}
	else{
		sentList = std::vector<LSAHeader>({i->m_lsRequestList.begin(), i->m_lsRequestList.begin() + maxLSAHeaderNum});
		i->m_requestTransmitted = sentList;
	}
	m_handleLSRTriggering(i->m_localAddress, i->m_neighborAddress, sentList);
	// retransmit schedule
	i->m_requestRetransmitEvent.Cancel();
	i->m_requestRetransmitEvent = Simulator::Schedule(m_rxmtInterval,
			&Neighbors::RequestRetransmit, this, i->m_localAddress, i->m_neighborAddress);
}

void
Neighbors::SendFromRetransmissionList(std::vector<Neighbor>::iterator i){
	if(i->m_lsRetransmitEvent.IsRunning() || i->m_lsRetransList.empty()){
		if(i->m_lsRetransList.empty()){
			i->m_lsRetransmitEvent.Cancel();
		}
		return;
	}
	if(!i->m_LSATransmitted.empty()){
		return;
	}

	// Ip header 20 bytes
	// typeID 1 byte
	// tlr header 10 bytes
	// LSU header 4 bytes
	// LSUPacket

	std::vector<std::pair<LSAHeader,LSAPacket>> sentList;
	std::vector<LSAHeader> sentHeader;
	uint32_t LSUPacketSize = 10 + 20 + 1 + 4;
	for(auto lsa : i->m_lsRetransList){
		LSUPacketSize += (lsa.first.GetSerializedSize() + lsa.second.GetSerializedSize());
		if(LSUPacketSize <= 1500){
			sentList.push_back(lsa);
			sentHeader.push_back(lsa.first);
		}
		else{
			break;
		}
	}
	i->m_LSATransmitted = sentHeader;
	m_handleLSUTriggering(i->m_localAddress, i->m_neighborAddress, sentList);

	// retransmit schedule
	i->m_lsRetransmitEvent.Cancel();
	i->m_lsRetransmitEvent = Simulator::Schedule(m_rxmtInterval,
			&Neighbors::UpdateRetransmit, this, i->m_localAddress, i->m_neighborAddress);

}

void
Neighbors::DeleteFromRequestList(std::vector<Neighbor>::iterator i, std::vector<LSAHeader> lasack){

	for(auto lsaHeader : lasack){
		auto it = find(i->m_requestTransmitted.begin(), i->m_requestTransmitted.end(), lsaHeader);
		if(it != i->m_requestTransmitted.end()){
			i->m_requestTransmitted.erase(it);
			i->m_lsRequestList.erase(find(i->m_lsRequestList.begin(), i->m_lsRequestList.end(), lsaHeader));
		}
	}
	if(i->m_requestTransmitted.empty()){
		i->m_requestRetransmitEvent.Cancel();
		SendFromRequestList(i);
	}

}

void
Neighbors::DeleteFromRetransmissionList(std::vector<Neighbor>::iterator i, std::vector<LSAHeader> lasack){

	for(auto lsaHeader : lasack){
		auto it = find(i->m_LSATransmitted.begin(), i->m_LSATransmitted.end(), lsaHeader);
		if(it != i->m_LSATransmitted.end()){
			i->m_LSATransmitted.erase(it);
			for(std::vector<std::pair<LSAHeader,LSAPacket>>::iterator iter = i->m_lsRetransList.begin(); iter != i->m_lsRetransList.end(); iter++){
				if(iter->first == lsaHeader){
					i->m_lsRetransList.erase(iter);
					break;
				}
			}
		}
	}
	if(i->m_LSATransmitted.empty()){
		i->m_lsRetransmitEvent.Cancel();
		SendFromRetransmissionList(i);
	}

}

uint16_t
Neighbors::GetLinkCost(Ipv4Address ad){


	uint32_t itface = m_ipv4->GetInterfaceForAddress(ad);
	if(m_ipv4->GetNetDevice(itface)->GetInstanceTypeId() == TypeId::LookupByName ("ns3::PointToPointLaserNetDevice")){

	Ptr<PointToPointLaserNetDevice> myDec =m_ipv4->GetNetDevice(itface)->GetObject<PointToPointLaserNetDevice>();
	Ptr<Node> my = m_ipv4->GetObject<Node>();
	Ptr<Node> dst = myDec->GetDestinationNode();
	Ptr<MobilityModel> dstMobility = dst->GetObject<MobilityModel>();
	Ptr<MobilityModel> myMobility = my->GetObject<MobilityModel>();
	double d = myMobility->GetDistanceFrom(dstMobility)/(3*1e4);
	uint16_t pDelay = (uint16_t)d;// unit: 0.1ms
	double q = myDec->GetQueueOccupancyRate();
	q=q/100*myDec->GetMaxsize();
	//if (q > 0 ) std::cout<<"QueueOccupancyRate"<<int(q)<<std::endl;
	uint64_t cap=myDec->GetDataRate();
	//std::cout<<"capacity"<<" "<<cap<<std::endl;
    double avepacketlength=1500*8;//set average packet length = 1500byte
    //double np=double(nowpacket);
    double expected_queuingdelay=q*avepacketlength/(1.0*cap);////MaxPacketSize=100
    expected_queuingdelay=expected_queuingdelay*10000;//unit 0.1ms

    uint16_t qDelay=(uint16_t) expected_queuingdelay;
	return  pDelay + qDelay;
	}

	else  return 2000;

}

//void
//Neighbors::DeleteLSAInRetransmissionList(std::vector<Neighbor>::iterator i, LSAHeader lsaHeader){
//	// just delete the first found
//	for(uint32_t k = 0; k < i->m_lsRetransList.size(); k++){
//		if(i->m_lsRetransList.at(k).first == lsaHeader){
//			i->m_lsRetransList.erase(i->m_lsRetransList.begin() + k);
//			break;
//		}
//	}
//
//}

//void
//Neighbors::DeleteLSAInLSRList(std::vector<Neighbor>::iterator i, LSAHeader lsaHeader){
//	std::vector<LSAHeader>::iterator it = find(i->m_lsRequestList.begin(),i->m_lsRequestList.end(),lsaHeader);
//	if(it == i->m_lsRequestList.end()){
//		//throw std::runtime_error ("Wrong interface address: Neighbors::DeleteLSAInLSRList");
//	}
//	else{
//		i->m_lsRequestList.erase(it);
//	}
//
//}



}  // namespace tlr
}  // namespace ns3

