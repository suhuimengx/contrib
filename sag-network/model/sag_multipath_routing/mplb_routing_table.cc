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

#include "mplb_routing_table.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SAGRoutingTable");

TypeId
SAGRoutingTable::GetTypeId(void)
{
	static TypeId tid = TypeId("ns3::SAGRoutingTable")
			.SetParent<Object>()
			.SetGroupName("Internet")
			.AddConstructor<SAGRoutingTable>()
			;
	return tid;
}

/*
 The Routing Table
 */

SAGRoutingTable::SAGRoutingTable ()
{

}

SAGRoutingTable::~SAGRoutingTable ()
{

}

bool
SAGRoutingTable::DeleteRoute (Ipv4Address dst)
{
	NS_LOG_FUNCTION (this << dst);
	if (m_ipv4AddressEntry.erase (dst) != 0)
	{
	  NS_LOG_LOGIC ("Route deletion to " << dst << " successful");
	  return true;
	}
	NS_LOG_LOGIC ("Route deletion to " << dst << " not successful");
	return false;
}

bool
SAGRoutingTable::AddRoute (SAGRoutingTableEntry & rt)
{
	NS_LOG_FUNCTION (this);

	auto it = m_ipv4AddressEntry.find(rt.GetDestination ());
	if(it != m_ipv4AddressEntry.end()){
		//delete it->second;
		//it->second = 0;
		m_ipv4AddressEntry.erase(it);
	}

	std::pair<std::map<Ipv4Address, SAGRoutingTableEntry>::iterator, bool> result =
	m_ipv4AddressEntry.insert (std::make_pair (rt.GetDestination (), rt));
	return result.second;
}

bool
SAGRoutingTable::LookupRoute (Ipv4Address dst, SAGRoutingTableEntry & rt)
{
	NS_LOG_FUNCTION (this << dst);
	if (m_ipv4AddressEntry.empty ())
	{
		NS_LOG_LOGIC ("Route to " << dst << " not found; m_ipv4AddressEntry is empty");
		return false;
	}
	std::map<Ipv4Address, SAGRoutingTableEntry>::const_iterator i =
	m_ipv4AddressEntry.find (dst);
	if (i == m_ipv4AddressEntry.end ())
	{
		NS_LOG_LOGIC ("Route to " << dst << " not found");
		return false;
	}
	rt = i->second;
	NS_LOG_LOGIC ("Route to " << dst << " found");
	return true;
}

bool
SAGRoutingTable::Update (SAGRoutingTableEntry & rt)
{
	NS_LOG_FUNCTION (this);
	std::map<Ipv4Address, SAGRoutingTableEntry>::iterator i =
	m_ipv4AddressEntry.find (rt.GetDestination ());
	if (i == m_ipv4AddressEntry.end ())
	{
		NS_LOG_LOGIC ("Route update to " << rt.GetDestination () << " fails; not found");
		return false;
	}
	i->second = rt;
	return true;
}

}
