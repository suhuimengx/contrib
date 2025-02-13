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

#include "ospf-link-state-database.h"


namespace ns3 {
namespace ospf {


void OSPFLSDB::Add(std::pair<LSAHeader,LSAPacket> lsa) {
	//UpdateAge();
	OSPFLinkStateIdentifier id(lsa.first.GetLSAType(),lsa.first.GetLinkStateID(),lsa.first.GetAdvertisiongRouter());
	if(this->Has(id)){
		//std::pair<LSAHeader,LSAPacket> oldlsa = this->Get(id);
		//m_calculateRoutingTable = NeedToCalRoutingTable(oldlsa,lsa);
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
	m_addLSAListNow.push_back(lsa);


	// cancel noting if need aging todo{
//	UpdateAge();
//
//	// arrange aging event
//	Time agingEventSchedule = Seconds(m_maxAge) - Seconds(m_addedAge.begin()->second);
//	OSPFLinkStateIdentifier IDForAgingEvent = m_addedAge.begin()->first;
//	for(auto lsa : m_db){
//		if((Seconds(m_maxAge) - Seconds(m_addedAge[lsa.first])) < agingEventSchedule){
//			agingEventSchedule = Seconds(m_maxAge) - Seconds(m_addedAge[lsa.first]);
//			IDForAgingEvent = lsa.first;
//		}
//	}
//	m_LSAAgingEvent.Cancel();
//	m_LSAAgingEvent = Simulator::Schedule(agingEventSchedule, &OSPFLSDB::LSAAgingEvent, this, IDForAgingEvent);
//}


}

void OSPFLSDB::UpdateRoute(){

	m_routerBuildCallBack(&m_db, m_addLSAListNow, m_routerId, m_interfaces);
	m_addLSAListNow.clear();

}



}
}
