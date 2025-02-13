/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 * (based on point-to-point helper)
 * Author: Andre Aguas         March 2020
 *         Simon               2020
 */


#include "ns3/abort.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/point-to-point-laser-net-device.h"
#include "ns3/point-to-point-laser-channel.h"
#include "ns3/queue.h"
#include "ns3/net-device-queue-interface.h"
#include "ns3/config.h"
#include "ns3/packet.h"
#include "ns3/names.h"
#include "ns3/string.h"

#ifdef NS3_MPI
#include "ns3/mpi-interface.h"
#include "ns3/mpi-receiver.h"
//#include "ns3/point-to-point-remote-channel.h"
#include "ns3/point-to-point-laser-remote-channel.h"
#endif

#include "ns3/trace-helper.h"
#include "point-to-point-laser-helper.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PointToPointLaserHelper");

PointToPointLaserHelper::PointToPointLaserHelper ()
{
  m_queueFactory.SetTypeId ("ns3::DropTailQueue<Packet>");
  m_deviceFactory.SetTypeId ("ns3::PointToPointLaserNetDevice");
  m_channelFactory.SetTypeId ("ns3::PointToPointLaserChannel");
  m_remoteChannelFactory.SetTypeId ("ns3::PointToPointLaserRemoteChannel");


}

PointToPointLaserHelper::PointToPointLaserHelper (Ptr<ns3::BasicSimulation> basicSimulation)
{
	m_basicSimulation = basicSimulation;
	m_queueFactory.SetTypeId ("ns3::DropTailQueue<Packet>");

	std::string netdevice_Type = "";
	std::string channel_Type = "";
	std::string remote_channel_Type = "";
	while(true){
		bool ppp = parse_boolean(basicSimulation->GetConfigParamOrDefault("enable_p2p_protocol", "false"));
		if(ppp){
			netdevice_Type = "ns3::PointToPointLaserNetDevice";
			channel_Type = "ns3::PointToPointLaserChannel";
			remote_channel_Type = "ns3::PointToPointLaserRemoteChannel";
			break;
		}
		bool hdlc = parse_boolean(basicSimulation->GetConfigParamOrDefault("enable_hdlc_protocol", "false"));
		if(hdlc){
			netdevice_Type = "ns3::HdlcNetDevice";
			channel_Type = "ns3::HdlcChannel";
			remote_channel_Type = "ns3::PointToPointLaserRemoteChannel";
			break;
		}

	}
	if(netdevice_Type == "" || channel_Type == ""){
		throw std::runtime_error("No Link Layer Protocol Set");
	}
	bool hdlc = parse_boolean(basicSimulation->GetConfigParamOrDefault("enable_hdlc_protocol", "false"));
	if(hdlc && MpiInterface::IsEnabled ()){
		throw std::runtime_error("Todo: test hdlc in distributed mode");
	}

	m_deviceFactory.SetTypeId (netdevice_Type);
	m_channelFactory.SetTypeId (channel_Type);
	m_remoteChannelFactory.SetTypeId (remote_channel_Type);
}

void 
PointToPointLaserHelper::SetQueue (std::string type,
                                   std::string n1, const AttributeValue &v1,
                                   std::string n2, const AttributeValue &v2,
                                   std::string n3, const AttributeValue &v3,
                                   std::string n4, const AttributeValue &v4)
{
  QueueBase::AppendItemTypeIfNotPresent (type, "Packet");

  m_queueFactory.SetTypeId (type);
  m_queueFactory.Set (n1, v1);
  m_queueFactory.Set (n2, v2);
  m_queueFactory.Set (n3, v3);
  m_queueFactory.Set (n4, v4);
}

void 
PointToPointLaserHelper::SetDeviceAttribute (std::string n1, const AttributeValue &v1)
{
  m_deviceFactory.Set (n1, v1);
}

void 
PointToPointLaserHelper::SetChannelAttribute (std::string n1, const AttributeValue &v1)
{
  m_channelFactory.Set (n1, v1);
  //m_remoteChannelFactory.Set (n1, v1);
}

NetDeviceContainer 
PointToPointLaserHelper::Install (NodeContainer c)
{
  NS_ASSERT (c.GetN () == 2);
  return Install (c.Get (0), c.Get (1));
}

