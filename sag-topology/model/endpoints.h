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

#include <vector>
#include <cfloat>
#include "ns3/exp-util.h"
#include "ns3/node-container.h"

#ifndef ENDPOINTS_H
#define ENDPOINTS_H

namespace ns3 {
/**
 * \ingroup SatelliteNetwork
 *
 * \brief Rule of ISL establishment
 */
class Endpoints : public Object
{
public:
	static TypeId GetTypeId (void);
	// constructor
	Endpoints();
	Endpoints (std::set<int64_t> endpoints, uint32_t satelliteNum);
	virtual ~Endpoints ();

	std::set<int64_t> GetEndpoints();
	uint32_t GetNumSatellites();
	bool IsValidEndpoint(int64_t node_id);

protected:

	std::set<int64_t> m_endpoints;
	uint32_t m_satelliteNum;

};


}

#endif /* ENDPOINTS_H */
