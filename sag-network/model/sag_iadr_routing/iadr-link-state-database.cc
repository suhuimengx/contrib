//
// Created by Mengy on 2023/1/30.
//

#include "iadr-link-state-database.h"


namespace ns3 {
namespace iadr {


void IADRLSDB::Add(std::pair<LSAHeader,LSAPacket> lsa) {
	UpdateAge();
	IADRLinkStateIdentifier id(lsa.first.GetLSAType(),lsa.first.GetLinkStateID(),lsa.first.GetAdvertisiongRouter());
	if(this->Has(id)){
		std::pair<LSAHeader,LSAPacket> oldlsa = this->Get(id);
		m_calculateRoutingTable = NeedToCalRoutingTable(oldlsa,lsa);
		this->Remove(id);
		m_db[id] = lsa;
		//m_addedTime[id] = Simulator::Now();
		m_LsAddedTime[id] = lsa.first.GetLSAddedTime();
		if(lsa.first.GetLSAge() == m_maxAge){
			m_addedAge[id] = m_maxAge;
		}
		else{
			//std::vector<uint32_t> ag((uint32_t)((Simulator::Now() - Seconds(lsa.first.GetLSAddedTime())).GetSeconds()), (uint32_t)MAXAGE);
			m_addedAge[id] = std::min((uint32_t)((Simulator::Now() - Seconds(lsa.first.GetLSAddedTime())).GetSeconds()), m_maxAge);
		}
	}
	else{
		m_calculateRoutingTable = true;
		m_db[id] = lsa;
		//m_addedTime[id] = Simulator::Now();
		m_LsAddedTime[id] = lsa.first.GetLSAddedTime();
		if(lsa.first.GetLSAge() == m_maxAge){
			m_addedAge[id] = m_maxAge;
		}
		else{
			//std::vector<uint32_t> ag((uint32_t)((Simulator::Now() - Seconds(lsa.first.GetLSAddedTime())).GetSeconds()), (uint32_t)MAXAGE);
			m_addedAge[id] = std::min((uint32_t)((Simulator::Now() - Seconds(lsa.first.GetLSAddedTime())).GetSeconds()), m_maxAge);
		}
	}
	UpdateAge();

	// arrange aging event
	Time agingEventSchedule = Seconds(m_maxAge) - Seconds(m_addedAge.begin()->second);
	IADRLinkStateIdentifier IDForAgingEvent = m_addedAge.begin()->first;
	for(auto lsa : m_db){
		if((Seconds(m_maxAge) - Seconds(m_addedAge[lsa.first])) < agingEventSchedule){
			agingEventSchedule = Seconds(m_maxAge) - Seconds(m_addedAge[lsa.first]);
			IDForAgingEvent = lsa.first;
		}
	}
	m_LSAAgingEvent.Cancel();
	m_LSAAgingEvent = Simulator::Schedule(agingEventSchedule, &IADRLSDB::LSAAgingEvent, this, IDForAgingEvent);

	// route calculation
	//        if(lastLsaInLsu && m_calculateRoutingTable){
	//        	m_routerBuildCallBack(&m_db, m_routerId, m_interfaces);
	//        }


}

void IADRLSDB::UpdateRoute(Time routcalInterval){
	//m_calculateRoutingTable=true;

	m_routerBuildCallBack(&m_db, m_routerId, m_interfaces, routcalInterval);

}



}
}
