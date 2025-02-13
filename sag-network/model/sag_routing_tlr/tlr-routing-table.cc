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
 * Author: 
 */

#include "tlr-routing-table.h"


namespace ns3 {

	NS_LOG_COMPONENT_DEFINE("TLRRoutingTable");

	TypeId
		TLRRoutingTable::GetTypeId(void)
	{
		static TypeId tid = TypeId("ns3::TLRRoutingTable")
			.SetParent<Object>()
			.SetGroupName("Internet")
			.AddConstructor<TLRRoutingTable>()
			;
		return tid;
	}

	/*
	 The Routing Table
	 */

	TLRRoutingTable::TLRRoutingTable()
	{

	}

	TLRRoutingTable::~TLRRoutingTable()
	{

	}

	bool
		TLRRoutingTable::DeleteRoute(Ipv4Address dst)
	{
		NS_LOG_FUNCTION(this << dst);
		if (m_ipv4AddressEntry.erase(dst) != 0)
		{
			NS_LOG_LOGIC("Route deletion to " << dst << " successful");
			return true;
		}
		NS_LOG_LOGIC("Route deletion to " << dst << " not successful");
		return false;
	}

	bool
		TLRRoutingTable::AddRoute(TLRRoutingTableEntry& rt)
	{
		NS_LOG_FUNCTION(this);

		auto it = m_ipv4AddressEntry.find(rt.GetDestination());
		if (it != m_ipv4AddressEntry.end()) {
			//delete it->second;
			//it->second = 0;
			m_ipv4AddressEntry.erase(it);
		}

		std::pair<std::map<Ipv4Address, TLRRoutingTableEntry>::iterator, bool> result =
			m_ipv4AddressEntry.insert(std::make_pair(rt.GetDestination(), rt));
		return result.second;
	}

    bool
	    TLRRoutingTable::AddInferfaceForNode(uint32_t dstNode,Ipv4Address interface)
    {
    	auto it = m_RecordInterface.find(dstNode);
    	if (it != m_RecordInterface.end()) m_RecordInterface.erase(it);
    	m_RecordInterface[dstNode] = interface;
    	return true;
    }

    bool
	    TLRRoutingTable::LookupInterfaceFromNode(uint32_t curNode,Ipv4Address & dst)
    {
    	if (m_RecordInterface.empty()) return false;
    	std::map<uint32_t,Ipv4Address>::const_iterator i= m_RecordInterface.find(curNode);
    	if (i == m_RecordInterface.end() )return false;
    	dst = m_RecordInterface[curNode];
    	return true;
    }

	bool
		TLRRoutingTable::LookupRoute(Ipv4Address dst, TLRRoutingTableEntry& rt)
	{
		NS_LOG_FUNCTION(this << dst);
		if (m_ipv4AddressEntry.empty())
		{
			NS_LOG_LOGIC("Route to " << dst << " not found; m_ipv4AddressEntry is empty");
			return false;
		}
		std::map<Ipv4Address, TLRRoutingTableEntry>::const_iterator i =
			m_ipv4AddressEntry.find(dst);
		if (i == m_ipv4AddressEntry.end())
		{
			NS_LOG_LOGIC("Route to " << dst << " not found");
			return false;
		}
		rt = i->second;
		NS_LOG_LOGIC("Route to " << dst << " found");
		return true;
	}

	bool
		TLRRoutingTable::Update(TLRRoutingTableEntry& rt)
	{
		NS_LOG_FUNCTION(this);
		std::map<Ipv4Address, TLRRoutingTableEntry>::iterator i =
			m_ipv4AddressEntry.find(rt.GetDestination());
		if (i == m_ipv4AddressEntry.end())
		{
			NS_LOG_LOGIC("Route update to " << rt.GetDestination() << " fails; not found");
			return false;
		}
		i->second = rt;
		return true;
	}

}