NetDeviceContainer 
PointToPointLaserHelper::Install (Ptr<Node> a, Ptr<Node> b)
{
  // set the initial delay of the channel as the delay estimation for the lookahead of the
  // distributed scheduler
  Ptr<MobilityModel> aMobility = a->GetObject<MobilityModel>();
  Ptr<MobilityModel> bMobility = b->GetObject<MobilityModel>();
  double propagation_speed(299792458.0);
  double distance = aMobility->GetDistanceFrom (bMobility);
  double delay = distance / propagation_speed;
  SetChannelAttribute("Delay", StringValue(std::to_string(delay) + "s"));

  NetDeviceContainer container;

  Ptr<SAGLinkLayer> devA = m_deviceFactory.Create<SAGLinkLayer> ();
  devA->SetAddress (Mac48Address::Allocate ());
  devA->SetDestinationNode(b);
  a->AddDevice (devA);
  Ptr<Queue<Packet> > queueA = m_queueFactory.Create<Queue<Packet> > ();
  devA->SetQueue (queueA);
  Ptr<SAGLinkLayer> devB = m_deviceFactory.Create<SAGLinkLayer> ();
  devB->SetAddress (Mac48Address::Allocate ());
  devB->SetDestinationNode(a);
  b->AddDevice (devB);
  Ptr<Queue<Packet> > queueB = m_queueFactory.Create<Queue<Packet> > ();
  devB->SetQueue (queueB);

  // Aggregate NetDeviceQueueInterface objects
  Ptr<NetDeviceQueueInterface> ndqiA = CreateObject<NetDeviceQueueInterface> ();
  ndqiA->GetTxQueue (0)->ConnectQueueTraces (queueA);
  devA->AggregateObject (ndqiA);
  Ptr<NetDeviceQueueInterface> ndqiB = CreateObject<NetDeviceQueueInterface> ();
  ndqiB->GetTxQueue (0)->ConnectQueueTraces (queueB);
  devB->AggregateObject (ndqiB);

  // Distributed mode
  //NS_ABORT_MSG_IF(MpiInterface::IsEnabled(), "Distributed mode is not currently supported for point-to-point lasers.");

  // Distributed mode is not currently supported, enable the below if it is:
  // If MPI is enabled, we need to see if both nodes have the same system id
  // (rank), and the rank is the same as this instance.  If both are true,
  // use a normal p2p channel, otherwise use a remote channel
  Ptr<SAGPhysicalLayer> channel = 0;

#ifdef NS3_MPI
  bool useNormalChannel = true;

  if (MpiInterface::IsEnabled ()) {
      uint32_t n1SystemId = a->GetSystemId ();
      uint32_t n2SystemId = b->GetSystemId ();
      uint32_t currSystemId = MpiInterface::GetSystemId ();
//      if (n1SystemId != currSystemId || n2SystemId != currSystemId) {
//          useNormalChannel = false;
//      }
		if (n1SystemId == currSystemId && n2SystemId != currSystemId) {
			useNormalChannel = false;
		}
		else if(n1SystemId != currSystemId && n2SystemId == currSystemId){
			useNormalChannel = false;
		}
		else{
		}
  }
  if (useNormalChannel) {
    channel = m_channelFactory.Create<SAGPhysicalLayer> ();
  }
  else {
    channel = m_remoteChannelFactory.Create<SAGPhysicalLayer>();
    Ptr<MpiReceiver> mpiRecA = CreateObject<MpiReceiver> ();
    Ptr<MpiReceiver> mpiRecB = CreateObject<MpiReceiver> ();
    mpiRecA->SetReceiveCallback (MakeCallback (&SAGLinkLayer::Receive, devA));
    mpiRecB->SetReceiveCallback (MakeCallback (&SAGLinkLayer::Receive, devB));
    devA->AggregateObject (mpiRecA);
    devB->AggregateObject (mpiRecB);
  }
#else
  channel = m_channelFactory.Create<SAGPhysicalLayer> ();
#endif
  // Create and attach channel
  //Ptr<PointToPointLaserChannel> channel = m_channelFactory.Create<PointToPointLaserChannel> ();
  devA->Attach (channel);
  devB->Attach (channel);
  container.Add (devA);
  container.Add (devB);

  return container;
}

void
PointToPointLaserHelper::EnablePcapInternal (std::string prefix, Ptr<NetDevice> nd, bool promiscuous, bool explicitFilename)
{
  //
  // All of the Pcap enable functions vector through here including the ones
  // that are wandering through all of devices on perhaps all of the nodes in
  // the system.  We can only deal with devices of type PointToPointNetDevice.
  //
  Ptr<SAGLinkLayer> device = nd->GetObject<SAGLinkLayer> ();
  if (device == 0)
    {
      NS_LOG_INFO ("PointToPointLaserHelper::EnablePcapInternal(): Device " << device << " not of type ns3::PointToPointLaserNetDevice");
      return;
    }

  PcapHelper pcapHelper;

  std::string filename;
  if (explicitFilename)
    {
      filename = prefix;
    }
  else
    {
      filename = pcapHelper.GetFilenameFromDevice (prefix, device);
    }

  bool use_lldp = parse_boolean(m_basicSimulation->GetConfigParamOrDefault("enable_lldp", "false"));
  if(use_lldp){
		//Mengy's::1207替换
		Ptr<PcapFileWrapper> file = pcapHelper.CreateFile (filename, std::ios::out,
														   PcapHelper::DLT_EN10MB);
		/*Ptr<PcapFileWrapper> file = pcapHelper.CreateFile (filename, std::ios::out,
														   PcapHelper::DLT_PPP);*/
		pcapHelper.HookDefaultSink<SAGLinkLayer> (device, "PromiscSniffer", file);
  }
  else{
	    Ptr<PcapFileWrapper> file = pcapHelper.CreateFile (filename, std::ios::out,
	                                                       PcapHelper::DLT_PPP);
	    pcapHelper.HookDefaultSink<SAGLinkLayer> (device, "PromiscSniffer", file);
  }


}

