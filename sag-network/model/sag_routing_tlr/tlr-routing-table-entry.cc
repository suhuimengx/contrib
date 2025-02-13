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

#include "tlr-routing-table-entry.h"

namespace ns3 {

	NS_LOG_COMPONENT_DEFINE("TLRRoutingTableEntry");

	TypeId
		TLRRoutingTableEntry::GetTypeId(void)
	{
		static TypeId tid = TypeId("ns3::TLRRoutingTableEntry")
			.SetParent<Object>()
			.SetGroupName("Internet")
			.AddConstructor<TLRRoutingTableEntry>()
			;
		return tid;
	}

	/*
	 The Routing Table
	 */

	TLRRoutingTableEntry::TLRRoutingTableEntry(Ptr<NetDevice> dev, Ipv4Address dst, Ipv4InterfaceAddress iface, Ipv4Address nextHop,uint32_t nextHopid,
			                                   Ptr<NetDevice> secdev, Ipv4Address secdst, Ipv4InterfaceAddress seciface, Ipv4Address secondnextHop,uint32_t secnextHopid)
		: m_iface(iface),
		  m_nextHopid(nextHopid),
		  m_secdev(secdev),
		  m_secdst(secdst),//dst==secdst
		  m_seciface(seciface),
		  m_secondnextHop(secondnextHop),
		  m_secnextHopid(secnextHopid)
	{
		m_ipv4Route = Create<Ipv4Route>();
		m_ipv4Route->SetDestination(dst);
		m_ipv4Route->SetGateway(nextHop);
		m_ipv4Route->SetSource(m_iface.GetLocal());
		m_ipv4Route->SetOutputDevice(dev);
	}

	TLRRoutingTableEntry::~TLRRoutingTableEntry()
	{

	}


}
