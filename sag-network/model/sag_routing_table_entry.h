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

#ifndef SAG_ROUTING_TABLE_ENTRY_H
#define SAG_ROUTING_TABLE_ENTRY_H

#include "ns3/log.h"
#include "ns3/ptr.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/ipv4-address.h"
#include <vector>
#include "ns3/ipv4.h"
#include "ns3/ipv4-route.h"

namespace ns3 {

class SAGRoutingTableEntry : public Object
{
public:
	static TypeId GetTypeId (void);
	/**
	* constructor
	*
	* \param dev the output device
	* \param dst the destination IP address
	* \param iface the output interface address
	* \param nextHop the IP address of the next hop
	*/
	SAGRoutingTableEntry (
		  Ptr<NetDevice> dev = 0,
		  Ipv4Address dst = Ipv4Address (),
		  Ipv4InterfaceAddress iface = Ipv4InterfaceAddress (),
		  Ipv4Address nextHop = Ipv4Address ()
		  );

	~SAGRoutingTableEntry ();


	// Fields
	/**
	* Get destination address function
	* \returns the IPv4 destination address
	*/
	Ipv4Address GetDestination () const
	{
		return m_ipv4Route->GetDestination ();
	}
	/**
	* Get route function
	* \returns The IPv4 route
	*/
	Ptr<Ipv4Route> GetRoute () const
	{
		return m_ipv4Route;
	}
	/**
	* Set route function
	* \param r the IPv4 route
	*/
	void SetRoute (Ptr<Ipv4Route> r)
	{
		m_ipv4Route = r;
	}
	/**
	* Set next hop address
	* \param nextHop the next hop IPv4 address
	*/
	void SetNextHop (Ipv4Address nextHop)
	{
		m_ipv4Route->SetGateway (nextHop);
	}
	/**
	* Get next hop address
	* \returns the next hop address
	*/
	Ipv4Address GetNextHop () const
	{
		return m_ipv4Route->GetGateway ();
	}
	/**
	* Set output device
	* \param dev The output device
	*/
	void SetOutputDevice (Ptr<NetDevice> dev)
	{
		m_ipv4Route->SetOutputDevice (dev);
	}
	/**
	* Get output device
	* \returns the output device
	*/
	Ptr<NetDevice> GetOutputDevice () const
	{
		return m_ipv4Route->GetOutputDevice ();
	}
	/**
	* Get the Ipv4InterfaceAddress
	* \returns the Ipv4InterfaceAddress
	*/
	Ipv4InterfaceAddress GetInterface () const
	{
		return m_iface;
	}
	/**
	* Set the Ipv4InterfaceAddress
	* \param iface The Ipv4InterfaceAddress
	*/
	void SetInterface (Ipv4InterfaceAddress iface)
	{
		m_iface = iface;
	}

	/**
	* \brief Compare destination address
	* \param dst IP address to compare
	* \return true if equal
	*/
	bool operator== (Ipv4Address const  dst) const
	{
		return (m_ipv4Route->GetDestination () == dst);
	}


private:

	/** Ip route, include
	*   - destination address
	*   - source address
	*   - next hop address (gateway)
	*   - output device
	*/
	Ptr<Ipv4Route> m_ipv4Route;
	/// Output interface address
	Ipv4InterfaceAddress m_iface;

};


}


#endif /* SAG_ROUTING_TABLE_ENTRY_H */
