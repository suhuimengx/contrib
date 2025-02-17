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
#include "ns3/sag_application_helper.h"
#include "ns3/sag_application_layer_rtp_sender.h"
#include "ns3/sag_application_layer_udp.h"
#include "ns3/sag_rtp_constants.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"
#include "ns3/string.h"



namespace ns3 {

//base sag application helper
SAGApplicationHelper::SAGApplicationHelper (Ptr<ns3::BasicSimulation> basicSimulation)
{
	m_factory.SetTypeId (basicSimulation->GetConfigParamOrDefault("application_layer_protocal", "ns3::SAGApplicationLayer"));
	m_basicSimulation = basicSimulation;
//	std::string traceType = m_basicSimulation->GetConfigParamOrDefault("application_layer_trace_type", "TRACE_TYPE_CHAT");
//	std::string codecType = m_basicSimulation->GetConfigParamOrDefault("application_layer_codec_type", "SYNCODEC_TYPE_PERFECT");
	SetAttribute (m_factory, "BaseLogsDir", StringValue (basicSimulation->GetLogsDir()));
	SetAttribute (m_factory, "BaseDir", StringValue (basicSimulation->GetRunDir()));

//	if(traceType=="TRACE_TYPE_CHAT")
//	{
//		m_traceType = TRACE_TYPE_CHAT;
//	}
//	else if (traceType=="TRACE_TYPE_BBB")
//	{
//		m_traceType = TRACE_TYPE_BBB;
//	}
//	else if (traceType=="TRACE_TYPE_CONCAT")
//	{
//		m_traceType = TRACE_TYPE_CONCAT;
//	}
//	else if (traceType=="TRACE_TYPE_ED")
//	{
//		m_traceType = TRACE_TYPE_ED;
//	}
//	else if (traceType=="TRACE_TYPE_FOREMAN")
//	{
//		m_traceType = TRACE_TYPE_FOREMAN;
//	}
//	else if (traceType=="TRACE_TYPE_NEWS")
//	{
//		m_traceType = TRACE_TYPE_NEWS;
//	}
//	else
//	{
//		m_traceType = TRACE_TYPE_CHAT;
//	}
//
//	if(codecType=="SYNCODEC_TYPE_PERFECT"){
//		m_codecType = SYNCODEC_TYPE_PERFECT;
//	}
//	else if (codecType=="SYNCODEC_TYPE_FIXFPS")
//	{
//		m_codecType = SYNCODEC_TYPE_FIXFPS;
//	}
//	else if (codecType=="SYNCODEC_TYPE_STATS")
//	{
//		m_codecType = SYNCODEC_TYPE_STATS;
//	}
//	else if (codecType=="SYNCODEC_TYPE_TRACE")
//	{
//		m_codecType = SYNCODEC_TYPE_TRACE;
//	}
//	else if (codecType=="SYNCODEC_TYPE_SHARING")
//	{
//		m_codecType = SYNCODEC_TYPE_SHARING;
//	}
//	else if (codecType=="SYNCODEC_TYPE_HYBRID")
//	{
//		m_codecType = SYNCODEC_TYPE_HYBRID;
//	}
//	else
//	{
//		m_codecType = SYNCODEC_TYPE_TRACE;
//	}
}

SAGApplicationHelper::SAGApplicationHelper (Ptr<ns3::BasicSimulation> basicSimulation, std::string codecType, std::string traceType){
	m_factory.SetTypeId (basicSimulation->GetConfigParamOrDefault("application_layer_protocal", "ns3::SAGApplicationLayer"));
	m_basicSimulation = basicSimulation;

	SetAttribute (m_factory, "BaseLogsDir", StringValue (basicSimulation->GetLogsDir()));
	SetAttribute (m_factory, "BaseDir", StringValue (basicSimulation->GetRunDir()));

	if(traceType=="TRACE_TYPE_CHAT")
	{
		m_traceType = TRACE_TYPE_CHAT;
	}
	else if(traceType=="TRACE_TYPE_BBB")
	{
		m_traceType = TRACE_TYPE_BBB;
	}
	else if(traceType=="TRACE_TYPE_CONCAT")
	{
		m_traceType = TRACE_TYPE_CONCAT;
	}
	else if(traceType=="TRACE_TYPE_ED")
	{
		m_traceType = TRACE_TYPE_ED;
	}
	else if(traceType=="TRACE_TYPE_FOREMAN")
	{
		m_traceType = TRACE_TYPE_FOREMAN;
	}
	else if(traceType=="TRACE_TYPE_NEWS")
	{
		m_traceType = TRACE_TYPE_NEWS;
	}
	else if(traceType=="0")
	{
		m_traceType = TRACE_TYPE_CHAT;
	}
	else{
	std::cout<<traceType<<std::endl;
        throw std::invalid_argument("Invaild trace type.");
	}

	if(codecType=="SYNCODEC_TYPE_PERFECT"){
		m_codecType = SYNCODEC_TYPE_PERFECT;
	}
	else if(codecType=="SYNCODEC_TYPE_FIXFPS")
	{
		m_codecType = SYNCODEC_TYPE_FIXFPS;
	}
	else if(codecType=="SYNCODEC_TYPE_STATS")
	{
		m_codecType = SYNCODEC_TYPE_STATS;
	}
	else if(codecType=="SYNCODEC_TYPE_TRACE")
	{
		m_codecType = SYNCODEC_TYPE_TRACE;
	}
	else if(codecType=="SYNCODEC_TYPE_SHARING")
	{
		m_codecType = SYNCODEC_TYPE_SHARING;
	}
	else if(codecType=="SYNCODEC_TYPE_HYBRID")
	{
		m_codecType = SYNCODEC_TYPE_HYBRID;
	}
	else
	{
        throw std::invalid_argument("Invaild codec type.");
	}
}

void
SAGApplicationHelper::SetAttribute (
  ObjectFactory& factory,
  std::string name,
  const AttributeValue &value)
{
  factory.Set (name, value);
}

ApplicationContainer
SAGApplicationHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
SAGApplicationHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i) {
      apps.Add (InstallPriv (*i));
  }
  return apps;
}

