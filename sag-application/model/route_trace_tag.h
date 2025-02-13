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

#ifndef ROUTE_TRACE_TAG_H
#define ROUTE_TRACE_TAG_H

#include "ns3/header.h"
#include "ns3/tag.h"
#include "ns3/address-utils.h"
#include "ns3/packet.h"
#include <vector>

namespace ns3 {

class RouteTraceTag : public Tag
{
public:
	RouteTraceTag (uint8_t hops = 0, std::vector<uint32_t> nodeId = {});


	std::vector<uint32_t> GetRouteTrace (void) const;


	static TypeId GetTypeId (void);

	// inherited function, no need to doc.
	virtual TypeId GetInstanceTypeId (void) const;

	// inherited function, no need to doc.
	virtual uint32_t GetSerializedSize (void) const;

	// inherited function, no need to doc.
	virtual void Serialize (TagBuffer i) const;

	// inherited function, no need to doc.
	virtual void Deserialize (TagBuffer i);

	// inherited function, no need to doc.
	virtual void Print (std::ostream &os) const;

	private:
	uint8_t m_hops; //!< the hop counts carried by the tag
	std::vector<uint32_t> m_nodeId;
};




}

#endif /* ROUTE_TRACE_TAG_H */
