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

#ifndef SAG_ROUTING_TABLE_H
#define SAG_ROUTING_TABLE_H


#include "ns3/log.h"
#include "ns3/ptr.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-routing-table-entry.h"
#include <vector>
#include "ns3/ipv4.h"
#include "ns3/ipv4-route.h"
#include "sag_routing_table_entry.h"

namespace ns3 {

class SAGRoutingTable : public Object
{
public:
	static TypeId GetTypeId (void);
	/**
	* constructor
	* \param t the routing table entry lifetime
	*/
	SAGRoutingTable ();
	~SAGRoutingTable ();

	/**
	* Add routing table entry if it doesn't yet exist in routing table
	* \param r routing table entry
	* \return true in success
	*/
	virtual bool AddRoute (SAGRoutingTableEntry & r);
	/**
	* Delete routing table entry with destination address dst, if it exists.
	* \param dst destination address
	* \return true on success
	*/
	virtual bool DeleteRoute (Ipv4Address dst);
	/**
	* Lookup routing table entry with destination address dst
	* \param dst destination address
	* \param rt entry with destination address dst, if exists
	* \return true on success
	*/
	virtual bool LookupRoute (Ipv4Address dst, SAGRoutingTableEntry & rt);
	/**
	* Update routing table
	* \param rt entry with destination address dst, if exists
	* \return true on success
	*/
	virtual bool Update (SAGRoutingTableEntry & rt);

	/// Delete all entries from routing table
	void Clear ()
	{
		m_ipv4AddressEntry.clear ();
	}

	uint32_t GetNRoute();

	SAGRoutingTableEntry GetRoute (uint32_t index) const;

private:
  /// The routing table
  std::map<Ipv4Address, SAGRoutingTableEntry> m_ipv4AddressEntry;

};

}

#endif /* SAG_ROUTING_TABLE_H */