Ptr<Application>
SAGApplicationHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<SAGApplicationLayer> app = m_factory.Create<SAGApplicationLayer> ();
//  app->SetTransportFactory(m_basicSimulation->GetConfigParamOrDefault("transport_layer_factory", "ns3::SAGTransportSocketFactory"));
//  app->SetTransportSocket(m_basicSimulation->GetConfigParamOrDefault("transport_layer_socket", "ns3::SAGTransportSocketImpl"));
  node->AddApplication ((Ptr<Application>)app);
  return (Ptr<Application>)app;
}


//udp sag application helper

SAGApplicationHelperUdp::SAGApplicationHelperUdp (Ptr<ns3::BasicSimulation> basicSimulation, std::string codecType, std::string traceType)
:SAGApplicationHelper(basicSimulation, codecType, traceType)
{
  m_udpFactory.SetTypeId (basicSimulation->GetConfigParamOrDefault("application_layer_protocal_udp", "ns3::SAGApplicationLayerUdp"));
  //SetAttribute (m_udpFactory, "Port", UintegerValue (port));
  SetAttribute (m_udpFactory, "BaseLogsDir", StringValue (basicSimulation->GetLogsDir()));
  SetAttribute (m_udpFactory, "BaseDir", StringValue (basicSimulation->GetRunDir()));
}

SAGApplicationHelperUdp::SAGApplicationHelperUdp (Ptr<ns3::BasicSimulation> basicSimulation)
:SAGApplicationHelper(basicSimulation)
{
  m_udpFactory.SetTypeId (basicSimulation->GetConfigParamOrDefault("application_layer_protocal_udp", "ns3::SAGApplicationLayerUdp"));
  //SetAttribute (m_udpFactory, "Port", UintegerValue (port));
  SetAttribute (m_udpFactory, "BaseLogsDir", StringValue (basicSimulation->GetLogsDir()));
  SetAttribute (m_udpFactory, "BaseDir", StringValue (basicSimulation->GetRunDir()));
}

