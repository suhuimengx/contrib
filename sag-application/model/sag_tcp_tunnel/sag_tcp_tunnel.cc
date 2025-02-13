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
 *
 */

#include "ns3/sag_tcp_tunnel.h"
#include "ns3/simulator.h"
#include "ns3/nstime.h"
#include "ns3/sag_application_helper.h"
#include "ns3/sag_application_layer_tcp_send.h"
#include "ns3/sag_application_layer_tcp_sink.h"


namespace ns3 {

void SAGTcpTunnel::SendPacket(Ptr<Packet> p) 
{
    m_tunnelBegin->GetObject<SAGApplicationLayerTcpSend>()->SetTunnelPacket(p);

}

void SAGTcpTunnel::ReceivePacket() 
{


}

SAGTcpTunnel::SAGTcpTunnel(Ptr<Node> srcNode, Ptr<Node> desNode) 
{
    printf("SAG TCP TUNNEL start from %u to %u \n", srcNode->GetId(), desNode->GetId());

    std::cout << "  > TCP TUNNEL is enabled" << std::endl;
    int64_t now_ns = Simulator::Now().GetNanoSeconds();

    // Install tcp sink on destination
    std::cout << "  > Setting up TCP Tunnel sink" << std::endl;
    SAGApplicationHelperTcpSink sink;
    ApplicationContainer appSink = sink.Install(desNode);
    appSink.Start(NanoSeconds(now_ns));
    m_tunnelEnd = appSink.Get(0);
    // m_tunnelEnd->GetObject<SAGApplicationLayerTcpSind>()->SetTunnelCallback(this);

    // Install tcp send on source
    SAGApplicationHelperTcpSend source;
    ApplicationContainer appSend = source.Install(srcNode);
    appSend.Start(NanoSeconds(now_ns));
    appSend.Get(0)->GetObject<SAGApplicationLayer>()->SetDestinationNode(desNode);
    m_tunnelBegin = appSend.Get(0);
    


    std::cout << std::endl;
}


}
