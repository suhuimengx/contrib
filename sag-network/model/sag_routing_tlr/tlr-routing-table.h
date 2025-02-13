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

#ifndef TLR_ROUTING_TABLE_H
#define TLR_ROUTING_TABLE_H


#include "ns3/log.h"
#include "ns3/ptr.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-routing-table-entry.h"
#include <vector>
#include "ns3/ipv4.h"
#include "ns3/ipv4-route.h"
#include "tlr-routing-table-entry.h"

namespace ns3 {

	class TLRRoutingTable : public Object
	{
	public:
		static TypeId GetTypeId(void);
		/**
		* constructor
		* \param t the routing table entry lifetime
		*/
		TLRRoutingTable();
		~TLRRoutingTable();

		/**
		* Add routing table entry if it doesn't yet exist in routing table
		* \param r routing table entry
		* \return true in success
		*/
		virtual bool AddRoute(TLRRoutingTableEntry& r);
		/**
		* Delete routing table entry with destination address dst, if it exists.
		* \param dst destination address
		* \return true on success
		*/
		virtual bool DeleteRoute(Ipv4Address dst);
		/**
		* Lookup routing table entry with destination address dst
		* \param dst destination address
		* \param rt entry with destination address dst, if exists
		* \return true on success
		*/
		virtual bool LookupRoute(Ipv4Address dst, TLRRoutingTableEntry& rt);
		/**
		* Update routing table
		* \param rt entry with destination address dst, if exists
		* \return true on success
		*/
		virtual bool Update(TLRRoutingTableEntry& rt);

		/// Delete all entries from routing table
		void Clear()
		{
			m_ipv4AddressEntry.clear();
			m_RecordInterface.clear();
		}

		bool AddInferfaceForNode(uint32_t dstNode,Ipv4Address interface);

		bool LookupInterfaceFromNode(uint32_t curNode,Ipv4Address & dst);



	private:
		/// The routing table
		std::map<Ipv4Address, TLRRoutingTableEntry> m_ipv4AddressEntry;
        std::map<uint32_t,Ipv4Address> m_RecordInterface;
	};

}

#endif /*TLR_ROUTING_TABLE_H */