Ptr<Application>
SAGApplicationHelperUdp::InstallPriv (Ptr<Node> node) const
{
  Ptr<SAGApplicationLayer> app = m_udpFactory.Create<SAGApplicationLayer> ();
  //app->SetTransportFactory(m_basicSimulation->GetConfigParamOrDefault("transport_layer_factory", "ns3::SAGTransportSocketFactory"));
  //app->SetTransportSocket(m_basicSimulation->GetConfigParamOrDefault("transport_layer_socket", "ns3::SAGTransportSocketImpl"));
  app->SetMaxPayLoadSizeByte(DEFAULT_PACKET_SIZE_UDP);  // 1500 - IP头(20) - UDP头(8) = 1472(Bytes)
  if(m_traceType != TRACE_TYPE_INVALID && m_codecType != SYNCODEC_TYPE_INVALID){
	  app->SetTrace(m_traceType);
	  app->SetCodecType(m_codecType);
  }
  app->SetTransportFactory("ns3::UdpSocketFactory");
  app->SetTransportSocket("ns3::UdpSocketImpl");
  node->AddApplication ((Ptr<Application>)app);
  return (Ptr<Application>)app;
}

//rtp sag application helper
SAGApplicationHelperRtpSender::SAGApplicationHelperRtpSender (Ptr<ns3::BasicSimulation> basicSimulation, SAGBurstInfoRtp entry, uint16_t port)
:SAGApplicationHelper(basicSimulation, entry.GetAdditionalParameters(), entry.GetMetadata())
{
  m_rtpsendFactory.SetTypeId (basicSimulation->GetConfigParamOrDefault("application_layer_protocal_rtp_send", "ns3::SAGApplicationLayerRTPSender"));
  SetAttribute (m_rtpsendFactory, "Port", UintegerValue (port));
  SetAttribute (m_rtpsendFactory, "BaseLogsDir", StringValue (basicSimulation->GetLogsDir()));
  SetAttribute (m_rtpsendFactory, "BaseDir", StringValue (basicSimulation->GetRunDir()));
}

Ptr<Application>
SAGApplicationHelperRtpSender::InstallPriv (Ptr<Node> node) const
{
  Ptr<SAGApplicationLayerRTPSender> app = m_rtpsendFactory.Create<SAGApplicationLayerRTPSender> (); //SAGApplicationLayer
	// Trace-based video source
  //std::cout << "  > Setting trace-based video source from trace file" << std::endl;
  app->SetMaxPayLoadSizeByte(DEFAULT_PACKET_SIZE);
  if(m_traceType != TRACE_TYPE_INVALID && m_codecType != SYNCODEC_TYPE_INVALID){
	  app->SetTrace(m_traceType);
	  app->SetCodecType(m_codecType);
  }

  app->SetTransportFactory("ns3::UdpSocketFactory");
  app->SetTransportSocket("ns3::UdpSocketImpl");

//  app->SetTransportFactory(m_basicSimulation->GetConfigParamOrDefault("transport_layer_factory", "ns3::SAGTransportSocketFactory"));
//  app->SetTransportSocket(m_basicSimulation->GetConfigParamOrDefault("transport_layer_socket", "ns3::SAGTransportSocketImpl"));
  node->AddApplication ((Ptr<Application>)app);
  return (Ptr<Application>)app;
}

//rtp sag application helper
SAGApplicationHelperRtpReceiver::SAGApplicationHelperRtpReceiver (Ptr<ns3::BasicSimulation> basicSimulation, uint16_t port)
:SAGApplicationHelper(basicSimulation)
{
  m_rtpreceiveFactory.SetTypeId (basicSimulation->GetConfigParamOrDefault("application_layer_protocal_rtp_receive", "ns3::SAGApplicationLayerRTPReceiver"));
  SetAttribute (m_rtpreceiveFactory, "Port", UintegerValue (port));
  SetAttribute (m_rtpreceiveFactory, "BaseLogsDir", StringValue (basicSimulation->GetLogsDir()));
  SetAttribute (m_rtpreceiveFactory, "BaseDir", StringValue (basicSimulation->GetRunDir()));
}

