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

#ifndef OSPF_LSDB_H
#define OSPF_LSDB_H

#include "ns3/nstime.h"
#include <set>
#include <map>
#include <algorithm>
#include <utility>
#include <unordered_map>

#include "ns3/ospf-lsa-identifier.h"
#include "ns3/simulator.h"

#include "ns3/ospf_link_state_packet.h"
#include "ns3/ipv4-address.h"
#include "ns3/simulator.h"
#include "ns3/timer.h"
#include "ns3/sag_rtp_constants.h"

class LSAHeader;
class LSAPacket;
class OSPFLSDB;
struct OSPFLinkStateIdentifier;

namespace ns3 {
namespace ospf {
class OSPFLSDB {
    std::unordered_map<OSPFLinkStateIdentifier, std::pair<LSAHeader,LSAPacket>, hash_ospfIdt, equal_ospfIdt> m_db; // LSA DataBase
    //std::map<OSPFLinkStateIdentifier, Time> m_addedTime; // LSA added time record
    std::unordered_map<OSPFLinkStateIdentifier, uint32_t, hash_ospfIdt, equal_ospfIdt> m_addedAge; // LSA Age
    std::unordered_map<OSPFLinkStateIdentifier, uint32_t, hash_ospfIdt, equal_ospfIdt> m_LsAddedTime; // LSA Generated time


    uint32_t m_maxAge = 36000;
    uint32_t m_maxAgeDiff = 18;
    //Time Minarri = Seconds(20);
    bool m_calculateRoutingTable = false;

    Callback<void, std::unordered_map<OSPFLinkStateIdentifier, std::pair<LSAHeader,LSAPacket>, hash_ospfIdt, equal_ospfIdt>*, std::vector<std::pair<LSAHeader,LSAPacket>>, Ipv4Address, std::unordered_map<Ipv4Address, std::pair<Ipv4Address, Ipv4Address>, hash_adr, equal_adr>> m_routerBuildCallBack;
    Callback<void, std::vector<std::pair<LSAHeader,LSAPacket>>, Ipv4Address, Ipv4Address> m_floodCallBack;
    Ipv4Address m_routerId;
    std::unordered_map<Ipv4Address, std::pair<Ipv4Address, Ipv4Address>, hash_adr, equal_adr> m_interfaces;

    // When an attempt is made to increment the sequence number past the maximum value of N - 1 (0x7fffffff;
    // also referred to as MaxSequenceNumber), the current instance of the LSA must first be flushed from the routing domain. todo
    // This is done by prematurely aging the LSA (see Section 14.1) and reflooding it. As soon as this flood has been acknowledged
    // by all adjacent neighbors, a new instance can be originated with sequence number ofInitialSequenceNumber.
    uint32_t m_seqForSelfOriginatedLSA = 0;

    /// LSA Aging Event
    EventId m_LSAAgingEvent;

	uint32_t m_satelliteNum;
	uint32_t m_gsNum;
	std::vector<std::pair<LSAHeader,LSAPacket>> m_addLSAListNow;


public:
    OSPFLSDB() {}
    // erase DB
    ~OSPFLSDB() {
    }

    std::unordered_map<OSPFLinkStateIdentifier, std::pair<LSAHeader,LSAPacket>, hash_ospfIdt, equal_ospfIdt> GetDataBase () {
        return m_db;
    }

    void SetRouterId(Ipv4Address routerId){
    	m_routerId = routerId;
    }

    uint32_t AddSeqForSelfOriginatedLSA(){
    	// todo
    	// When an attempt is made to increment the sequence number past the maximum value of N - 1 (0x7fffffff;
		// also referred to as MaxSequenceNumber), the current instance of the LSA must first be flushed from the routing domain. todo
		// This is done by prematurely aging the LSA (see Section 14.1) and reflooding it. As soon as this flood has been acknowledged
		// by all adjacent neighbors, a new instance can be originated with sequence number ofInitialSequenceNumber.
    	m_seqForSelfOriginatedLSA++;
    	if(m_seqForSelfOriginatedLSA == UINT32_MAX){
			throw std::runtime_error("An attempt is made to increment the sequence number past the maximum value. Waiting for development!");
    	}
    	return m_seqForSelfOriginatedLSA;
	}

