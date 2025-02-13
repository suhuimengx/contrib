/*
 * iadr-neighbor.cc
 *
 *  Created on: 2023年1月5日
 *      Author: root
 */
#include <iostream>

#define NS_LOG_APPEND_CONTEXT							\
   std::clog << "[node " << m_myRouterId.Get() << "] ";

#include "iadr-neighbor.h"

#include <algorithm>
#include "ns3/log.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/point-to-point-laser-net-device.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("IADRStateMachine");

namespace iadr {
Neighbors::Neighbors (Time helloInterval, Time rtrDeadInterval, Time rxmtInterval, Time LSRefreshInterval, Time routcalInterval, uint16_t ContrllerNo)
  : m_ntimer (Timer::CANCEL_ON_DESTROY)
{
	//bool floodornot=false;
	m_helloInterval = helloInterval;
	m_ntimer.SetDelay (helloInterval);
	m_ntimer.SetFunction (&Neighbors::Purge, this);
	m_rxmtInterval = rxmtInterval;
	m_routerDeadInterval = rtrDeadInterval;
	m_LSRefreshTime = LSRefreshInterval;
	m_routcalInterval=routcalInterval;
	m_lsdb = IADRLSDB();
	m_lsdb.SetFloodCallback(MakeCallback(&Neighbors::Flood,this));
	ContrllerSeq=ContrllerNo;

}

Neighbors ::Neighbors()
  : m_ntimer (Timer::CANCEL_ON_DESTROY)
{
	m_lsdb = IADRLSDB();
	m_lsdb.Clear();

	m_lsdb.SetFloodCallback(MakeCallback(&Neighbors::Flood,this));
	//m_buildrouting=IadrBuildRouting();
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
 // bool
  bool floodornot=false;
  Purge (floodornot);
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
    for (std::vector<Neighbor>::iterator i = m_nb.begin (); i != m_nb.end (); ++i)
    {
      if (i->m_neighborAddress == neighborAddress)
        {
    	  // update expireTime
          i->m_expireTime = std::max (expireTime + Simulator::Now (), i->m_expireTime);
          NS_LOG_LOGIC ("Neighbor " << neighborAddress << " exists, just update its expire time");
          // update state
          //std::cout<<"my own r"
          UpdateState(i, neighors);
          // update LSDB
         /* if(murouterId.Get()==2)
			{
				std::cout<< murouterId.Get()  <<" 2222has neighbors: "<<m_nb.size()<<std::endl;
			}*/
          LSDBUpdateDecision();
          //todo
          std::map<IADRLinkStateIdentifier, std::pair<LSAHeader,LSAPacket>> ldb=m_lsdb.GetDataBase ();
          //std::cout<< "my neighbor ID is "<< m_myRouterId <<" m_lsdb: "<<ldb.size() <<std::endl;
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
		// send first DD sent
		m_handleDDTriggering(i->m_localAddress, i->m_neighborAddress, 7, i->m_ddSeqNum,{});
		i->m_DDRetransmitEvent.Cancel();
		i->m_DDRetransmitEvent = Simulator::Schedule(m_rxmtInterval,
								&Neighbors::DDRetransmit, this, i->m_localAddress, i->m_neighborAddress, 7, i->m_ddSeqNum);
		NS_LOG_LOGIC ("Set DD retransmission event from "<<i->m_localAddress<<" to "<<i->m_neighborAddress<<" in "<<m_rxmtInterval.GetSeconds()<<" s");


		//bool
		bool floodornot=false;
		Purge(floodornot);
	}

	// Update neighbors and expireTime
	//todo


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
		//i->m_lsRequestList.clear();
		i->m_lsRetransList.clear();
		//i->m_dbSummaryList.clear();
	}
	// update neighbors and expireTime
	//bool
	bool floodornot=false;
	Purge(floodornot);
	// Here need to reset hello mission for local interface
	NS_LOG_LOGIC ("Send Hello from "<<i->m_localAddress<<" to "<<i->m_neighborAddress);

	m_handleHelloTriggering(i->m_localAddress);

}


void
Neighbors::NegotiationDone (std::vector<Neighbor>::iterator i)
{
	//begin to send lsas
	NS_LOG_FUNCTION (this);
	i->m_state = NeighborState_Exchange;
	NS_LOG_LOGIC ("Change to neighborState: " << "Exchange " << "myAdr: " << i->m_localAddress << " neighborAdr: " << i->m_neighborAddress);

	// Each LSA in the area’s link-state database (at the time the neighbor transitions into Exchange state) is
	// listed in the neighbor Database summary list.
	//##########Database summary list maintain
	NS_LOG_LOGIC("Send DD packet for information report");


	std::map<IADRLinkStateIdentifier, std::pair<LSAHeader,LSAPacket>> db=m_lsdb.GetDataBase ();

	std::vector<std::pair<LSAHeader,LSAPacket>> lsas = {};
	uint32_t counter = 0;
	for(uint32_t it = 1; it < m_ipv4->GetNInterfaces(); it++){
		bool interfaceUp = m_ipv4->GetNetDevice(it)->IsLinkUp();
		if(interfaceUp){
			counter++;
		}
	}
	//std::cout<< "interfaceUp is "<< counter <<std::endl;
	//if(m_ipv4->GetObject<Node>()->GetId()!=3){
	//std::cout<< ContrllerSeq <<" ContrllerSeq "<<std::endl;
	if(m_ipv4->GetObject<Node>()->GetId()!= ContrllerSeq){
		for (auto it = db.begin(); it != db.end(); ++it)
		//for(auto& elem : m_lsdb.GetDataBase ())
		{
			//if(it->second.second.Getlinkn()>= counter-1){
				lsas.push_back(it->second);
				//IADRLinkStateIdentifier lsaidenti(lsas.begin()->first.GetLSAType(),lsas.begin()->first.GetLinkStateID(),lsas.begin()->first.GetAdvertisiongRouter());
				//m_lsdb.addforwardedDD(lsas.front());
				m_lsdb.addforwardedDD(lsas.front().first);
				for (std::vector<Neighbor>::iterator iter = m_nb.begin (); iter != m_nb.end (); ++iter)
				{
					m_handleDDTriggering(iter->m_localAddress, iter->m_neighborAddress, 3, iter->m_ddSeqNum, lsas); // 00000011
					i->m_DDRetransmitEvent.Cancel();
					i->m_DDRetransmitEvent = Simulator::Schedule(m_rxmtInterval,
																 &Neighbors::DDRetransmit, this, iter->m_localAddress, iter->m_neighborAddress, 7, iter->m_ddSeqNum);
					NS_LOG_LOGIC("Set DD packet retransmission event in "<<m_rxmtInterval.GetSeconds()<<" s");
				}
			//}
		}
	}

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
Neighbors::DDReceived (Ipv4Address myOwnrouterID, Ipv4Address nbAddress, uint8_t flags, uint32_t seqNum, std::vector<std::pair<LSAHeader,LSAPacket>> lsas)
{
	NS_LOG_FUNCTION (this);

	uint32_t rtrID = 100;
	bool debugForRtrID = false;
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
            //std::cout<<Simulator::Now()<<"  "<<"no nb"<<std::endl;
            for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
                //std::cout<<i->m_neighborAddress<<"  "<<i->m_state<<std::endl;
            }
        }
		NS_LOG_LOGIC ("Neighbor "<<nbAddress<<" not found");
		return;
	}

	if(it->m_state == NeighborState_Down){
		// The packet should be rejected.
		if(myOwnrouterID == Ipv4Address(rtrID) && debugForRtrID){
			//std::cout<<Simulator::Now()<<"  "<<"down"<<std::endl;
			for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
				//std::cout<<i->m_neighborAddress<<"  "<<i->m_state<<std::endl;
			}
		}
		NS_LOG_LOGIC ("Current NeighborState: Down, reject the DD packet");
	}
	else if(it->m_state == NeighborState_Init){
		NS_LOG_LOGIC ("Current neighborState: Init");
		TwoWayReceived(it);
		DDReceived (myOwnrouterID, nbAddress, flags, seqNum, lsas);
	}
	else if(it->m_state == NeighborState_2Way){
		// The packet should be ignored.
		// Actually, interfaces in p2p network enter exstart by default
		if(myOwnrouterID == Ipv4Address(rtrID) && debugForRtrID){
			//std::cout<<Simulator::Now()<<"  "<<"2way"<<std::endl;
			for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
				//std::cout<<i->m_neighborAddress<<"  "<<i->m_state<<std::endl;
			}
		}
		NS_LOG_LOGIC ("Current neighborState: 2Way");
		NegotiationDone(it);
	}

	else if(it->m_state == NeighborState_Exchange){
		NS_LOG_LOGIC ("Current neighborState: Exchange");
		if(flags == 7){
			return;
		}

		// Duplicate Database Description packets are discarded by the
		// master, and cause the slave to retransmit the last Database
		// Description packet that it had sent.

		//xhqin
		// (4) Database Description packets must be processed in sequence, as indicated by the packets’ DD sequence numbers.
		//xhqin if(it->m_mode == 1 && seqNum == it->m_ddSeqNum){
        //if(seqNum == it->m_ddSeqNum){
			// accept and processed
        //it->m_ddSeqNumLast = seqNum;
        //it->m_flagsLast = flags;
        DDProcessed(it, flags, seqNum, lsas);
        if(myOwnrouterID == Ipv4Address(rtrID) && debugForRtrID){
            //std::cout<<Simulator::Now()<<"  "<<"sxhcange3"<<std::endl;
            for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
                //std::cout<<i->m_neighborAddress<<"  "<<i->m_state<<std::endl;
            }
        }
        return;
		}

}