Ptr<Application>
SAGApplicationHelperRtpReceiver::InstallPriv (Ptr<Node> node) const
{
  Ptr<SAGApplicationLayer> app = m_rtpreceiveFactory.Create<SAGApplicationLayer> ();
//  app->SetTransportFactory(m_basicSimulation->GetConfigParamOrDefault("transport_layer_factory", "ns3::SAGTransportSocketFactory"));
//  app->SetTransportSocket(m_basicSimulation->GetConfigParamOrDefault("transport_layer_socket", "ns3::SAGTransportSocketImpl"));
  app->SetTransportFactory("ns3::UdpSocketFactory");
  app->SetTransportSocket("ns3::UdpSocketImpl");
  node->AddApplication ((Ptr<Application>)app);
  return (Ptr<Application>)app;
}


//tcp sag application helper

SAGApplicationHelperTcpSend::SAGApplicationHelperTcpSend (Ptr<ns3::BasicSimulation> basicSimulation, SAGBurstInfoTcp entry, uint16_t dstPort, bool enableFlowLoggingToFile)
:SAGApplicationHelper(basicSimulation, entry.GetAdditionalParameters(), entry.GetMetadata())
{
  m_tcpSendFactory.SetTypeId (basicSimulation->GetConfigParamOrDefault("application_layer_protocal_tcp_send", "ns3::SAGApplicationLayerTcpSend"));
  SetAttribute (m_tcpSendFactory, "Protocol", StringValue ("ns3::TcpSocketFactory"));
  SetAttribute (m_tcpSendFactory, "DestinationPort", UintegerValue (dstPort));
  //SetAttribute (m_tcpSendFactory, "MaxBytes", UintegerValue (entry.GetSizeByte()));
  SetAttribute (m_tcpSendFactory, "TcpFlowId", UintegerValue (entry.GetTcpFlowId()));
  SetAttribute (m_tcpSendFactory, "EnableFlowLoggingToFile", BooleanValue (enableFlowLoggingToFile));
  SetAttribute (m_tcpSendFactory, "AdditionalParameters", StringValue (entry.GetAdditionalParameters()));

  SetAttribute (m_tcpSendFactory, "BaseLogsDir", StringValue (basicSimulation->GetLogsDir()));
  SetAttribute (m_tcpSendFactory, "BaseDir", StringValue (basicSimulation->GetRunDir()));
}

Ptr<Application>
SAGApplicationHelperTcpSend::InstallPriv (Ptr<Node> node) const
{
  Ptr<SAGApplicationLayer> app = m_tcpSendFactory.Create<SAGApplicationLayer> ();
  //app->SetTransportFactory(m_basicSimulation->GetConfigParamOrDefault("transport_layer_factory", "ns3::SAGTransportSocketFactory"));
  //app->SetTransportSocket(m_basicSimulation->GetConfigParamOrDefault("transport_layer_socket", "ns3::SAGTransportSocketImpl"));
  app->SetMaxPayLoadSizeByte(DEFAULT_PACKET_SIZE);  // 1500 - IP头(20) - TCP头(20) = 1460 (Bytes)
  if(m_traceType != TRACE_TYPE_INVALID && m_codecType != SYNCODEC_TYPE_INVALID){
	  app->SetTrace(m_traceType);
	  app->SetCodecType(m_codecType);
  }
  node->AddApplication ((Ptr<Application>)app);
  return (Ptr<Application>)app;
}

SAGApplicationHelperTcpSink::SAGApplicationHelperTcpSink (Ptr<ns3::BasicSimulation> basicSimulation, std::string protocol, Address address, bool enableFlowLoggingToFile)
:SAGApplicationHelper(basicSimulation)
{
  m_tcpSinkFactory.SetTypeId (basicSimulation->GetConfigParamOrDefault("application_layer_protocal_tcp_sink", "ns3::SAGApplicationLayerTcpSink"));
  SetAttribute (m_tcpSinkFactory, "Protocol", StringValue (protocol));
  SetAttribute (m_tcpSinkFactory, "Local", AddressValue (address));
  SetAttribute (m_tcpSinkFactory, "EnableFlowLoggingToFile", BooleanValue (enableFlowLoggingToFile));
}

