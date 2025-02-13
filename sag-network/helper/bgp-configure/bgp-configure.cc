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
 * Author: Yuze Liu
 */

#include "bgp-configure.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("BgpConfigure");
NS_OBJECT_ENSURE_REGISTERED(BgpConfigure);

	TypeId BgpConfigure::GetTypeId (void)
	{
		static TypeId tid = TypeId("ns3::BgpConfigure")
			.SetParent<Object>()
			.SetGroupName("BgpConfigure")
			.AddConstructor<BgpConfigure>()
		;
		return tid;
	}

	BgpConfigure::BgpConfigure() {

	}

	BgpConfigure::~BgpConfigure() {

	}

	BgpConfigure::BgpConfigure(Ptr<BasicSimulation> basicSimulation, Ptr<TopologySatelliteNetwork> topology) {

	}

	void BgpConfigure::InstallIbgpPeer() {

	}
	void BgpConfigure::InstallEbgpPeer() {

	}

}
