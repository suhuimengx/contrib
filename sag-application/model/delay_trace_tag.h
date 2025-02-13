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

#ifndef DELAY_TRACE_TAG_H
#define DELAY_TRACE_TAG_H

#include "ns3/header.h"
#include "ns3/tag.h"
#include "ns3/address-utils.h"
#include "ns3/packet.h"
#include <vector>

namespace ns3 {

class DelayTraceTag : public Tag
{
public:
	DelayTraceTag ();

	DelayTraceTag (uint64_t startTimeInMs);

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

	uint64_t GetStartTime();

private:
	uint64_t m_startTimeInMs;
};




}



#endif /* CONTRIB_PROTOCOLS_MODEL_DELAY_TRACE_TAG_H_ */