Ptr<Application>
SAGApplicationHelperTcpSink::InstallPriv (Ptr<Node> node) const
{
  Ptr<SAGApplicationLayer> app = m_tcpSinkFactory.Create<SAGApplicationLayer> ();
  //app->SetTransportFactory(m_basicSimulation->GetConfigParamOrDefault("transport_layer_factory", "ns3::SAGTransportSocketFactory"));
  //app->SetTransportSocket(m_basicSimulation->GetConfigParamOrDefault("transport_layer_socket", "ns3::SAGTransportSocketImpl"));
  node->AddApplication ((Ptr<Application>)app);
  return (Ptr<Application>)app;
}

//scpstp sag application helper

SAGApplicationHelperScpsTpSend::SAGApplicationHelperScpsTpSend (Ptr<ns3::BasicSimulation> basicSimulation, SAGBurstInfoScpsTp entry, uint16_t dstPort, bool enableFlowLoggingToFile)
:SAGApplicationHelper(basicSimulation, entry.GetAdditionalParameters(), entry.GetMetadata())
{
  m_scpstpSendFactory.SetTypeId (basicSimulation->GetConfigParamOrDefault("application_layer_protocal_scps_tp_send", "ns3::SAGApplicationLayerScpsTpSend"));
  SetAttribute (m_scpstpSendFactory, "Protocol", StringValue ("ns3::ScpsTpSocketFactory"));
  SetAttribute (m_scpstpSendFactory, "DestinationPort", UintegerValue (dstPort));
  //SetAttribute (m_scpstpSendFactory, "MaxBytes", UintegerValue (entry.GetSizeByte()));
  SetAttribute (m_scpstpSendFactory, "ScpsTpFlowId", UintegerValue (entry.GetScpsTpFlowId()));
  SetAttribute (m_scpstpSendFactory, "EnableFlowLoggingToFile", BooleanValue (enableFlowLoggingToFile));
  SetAttribute (m_scpstpSendFactory, "AdditionalParameters", StringValue (entry.GetAdditionalParameters()));

  SetAttribute (m_scpstpSendFactory, "BaseLogsDir", StringValue (basicSimulation->GetLogsDir()));
  SetAttribute (m_scpstpSendFactory, "BaseDir", StringValue (basicSimulation->GetRunDir()));
}

Ptr<Application>
SAGApplicationHelperScpsTpSend::InstallPriv (Ptr<Node> node) const
{
  Ptr<SAGApplicationLayer> app = m_scpstpSendFactory.Create<SAGApplicationLayer> ();
  //app->SetTransportFactory(m_basicSimulation->GetConfigParamOrDefault("transport_layer_factory", "ns3::SAGTransportSocketFactory"));
  //app->SetTransportSocket(m_basicSimulation->GetConfigParamOrDefault("transport_layer_socket", "ns3::SAGTransportSocketImpl"));
  app->SetMaxPayLoadSizeByte(DEFAULT_PACKET_SIZE);  // 1500 - IP头(20) - TCP头(20) = 1460 (Bytes)
  if(m_traceType != TRACE_TYPE_INVALID && m_codecType != SYNCODEC_TYPE_INVALID){
	  app->SetTrace(m_traceType);
	  app->SetCodecType(m_codecType);
  }
  node->AddApplication ((Ptr<Application>)app);
  return (Ptr<Application>)app;
}

SAGApplicationHelperScpsTpSink::SAGApplicationHelperScpsTpSink (Ptr<ns3::BasicSimulation> basicSimulation, std::string protocol, Address address, bool enableFlowLoggingToFile)
:SAGApplicationHelper(basicSimulation)
{
  m_scpstpSinkFactory.SetTypeId (basicSimulation->GetConfigParamOrDefault("application_layer_protocal_scps_tp_sink", "ns3::SAGApplicationLayerScpsTpSink"));
  SetAttribute (m_scpstpSinkFactory, "Protocol", StringValue (protocol));
  SetAttribute (m_scpstpSinkFactory, "Local", AddressValue (address));
  SetAttribute (m_scpstpSinkFactory, "EnableFlowLoggingToFile", BooleanValue (enableFlowLoggingToFile));
}

