
#include "fybbr-routing-table.h"


namespace ns3 {


namespace fybbr {

/*
//TypeId
//OspfRoutingTableEntry::GetTypeId ()
//{
//  static TypeId tid = TypeId ("ns3::ospf::OspfRoutingTableEntry")
//    .SetParent<Object> ()
//    .SetGroupName ("OSPF")
//    .AddConstructor<OspfRoutingTableEntry> ()
//  ;
//  return tid;
//}
//OspfRoutingTableEntry::OspfRoutingTableEntry (Ipv4Address dst, uint16_t costs, Ipv4Address nextHop, Ipv4InterfaceAddress iface){}
//OspfRoutingTableEntry::~OspfRoutingTableEntry (){}
*/

TypeId
FybbrRoutingTable::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::fybbr::FybbrRoutingTable")
    .SetParent<Object> ()
    .SetGroupName ("FYBBR")
    .AddConstructor<FybbrRoutingTable> ()
  ;
  return tid;
}
FybbrRoutingTable::FybbrRoutingTable() {}
FybbrRoutingTable::~FybbrRoutingTable() {}

bool
FybbrRoutingTable::AddRoute (Ptr<NetDevice> dev, Ipv4Address dst, Ipv4InterfaceAddress iface, Ipv4Address nextHop){

	auto it = m_entries.find(dst);
	if(it != m_entries.end()){
		//delete it->second;
		//it->second = 0;
		m_entries.erase(it);
	}

	Ptr<Ipv4Route> ipv4Route = Create<Ipv4Route> ();
	ipv4Route->SetDestination (dst);
	ipv4Route->SetGateway (nextHop);
	ipv4Route->SetSource (iface.GetLocal ());
	ipv4Route->SetOutputDevice (dev);
	m_entries.insert(std::make_pair(dst, ipv4Route));

	return 1;

}

Ptr<Ipv4Route>
FybbrRoutingTable::LookupRoute (Ipv4Address dst){
	if(m_entries.find(dst) != m_entries.end()){
		return m_entries[dst];
	}
	else{
		return 0;
	}

}

void
FybbrRoutingTable::Clear(){
//	for(auto en : m_entries){
//		//delete en.second;
//		//en.second = 0;
//	}
	m_entries.clear();
}





}

}
