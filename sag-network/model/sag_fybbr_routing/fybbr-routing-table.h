/*
 * fybbr-routing-table.h
 *
 *  Created on: 2023年2月1日
 *      Author: xiaoyu
 */

#ifndef FYBBR_ROUTING_TABLE_H
#define FYBBR_ROUTING_TABLE_H

#include "ns3/log.h"
#include "ns3/ptr.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-routing-table-entry.h"
#include <vector>
#include "ns3/ipv4.h"
#include "ns3/ipv4-route.h"


namespace ns3 {
namespace fybbr {

/*//class OspfRoutingTableEntry : public Object
//{
//public:
//	static TypeId GetTypeId ();
//	OspfRoutingTableEntry (Ipv4Address dst = Ipv4Address (),uint16_t costs = 65535,
//					 Ipv4Address nextHop = Ipv4Address (),Ipv4InterfaceAddress iface = Ipv4InterfaceAddress());
//	~OspfRoutingTableEntry ();
//};*/

class FybbrRoutingTable: public Object
{
public:
	static TypeId GetTypeId ();
    //constructor
    FybbrRoutingTable();
    ~FybbrRoutingTable();
    bool AddRoute (Ptr<NetDevice> dev = 0,Ipv4Address dst = Ipv4Address (),
    		Ipv4InterfaceAddress iface = Ipv4InterfaceAddress (), Ipv4Address nextHop = Ipv4Address ());
    Ptr<Ipv4Route> LookupRoute (Ipv4Address dst);
    void Clear();

private:
  // The routing table
  std::map<Ipv4Address, Ptr<Ipv4Route>> m_entries;

};



}  // namespace fybbr
}  // namespace ns3

#endif /* FYBBR_RTABLE_H */
