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

#include "mplb_routing_table_entry.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SAGRoutingTableEntry");

TypeId
SAGRoutingTableEntry::GetTypeId(void)
{
	static TypeId tid = TypeId("ns3::SAGRoutingTableEntry")
			.SetParent<Object>()
			.SetGroupName("Internet")
			.AddConstructor<SAGRoutingTableEntry>()
			;
	return tid;
}

/*
 The Routing Table
 */

SAGRoutingTableEntry::SAGRoutingTableEntry (Ptr<NetDevice> dev, Ipv4Address dst, Ipv4InterfaceAddress iface, Ipv4Address nextHop)
  : m_iface (iface)
{
  m_ipv4Route = Create<Ipv4Route> ();
  m_ipv4Route->SetDestination (dst);
  m_ipv4Route->SetGateway (nextHop);
  m_ipv4Route->SetSource (m_iface.GetLocal ());
  m_ipv4Route->SetOutputDevice (dev);
}

SAGRoutingTableEntry::~SAGRoutingTableEntry ()
{

}


}
