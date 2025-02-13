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

 #ifndef SAG_APPLICATION_LAYER_SCPS_TP_SEND_H
 #define SAG_APPLICATION_LAYER_SCPS_TP_SEND_H
 
 #include "ns3/sag_application_layer.h"
 
 namespace ns3 {
 
 class Address;
 class Socket;
 
 class SAGApplicationLayerScpsTpSend : public SAGApplicationLayer
 {
 public:
   static TypeId GetTypeId (void);
   SAGApplicationLayerScpsTpSend ();
   virtual ~SAGApplicationLayerScpsTpSend ();
 
   virtual void NotifyAddressChange();
   //virtual void SetTransportFactory(std::string transport_factory_name);
   //virtual void SetTransportSocket(std::string transport_socket_name);
 
   int64_t GetAckedBytes();
   int64_t GetTotalBytes();
   int64_t GetCompletionTimeNs();
   bool IsCompleted();
   bool IsConnFailed();
   bool IsClosedByError();
   bool IsClosedNormally();
   void FinalizeDetailedLogs();
   void SetFlowEntry(SAGBurstInfoScpsTp entry);
 
 protected:
   virtual void DoDispose (void);
 private:
   virtual void StartApplication (void);    // Called at time specified by Start
   virtual void StopApplication (void);     // Called at time specified by Stop
 
   /**
    * Send data until the L4 transmission buffer is full.
    */
   void SendData ();
 
   //virtual void DoSomeThingWhenSendPkt(Ptr<Packet> packet);
   //virtual void DoSomeThingWhenReceivePkt(Ptr<Packet> packet);
 
   //Ptr<Socket>     m_socket;       //!< Associated socket
   
   Address         m_peer;         //!< Peer address
   bool            m_connected;    //!< True if connected
   uint32_t        m_sendSize;     //!< Size of data to send each time
   //uint64_t        m_maxBytes;     //!< Limit total number of bytes sent
   uint64_t        m_scpstpFlowId;       //!< Flow identifier
   uint64_t        m_totBytes;     //!< Total bytes sent so far
   TypeId          m_tid;          //!< The type of protocol to use.
   int64_t         m_completionTimeNs; //!< Completion time in nanoseconds
   bool            m_connFailed;       //!< Whether the connection failed
   bool            m_closedNormally;   //!< Whether the connection closed normally
   bool            m_closedByError;    //!< Whether the connection closed by error
   uint64_t        m_ackedBytes;       //!< Amount of acknowledged bytes cached after close of the socket
   bool            m_isCompleted;      //!< True iff the flow is completed fully AND closed normally
   std::string     m_additionalParameters; //!< Not used in this version of the application
   uint32_t        m_current_cwnd_byte;     //!< Current congestion window (detailed logging)
   int64_t         m_current_rtt_ns;        //!< Current last RTT sample (detailed logging)
   Ipv4Address     m_myAddress; //!< Record my inteface address
 
   // SCPSTP flow logging
   //bool m_enableDetailedLogging;            //!< True iff you want to write detailed logs
   //std::string m_baseLogsDir;               //!< Where the logs will be written to:
                                            //!<   logs_dir/scps_tp_flow_[id]_{progress, cwnd, rtt}.csv
   TracedCallback<Ptr<const Packet> > m_txTrace;
   bool m_connectReTry;
   EventId m_connectReTryEvent;
   uint64_t m_connectReTryInterval; //!< ms
 
   uint32_t rtt_record_n = 0;
   uint32_t process_record_n = 0;
 
 
   SAGBurstInfoScpsTp m_entry;
   EventId m_flowOnfly;
 
 private:
   void SetCallbacks();
   void ConnectionSucceeded (Ptr<Socket> socket);
   void ConnectionFailed (Ptr<Socket> socket);
   void DataSend (Ptr<Socket>, uint32_t);
   void SocketClosedNormal(Ptr<Socket> socket);
   void SocketClosedError(Ptr<Socket> socket);
   void CwndChange(uint32_t, uint32_t newCwnd);
   void RttChange (Time, Time newRtt);
   void InsertCwndLog(int64_t timestamp, uint32_t cwnd_byte);
   void InsertRttLog (int64_t timestamp, int64_t rtt_ns);
   void InsertProgressLog (int64_t timestamp, int64_t progress_byte);
 
 };
 
 } // namespace ns3
 
 #endif /* SCPS_TPs_FLOW_SEND_APPLICATION_H */
 