Ptr<Application>
SAGApplicationHelperScpsTpSink::InstallPriv (Ptr<Node> node) const
{
  Ptr<SAGApplicationLayer> app = m_scpstpSinkFactory.Create<SAGApplicationLayer> ();
  //app->SetTransportFactory(m_basicSimulation->GetConfigParamOrDefault("transport_layer_factory", "ns3::SAGTransportSocketFactory"));
  //app->SetTransportSocket(m_basicSimulation->GetConfigParamOrDefault("transport_layer_socket", "ns3::SAGTransportSocketImpl"));
  node->AddApplication ((Ptr<Application>)app);
  return (Ptr<Application>)app;
}



//Mengy's:: Quic Application Setup
SAGApplicationHelperQuicSend::SAGApplicationHelperQuicSend (Ptr<ns3::BasicSimulation> basicSimulation, SAGBurstInfoTcp entry, uint16_t dstPort, bool enableFlowLoggingToFile)
:SAGApplicationHelper(basicSimulation, entry.GetAdditionalParameters(), entry.GetMetadata())
{
  m_quicSendFactory.SetTypeId (basicSimulation->GetConfigParamOrDefault("application_layer_protocal_quic_send", "ns3::SAGApplicationLayerQuicSend"));
  SetAttribute (m_quicSendFactory, "Protocol", StringValue ("ns3::QuicSocketFactory"));
  SetAttribute (m_quicSendFactory, "DestinationPort", UintegerValue (dstPort));
  //SetAttribute (m_tcpSendFactory, "MaxBytes", UintegerValue (entry.GetSizeByte()));
  SetAttribute (m_quicSendFactory, "TcpFlowId", UintegerValue (entry.GetTcpFlowId()));
  SetAttribute (m_quicSendFactory, "EnableFlowLoggingToFile", BooleanValue (enableFlowLoggingToFile));
  SetAttribute (m_quicSendFactory, "AdditionalParameters", StringValue (entry.GetAdditionalParameters()));

  SetAttribute (m_quicSendFactory, "BaseLogsDir", StringValue (basicSimulation->GetLogsDir()));
  SetAttribute (m_quicSendFactory, "BaseDir", StringValue (basicSimulation->GetRunDir()));
}

Ptr<Application>
SAGApplicationHelperQuicSend::InstallPriv (Ptr<Node> node) const
{
  Ptr<SAGApplicationLayer> app = m_quicSendFactory.Create<SAGApplicationLayer> ();
  //app->SetTransportFactory(m_basicSimulation->GetConfigParamOrDefault("transport_layer_factory", "ns3::SAGTransportSocketFactory"));
  //app->SetTransportSocket(m_basicSimulation->GetConfigParamOrDefault("transport_layer_socket", "ns3::SAGTransportSocketImpl"));
  app->SetMaxPayLoadSizeByte(DEFAULT_PACKET_SIZE);  // 1500 - IP头(20) - TCP头(20) = 1460 (Bytes)
  if(m_traceType != TRACE_TYPE_INVALID && m_codecType != SYNCODEC_TYPE_INVALID){
	  app->SetTrace(m_traceType);
	  app->SetCodecType(m_codecType);
  }
  node->AddApplication ((Ptr<Application>)app);
  return (Ptr<Application>)app;
}

SAGApplicationHelperQuicSink::SAGApplicationHelperQuicSink (Ptr<ns3::BasicSimulation> basicSimulation, std::string protocol, Address address, bool enableFlowLoggingToFile)
:SAGApplicationHelper(basicSimulation)
{
  m_quicSinkFactory.SetTypeId (basicSimulation->GetConfigParamOrDefault("application_layer_protocal_quic_sink", "ns3::SAGApplicationLayerQuicSink"));
  SetAttribute (m_quicSinkFactory, "Protocol", StringValue (protocol));
  SetAttribute (m_quicSinkFactory, "Local", AddressValue (address));
  SetAttribute (m_quicSinkFactory, "EnableFlowLoggingToFile", BooleanValue (enableFlowLoggingToFile));
}