    std::vector<LSAHeader> GetDataBaseSummary (){
		std::vector<LSAHeader> dbSummary = {};
		for(auto it = m_db.begin(); it != m_db.end(); it++){
			dbSummary.push_back(it->second.first);
		}
		return dbSummary;
	}

    void SetRouterBuildCallback (Callback<
    		void,
    		std::unordered_map<OSPFLinkStateIdentifier, std::pair<LSAHeader,LSAPacket>, hash_ospfIdt, equal_ospfIdt>*,
			std::vector<std::pair<LSAHeader,LSAPacket>>,
			Ipv4Address,
			std::unordered_map<Ipv4Address, std::pair<Ipv4Address, Ipv4Address>, hash_adr, equal_adr>> cb)
	{
    	m_routerBuildCallBack = cb;
	}

    void SetFloodCallback (Callback<void, std::vector<std::pair<LSAHeader,LSAPacket>>, Ipv4Address, Ipv4Address> cb)
	{
		m_floodCallBack = cb;
	}

    uint16_t UpdateCurrentPacketAge(uint32_t age, uint32_t addedTime){

		return std::min(((uint16_t)(Simulator::Now() - Seconds(addedTime)).GetSeconds()), (uint16_t)m_maxAge);
		//return std::min(((uint16_t)(Simulator::Now() - Seconds(addedTime)).GetSeconds()), (uint16_t)360);

    }

    void SetInterfaceAddress(std::unordered_map<Ipv4Address, std::pair<Ipv4Address, Ipv4Address>, hash_adr, equal_adr> ads){
    	m_interfaces = ads;
    }

    void Add(std::pair<LSAHeader,LSAPacket> lsa);

    void UpdateRoute();

    void LSAAgingEvent(OSPFLinkStateIdentifier IDForAgingEvent){
    	UpdateAge();
    	if(m_db.find(IDForAgingEvent) == m_db.end()){
    		throw std::runtime_error("Process Wrong: OSPFLSDB::LSAAgingEvent");
    	}
    	else{
    		if(m_db[IDForAgingEvent].first.GetLSAge() != m_maxAge){
    			throw std::runtime_error("Process Wrong: OSPFLSDB::LSAAgingEvent");
    		}
    	}
    	m_floodCallBack({m_db[IDForAgingEvent]}, Ipv4Address::GetAny(), Ipv4Address::GetAny());
    }

    // whether router's age exceed max age
    bool DetectMaxAge(OSPFLinkStateIdentifier id) {
        return m_db[id].first.GetLSAge() == m_maxAge;
    }

    bool DetectMaxSeq(OSPFLinkStateIdentifier id) {
		return m_db[id].first.GetLSSequence() == UINT32_MAX;
	}

//    void AddAgeByFlooding(OSPFLinkStateIdentifier id)
//    {
//    	this->Get(id).first.AddLSAge();
//    	m_addedAge[id] +=  this->Get(id).first.GetInfTransDelay();
//    }

    // check if the database has this router id
    bool Has(OSPFLinkStateIdentifier id) {
        //return m_db.count(id);
        if(m_db.find(id) == m_db.end()){
        	return false;
        }
        else{
        	return true;
        }
    }
    bool Has(LSAHeader tempHeader) {
		OSPFLinkStateIdentifier id(tempHeader.GetLSAType(),tempHeader.GetLinkStateID(),tempHeader.GetAdvertisiongRouter());
		return m_db.count(id);
	}
    // calculate lsa age for now
    uint32_t CalcAge(const OSPFLinkStateIdentifier& id) {
    	//uint32_t age = (ns3::Simulator::Now() - m_addedTime[id]).GetSeconds() + m_addedAge[id];
    	uint32_t age = (uint32_t)((Simulator::Now() - Seconds(m_LsAddedTime[id])).GetSeconds());

    	if(age >= m_maxAge){
    		return m_maxAge;
    	}
    	else{
    		return age;
    	}
    }

    // Get pair<LSAHeader,LSAPacket> for the router id
    std::pair<LSAHeader,LSAPacket> Get(const OSPFLinkStateIdentifier& id) {
        m_db[id].first.SetLSAge(std::min(CalcAge(id), m_maxAge));
        return m_db[id];
    }

