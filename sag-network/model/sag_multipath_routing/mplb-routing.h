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

#ifndef MULTI_PATH_LOAD_BALANCE_H
#define MULTI_PATH_LOAD_BALANCE_H

#include <list>
#include <utility>
#include <stdint.h>

#include "ns3/ipv4-address.h"
#include "ns3/ipv4-header.h"
#include "ns3/socket.h"
#include "ns3/ptr.h"
#include "ns3/ipv4.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/point-to-point-channel.h"
#include "ns3/sag_routing_protocal.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-l3-protocol.h"
#include <map>
#include <unordered_map>
#include "ns3/sag_routing_table.h"
#include "ns3/sag_routing_table_entry.h"
#include "mplb-build-routing.h"

namespace ns3 {
namespace mplb {
/**
 * \ingroup MPLB
 *
 * \brief MPLB routing protocol
 */
class MultiPathLoadBalance: public SAGRoutingProtocal {
public:
	/**
	* \brief Get the type ID.
	* \return the object TypeId
	*/
	static TypeId GetTypeId(void);
	static const uint8_t MPLB_PROTOCOL;

	/// constructor
	MultiPathLoadBalance ();
	virtual ~MultiPathLoadBalance ();

	// Inherited from SAGRoutingProtocal
	Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr);
	bool RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
				   UnicastForwardCallback ucb, MulticastForwardCallback mcb,
				   LocalDeliverCallback lcb, ErrorCallback ecb);
	virtual void NotifyInterfaceUp (uint32_t i);
	virtual void NotifyInterfaceDown (uint32_t i);
	virtual void NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address);
	virtual void NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address);


protected:
    virtual void DoInitialize (void);

private:

    /// Routing table
    Ptr<SAGRoutingTable> m_routingTable;
    /// Routing algorithm
    Ptr<MplbBuildRouting> m_routeBuild;


};

} // namespace MPLB
} // namespace ns3



#endif /* MULTI_PATH_LOAD_BALANCE_H */
