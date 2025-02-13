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

 #include "ns3/log.h"
 #include "ns3/address.h"
 #include "ns3/node.h"
 #include "ns3/nstime.h"
 #include "ns3/socket.h"
 #include "ns3/string.h"
 #include "ns3/simulator.h"
 #include "ns3/socket-factory.h"
 #include "ns3/packet.h"
 #include "ns3/uinteger.h"
 #include "ns3/trace-source-accessor.h"
 #include "ns3/tcp-socket-factory.h"
 #include "ns3/tcp-socket-base.h"
 #include "ns3/tcp-tx-buffer.h"
 #include "ns3/scpstp-socket-factory.h"
 #include "ns3/scpstp-socket-base.h"
 #include "ns3/scpstp-tx-buffer.h"
 #include "ns3/sag_application_layer_scps_tp_send.h"
 #include "ns3/basic-simulation.h"
 #include <fstream>
 #include "ns3/route_trace_tag.h"
 #include "ns3/delay_trace_tag.h"
 
 namespace ns3 {
 
 NS_LOG_COMPONENT_DEFINE ("SAGApplicationLayerScpsTpSend");
 
 NS_OBJECT_ENSURE_REGISTERED (SAGApplicationLayerScpsTpSend);
 
 TypeId
 SAGApplicationLayerScpsTpSend::GetTypeId(void) {
     static TypeId tid = TypeId("ns3::SAGApplicationLayerScpsTpSend")
             .SetParent<SAGApplicationLayer>()
             .SetGroupName("Applications")
             .AddConstructor<SAGApplicationLayerScpsTpSend>()
             .AddAttribute("SendSize", "The amount of data to send each time.",
                           UintegerValue(100000),
                           MakeUintegerAccessor(&SAGApplicationLayerScpsTpSend::m_sendSize),
                           MakeUintegerChecker<uint32_t>(1))
 //            .AddAttribute("Remote", "The address of the destination",
 //                          AddressValue(),
 //                          MakeAddressAccessor(&SAGApplicationLayerScpsTpSend::m_peer),
 //                          MakeAddressChecker())
         .AddAttribute("DestinationPort",
               "The destination port",
               UintegerValue(1024),
               MakeUintegerAccessor(&SAGApplicationLayerScpsTpSend::m_dstPort),
               MakeUintegerChecker<uint16_t>())
 //            .AddAttribute("MaxBytes",
 //                          "The total number of bytes to send. "
 //                          "Once these bytes are sent, "
 //                          "no data  is sent again. The value zero means "
 //                          "that there is no limit.",
 //                          UintegerValue(0),
 //                          MakeUintegerAccessor(&SAGApplicationLayerScpsTpSend::m_maxBytes),
 //                          MakeUintegerChecker<uint64_t>())
             .AddAttribute("ScpsTpFlowId",
                           "SCPSTP flow identifier",
                           UintegerValue(0),
                           MakeUintegerAccessor(&SAGApplicationLayerScpsTpSend::m_scpstpFlowId),
                           MakeUintegerChecker<uint64_t>())
             .AddAttribute("Protocol", "The type of protocol to use.",
                           TypeIdValue(ScpsTpSocketFactory::GetTypeId()),
                           MakeTypeIdAccessor(&SAGApplicationLayerScpsTpSend::m_tid),
                           MakeTypeIdChecker())
             .AddAttribute ("AdditionalParameters",
                            "Additional parameter string; this might be parsed in another version of this application to "
                            "do slightly different behavior (e.g., set priority on SCPSTP socket etc.)",
                            StringValue (""),
                            MakeStringAccessor (&SAGApplicationLayerScpsTpSend::m_additionalParameters),
                            MakeStringChecker ())
             .AddTraceSource("Tx", "A new packet is created and is sent",
                             MakeTraceSourceAccessor(&SAGApplicationLayerScpsTpSend::m_txTrace),
                             "ns3::Packet::TracedCallback");
     return tid;
 }
 
 
 SAGApplicationLayerScpsTpSend::SAGApplicationLayerScpsTpSend()
         : m_connected(false),
           m_totBytes(0),
           m_completionTimeNs(-1),
           m_connFailed(false),
           m_closedNormally(false),
           m_closedByError(false),
           m_ackedBytes(0),
           m_isCompleted(false) {
     NS_LOG_FUNCTION(this);
     m_connectReTry = false;
     m_connectReTryEvent = EventId();
     m_connectReTryInterval =  100;
 }
 
 SAGApplicationLayerScpsTpSend::~SAGApplicationLayerScpsTpSend() {
     NS_LOG_FUNCTION(this);
 }
 
 void
 SAGApplicationLayerScpsTpSend::DoDispose(void) {
     NS_LOG_FUNCTION(this);
 
     // chain up
     SAGApplicationLayer::DoDispose();
 }
 
 void SAGApplicationLayerScpsTpSend::StartApplication(void) { // Called at time specified by Start
     NS_LOG_FUNCTION(this);
 
     // Create the socket if not already
     if (!m_socket) {
 
       m_peer = GetDestinationAddress();
       m_myAddress = GetMyAddress();// for the unique interface, to be improved todo
 
         m_socket = Socket::CreateSocket(GetNode(), m_tid);
 
         // Must be SCPSTP basically
         if (m_socket->GetSocketType() != Socket::NS3_SOCK_STREAM &&
             m_socket->GetSocketType() != Socket::NS3_SOCK_SEQPACKET) {
             NS_FATAL_ERROR("Using SAGApplicationLayerScpsTpSend with an incompatible socket type. "
                            "SAGApplicationLayerScpsTpSend requires SOCK_STREAM or SOCK_SEQPACKET. "
                            "In other words, use SCPSTP instead of UDP.");
         }
 
         // Bind socket
         if (Inet6SocketAddress::IsMatchingType(m_peer)) {
             if (m_socket->Bind6() == -1) {
                 NS_FATAL_ERROR("Failed to bind socket");
             }
         } else if (InetSocketAddress::IsMatchingType(m_peer)) {
             if (m_socket->Bind() == -1) {
                 NS_FATAL_ERROR("Failed to bind socket");
             }
         }
 
         // Connect, no receiver
         if(m_socket->Connect(m_peer) == -1){
           m_connectReTryEvent.Cancel();
           m_connectReTryEvent = Simulator::Schedule(MilliSeconds(m_connectReTryInterval),
                 &SAGApplicationLayerScpsTpSend::StartApplication, this);
           return;
         }
         m_socket->ShutdownRecv();
 
         SetCallbacks();
 
     }
     else if(m_socket && !m_connected){
 
       m_peer = GetDestinationAddress();
       m_myAddress = GetMyAddress();// for the unique interface, to be improved todo
 
       // Connect, no receiver
     if(m_socket->Connect(m_peer) == -1){
       m_connectReTryEvent.Cancel();
       m_connectReTryEvent = Simulator::Schedule(MilliSeconds(m_connectReTryInterval),
             &SAGApplicationLayerScpsTpSend::StartApplication, this);
       return;
     }
     m_socket->ShutdownRecv();
 
     SetCallbacks();
     }
     if (m_connected) {
         SendData();
     }
 
 }
 
 void SAGApplicationLayerScpsTpSend::SetFlowEntry(SAGBurstInfoScpsTp entry){
   m_entry = entry;
 }
 
 
 void SAGApplicationLayerScpsTpSend::StopApplication(void) { // Called at time specified by Stop
     NS_LOG_FUNCTION(this);
     if (m_socket != 0) {
         m_socket->Close();
         m_connected = false;
     } else {
         NS_LOG_WARN("SAGApplicationLayerScpsTpSend found null socket to close in StopApplication");
     }
 }
 
 void SAGApplicationLayerScpsTpSend::SendData(void) {
     NS_LOG_FUNCTION(this);
     if(!m_connected || m_socket == 0){
       return;
     }
 //    while (m_maxBytes == 0 || m_totBytes < m_maxBytes) { // Time to send more
 //
 //        // uint64_t to allow the comparison later.
 //        // the result is in a uint32_t range anyway, because
 //        // m_sendSize is uint32_t.
 //        uint64_t toSend = m_sendSize;
 //        // Make sure we don't send too many
 //        if (m_maxBytes > 0) {
 //            toSend = std::min(toSend, m_maxBytes - m_totBytes);
 //        }
 //
 //        NS_LOG_LOGIC("sending packet at " << Simulator::Now());
 //        Ptr <Packet> packet = Create<Packet>(toSend);
 //        int actual = m_socket->Send(packet);
 //        if (actual > 0) {
 //            m_totBytes += actual;
 //            m_txTrace(packet);
 //        }
 //        // We exit this loop when actual < toSend as the send side
 //        // buffer is full. The "DataSent" callback will pop when
 //        // some buffer space has freed up.
 //        if ((unsigned) actual != toSend) {
 //            break;
 //        }
 //    }
 //    // Check if time to close (all sent)
 //    if (m_totBytes == m_maxBytes && m_connected) {
 //        m_socket->Close(); // Close will only happen after send buffer is finished
 //        m_connected = false;
 //    }
 
   // uint64_t to allow the comparison later.
   // the result is in a uint32_t range anyway, because
   // m_sendSize is uint32_t.
   // std::cout<<Simulator::Now().GetSeconds()<<std::endl;
 
   if(m_flowOnfly.IsRunning() == true){
     return;
   }
 
   syncodecs::Codec& codec = *m_codec;
 
   float targetRate = m_entry.GetTargetRateMegabitPerSec()*1e6; // bps
   codec.setTargetRate (targetRate); //m_rVin
   ++codec; // Advance codec/packetizer to next frame/packet
   const auto bytesToSend = codec->first.size ();
 
   NS_LOG_LOGIC("sending packet at " << Simulator::Now());
 
   //std::cout<<bytesToSend<<std::endl;
   Ptr <Packet> packet = Create<Packet>(bytesToSend);
 
     // Tag for trace
     if (m_enableDetailedLogging){
         RouteTraceTag rtTrTag;
         packet->AddPacketTag(rtTrTag);
 
         DelayTraceTag delayTag(Simulator::Now().GetNanoSeconds());
         packet->AddPacketTag(delayTag);
     }
 
   int actual = m_socket->Send(packet);
   if (actual > 0) {
     m_totBytes += actual;
     m_txTrace(packet);
   }
   // We exit this loop when actual < toSend as the send side
   // buffer is full. The "DataSent" callback will pop when
   // some buffer space has freed up.
   if ((unsigned) actual != bytesToSend) {
     m_flowOnfly.Cancel();
     return;
   }
 
   double secsToNextEnqPacket = codec->second;
   //std::cout<<"SecsToNext: "<<codec->second<<std::endl;
   Time tNext = Seconds (secsToNextEnqPacket);
   //std::cout<<"tNext: "<<tNext<<std::endl;
 
   uint64_t now_ns = Simulator::Now().GetNanoSeconds();
   if (tNext.GetNanoSeconds() + now_ns < (uint64_t) (m_entry.GetStartTimeNs() + m_entry.GetDurationNs())) {
     m_flowOnfly = Simulator::Schedule(tNext, &SAGApplicationLayerScpsTpSend::SendData, this);
 
   }
   else{
     m_flowOnfly.Cancel();
     m_socket->Close(); // Close will only happen after send buffer is finished
     //std::cout<<Simulator::Now().GetSeconds()<<std::endl;
     m_connected = false;
   }
   //++codec; // Advance codec/packetizer to next frame/packet
 
 
 }
 
 void SAGApplicationLayerScpsTpSend::SetCallbacks(){
 
   // Callbacks
   m_socket->SetConnectCallback(
       MakeCallback(&SAGApplicationLayerScpsTpSend::ConnectionSucceeded, this),
       MakeCallback(&SAGApplicationLayerScpsTpSend::ConnectionFailed, this)
   );
   m_socket->SetSendCallback(MakeCallback(&SAGApplicationLayerScpsTpSend::DataSend, this));
   m_socket->SetCloseCallbacks(
       MakeCallback(&SAGApplicationLayerScpsTpSend::SocketClosedNormal, this),
       MakeCallback(&SAGApplicationLayerScpsTpSend::SocketClosedError, this)
   );
   if (m_enableDetailedLogging) {
 //		std::ofstream ofs;
 
 //		if(m_connectReTry){
 ////			ofs.open(m_baseLogsDir + "/" + format_string("scps_tp_flow_%" PRIu64 "_progress.csv", m_scpstpFlowId), std::ofstream::out | std::ofstream::app);
 ////			ofs << m_scpstpFlowId << "," << Simulator::Now ().GetNanoSeconds () << "," << GetAckedBytes() << std::endl;
 ////			ofs.close();
 //
 //			ofs.open(m_baseLogsDir + "/" + format_string("scps_tp_flow_%" PRIu64 "_cwnd.csv", m_scpstpFlowId), std::ofstream::out | std::ofstream::app);
 //			// Congestion window is only set upon SYN reception, so retrieving it early will just yield 0
 //			// As such there "is" basically no congestion window till then, so we are not going to write 0
 //			ofs.close();
 //
 //			ofs.open(m_baseLogsDir + "/" + format_string("scps_tp_flow_%" PRIu64 "_rtt.csv", m_scpstpFlowId), std::ofstream::out | std::ofstream::app);
 //			// At the socket creation, there is no RTT measurement, so retrieving it early will just yield 0
 //			// As such there "is" basically no RTT measurement till then, so we are not going to write 0
 //			ofs.close();
 //		}
 //		else{
 ////			ofs.open(m_baseLogsDir + "/" + format_string("scps_tp_flow_%" PRIu64 "_progress.csv", m_scpstpFlowId));
 ////			ofs << m_scpstpFlowId << "," << Simulator::Now ().GetNanoSeconds () << "," << GetAckedBytes() << std::endl;
 ////			ofs.close();
 //
 //			ofs.open(m_baseLogsDir + "/" + format_string("scps_tp_flow_%" PRIu64 "_cwnd.csv", m_scpstpFlowId));
 //			// Congestion window is only set upon SYN reception, so retrieving it early will just yield 0
 //			// As such there "is" basically no congestion window till then, so we are not going to write 0
 //			ofs.close();
 //
 //			ofs.open(m_baseLogsDir + "/" + format_string("scps_tp_flow_%" PRIu64 "_rtt.csv", m_scpstpFlowId));
 //			// At the socket creation, there is no RTT measurement, so retrieving it early will just yield 0
 //			// As such there "is" basically no RTT measurement till then, so we are not going to write 0
 //			ofs.close();
 //		}
 //
 //		m_socket->TraceConnectWithoutContext ("CongestionWindow", MakeCallback (&SAGApplicationLayerScpsTpSend::CwndChange, this));
     m_socket->TraceConnectWithoutContext ("RTT", MakeCallback (&SAGApplicationLayerScpsTpSend::RttChange, this));
   }
 }
 
 void SAGApplicationLayerScpsTpSend::ConnectionSucceeded(Ptr <Socket> socket) {
 
     NS_LOG_FUNCTION(this << socket);
     NS_LOG_LOGIC("SAGApplicationLayerScpsTpSend Connection succeeded");
     m_connected = true;
     SendData();
 }
 
 void SAGApplicationLayerScpsTpSend::ConnectionFailed(Ptr <Socket> socket) {
     NS_LOG_FUNCTION(this << socket);
     NS_LOG_LOGIC("SAGApplicationLayerScpsTpSend, Connection Failed");
     m_completionTimeNs = Simulator::Now().GetNanoSeconds();
     m_connFailed = true;
     m_closedByError = false;
     m_closedNormally = false;
     m_ackedBytes = 0;
     m_isCompleted = false;
     m_socket = 0;
 }
 
 void SAGApplicationLayerScpsTpSend::DataSend(Ptr <Socket>, uint32_t) {
     NS_LOG_FUNCTION(this);
     if (m_connected) { // Only send new data if the connection has completed
         SendData();
     }
 
     // Log the progress as DataSend() is called anytime space in the transmission buffer frees up
 //    if (m_enableDetailedLogging) {
 //        InsertProgressLog(
 //                Simulator::Now ().GetNanoSeconds (),
 //                GetAckedBytes()
 //        );
 //    }
     if (m_enableDetailedLogging) {
         process_record_n++;
       if(process_record_n % 50 != 0){
         return;
       }
       m_recordProcessTimeStampLog_us.push_back(Simulator::Now().GetMicroSeconds());
       m_pktSizeBytes.push_back(GetAckedBytes());
     }
 
 }
 
 void SAGApplicationLayerScpsTpSend::SocketClosedNormal(Ptr <Socket> socket) {
   //std::cout<<Simulator::Now().GetSeconds()<<std::endl;
 
     m_completionTimeNs = Simulator::Now().GetNanoSeconds();
     m_connFailed = false;
     m_closedByError = false;
     m_closedNormally = true;
     if (m_socket->GetObject<ScpsTpSocketBase>()->GetTxBuffer()->Size() != 0) {
         throw std::runtime_error("Socket closed normally but send buffer is not empty");
     }
     m_ackedBytes = m_totBytes - m_socket->GetObject<ScpsTpSocketBase>()->GetTxBuffer()->Size();
     //m_isCompleted = m_ackedBytes == m_maxBytes;  // todo
     m_isCompleted = m_ackedBytes == m_totBytes;  // todo
     m_socket = 0;
 }
 
 void SAGApplicationLayerScpsTpSend::SocketClosedError(Ptr <Socket> socket) {
 
   if(m_socket==0){
     return;
   }
     m_completionTimeNs = Simulator::Now().GetNanoSeconds();
     m_connFailed = false;
     m_closedByError = true;
     m_closedNormally = false;
     m_ackedBytes = m_totBytes - m_socket->GetObject<ScpsTpSocketBase>()->GetTxBuffer()->Size();
     m_isCompleted = false;
     m_socket = 0;
 
 }
 
 
 void
 SAGApplicationLayerScpsTpSend::NotifyAddressChange(){
 
 
   Address curPeer= GetDestinationAddress();
   Ipv4Address curMyAdr = GetMyAddress();
 
   //std::cout<<"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<m_peer<<"  "<<curPeer<<"  "<<curMyAdr<<std::endl;
 
   if(m_socket == 0 || (curPeer == m_peer && curMyAdr == m_myAddress)){
     return;
   }
   else{
     //std::cout<<"################################"<<m_peer<<"  "<<curPeer<<"  "<<curMyAdr<<std::endl;
     //m_socket->Close(); // need to deallocate L4 endpoint todo?
     m_ackedBytes = m_totBytes - m_socket->GetObject<ScpsTpSocketBase>()->GetTxBuffer()->Size();
     m_totBytes = m_ackedBytes;
     m_socket = 0;
     m_connected = false;
     m_connectReTry = true;
     StartApplication();
 
   }
 
 
 }
 
 
 int64_t SAGApplicationLayerScpsTpSend::GetAckedBytes() {
     if (m_connFailed || m_closedNormally || m_closedByError) {
         return m_ackedBytes;
     } else {
         return m_totBytes - m_socket->GetObject<ScpsTpSocketBase>()->GetTxBuffer()->Size();
     }
 }
 
 int64_t SAGApplicationLayerScpsTpSend::GetTotalBytes(){
   return  m_totBytes;
 }
 
 int64_t SAGApplicationLayerScpsTpSend::GetCompletionTimeNs() {
     return m_completionTimeNs;
 }
 
 bool SAGApplicationLayerScpsTpSend::IsCompleted() {
     return m_isCompleted;
 }
 
 bool SAGApplicationLayerScpsTpSend::IsConnFailed() {
     return m_connFailed;
 }
 
 bool SAGApplicationLayerScpsTpSend::IsClosedNormally() {
     return m_closedNormally;
 }
 
 bool SAGApplicationLayerScpsTpSend::IsClosedByError() {
     return m_closedByError;
 }
 
 void
 SAGApplicationLayerScpsTpSend::CwndChange(uint32_t oldCwnd, uint32_t newCwnd)
 {
     InsertCwndLog(Simulator::Now ().GetNanoSeconds (), newCwnd);
 }
 
 void
 SAGApplicationLayerScpsTpSend::RttChange (Time oldRtt, Time newRtt)
 {
 //    InsertRttLog(Simulator::Now ().GetNanoSeconds (), newRtt.GetNanoSeconds());
 
   rtt_record_n++;
     //InsertRttLog(Simulator::Now ().GetNanoSeconds (), newRtt.GetNanoSeconds());
 
   if(rtt_record_n % 50 != 0){
     return;
   }
   m_delaymsDetailsTimeStampLog_us.push_back(newRtt.GetMicroSeconds());
   m_recordTimeStampLog_us.push_back(Simulator::Now().GetMicroSeconds());
 }
 
 void
 SAGApplicationLayerScpsTpSend::InsertCwndLog(int64_t timestamp, uint32_t cwnd_byte)
 {
     std::ofstream ofs;
     ofs.open (m_baseLogsDir + "/" + format_string("scps_tp_flow_%" PRIu64 "_cwnd.csv", m_scpstpFlowId), std::ofstream::out | std::ofstream::app);
     ofs << m_scpstpFlowId << "," << timestamp << "," << cwnd_byte << std::endl;
     m_current_cwnd_byte = cwnd_byte;
     ofs.close();
 }
 
 void
 SAGApplicationLayerScpsTpSend::InsertRttLog (int64_t timestamp, int64_t rtt_ns)
 {
     std::ofstream ofs;
     ofs.open (m_baseLogsDir + "/" + format_string("scps_tp_flow_%" PRIu64 "_rtt.csv", m_scpstpFlowId), std::ofstream::out | std::ofstream::app);
     ofs << m_scpstpFlowId << "," << timestamp << "," << rtt_ns << std::endl;
     m_current_rtt_ns = rtt_ns;
     ofs.close();
 }
 
 void
 SAGApplicationLayerScpsTpSend::InsertProgressLog (int64_t timestamp, int64_t progress_byte) {
 //    std::ofstream ofs;
 //    ofs.open (m_baseLogsDir + "/" + format_string("scps_tp_flow_%" PRIu64 "_progress.csv", m_scpstpFlowId), std::ofstream::out | std::ofstream::app);
 //    ofs << m_scpstpFlowId << "," << timestamp << "," << progress_byte << std::endl;
 //    ofs.close();
 }
 
 void
 SAGApplicationLayerScpsTpSend::FinalizeDetailedLogs() {
     if (m_enableDetailedLogging) {
 //        int64_t timestamp;
 //        if (m_connFailed || m_closedByError || m_closedNormally) {
 //            timestamp = m_completionTimeNs;
 //        } else {
 //            timestamp = Simulator::Now ().GetNanoSeconds ();
 //        }
 //        InsertCwndLog(timestamp, m_current_cwnd_byte);
 //        InsertRttLog(timestamp, m_current_rtt_ns);
 //        //InsertProgressLog(timestamp, GetAckedBytes());
     }
 }
 
 } // Namespace ns3
 