void
Neighbors::DDProcessed (std::vector<Neighbor>::iterator i, uint8_t flags, uint32_t seqNum, std::vector<std::pair<LSAHeader,LSAPacket>> lsas)
{
	NS_LOG_FUNCTION (this);

    //xhqin

	if (i->m_myRouterID.Get() != ContrllerSeq) {
		for(auto lsa : lsas) {
			if (m_lsdb.HasDD(lsa.first)) {
				//std::cout<<i->m_localAddress.Get()<<"has received lsa"<< lsa.first.GetAdvertisiongRouter() <<std::endl;
				NS_LOG_LOGIC("A duplicate DD packet received");
				NS_LOG_LOGIC("Discards the duplicate DD packet");
			}
			else {

				//std::cout<<i->m_myRouterID.Get()<<"receives new lsa and will forward"<<lsa.first.GetAdvertisiongRouter() << std::endl;

            	std::vector<std::pair<LSAHeader,LSAPacket>> sentList;
            	sentList.push_back(lsa);
            	for (std::vector<Neighbor>::iterator it = m_nb.begin (); it != m_nb.end (); ++it)
				{
					m_handleDDTriggering(it->m_localAddress, it->m_neighborAddress, 3, it->m_ddSeqNum, sentList); // 00000011
					i->m_DDRetransmitEvent.Cancel();
					i->m_DDRetransmitEvent = Simulator::Schedule(m_rxmtInterval,
																 &Neighbors::DDRetransmit, this, it->m_localAddress, it->m_neighborAddress, 3, it->m_ddSeqNum);
					NS_LOG_LOGIC("Set DD packet retransmission event in "<<m_rxmtInterval.GetSeconds()<<" s");
				}

                m_lsdb.addforwardedDD(lsa.first);

                //Flood(lsa, i->m_localAddress, i->m_neighborAddress);
            }
        }
    }
    //SendFromRequestList(i);

    if(i->m_myRouterID.Get() == ContrllerSeq){
    	//std::cout<< m_LSRefreshTime<<" test~~~~~~~~~: "<<std::endl;
    	//std::cout<< m_lsdb.GetDataBase().size() <<" m_db size"<<std::endl;
    	if(firstrecvlsa){
    		firstrecvlsa = false;
    		m_lsdb.UpdateRoute(m_routcalInterval);
    	}


        std::map<Ipv4Address, std::pair<Ipv4Address, Ipv4Address>> interfaces;
		for(std::vector<Neighbor>::iterator it = m_nb.begin (); it != m_nb.end (); ++it){
				interfaces[it->m_routerID] = std::make_pair(it->m_localAddress, it->m_neighborAddress);
        }
        m_lsdb.SetInterfaceAddress(interfaces);
        m_lsdb.UpdateAge();

        for (std::vector<std::pair<LSAHeader,LSAPacket>>::iterator ite = lsas.begin(); ite != lsas.end(); ++ite)
        {
			ite->first.SetLSAge(m_lsdb.UpdateCurrentPacketAge(ite->first.GetLSAge(), ite->first.GetLSAddedTime()));

            ///check the lsatype, if type not equal to 1, discard it and process next lsa
            uint8_t LinkStateType = ite->first.GetLSAType();

            /// construct LSA Identifier
            Ipv4Address LinkStateID = ite->first.GetLinkStateID();
            Ipv4Address AdvertisingRouter = ite->first.GetAdvertisiongRouter();
            IADRLinkStateIdentifier lsaidenti(LinkStateType,LinkStateID,AdvertisingRouter);
            bool exist = m_lsdb.Has(lsaidenti);
            bool newer, older, same;
            if(exist == true){
                newer = m_lsdb.checknewer(*ite);
                older = m_lsdb.checkolder(*ite);
                same = !(newer || older);
            }

            /// If there is no database copy, or the received LSA is more recent than the database copy
            /// if the lsa is in the database and database's lsa is received by flooding and database's lsa's (now-added_time)<MinLSArrival,check next lsa
            if(exist == false || (exist == true && newer == true))
            {
                /// 如果没有数据库副本或者接受到的数据库副本更新, flooding first(actually flooding lsa after the end of for)
                // Add the new LSA in the LSDB, this may result in routing table calculation
            	//bool lastLsaInLsu = true;

                m_lsdb.Add(*ite/*,lastLsaInLsu*/);
            	//std::cout<< m_routcalInterval <<" test~~~~~~~~~: "<<std::endl;
                m_lsdb.UpdateRoute(m_routcalInterval);

                // If the LSA is originated by self
                // In most cases, the router must then advance the LSA’s LS sequence number one past the received LS sequence number, and
                // originate a new instance of the LSA.

            }
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
                }
            }
        }
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
				std::vector<std::pair<LSAHeader,LSAPacket>> lsa={};
				if(flags == 7){
					lsa = {};
					m_handleDDTriggering(localAddress, neighborAddress, flags, seqNum, lsa);
				}
				else{
					if(m_ipv4->GetObject<Node>()->GetId()!= ContrllerSeq){
					//if(m_ipv4->GetObject<Node>()->GetId()!=3){
						std::map<IADRLinkStateIdentifier, std::pair<LSAHeader,LSAPacket>> db=m_lsdb.GetDataBase ();
						//if (db.begin()->second.second.Getlinkn()== 4)
							//{
						uint32_t counter = 0;
						for(uint32_t it = 1; it < m_ipv4->GetNInterfaces(); it++){
							bool interfaceUp = m_ipv4->GetNetDevice(it)->IsLinkUp();
							if(interfaceUp){
								counter++;
							}
						}
						for(auto it = db.begin(); it != db.end(); ++it)
						{
							//if (it->second.second.Getlinkn()>= counter-1){
								lsa.push_back(it->second);
							//}
						}
						m_handleDDTriggering(localAddress, neighborAddress, flags, seqNum, lsa); // 00000011
					}
				}
				//m_handleDDTriggering(localAddress, neighborAddress, flags, seqNum, lsa);
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
Neighbors::Flood (std::vector<std::pair<LSAHeader,LSAPacket>> lsas, Ipv4Address myOwnrouterID, Ipv4Address nbAddress){
	std::vector<std::pair<LSAHeader,LSAPacket>> sendlist = {};

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

				std::map<IADRLinkStateIdentifier, std::pair<LSAHeader,LSAPacket>> db=m_lsdb.GetDataBase ();
				for (auto lt = db.begin();lt!=db.end();lt++){
					if(lsa.first == lt->second.first){
						if(m_lsdb.checknewer(lt->second.first, lsa.first)){
							continue;
						}
						else if(m_lsdb.checknewer(lsa.first, lt->second.first)){
							//DeleteLSAInLSRList(iter, *lt);
						}
						else{
							//DeleteLSAInLSRList(iter, lsa.first);
							continue;
						}
					}
					sendlist.push_back(lsa);
					m_lsdb.addforwardedDD(lsa.first);
				}
			}
		}
	}
	//std::cout<<" Isl interuption flood "<< m_ipv4->GetObject<Node>()->GetId() <<std::endl;
	for(std::vector<Neighbor>::iterator iterNb = m_nb.begin(); iterNb!=m_nb.end(); ++iterNb)
	{
		m_handleDDTriggering(iterNb->m_localAddress, iterNb->m_neighborAddress, 3, iterNb->m_ddSeqNum, sendlist); // 00000011
		iterNb->m_DDRetransmitEvent.Cancel();
		iterNb->m_DDRetransmitEvent = Simulator::Schedule(m_rxmtInterval,
													 &Neighbors::DDRetransmit, this, iterNb->m_localAddress, iterNb->m_neighborAddress, 3, iterNb->m_ddSeqNum);
		NS_LOG_LOGIC("Set DD packet retransmission event in "<<m_rxmtInterval.GetSeconds()<<" s");
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
Neighbors::Purge (bool floodornot)
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
  UpdateLSDB(floodornot);
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

void
Neighbors::LSDBUpdateDecision(){
	NS_LOG_FUNCTION (this);
	// only for router-LSA
	std::vector<LSALinkData> lsaLinkData;
	std::map<Ipv4Address, std::pair<Ipv4Address, Ipv4Address>> interfaces;
	for(std::vector<Neighbor>::iterator i = m_nb.begin (); i != m_nb.end (); ++i){
		/*if(i->m_myRouterID.Get() == 6){
				std::cout<< i->m_routerID  <<" has links info!!!!!: "<<std::endl;
		}*/
		if(i->m_state >= NeighborState_2Way){
			lsaLinkData.push_back(LSALinkData(i->m_routerID, i->m_localAddress, i->m_neighborAddress, GetLinkCost(i->m_localAddress), GetLinkCost2(i->m_localAddress), GetLinkCost(i->m_localAddress))); // metric = 1 by default todo
			if(i->m_routerID.Get() == 6){
				//std::cout<< i->m_routerID  <<" has links info!!!!!: "<<std::endl;
			}
			interfaces[i->m_routerID] = std::make_pair(i->m_localAddress, i->m_neighborAddress);
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
	/*std::cout<< counter <<" count is"<<std::endl;
	std::cout<< lsaLinkData.size() <<" lsalinkdata is"<<std::endl;*/
	//if(lsaLinkData.size()==4)
	//{
		//std::cout<< lsaLinkData.size() <<" lsalinkdata stop"<<std::endl;
	//}
	if(lsaLinkData.size() < counter){
		//m_lsdb.Clear();
		return;
	}

	if(lsaLinkData.empty()){
		//m_lsdb.Clear();
		return;
	}

	// router-lsa by default, because we just consider single domain in current version
	LSAHeader tempLsaHeader(/*LSage=*/ 0, /*options=*/0, /*LinkStateID=*/m_myRouterId, /*AdvertisingRouter=*/m_myRouterId,
				/*LSsequence=*/0, /*checkSum=*/0, /*PacketLength=*/0, /*addedTime=*/(uint32_t)Simulator::Now().GetSeconds());
	LSAHeader lsaHeader(/*LSage=*/ 0, /*options=*/0, /*LinkStateID=*/m_myRouterId, /*AdvertisingRouter=*/m_myRouterId,
						/*LSsequence=*/0, /*checkSum=*/0, /*PacketLength=*/0, /*addedTime=*/(uint32_t)Simulator::Now().GetSeconds());

	// just update when interface number changes (interface up or down)
	//std::cout<< m_lsdb.Has(tempLsaHeader) <<" LSDB has "<<std::endl;
	if(m_lsdb.Has(tempLsaHeader)){

		std::pair<LSAHeader,LSAPacket> lsa = m_lsdb.Get(tempLsaHeader);//linshi der
		//std::cout<< lsa.second.Getlinkn() <<" lsaLinkData size "<<std::endl;

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
	//std::cout<< m_myRouterId.Get() <<" test hello send twice "<<std::endl;
    NS_LOG_LOGIC ("Originate new Router-LSA ");
    uint32_t seqLast = m_lsdb.AddSeqForSelfOriginatedLSA();
    lsaHeader.SetLSSequence(seqLast);
	LSAPacket lsaPacket(0, lsaLinkData.size(), lsaLinkData);
	m_lsdb.SetInterfaceAddress(interfaces);
	m_lsdb.Add(std::make_pair(lsaHeader,lsaPacket));
	//m_lsdb.UpdateRoute();
	std::vector<std::pair<LSAHeader,LSAPacket>> lsaTemp = {std::make_pair(lsaHeader,lsaPacket)};
	//Flood(lsaTemp,Ipv4Address::GetAny(),Ipv4Address::GetAny());
	// This guarantees periodic originations of all LSAs.
	m_LSOriginatedEvent.Cancel();
	m_LSOriginatedEvent = Simulator::Schedule(m_LSRefreshTime, &Neighbors::LSAPeriodicOrigination, this);
	if(firstdellsdb){
		firstdellsdb=false;
		m_DelControllerLSDBEvent.Cancel();
		m_DelControllerLSDBEvent = Simulator::Schedule(m_LSRefreshTime, &Neighbors::DeleteControllerLSDB, this);
	}

}


void
Neighbors::UpdateLSDB(bool floodornot){
	NS_LOG_FUNCTION (this);
	// only for router-LSA
	std::vector<LSALinkData> lsaLinkData;
	std::map<Ipv4Address, std::pair<Ipv4Address, Ipv4Address>> interfaces;
	for(std::vector<Neighbor>::iterator i = m_nb.begin (); i != m_nb.end (); ++i){
		if(i->m_state >= NeighborState_2Way){
			lsaLinkData.push_back(LSALinkData(i->m_routerID, i->m_localAddress, i->m_neighborAddress, GetLinkCost(i->m_localAddress),GetLinkCost(i->m_localAddress),GetLinkCost(i->m_localAddress))); // metric = 1 by default todo
			interfaces[i->m_routerID] = std::make_pair(i->m_localAddress, i->m_neighborAddress);
		}
	}

	// router-lsa by default, because we just consider single domain in current version
	LSAHeader tempLsaHeader(/*LSage=*/ 0, /*options=*/0, /*LinkStateID=*/m_myRouterId, /*AdvertisingRouter=*/m_myRouterId,
				/*LSsequence=*/0, /*checkSum=*/0, /*PacketLength=*/0, /*addedTime=*/(uint32_t)Simulator::Now().GetSeconds());
	LSAHeader lsaHeader(/*LSage=*/ 0, /*options=*/0, /*LinkStateID=*/m_myRouterId, /*AdvertisingRouter=*/m_myRouterId,
						/*LSsequence=*/0, /*checkSum=*/0, /*PacketLength=*/0, /*addedTime=*/(uint32_t)Simulator::Now().GetSeconds());
	if(lsaLinkData.empty()){
		//m_lsdb.Clear();
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
	//std::cout<< m_lsdb.GetDataBase().size() <<" lsalinkdata is"<<std::endl;
	//std::cout<< m_myRouterId.Get() << "m_db has size!!!!!"<< m_lsdb.GetDataBase().size()<<std::endl;
	//m_lsdb.UpdateRoute();
	std::vector<std::pair<LSAHeader,LSAPacket>> lsaTemp = {std::make_pair(lsaHeader,lsaPacket)};
	if (floodornot && m_ipv4->GetObject<Node>()->GetId()!= ContrllerSeq){
		Flood(lsaTemp,Ipv4Address::GetAny(),Ipv4Address::GetAny());
	}
	// This guarantees periodic originations of all LSAs.
	m_LSOriginatedEvent.Cancel();
	m_LSOriginatedEvent = Simulator::Schedule(m_LSRefreshTime, &Neighbors::LSAPeriodicOrigination, this);
}



void
Neighbors::LSAPeriodicOrigination(){
	NS_LOG_FUNCTION (this);
	//std::cout<< m_myRouterId.Get() <<" LSAPeriodicOrigination "<<std::endl;

	// only for router-LSA
	std::vector<LSALinkData> lsaLinkData;
	std::map<Ipv4Address, std::pair<Ipv4Address, Ipv4Address>> interfaces;
	for(std::vector<Neighbor>::iterator i = m_nb.begin (); i != m_nb.end (); ++i){
		if(i->m_state >= NeighborState_2Way){
			lsaLinkData.push_back(LSALinkData(i->m_routerID, i->m_localAddress, i->m_neighborAddress, GetLinkCost(i->m_localAddress),GetLinkCost(i->m_localAddress),GetLinkCost(i->m_localAddress))); // metric = 1 by default todo
			interfaces[i->m_routerID] = std::make_pair(i->m_localAddress, i->m_neighborAddress);
		}
	}

	// router-lsa by default, because we just consider single domain in current version
	LSAHeader tempLsaHeader(/*LSage=*/ 0, /*options=*/0, /*LinkStateID=*/m_myRouterId, /*AdvertisingRouter=*/m_myRouterId,
				/*LSsequence=*/0, /*checkSum=*/0, /*PacketLength=*/0, /*addedTime=*/(uint32_t)Simulator::Now().GetSeconds());
	LSAHeader lsaHeader(/*LSage=*/ 0, /*options=*/0, /*LinkStateID=*/m_myRouterId, /*AdvertisingRouter=*/m_myRouterId,
						/*LSsequence=*/0, /*checkSum=*/0, /*PacketLength=*/0, /*addedTime=*/(uint32_t)Simulator::Now().GetSeconds());


	if(lsaLinkData.empty()){
		//m_lsdb.Clear();
		return;
	}

	// just update

	uint32_t seqLast = m_lsdb.AddSeqForSelfOriginatedLSA();
	lsaHeader.SetLSSequence(seqLast);

    NS_LOG_LOGIC ("Originate Router-LSA ");
	LSAPacket lsaPacket(0, lsaLinkData.size(), lsaLinkData);
	m_lsdb.SetInterfaceAddress(interfaces);
	m_lsdb.Add(std::make_pair(lsaHeader,lsaPacket)/*, true*/);
	std::vector<std::pair<LSAHeader,LSAPacket>> lsaTemp = {std::make_pair(lsaHeader,lsaPacket)};
	//if(m_ipv4->GetObject<Node>()->GetId()!=3){
	if(m_ipv4->GetObject<Node>()->GetId()!= ContrllerSeq){
	Flood(lsaTemp,Ipv4Address::GetAny(),Ipv4Address::GetAny());
	}
	firstrecvlsa = false;
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
	//bool
	bool floodornot=true;
	Purge (floodornot);

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
	//bool
	bool floodornot=true;
	Purge (floodornot);


}

void
Neighbors::DeleteNeighbor(std::vector<Neighbor>::iterator i){
	// cancel its events
	i->m_DDRetransmitEvent.Cancel();
	//i->m_lastDDFreeingEvent.Cancel();
	i->m_requestRetransmitEvent.Cancel();
	i->m_lsRetransmitEvent.Cancel();
	//i->m_requestTransmitted.clear();
	i->m_LSATransmitted.clear();
	//i->m_lsRequestList.clear();
	i->m_lsRetransList.clear();
	//i->m_dbSummaryList.clear();

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

	// standard: ospf header 24 bytes     simulation: ospf header 10 bytes
	// standard: 12 bytes for one LSA header         simulation: 12 bytes for one LSA header + 11 bytes

	std::vector<std::pair<LSAHeader,LSAPacket>> sentList;
	//std::vector<LSAHeader> sentHeader;
	//uint32_t LSUPacketSize = 24;
	for(auto lsa : i->m_lsRetransList){
        sentList.push_back(lsa);

        NS_LOG_LOGIC("Forward DD packet");

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

uint8_t
Neighbors::GetLinkCost(Ipv4Address ad){

//	uint32_t itface = m_ipv4->GetInterfaceForAddress(ad);
//	auto myDec =m_ipv4->GetNetDevice(itface)->GetObject<SAGLinkLayer>();//todo
//	Ptr<Node> my = m_ipv4->GetObject<Node>();
//	Ptr<Node> dst = myDec->GetDestinationNode();
//	Ptr<MobilityModel> dstMobility = dst->GetObject<MobilityModel>();
//	Ptr<MobilityModel> myMobility = my->GetObject<MobilityModel>();
//	//std::cout<< myMobility->GetDistanceFrom(dstMobility)<<std::endl;
//	//std::cout<< myMobility->GetDistanceFrom(dstMobility)/(3*1e5)<<std::endl;
//	uint16_t cost1 = myMobility->GetDistanceFrom(dstMobility)/(3*1e4);
//	uint8_t cost = myMobility->GetDistanceFrom(dstMobility)/(3*1e4);// / 10 = ms
//	if(cost1 > 255){
//		throw std::runtime_error("Process Wrong: Neighbors::GetLinkCost");
//	}
	return 1;

}

uint16_t
Neighbors::GetLinkCost2(Ipv4Address ad){

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

void
Neighbors::RTSReceived (Ipv4Address myOwnrouterID, Ipv4Address nbAddress, std::pair<RTSHeader,RTSPacket> rts)
{
	NS_LOG_FUNCTION (this);
	///////////////////////////////
	// these just for debug, finally will be deleted
	uint32_t rtrID = 100;
	bool debugForRtrID = false;

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
            //std::cout<<Simulator::Now()<<"  "<<"no nb"<<std::endl;
            for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
                //std::cout<<i->m_neighborAddress<<"  "<<i->m_state<<std::endl;
            }
        }
		NS_LOG_LOGIC ("Neighbor "<<nbAddress<<" not found");
		return;
	}

	if(it->m_state == NeighborState_Down){
		// The packet should be rejected.
		if(myOwnrouterID == Ipv4Address(rtrID) && debugForRtrID){
			//std::cout<<Simulator::Now()<<"  "<<"down"<<std::endl;
			for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
				//std::cout<<i->m_neighborAddress<<"  "<<i->m_state<<std::endl;
			}
		}
		NS_LOG_LOGIC ("Current NeighborState: Down, reject the DD packet");
	}
	else if(it->m_state == NeighborState_Init){
		NS_LOG_LOGIC ("Current neighborState: Init");
		TwoWayReceived(it);

	}
	else if(it->m_state == NeighborState_2Way){
		// The packet should be ignored.
		// Actually, interfaces in p2p network enter exstart by default
		if(myOwnrouterID == Ipv4Address(rtrID) && debugForRtrID){
			//std::cout<<Simulator::Now()<<"  "<<"2way"<<std::endl;
			for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
				//std::cout<<i->m_neighborAddress<<"  "<<i->m_state<<std::endl;
			}
		}
		NS_LOG_LOGIC ("Current neighborState: 2Way");
	}
	else if(it->m_state == NeighborState_Exchange){
		NS_LOG_LOGIC ("Current neighborState: Exchange");


		if(it->m_myRouterID.Get() != ContrllerSeq){
			RTSProcessed(myOwnrouterID, it, rts);
		}
        if(myOwnrouterID == Ipv4Address(rtrID) && debugForRtrID){
            //std::cout<<Simulator::Now()<<"  "<<"sxhcange3"<<std::endl;
            for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i) {
                //std::cout<<i->m_neighborAddress<<"  "<<i->m_state<<std::endl;
            }
        }
        return;
		}

}

void
Neighbors::RTSProcessed (Ipv4Address myOwnrouterID, std::vector<Neighbor>::iterator i, std::pair<RTSHeader,RTSPacket> rts)
{
	NS_LOG_FUNCTION (this);
    //xhqin
	/*if (rts.first.GetPathage() == 0){
		m_lsdb.DeleteforwardedRTS(rts);
	}*/
	//rts.first.GetPathage()
	if (m_lsdb.HasRTS(rts.first)) {
		//std::cout<<i->m_myRouterID.Get()<<"has received RTS"<< rts.first.GetLocal_RouterID () <<std::endl;
		NS_LOG_LOGIC("A duplicate RTS packet received");
		NS_LOG_LOGIC("Discards the duplicate RTS packet");
	}
	else{
		//std::cout<<i->m_myRouterID.Get()<<"receives new RTS and will forward"<< rts.first.GetLocal_RouterID ()  << std::endl;

		m_lsdb.addforwardedRTS(rts.first);
		//m_lsdb.updateForwardRTS();
		for (std::vector<Neighbor>::iterator it = m_nb.begin (); it != m_nb.end (); ++it)
		{
			//std::cout<<i->m_myRouterID.Get()<<"has received RTS"<< rts.first.GetLocal_RouterID () <<std::endl;
			m_handleRTSTriggering(it->m_localAddress, it->m_neighborAddress, rts); // 00000011
		}
		//m_handleRTSTriggering(i->m_localAddress, i->m_neighborAddress); // 00000011
		//rts.first.SetPathage(rts.first.GetPathage()+1);
		//;

	}

}
void
Neighbors::DeleteControllerLSDB(){
	// cancel its events
	NS_LOG_FUNCTION (this);
	if(m_ipv4->GetObject<Node>()->GetId()== ContrllerSeq){
		m_lsdb.Clear();
		m_DelControllerLSDBEvent.Cancel();
		m_DelControllerLSDBEvent = Simulator::Schedule(m_LSRefreshTime, &Neighbors::DeleteControllerLSDB, this);
	}

}










}  // namespace ospf
}  // namespace ns3

