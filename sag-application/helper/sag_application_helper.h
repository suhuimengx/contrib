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
#ifndef SAG_APPLICATION_HELPER_H
#define SAG_APPLICATION_HELPER_H

#include <stdint.h>
#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/sag_application_layer.h"
#include "ns3/basic-simulation.h"
#include "ns3/sag_rtp_constants.h"

namespace ns3 {

class SAGApplicationHelper
{
public:
	SAGApplicationHelper (Ptr<ns3::BasicSimulation> basicSimulation);
	SAGApplicationHelper (Ptr<ns3::BasicSimulation> basicSimulation, std::string codecType, std::string traceType);
	void SetAttribute (ObjectFactory& factory, std::string name, const AttributeValue &value);
	ApplicationContainer Install (Ptr<Node> node) const;
	ApplicationContainer Install (NodeContainer c) const;

protected:
  Ptr<ns3::BasicSimulation> m_basicSimulation;
  TraceType m_traceType = TRACE_TYPE_INVALID;
  SyncodecType m_codecType = SYNCODEC_TYPE_INVALID;

private:
  virtual Ptr<Application> InstallPriv (Ptr<Node> node) const;
  ObjectFactory m_factory;
};


class SAGApplicationHelperUdp : public SAGApplicationHelper
{
public:
  //SAGApplicationHelperUdp (Ptr<ns3::BasicSimulation> basicSimulation);
  SAGApplicationHelperUdp (Ptr<ns3::BasicSimulation> basicSimulation, std::string codecType, std::string traceType);
  SAGApplicationHelperUdp (Ptr<ns3::BasicSimulation> basicSimulation);

private:
  virtual Ptr<Application> InstallPriv (Ptr<Node> node) const;


  ObjectFactory m_udpFactory;
};


class SAGApplicationHelperRtpSender: public SAGApplicationHelper
{
public:
  SAGApplicationHelperRtpSender (Ptr<ns3::BasicSimulation> basicSimulation, SAGBurstInfoRtp entry, uint16_t port);

private:
  virtual Ptr<Application> InstallPriv (Ptr<Node> node) const;

  ObjectFactory m_rtpsendFactory;
};

class SAGApplicationHelperRtpReceiver: public SAGApplicationHelper
{
public:
  SAGApplicationHelperRtpReceiver (Ptr<ns3::BasicSimulation> basicSimulation, uint16_t port);

private:
  virtual Ptr<Application> InstallPriv (Ptr<Node> node) const;

  ObjectFactory m_rtpreceiveFactory;
};



class SAGApplicationHelperTcpSend : public SAGApplicationHelper
{
public:
  SAGApplicationHelperTcpSend (Ptr<ns3::BasicSimulation> basicSimulation, SAGBurstInfoTcp entry, uint16_t dstPort, bool enableFlowLoggingToFile);

private:
  virtual Ptr<Application> InstallPriv (Ptr<Node> node) const;

  ObjectFactory m_tcpSendFactory;
};


class SAGApplicationHelperTcpSink : public SAGApplicationHelper
{
  public:
  SAGApplicationHelperTcpSink (Ptr<ns3::BasicSimulation> basicSimulation, std::string protocol, Address address, bool enableFlowLoggingToFile);

private:
  virtual Ptr<Application> InstallPriv (Ptr<Node> node) const;

  ObjectFactory m_tcpSinkFactory;
};

//Mengy's:: Quic Send Application
class SAGApplicationHelperQuicSend : public SAGApplicationHelper
{
public:
  SAGApplicationHelperQuicSend (Ptr<ns3::BasicSimulation> basicSimulation, SAGBurstInfoTcp entry, uint16_t dstPort, bool enableFlowLoggingToFile);

private:
  virtual Ptr<Application> InstallPriv (Ptr<Node> node) const;

  ObjectFactory m_quicSendFactory;
};

//Mengy's:: Quic Sink Application
class SAGApplicationHelperQuicSink : public SAGApplicationHelper
{
  public:
  SAGApplicationHelperQuicSink (Ptr<ns3::BasicSimulation> basicSimulation, std::string protocol, Address address, bool enableFlowLoggingToFile);

private:
  virtual Ptr<Application> InstallPriv (Ptr<Node> node) const;

  ObjectFactory m_quicSinkFactory;
};



class SAGApplicationHelperFTPSend : public SAGApplicationHelper
{
public:
	SAGApplicationHelperFTPSend (Ptr<ns3::BasicSimulation> basicSimulation, SAGBurstInfoFTP entry, uint16_t dstPort, bool enableFlowLoggingToFile);

private:
  virtual Ptr<Application> InstallPriv (Ptr<Node> node) const;

  ObjectFactory m_ftpSendFactory;
};


class SAGApplicationHelperFTPSink : public SAGApplicationHelper
{
  public:
	SAGApplicationHelperFTPSink (Ptr<ns3::BasicSimulation> basicSimulation, std::string protocol, Address address, bool enableFlowLoggingToFile);

private:
  virtual Ptr<Application> InstallPriv (Ptr<Node> node) const;

  ObjectFactory m_ftpSinkFactory;
};

} // namespace ns3

#endif /* SAG_APPLICATION_HELPER_H */