Ptr<Application>
SAGApplicationHelperQuicSink::InstallPriv (Ptr<Node> node) const
{
  Ptr<SAGApplicationLayer> app = m_quicSinkFactory.Create<SAGApplicationLayer> ();
  //app->SetTransportFactory(m_basicSimulation->GetConfigParamOrDefault("transport_layer_factory", "ns3::SAGTransportSocketFactory"));
  //app->SetTransportSocket(m_basicSimulation->GetConfigParamOrDefault("transport_layer_socket", "ns3::SAGTransportSocketImpl"));
  node->AddApplication ((Ptr<Application>)app);
  return (Ptr<Application>)app;
}


//ftp sag application helper

SAGApplicationHelperFTPSend::SAGApplicationHelperFTPSend (Ptr<ns3::BasicSimulation> basicSimulation, SAGBurstInfoFTP entry, uint16_t dstPort, bool enableFlowLoggingToFile)
:SAGApplicationHelper(basicSimulation)
{
  m_ftpSendFactory.SetTypeId ("ns3::FTPSend");
  SetAttribute (m_ftpSendFactory, "Protocol", StringValue ("ns3::TcpSocketFactory"));
  SetAttribute (m_ftpSendFactory, "DestinationPort", UintegerValue (dstPort));
  SetAttribute (m_ftpSendFactory, "MaxBytes", UintegerValue (entry.GetSizeByte()));
  SetAttribute (m_ftpSendFactory, "FTPFlowId", UintegerValue (entry.GetFtpFlowId()));
  SetAttribute (m_ftpSendFactory, "EnableFlowLoggingToFile", BooleanValue (enableFlowLoggingToFile));

  SetAttribute (m_ftpSendFactory, "BaseLogsDir", StringValue (basicSimulation->GetLogsDir()));
  SetAttribute (m_ftpSendFactory, "BaseDir", StringValue (basicSimulation->GetRunDir()));
}

Ptr<Application>
SAGApplicationHelperFTPSend::InstallPriv (Ptr<Node> node) const
{
  Ptr<SAGApplicationLayer> app = m_ftpSendFactory.Create<SAGApplicationLayer> ();
  //app->SetTransportFactory(m_basicSimulation->GetConfigParamOrDefault("transport_layer_factory", "ns3::SAGTransportSocketFactory"));
  //app->SetTransportSocket(m_basicSimulation->GetConfigParamOrDefault("transport_layer_socket", "ns3::SAGTransportSocketImpl"));
  app->SetMaxPayLoadSizeByte(DEFAULT_PACKET_SIZE);  // 1500 - IP头(20) - TCP头(20) = 1460 (Bytes)
  node->AddApplication ((Ptr<Application>)app);
  return (Ptr<Application>)app;
}

SAGApplicationHelperFTPSink::SAGApplicationHelperFTPSink (Ptr<ns3::BasicSimulation> basicSimulation, std::string protocol, Address address, bool enableFlowLoggingToFile)
:SAGApplicationHelper(basicSimulation)
{
  m_ftpSinkFactory.SetTypeId ("ns3::FTPSink");
  SetAttribute (m_ftpSinkFactory, "Protocol", StringValue (protocol));
  SetAttribute (m_ftpSinkFactory, "Local", AddressValue (address));
  SetAttribute (m_ftpSinkFactory, "EnableFlowLoggingToFile", BooleanValue (enableFlowLoggingToFile));

}

Ptr<Application>
SAGApplicationHelperFTPSink::InstallPriv (Ptr<Node> node) const
{
  Ptr<SAGApplicationLayer> app = m_ftpSinkFactory.Create<SAGApplicationLayer> ();
  //app->SetTransportFactory(m_basicSimulation->GetConfigParamOrDefault("transport_layer_factory", "ns3::SAGTransportSocketFactory"));
  //app->SetTransportSocket(m_basicSimulation->GetConfigParamOrDefault("transport_layer_socket", "ns3::SAGTransportSocketImpl"));
  node->AddApplication ((Ptr<Application>)app);
  return (Ptr<Application>)app;
}




} // namespace ns3