    std::pair<LSAHeader,LSAPacket> Get(LSAHeader tempHeader) {
		OSPFLinkStateIdentifier id(tempHeader.GetLSAType(),tempHeader.GetLinkStateID(),tempHeader.GetAdvertisiongRouter());
		m_db[id].first.SetLSAge(std::min(CalcAge(id), m_maxAge));
		return m_db[id];
	}

    void Remove(OSPFLinkStateIdentifier id) {
        m_db.erase(id);
        //m_addedTime.erase(id);
        m_LsAddedTime.erase(id);
        m_addedAge.erase(id);
    }

    void Remove(LSAHeader tempHeader) {
    	// todo
    	// An LSA is deleted from a router’s database when either a) it has been overwritten by a newer instance during the flooding process
    	// (Section 13) or b) the router originates a newer instance of one of its self-originated LSAs (Section 12.4) or c) the LSA ages
    	// out and is flushed from the routing domain (Section 14).
    	// Whenever an LSA is deleted from the database it must also be removed from all neighbors’ Link state retransmission lists (see
    	// Section 10).
    	OSPFLinkStateIdentifier id(tempHeader.GetLSAType(),tempHeader.GetLinkStateID(),tempHeader.GetAdvertisiongRouter());
    	Remove(id);
	}

    void Clear() {
    	m_db.clear();
    	//m_addedTime.clear();
    	m_LsAddedTime.clear();
    	m_addedAge.clear();
    	m_interfaces.clear();
    	// route calculation
		m_routerBuildCallBack(&m_db, {}, m_routerId, m_interfaces);

    }

//    Time CalcExistDuration(OSPFLinkStateIdentifier id)
//    {
//    	return(ns3::Simulator::Now() - m_addedTime[id]);
//    }

    bool NeedToCalRoutingTable(std::pair<LSAHeader,LSAPacket> oldlsa,std::pair<LSAHeader,LSAPacket> lsa)
    {
    	UpdateAge();
    	if(oldlsa.first.GetOption() != lsa.first.GetOption())
    	{
    		return true;
    	}// todo
    	else if((oldlsa.first.GetLSAge() == m_maxAge && lsa.first.GetLSAge() != m_maxAge)|| (lsa.first.GetLSAge() == m_maxAge &&  oldlsa.first.GetLSAge() != m_maxAge))
    	{
    		return true;
    	}
    	else if(oldlsa.first.GetPacketLength() != lsa.first.GetPacketLength())
    	{
    		return true;
    	}
    	// The body of the LSA (i.e., anything outside the 20-byte LSA header) has changed. Note that this excludes changes
    	// in LS Sequence Number and LS Checksum.
    	else if(!(oldlsa.second == lsa.second))
    	{
    		return true;
    	}
    	return false;
    }

    /// if the lsa newer than local lsa
    bool checknewer(std::pair<LSAHeader,LSAPacket> lsa)
    {
    	//UpdateAge();
    	OSPFLinkStateIdentifier locallsaiden(lsa.first.GetLSAType(),lsa.first.GetLinkStateID(),lsa.first.GetAdvertisiongRouter());
    	std::pair<LSAHeader,LSAPacket> locallsa = m_db[locallsaiden];
    	/// compare LS_sequence
        uint32_t Seq = lsa.first.GetLSSequence();
        uint32_t localSeq = locallsa.first.GetLSSequence();
        if(Seq > localSeq)
        {
        	return true;
        }

        else if(Seq < localSeq)
        {
        	return false;
        }

        /// if LS_sequence equal, check checksum
        else if(Seq == localSeq)
        {
        	uint8_t checksum = lsa.first.GetCheckSum();
        	uint8_t localchecksum = locallsa.first.GetCheckSum();
        	if(checksum > localchecksum)
			{
				return true;
			}
        	else if (checksum < localchecksum)
			{
				return false;
			}
        	else if (checksum == localchecksum)
        	{
        		/// if the Age of lsa is maxage, it is higher
        		uint32_t age = lsa.first.GetLSAge();
        		uint32_t localage = locallsa.first.GetLSAge();
        		if (age == m_maxAge && localage != m_maxAge)
        		{
        			return true;
        		}
        		else if (localage == m_maxAge && age != m_maxAge)
        		{
        			return false;
        		}
        		else if((localage == m_maxAge && age == m_maxAge) || (localage != m_maxAge && age != m_maxAge))
        		{
        			/// check the difference of Age, if the difference > MaxAgeDiff, the lower is newer
        			uint32_t diff1 = age - localage;
        			uint32_t diff2 = localage - age;
        			if (diff1 > m_maxAgeDiff)
        			{
        				return false;
        			}
        			else if (diff2 > m_maxAgeDiff)
        			{
        				return true;
        			}
        		}
        	}
		}
        return false;

    }

