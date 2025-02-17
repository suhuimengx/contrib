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

 #include "ns3/sag_scps_tp_tunnel.h"
 #include "ns3/simulator.h"
 #include "ns3/nstime.h"
 #include "ns3/sag_application_helper.h"
 #include "ns3/sag_application_layer_scps_tp_send.h"
 #include "ns3/sag_application_layer_scps_tp_sink.h"
 
 
 namespace ns3 {
 
 void SAGScpsTpTunnel::SendPacket(Ptr<Packet> p) 
 {
     m_tunnelBegin->GetObject<SAGApplicationLayerScpsTpSend>()->SetTunnelPacket(p);
 
 }
 
 void SAGScpsTpTunnel::ReceivePacket() 
 {
 
 
 }
 
 SAGScpsTpTunnel::SAGScpsTpTunnel(Ptr<Node> srcNode, Ptr<Node> desNode) 
 {
     printf("SAG SCPSTP TUNNEL start from %u to %u \n", srcNode->GetId(), desNode->GetId());
 
     std::cout << "  > SCPSTP TUNNEL is enabled" << std::endl;
     int64_t now_ns = Simulator::Now().GetNanoSeconds();
 
     // Install scpstp sink on destination
     std::cout << "  > Setting up SCPSTP Tunnel sink" << std::endl;
     SAGApplicationHelperScpsTpSink sink;
     ApplicationContainer appSink = sink.Install(desNode);
     appSink.Start(NanoSeconds(now_ns));
     m_tunnelEnd = appSink.Get(0);
     // m_tunnelEnd->GetObject<SAGApplicationLayerScpsTpSind>()->SetTunnelCallback(this);
 
     // Install ScpsTp send on source
     SAGApplicationHelperScpsTpSend source;
     ApplicationContainer appSend = source.Install(srcNode);
     appSend.Start(NanoSeconds(now_ns));
     appSend.Get(0)->GetObject<SAGApplicationLayer>()->SetDestinationNode(desNode);
     m_tunnelBegin = appSend.Get(0);
     
 
 
     std::cout << std::endl;
 }
 
 
 }
 