void
PointToPointLaserHelper::EnableAsciiInternal (
  Ptr<OutputStreamWrapper> stream,
  std::string prefix,
  Ptr<NetDevice> nd,
  bool explicitFilename)
{
  //
  // All of the ascii enable functions vector through here including the ones
  // that are wandering through all of devices on perhaps all of the nodes in
  // the system.  We can only deal with devices of type PointToPointNetDevice.
  //
  Ptr<SAGLinkLayer> device = nd->GetObject<SAGLinkLayer> ();
  if (device == 0)
    {
      NS_LOG_INFO ("PointToPointHelper::EnableAsciiInternal(): Device " << device <<
                   " not of type ns3::PointToPointLaserNetDevice");
      return;
    }

  //
  // Our default trace sinks are going to use packet printing, so we have to
  // make sure that is turned on.
  //
  Packet::EnablePrinting ();

  //
  // If we are not provided an OutputStreamWrapper, we are expected to create
  // one using the usual trace filename conventions and do a Hook*WithoutContext
  // since there will be one file per context and therefore the context would
  // be redundant.
  //
  if (stream == 0)
    {
      //
      // Set up an output stream object to deal with private ofstream copy
      // constructor and lifetime issues.  Let the helper decide the actual
      // name of the file given the prefix.
      //
      AsciiTraceHelper asciiTraceHelper;

      std::string filename;
      if (explicitFilename)
        {
          filename = prefix;
        }
      else
        {
          filename = asciiTraceHelper.GetFilenameFromDevice (prefix, device);
        }

      Ptr<OutputStreamWrapper> theStream = asciiTraceHelper.CreateFileStream (filename);

      //
      // The MacRx trace source provides our "r" event.
      //
      asciiTraceHelper.HookDefaultReceiveSinkWithoutContext<SAGLinkLayer> (device, "MacRx", theStream);

      //
      // The "+", '-', and 'd' events are driven by trace sources actually in the
      // transmit queue.
      //
      Ptr<Queue<Packet> > queue = device->GetQueue ();
      asciiTraceHelper.HookDefaultEnqueueSinkWithoutContext<Queue<Packet> > (queue, "Enqueue", theStream);
      asciiTraceHelper.HookDefaultDropSinkWithoutContext<Queue<Packet> > (queue, "Drop", theStream);
      asciiTraceHelper.HookDefaultDequeueSinkWithoutContext<Queue<Packet> > (queue, "Dequeue", theStream);

      // PhyRxDrop trace source for "d" event
      asciiTraceHelper.HookDefaultDropSinkWithoutContext<SAGLinkLayer> (device, "PhyRxDrop", theStream);

      return;
    }

  //
  // If we are provided an OutputStreamWrapper, we are expected to use it, and
  // to providd a context.  We are free to come up with our own context if we
  // want, and use the AsciiTraceHelper Hook*WithContext functions, but for
  // compatibility and simplicity, we just use Config::Connect and let it deal
  // with the context.
  //
  // Note that we are going to use the default trace sinks provided by the
  // ascii trace helper.  There is actually no AsciiTraceHelper in sight here,
  // but the default trace sinks are actually publicly available static
  // functions that are always there waiting for just such a case.
  //
  uint32_t nodeid = nd->GetNode ()->GetId ();
  uint32_t deviceid = nd->GetIfIndex ();
  std::ostringstream oss;

  oss << "/NodeList/" << nd->GetNode ()->GetId () << "/DeviceList/" << deviceid << "/$ns3::SAGLinkLayer/MacRx";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultReceiveSinkWithContext, stream));

  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::SAGLinkLayer/TxQueue/Enqueue";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultEnqueueSinkWithContext, stream));

  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::SAGLinkLayer/TxQueue/Dequeue";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultDequeueSinkWithContext, stream));

  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::SAGLinkLayer/TxQueue/Drop";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultDropSinkWithContext, stream));

  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::SAGLinkLayer/PhyRxDrop";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultDropSinkWithContext, stream));
}

} // namespace ns3
