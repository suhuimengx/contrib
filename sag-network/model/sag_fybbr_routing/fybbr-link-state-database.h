#ifndef FYBBR_LSDB_H
#define FYBBR_LSDB_H

#include "ns3/nstime.h"
#include <set>
#include <map>
#include <algorithm>
#include <utility>
#include "fybbr-lsa-identifier.h"
#include "ns3/simulator.h"
#include <vector>

#include "ns3/fybbr_link_state_packet.h"
#include "centralized_packet.h"

#include "ns3/ipv4-address.h"
#include "ns3/simulator.h"
#include "ns3/timer.h"


class LSAHeader;
class LSAPacket;
class FYBBRLSDB;
struct FYBBRLinkStateIdentifier;

namespace ns3 {
namespace fybbr {



class FYBBRLSDB {
    std::map<FYBBRLinkStateIdentifier, std::pair<LSAHeader,LSAPacket>> m_db; // LSA DataBase
    //std::map<FYBBRLinkStateIdentifier, Time> m_addedTime; // LSA added time record
    std::map<FYBBRLinkStateIdentifier, uint32_t> m_addedAge; // LSA Age for the added time
    std::map<FYBBRLinkStateIdentifier, uint32_t> m_LsAddedTime; // LSA Generated time
    //xhqin
    //std::map<FYBBRLinkStateIdentifier, uint32_t> m_forwardedDD;//DD that has been forwarded
    //std::vector<std::pair<LSAHeader,LSAPacket>> m_forwardedDD;//DD that has been forwarded
    std::vector<LSAHeader> m_forwardedDD;//DD that has been forwarded
    //std::vector<std::pair<RTSHeader,RTSPacket>> m_forwardedRTS;//RTS that has been forwarded
    std::vector<RTSHeader> m_forwardedRTS;//RTS that has been forwarded

    uint32_t m_maxAge = 360;
    uint32_t m_maxPathAge=360;
    uint32_t m_maxAgeDiff = 18;
    //Time Minarri = Seconds(20);
    bool m_calculateRoutingTable = false;

    Callback<void, std::map<FYBBRLinkStateIdentifier, std::pair<LSAHeader,LSAPacket>>*, Ipv4Address, std::map<Ipv4Address, std::pair<Ipv4Address, Ipv4Address>>, Time> m_routerBuildCallBack;
    Callback<void, std::vector<std::pair<LSAHeader,LSAPacket>>, Ipv4Address, Ipv4Address> m_floodCallBack;
    Ipv4Address m_routerId;
    std::map<Ipv4Address, std::pair<Ipv4Address, Ipv4Address>> m_interfaces;

    // When an attempt is made to increment the sequence number past the maximum value of N - 1 (0x7fffffff;
    // also referred to as MaxSequenceNumber), the current instance of the LSA must first be flushed from the routing domain. todo
    // This is done by prematurely aging the LSA (see Section 14.1) and reflooding it. As soon as this flood has been acknowledged
    // by all adjacent neighbors, a new instance can be originated with sequence number of InitialSequenceNumber.
    uint32_t m_seqForSelfOriginatedLSA = 0;
    uint32_t m_seqForSelfOriginatedRTS = 0;
    /// LSA Aging Event
    EventId m_LSAAgingEvent;


public:
    FYBBRLSDB() {}
    // erase DB
    ~FYBBRLSDB() {}

    std::map<FYBBRLinkStateIdentifier, std::pair<LSAHeader,LSAPacket>> GetDataBase () {
        return m_db;
    }

