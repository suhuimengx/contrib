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

#include "endpoints.h"

namespace ns3 {
	NS_OBJECT_ENSURE_REGISTERED (Endpoints);

    TypeId
	Endpoints::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::Endpoints")
                .SetParent<Object>()
                .SetGroupName("SatelliteNetwork")
                .AddConstructor<Endpoints>()
				;
        return tid;
    }

    Endpoints::Endpoints(){

    }

	Endpoints::Endpoints (std::set<int64_t> endpoints, uint32_t satelliteNum){
		m_endpoints = endpoints;
		m_satelliteNum = satelliteNum;
    }

	Endpoints::~Endpoints (){

	}

	std::set<int64_t> Endpoints::GetEndpoints(){
		  return m_endpoints;
	}

	uint32_t Endpoints::GetNumSatellites(){
		return m_satelliteNum;
	}

	bool Endpoints::IsValidEndpoint(int64_t node_id){
		return m_endpoints.find(node_id) != m_endpoints.end();
	}


}