    bool checknewer(LSAHeader lsaheader)
	{
    	UpdateAge();
		OSPFLinkStateIdentifier locallsaiden(lsaheader.GetLSAType(),lsaheader.GetLinkStateID(),lsaheader.GetAdvertisiongRouter());
		std::pair<LSAHeader,LSAPacket> locallsa = m_db[locallsaiden];
		/// compare LS_sequence
		uint32_t Seq = lsaheader.GetLSSequence();
		uint32_t localSeq = locallsa.first.GetLSSequence();
		if(Seq > localSeq)
		{
			return true;
		}

		else if(Seq < localSeq)
		{
			return false;
		}

		/// if LS_sequence equal, check checksum
		else if(Seq == localSeq)
		{
			uint8_t checksum = lsaheader.GetCheckSum();
			uint8_t localchecksum = locallsa.first.GetCheckSum();
			if(checksum > localchecksum)
				{return true;}
			else if (checksum < localchecksum)
				{
				return false;
				}
			else if (checksum == localchecksum)
			{
				/// if the Age of lsa is maxage, it is higher
				uint32_t age = lsaheader.GetLSAge();
				uint32_t localage = locallsa.first.GetLSAge();
				if (age == m_maxAge && localage != m_maxAge)
				{
					return true;
				}
				else if (localage == m_maxAge && age != m_maxAge)
				{
					return false;
				}
				else if((localage == m_maxAge && age == m_maxAge) ||(localage != m_maxAge && age != m_maxAge))
				{
					/// check the difference of Age, if the difference > MaxAgeDiff, the lower is newer
					uint32_t diff1 = age - localage;
					uint32_t diff2 = localage - age;
					if (diff1 > m_maxAgeDiff)
					{
						return false;
					}
					else if (diff2 > m_maxAgeDiff)
					{return true;}
				}
			}
		}
		return false;

	}

    bool checknewer(LSAHeader lsaheader1, LSAHeader lsaheader2)
	{
    	/// if lsaheader1 > lsaheader2, return true
		/// compare LS_sequence
		uint32_t localSeq = lsaheader2.GetLSSequence();
		uint32_t Seq = lsaheader1.GetLSSequence();
		if(Seq > localSeq)
		{
			return true;
		}

		else if(Seq < localSeq)
		{
			return false;
		}

		/// if LS_sequence equal, check checksum
		else if(Seq == localSeq)
		{
			uint8_t checksum = lsaheader1.GetCheckSum();
			uint8_t localchecksum = lsaheader2.GetCheckSum();
			if(checksum > localchecksum)
			{
				return true;
			}
			else if (checksum < localchecksum)
			{
				return false;
			}
			else if (checksum == localchecksum)
			{
				/// if the Age of lsa is maxage, it is higher
				uint32_t age = lsaheader1.GetLSAge();
				uint32_t localage = lsaheader2.GetLSAge();
				if (age == m_maxAge && localage != m_maxAge)
				{
					return true;
				}
				else if (localage == m_maxAge && age != m_maxAge)
				{
					return false;
				}
				else if((localage == m_maxAge && age == m_maxAge) || (localage != m_maxAge && age != m_maxAge))
				{
					/// check the difference of Age, if the difference > MaxAgeDiff, the lower is newer todo
					uint32_t diff1 = age - localage;
					uint32_t diff2 = localage - age;
					if (diff1 > m_maxAgeDiff)
					{
						return false;
					}
					else if (diff2 > m_maxAgeDiff)
					{
						return true;
					}
				}
			}
		}
		return false;

	}

