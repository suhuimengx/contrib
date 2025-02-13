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
#include "ns3/sag_application_layer_scps_tp_sink.h"

#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/scpstp-socket-factory.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SAGApplicationLayerScpsTpSink");

NS_OBJECT_ENSURE_REGISTERED (SAGApplicationLayerScpsTpSink);

TypeId
SAGApplicationLayerScpsTpSink::GetTypeId(void) {
    static TypeId tid = TypeId("ns3::SAGApplicationLayerScpsTpSink")
            .SetParent<SAGApplicationLayer>()
            .SetGroupName("Applications")
            .AddConstructor<SAGApplicationLayerScpsTpSink>()
            .AddAttribute("Local",
                          "The Address on which to Bind the rx socket.",
                          AddressValue(),
                          MakeAddressAccessor(&SAGApplicationLayerScpsTpSink::m_local),
                          MakeAddressChecker())
            .AddAttribute("Protocol",
                          "The type id of the protocol to use for the rx socket.",
                          TypeIdValue(ScpsTpSocketFactory::GetTypeId()),
                          MakeTypeIdAccessor(&SAGApplicationLayerScpsTpSink::m_tid),
                          MakeTypeIdChecker());
    return tid;
}

SAGApplicationLayerScpsTpSink::SAGApplicationLayerScpsTpSink() {
    NS_LOG_FUNCTION(this);
    m_totalRx = 0;
}

SAGApplicationLayerScpsTpSink::~SAGApplicationLayerScpsTpSink() {
    NS_LOG_FUNCTION(this);
}

void SAGApplicationLayerScpsTpSink::DoDispose(void) {
    NS_LOG_FUNCTION(this);
    m_socketList.clear();

    // chain up
    SAGApplicationLayer::DoDispose();
}


void SAGApplicationLayerScpsTpSink::StartApplication() { // Called at time specified by Start
    NS_LOG_FUNCTION(this);

    // Create a socket which is always in LISTEN state
    // As soon as it processes a SYN in ProcessListen(),
    // it forks itself into a new socket, which
    // keeps the accept and close callbacks
    if (!m_socket) {
        m_socket = Socket::CreateSocket(GetNode(), m_tid);
        if (m_socket->Bind(m_local) == -1) {
            NS_FATAL_ERROR("Failed to bind socket");
        }
        m_socket->Listen();
        m_socket->ShutdownSend();
        if (addressUtils::IsMulticast(m_local)) {
            throw std::runtime_error("No support for UDP here");
        }
    }

    // Callbacks
    m_socket->SetRecvCallback(MakeCallback(&SAGApplicationLayerScpsTpSink::HandleRead, this));
    m_socket->SetAcceptCallback(
            MakeNullCallback<bool, Ptr<Socket>,const Address &>(),
            MakeCallback(&SAGApplicationLayerScpsTpSink::HandleAccept, this)
    );

}

void SAGApplicationLayerScpsTpSink::StopApplication() {  // Called at time specified by Stop
    NS_LOG_FUNCTION(this);
    while (!m_socketList.empty()) {
        Ptr <Socket> socket = m_socketList.front();
        m_socketList.pop_front();
        socket->Close();
    }
    if (m_socket) {
        m_socket->Close();
    }
}

void SAGApplicationLayerScpsTpSink::HandleAccept(Ptr<Socket> socket, const Address &from) {
    NS_LOG_FUNCTION(this << socket << from);
    socket->SetRecvCallback (MakeCallback (&SAGApplicationLayerScpsTpSink::HandleRead, this));
    socket->SetCloseCallbacks(
            MakeCallback(&SAGApplicationLayerScpsTpSink::HandlePeerClose, this),
            MakeCallback(&SAGApplicationLayerScpsTpSink::HandlePeerError, this)
    );
    m_socketList.push_back(socket);
}

void SAGApplicationLayerScpsTpSink::HandleRead(Ptr<Socket> socket) {
    NS_LOG_FUNCTION (this << socket);

    // Immediately from the socket drain all the packets it has received
    Ptr<Packet> packet;
    Address from;
    while ((packet = socket->RecvFrom(from))) {
        if (packet->GetSize() == 0) { // EOFs
            break;
        }
        //std::cout<<Simulator::Now().GetSeconds()<<"  "<<packet->GetSize ()<<std::endl;
        m_totalRx += packet->GetSize ();

        m_totalRxBytes += packet->GetSize ();
        m_totalRxPacketNumber += 1;

        // Other fields that could be useful here if actually did something:
        // Size: packet->GetSize()
        // Source IP: InetSocketAddress::ConvertFrom(from).GetIpv4 ()
        // Source port: InetSocketAddress::ConvertFrom (from).GetPort ()
        // Our own IP / port: Address localAddress; socket->GetSockName (localAddress);

        // Log precise trace
		if (m_enableDetailedLogging) {
			RecordDetailsLogRouteOnly(packet);
		}

    }
}

void SAGApplicationLayerScpsTpSink::HandlePeerClose(Ptr<Socket> socket) {
    NS_LOG_FUNCTION(this << socket);
    CleanUp(socket);
}

void SAGApplicationLayerScpsTpSink::HandlePeerError(Ptr<Socket> socket) {
    NS_LOG_FUNCTION(this << socket);
    CleanUp(socket);
}

void SAGApplicationLayerScpsTpSink::CleanUp(Ptr<Socket> socket) {
    NS_LOG_FUNCTION(this << socket);
    // This function can be called 2x if the LAST_ACK retries fail.
    // That would result in first a normal close, and then an error close.
    std::list<Ptr<Socket>>::iterator it;
    for (it = m_socketList.begin(); it != m_socketList.end(); ++it) {
        if (*it == socket) {
            m_socketList.erase(it);
            break;
        }
    }
}

} // Namespace ns3

 