/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019 ETH Zurich
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
 */
#ifndef SAG_TCP_TUNNEL_H
#define SAG_TCP_TUNNEL_H

#include "ns3/ptr.h"
#include "ns3/node.h"
#include "ns3/packet.h"


namespace ns3 {

class SAGTcpTunnel
{

public:
    SAGTcpTunnel(Ptr<Node> srcNode, Ptr<Node> desNode);
    void SendPacket(Ptr<Packet> p);
    void ReceivePacket();

private:
    void StartNextFlow(int i);

    Ptr<Application> m_tunnelBegin; 
    Ptr<Application> m_tunnelEnd; 
};



}

#endif /* TCP_FLOW_SCHEDULER_H */