    /// if the lsa older than local lsa
    bool checkolder(std::pair<LSAHeader,LSAPacket> lsa)
    {
    	//UpdateAge();
    	OSPFLinkStateIdentifier locallsaiden(lsa.first.GetLSAType(),lsa.first.GetLinkStateID(),lsa.first.GetAdvertisiongRouter());
    	std::pair<LSAHeader,LSAPacket> locallsa = m_db[locallsaiden];
    	/// compare LS_sequence
        uint32_t Seq = lsa.first.GetLSSequence();
        uint32_t localSeq = locallsa.first.GetLSSequence();
        if(Seq > localSeq)
        {
        	return false;
        }

        else if(Seq < localSeq)
        {
    	return true;
        }

        /// if LS_sequence equal, check checksum
        else if(Seq == localSeq)
        {
        	uint8_t checksum = lsa.first.GetCheckSum();
        	uint8_t localchecksum = locallsa.first.GetCheckSum();
        	if(checksum > localchecksum)
        		{return false;}
        	else if (checksum < localchecksum)
				{
        		return true;
				}
        	else if (checksum == localchecksum)
        	{
        		/// if the Age of lsa is maxage, it is higher
        		uint32_t age = lsa.first.GetLSAge();
        		uint32_t localage = locallsa.first.GetLSAge();
        		if (age == m_maxAge && localage != m_maxAge)
        		{
        			return false;
        		}
        		else if (localage == m_maxAge && age != m_maxAge)
        		{
        			return true;
        		}
        		else if((localage == m_maxAge && age == m_maxAge) ||(localage != m_maxAge && age != m_maxAge))
        		{
        			/// check the difference of Age, if the difference > MaxAgeDiff, the lower is newer
        			uint32_t diff1 = age - localage;
        			uint32_t diff2 = localage - age;
        			if (diff1 > m_maxAgeDiff)
        			{
        				return true;
        			}
        			else if (diff2 > m_maxAgeDiff)
        			{return false;}
        		}
        	}
		}
        return false;

    }

    void UpdateAge()
    {

    	//std::cout<<Simulator::Now().GetSeconds()<<std::endl;
        std::unordered_map<OSPFLinkStateIdentifier, std::pair<LSAHeader,LSAPacket>, hash_ospfIdt, equal_ospfIdt>::iterator iter = m_db.begin();
        while(iter != m_db.end())
        {

        	if(iter->second.first.GetLSAge() == m_maxAge){
        		continue;
        	}
        	uint32_t newage = CalcAge(iter->first);
        	iter->second.first.SetLSAge(newage);
            iter++;
        }
    }


    uint32_t GetMaxAge(){

    	return m_maxAge;

   }

	void SetNodeNum(uint32_t satelliteNum, uint32_t gsNum){
		m_satelliteNum = satelliteNum;
		m_gsNum = gsNum;
	}

//    ns3::Time GetMinLSArrival()
//   {
//   	return Minarri;
//   }
//    bool GetCalculateRoutingTable()
//    {
//    	return CalculateRoutingTable;
//    }

    // if the lsa in the databse is sent, use this function after sendlsu
//    void UpdateSendTime(OSPFLinkStateIdentifier id)
//    {
//    	m_addedTime[id] = ns3::Simulator::Now();
//    }
//
//    bool HigherThanMinArrivalTimeForSendingLSU(OSPFLinkStateIdentifier id)
//    {
//    	return((ns3::Simulator::Now()-m_addedTime[id]) > Minarri);
//    }
//
//    void oldenlsa(OSPFLinkStateIdentifier id)
//    {
//    	this->Get(id).first.SetLSAge(g_maxAge);
//    }

/*    bool IsElapsedMinLsArrival(OSPFLinkStateIdentifier id) {
        return m_addedTime[id] + g_minLsArrival <= Now();
    }*/

/*    std::vector<pair<LSAHeader,LSAPakcet> > Aggregate(std::set<OSPFLinkStateIdentifier> id_set) {
        std::vector<pair<LSAHeader,LSAPakcet> > ret;
        for (const OSPFLinkStateIdentifier& id : id_set) {
            ret.push_back(Get(id));
        }
        return ret;
    }*/

/*    void GetSummary (std::vector<Ptr<OSPFLSAHeader> >& summary, std::vector<Ptr<OSPFLSA> >& rxmt) {
        for (auto& kv : m_db) {
            if (kv.second->GetHeader()->IsASScope()) continue;
            summary.push_back(kv.second->GetHeader());
            if (DetectMaxAge(kv.first)) {
                rxmt.push_back(kv.second);
            }
        }*/
    };


}
}

#endif