    bool GetcalculateRoutingTable(){
       	return m_calculateRoutingTable;
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

	//xhqin
  /*  std::vector<std::pair<LSAHeader,LSAPacket>> GetForwardedDD (){
        return m_forwardedDD;
    }*/
    /*std::vector<std::pair<RTSHeader,RTSPacket>> GetForwardedRTS (){
        return m_forwardedRTS;
    }*/

    void SetRouterBuildCallback (Callback<void, std::map<FYBBRLinkStateIdentifier, std::pair<LSAHeader,LSAPacket>>*, Ipv4Address, std::map<Ipv4Address, std::pair<Ipv4Address, Ipv4Address>>, Time> cb)
	{
    	m_routerBuildCallBack = cb;
	}

    void SetFloodCallback (Callback<void, std::vector<std::pair<LSAHeader,LSAPacket>>, Ipv4Address, Ipv4Address> cb)
	{
		m_floodCallBack = cb;
	}

    uint32_t UpdateCurrentPacketAge(uint32_t age, uint32_t addedTime){

		return std::min(((uint32_t)(Simulator::Now() - Seconds(addedTime)).GetSeconds()), m_maxAge);

    }

    void SetInterfaceAddress(std::map<Ipv4Address, std::pair<Ipv4Address, Ipv4Address>> ads){
    	m_interfaces = ads;
    }

   /* std::map<Ipv4Address, std::pair<Ipv4Address, Ipv4Address>> GetInterfaceAddress(){
       	return m_interfaces;
    }*/

    void Add(std::pair<LSAHeader,LSAPacket> lsa);

    void UpdateRoute(Time routcalInterval);

    void LSAAgingEvent(FYBBRLinkStateIdentifier IDForAgingEvent){
    	UpdateAge();
    	if(m_db.find(IDForAgingEvent) == m_db.end()){
    		throw std::runtime_error("Process Wrong: FYBBRLSDB::LSAAgingEvent");
    	}
    	else{
    		if(m_db[IDForAgingEvent].first.GetLSAge() != m_maxAge){
    			throw std::runtime_error("Process Wrong: FYBBRLSDB::LSAAgingEvent");
    		}
    	}
    	m_floodCallBack({m_db[IDForAgingEvent]}, Ipv4Address::GetAny(), Ipv4Address::GetAny());
    }

    // whether router's age exceed max age
    bool DetectMaxAge(FYBBRLinkStateIdentifier id) {
        return m_db[id].first.GetLSAge() == m_maxAge;
    }

    bool DetectMaxSeq(FYBBRLinkStateIdentifier id) {
		return m_db[id].first.GetLSSequence() == UINT32_MAX;
	}

    // check if the database has this router id
    bool Has(FYBBRLinkStateIdentifier id) {
        //return m_db.count(id);
        if(m_db.find(id) == m_db.end()){
        	return false;
        }
        else{
        	return true;
        }
    }
    bool Has(LSAHeader tempHeader) {
        FYBBRLinkStateIdentifier id(tempHeader.GetLSAType(),tempHeader.GetLinkStateID(),tempHeader.GetAdvertisiongRouter());
		return Has(id);
	}

	// xhqin check if the database has this router id with the complete packet
    bool Has(std::pair<LSAHeader,LSAPacket> lsa) {
        FYBBRLinkStateIdentifier locallsaiden(lsa.first.GetLSAType(),lsa.first.GetLinkStateID(),lsa.first.GetAdvertisiongRouter());
        if(m_db.find(locallsaiden) == m_db.end()){
			return false;
		}
		else{
			return true;
		}
        //Has(locallsaiden);
    }
    // calculate lsa age for now
    uint32_t CalcAge(const FYBBRLinkStateIdentifier& id) {
    	uint32_t age = (uint32_t)((Simulator::Now() - Seconds(m_LsAddedTime[id])).GetSeconds());
		if(age >= m_maxAge){
			return m_maxAge;
		}
		else{
			return age;
		}
    }

    // Get pair<LSAHeader,LSAPacket> for the router id
    std::pair<LSAHeader,LSAPacket> Get(const FYBBRLinkStateIdentifier& id) {
        m_db[id].first.SetLSAge(std::min(CalcAge(id), m_maxAge));
        return m_db[id];
    }

    std::pair<LSAHeader,LSAPacket> Get(LSAHeader tempHeader) {
        FYBBRLinkStateIdentifier id(tempHeader.GetLSAType(),tempHeader.GetLinkStateID(),tempHeader.GetAdvertisiongRouter());
		m_db[id].first.SetLSAge(std::min(CalcAge(id), m_maxAge));
		return m_db[id];
	}

    void Remove(FYBBRLinkStateIdentifier id) {
        m_db.erase(id);
        //m_addedTime.erase(id);
        m_LsAddedTime.erase(id);
        m_addedAge.erase(id);
        //m_forwardedDD.erase(id);
        //m_forwardedRTS.erase(id);

    }




    // xhqin
   	void addforwardedDD(LSAHeader lsa) {
   		for (auto iter = m_forwardedDD.begin();iter!=m_forwardedDD.end();iter++){
			if(*iter==lsa){
				return ;
			}
   		}
   		m_forwardedDD.push_back(lsa);
   	}
   	void DeleteforwardedDD(LSAHeader lsa) {
   		for (auto iter = m_forwardedDD.begin();iter!=m_forwardedDD.end();iter++){
   			if(lsa==*iter){
   				m_forwardedDD.erase(iter);
   				break;
   			}
   		}
   	}
   	bool HasDD(LSAHeader lsa) {
		uint32_t Seq = lsa.GetLSSequence();
		for (auto iter = m_forwardedDD.begin();iter!=m_forwardedDD.end();iter++){

			if(*iter==lsa && iter->GetLSSequence() < Seq){
				DeleteforwardedDD(*iter);
				return false;
			}
			if(*iter==lsa && iter->GetLSSequence() >= Seq){
				return true;
			}

		}
		return false;
   	}

   	void addforwardedRTS(RTSHeader rts) {
		for (auto iter = m_forwardedRTS.begin();iter!=m_forwardedRTS.end();iter++){
			if(*iter==rts){
				return ;
			}
		}
		m_forwardedRTS.push_back(rts);
	}
	void DeleteforwardedRTS(RTSHeader rts) {
		for (auto iter = m_forwardedRTS.begin();iter!=m_forwardedRTS.end();iter++){
			if(rts==*iter){
				m_forwardedRTS.erase(iter);
				break;
			}
		}
	}
	bool HasRTS(RTSHeader rts) {
		uint16_t Seq = rts.GetPathage();
		for (auto iter = m_forwardedRTS.begin();iter!=m_forwardedRTS.end();iter++){

			if(*iter==rts && iter->GetPathage() < Seq){
				DeleteforwardedRTS(*iter);
				return false;
			}
			if(*iter==rts && iter->GetPathage() >= Seq){
				return true;
			}

		}
		return false;
	}


    void Remove(LSAHeader tempHeader) {
    	// todo
    	// An LSA is deleted from a router’s database when either a) it has been overwritten by a newer instance during the flooding process
    	// (Section 13) or b) the router originates a newer instance of one of its self-originated LSAs (Section 12.4) or c) the LSA ages
    	// out and is flushed from the routing domain (Section 14).
    	// Whenever an LSA is deleted from the database it must also be removed from all neighbors’ Link state retransmission lists (see
    	// Section 10).
        FYBBRLinkStateIdentifier id(tempHeader.GetLSAType(),tempHeader.GetLinkStateID(),tempHeader.GetAdvertisiongRouter());
    	Remove(id);
	}

    void Clear() {
    		m_db.clear();
			//m_addedTime.clear();
			m_LsAddedTime.clear();
			m_addedAge.clear();
			m_interfaces.clear();

        	m_forwardedDD.clear();
        	m_forwardedRTS.clear();
        	// route calculation
    		//m_routerBuildCallBack(&m_db, m_routerId, m_interfaces);
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
		UpdateAge();
		FYBBRLinkStateIdentifier locallsaiden(lsa.first.GetLSAType(),lsa.first.GetLinkStateID(),lsa.first.GetAdvertisiongRouter());
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
		FYBBRLinkStateIdentifier locallsaiden(lsaheader.GetLSAType(),lsaheader.GetLinkStateID(),lsaheader.GetAdvertisiongRouter());
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
		UpdateAge();
		FYBBRLinkStateIdentifier locallsaiden(lsa.first.GetLSAType(),lsa.first.GetLinkStateID(),lsa.first.GetAdvertisiongRouter());
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
		std::map<FYBBRLinkStateIdentifier, std::pair<LSAHeader,LSAPacket>>::iterator iter = m_db.begin();
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

  };


}
}

#endif
