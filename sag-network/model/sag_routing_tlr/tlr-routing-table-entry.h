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

#ifndef TLR_ROUTING_TABLE_ENTRY_H
#define TLR_ROUTING_TABLE_ENTRY_H

#include "ns3/log.h"
#include "ns3/ptr.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/ipv4-address.h"
#include <vector>
#include "ns3/ipv4.h"
#include "ns3/ipv4-route.h"

namespace ns3 {

	class TLRRoutingTableEntry : public Object
	{
	public:
		static TypeId GetTypeId(void);
		/**
		* constructor
		*
		* \param dev the device
		* \param dst the destination IP address
		* \param iface the interface
		* \param nextHop the IP address of the next 
		* \param seconfnextHop the IP address of the secondnext hop
		*/
		TLRRoutingTableEntry(
			Ptr<NetDevice> dev = 0,
			Ipv4Address dst = Ipv4Address(),
			Ipv4InterfaceAddress iface = Ipv4InterfaceAddress(),
			Ipv4Address nextHop = Ipv4Address(),
			uint32_t nextHopid = 0,
			Ptr<NetDevice> secdev = 0,
			Ipv4Address secdst = Ipv4Address(),
			Ipv4InterfaceAddress seciface = Ipv4InterfaceAddress(),
			Ipv4Address secondnextHop = Ipv4Address(),
			uint32_t secnextHopid = 0
		);

		~TLRRoutingTableEntry();


		// Fields
		/**
		* Get destination address function
		* \returns the IPv4 destination address
		*/
		Ipv4Address GetDestination() const
		{
			return m_ipv4Route->GetDestination();
		}
		Ipv4Address GetSecDestination() const
		{
			return m_secdst;
		}
		/**
		* Get route function
		* \returns The IPv4 route
		*/
		Ptr<Ipv4Route> GetRoute() const
		{
			return m_ipv4Route;
		}
		Ptr<Ipv4Route> GetSecRoute() const
		{
			Ptr<Ipv4Route> now=Create<Ipv4Route>();
			now->SetDestination(m_secdst);
			now->SetGateway(m_secondnextHop);
			now->SetSource(m_seciface.GetLocal());
			now->SetOutputDevice(m_secdev);
			return now;
		}
		/**
		* Set route function
		* \param r the IPv4 route
		*/
		void SetRoute(Ptr<Ipv4Route> r)
		{
			m_ipv4Route = r;
		}
		/**
		* Set next hop address
		* \param nextHop the next hop IPv4 address
		*/
		void SetNextHop(Ipv4Address nextHop)
		{
			m_ipv4Route->SetGateway(nextHop);
		}

		 void SetSecondNextHop(Ipv4Address secondnextHop)
		{
			//m_ipv4Route->SetGateway(secondnextHop);
			 m_secondnextHop = secondnextHop;
		}
		/**
		* Get next hop address
		* \returns the next hop address
		*/
		Ipv4Address GetNextHop() const
		{
			return m_ipv4Route->GetGateway();
		}
		uint32_t GetNextHopId() const
		{
			return m_nextHopid;
		}
		Ipv4Address GetSecondNextHop() const
		{
			return m_secondnextHop;
		}
		uint32_t GetSecNextHopId() const
		{
			return m_secnextHopid;
		}
		/**
		* Set output device
		* \param dev The output device
		*/
		void SetOutputDevice(Ptr<NetDevice> dev)
		{
			m_ipv4Route->SetOutputDevice(dev);
		}
		void SetSecOutputDevice(Ptr<NetDevice> dev)
		{
			m_secdev=dev;
		}
		/**
		* Get output device
		* \returns the output device
		*/
		Ptr<NetDevice> GetOutputDevice() const
		{
			return m_ipv4Route->GetOutputDevice();
		}
		Ptr<NetDevice> GetSecOutputDevice() const
		{
			return m_secdev;
		}
		/**
		* Get the Ipv4InterfaceAddress
		* \returns the Ipv4InterfaceAddress
		*/
		Ipv4InterfaceAddress GetInterface() const
		{
			return m_iface;
		}
		Ipv4InterfaceAddress GetSecInterface() const
		{
			return m_seciface;
		}
		/**
		* Set the Ipv4InterfaceAddress
		* \param iface The Ipv4InterfaceAddress
		*/
		void SetInterface(Ipv4InterfaceAddress iface)
		{
			m_iface = iface;
		}
		void SetSecInterface(Ipv4InterfaceAddress iface)
		{
			m_seciface = iface;
		}
		/**
		* \brief Compare destination address
		* \param dst IP address to compare
		* \return true if equal
		*/
		bool operator== (Ipv4Address const  dst) const
		{
			return (m_ipv4Route->GetDestination() == dst);
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
		uint32_t m_nextHopid;
		///Second NextHop
		Ptr<NetDevice> m_secdev;
		Ipv4Address m_secdst;
		Ipv4InterfaceAddress m_seciface;
		Ipv4Address m_secondnextHop;
		uint32_t m_secnextHopid;

	};


}


#endif /* TLR_ROUTING_TABLE_ENTRY_H */
