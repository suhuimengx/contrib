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


#include "ns3/abort.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/queue.h"
#include "ns3/net-device-queue-interface.h"
#include "ns3/config.h"
#include "ns3/packet.h"
#include "ns3/names.h"
#include "ns3/string.h"
#include "ns3/mpi-interface.h"
#include "ns3/mpi-receiver.h"

#include "ns3/trace-helper.h"
#include "ns3/sag_physical_gsl_helper.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SAGPhysicalGSLHelper");

SAGPhysicalGSLHelper::SAGPhysicalGSLHelper (Ptr<ns3::BasicSimulation> basicSimulation)
{
	m_basicSimulation = basicSimulation;
	m_queueFactory.SetTypeId ("ns3::DropTailQueue<Packet>");
	std::string netdevice_Type = "";
	std::string channel_Type = "";
	while(true){
		bool fdma = parse_boolean(m_basicSimulation->GetConfigParamOrDefault("enable_fdma", "false"));
		if(fdma){
			netdevice_Type = "ns3::SAGLinkLayerGSL";
			channel_Type = "ns3::SAGPhysicalLayerGSL";
			break;
		}
		bool aloha = parse_boolean(m_basicSimulation->GetConfigParamOrDefault("enable_aloha", "false"));
		if(aloha){
			netdevice_Type = "ns3::SAGAlohaNetDevice";
			channel_Type = "ns3::SAGAlohaChannel";
			break;
		}
		bool csma = parse_boolean(m_basicSimulation->GetConfigParamOrDefault("enable_csma", "false"));
		if(csma){
			netdevice_Type = "ns3::SAGCsmaNetDevice";
			channel_Type = "ns3::SAGCsmaChannel";
			break;
		}

	}
	if(netdevice_Type == "" || channel_Type == ""){
		throw std::runtime_error("No Mac Protocol Set");
	}

	m_deviceFactory.SetTypeId (netdevice_Type);
	m_channelFactory.SetTypeId (channel_Type);

	//xhqin
	//<! physical_global_attribute.json
	std::string filename = basicSimulation->GetRunDir() + "/config_protocol/physical_global_attribute.json";
	// Check that the file exists
	if (!file_exists(filename)) {
		throw std::runtime_error(format_string("File %s does not exist.", filename.c_str()));
	}
	else{
		ifstream jfile(filename);
		if (jfile) {
			json j;
			jfile >> j;
			m_deviceFactory.Set ("BaseDir", StringValue(basicSimulation->GetRunDir()));
			double trans_power= j["antenna"]["transmit_power_dbm"];
			m_deviceFactory.Set ("TxPower", DoubleValue(trans_power));
			double trans_ant_gain= j["antenna"]["transmit_antenna_gain_dbi"];
			m_deviceFactory.Set ("TxAntennaGain", DoubleValue(trans_ant_gain));
			double frequency= j["antenna"]["frequency_hz"];
			m_channelFactory.Set ("Frequency", DoubleValue(frequency));
			std::string scenario = j["antenna"]["scenario"];
			m_channelFactory.Set ("Scenario", StringValue(scenario));
			bool enable_ber = j["antenna"]["enable_ber"];
			//std::string enable_ber = remove_start_end_double_quote_if_present(trim(j[""]["enable_ber"]));
			m_channelFactory.Set ("Enable_ber", BooleanValue(enable_ber));
			bool DVBS2 = j["DVB-S2"]["enable_DVB-S2_protocol"];
			if(DVBS2){
				std::string MCS = remove_start_end_double_quote_if_present(trim(j["DVB-S2"]["MCS"]));
				m_deviceFactory.Set ("Protocol", StringValue("DVB-S2"));
				m_deviceFactory.Set ("MCS", StringValue(MCS));
			}
			//bool DVBS2X = parse_boolean(m_basicSimulation->GetConfigParamOrDefault("enable_DVB-S2X_protocol", "false"));
			bool DVBS2X = j["DVB-S2X"]["enable_DVB-S2X_protocol"];
			if(DVBS2X){
				std::string MCS = remove_start_end_double_quote_if_present(trim(j["DVB-S2X"]["MCS"]));
				m_deviceFactory.Set ("Protocol", StringValue("DVB-S2X"));
				m_deviceFactory.Set ("MCS", StringValue(MCS));
			}
			//bool DVBRCS2 = parse_boolean(m_basicSimulation->GetConfigParamOrDefault("enable_DVB-RCS2_protocol", "false"));
			//if(DVBRCS2){
			std::string Waveform = to_string(j["DVB-RCS2"]["Waveform"]);
			//m_deviceFactory.Set ("Protocol", StringValue("DVB-RCS2"));
			//double Waveform1= stod(Waveform);
			m_deviceFactory.Set ("Waveform", StringValue(Waveform));
			//}
		}
		else{
			throw std::runtime_error(format_string("File %s could not be read.", filename.c_str()));
		}
		jfile.close();
	}
}

void
SAGPhysicalGSLHelper::SetQueue (std::string type,
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
SAGPhysicalGSLHelper::SetDeviceAttribute (std::string n1, const AttributeValue &v1)
{
  m_deviceFactory.Set (n1, v1);
}

void
SAGPhysicalGSLHelper::SetChannelAttribute (std::string n1, const AttributeValue &v1)
{
  m_channelFactory.Set (n1, v1);
}

NetDeviceContainer
SAGPhysicalGSLHelper::SatelliteInstall (NodeContainer satellites){

	// All gsl network devices satellites owning
	NetDeviceContainer allNetDevices;

	// Satellite network devices
	for (size_t sid = 0; sid < satellites.GetN(); sid++)  {
		Ptr<Node> sat = satellites.Get(sid);
		// Primary channel
		Ptr<SAGPhysicalLayerGSL> channel;// = m_channelFactory.Create<SAGPhysicalLayerGSL> ();


		// Create device todo
		Ptr<SAGLinkLayerGSL> dev = m_deviceFactory.Create<SAGLinkLayerGSL>();

		// Set unique MAC address
		dev->SetAddress (Mac48Address::Allocate ());

		// Add device to the node
		sat->AddDevice (dev);

		// Set device queue
		Ptr<Queue<Packet> > queue = m_queueFactory.Create<Queue<Packet>>();
		dev->SetQueue (queue);

		// Aggregate NetDeviceQueueInterface objects to connect
		// the device queue to the interface (used by traffic control layer)
		Ptr<NetDeviceQueueInterface> ndqi = CreateObject<NetDeviceQueueInterface>();
		ndqi->GetTxQueue (0)->ConnectQueueTraces (queue);
		dev->AggregateObject (ndqi);

		// Aggregate MPI receivers // TODO: Why?
		//Ptr<MpiReceiver> mpiRec = CreateObject<MpiReceiver> ();
		//mpiRec->SetReceiveCallback (MakeCallback (&SAGLinkLayerGSL::Receive, dev));
		//dev->AggregateObject(mpiRec);
		#ifdef NS3_MPI
		  bool useNormalChannel = true;
		  if (MpiInterface::IsEnabled () && (m_basicSimulation->GetNodeAssignmentAlogirthm() == "algorithm1" || m_basicSimulation->GetNodeAssignmentAlogirthm() == "customize")) {
			  useNormalChannel = false;
		  }
		  if (useNormalChannel) {
			channel = m_channelFactory.Create<SAGPhysicalLayerGSL> ();
		  }
		  else {
			channel = m_channelFactory.Create<SAGPhysicalLayerGSL> ();
			Ptr<MpiReceiver> mpiRecA = CreateObject<MpiReceiver> ();
			mpiRecA->SetReceiveCallback (MakeCallback (&SAGLinkLayerGSL::Receive, dev));
			dev->AggregateObject (mpiRecA);
		  }
		#else
		  channel = m_channelFactory.Create<SAGPhysicalLayerGSL> ();
		#endif


		// Attach to channel
		dev->Attach (channel, 1);

		allNetDevices.Add(dev);

	}

	return allNetDevices;

}

NetDeviceContainer
SAGPhysicalGSLHelper::GroundStationInstall (NodeContainer groundStations){
	// All gsl network devices satellites owning
	NetDeviceContainer allNetDevices;

	// Satellite network devices
	for (size_t sid = 0; sid < groundStations.GetN(); sid++)  {
		Ptr<Node> sat = groundStations.Get(sid);

		// Create device todo
		Ptr<SAGLinkLayerGSL> dev = m_deviceFactory.Create<SAGLinkLayerGSL>();

		// Set unique MAC address
		dev->SetAddress (Mac48Address::Allocate ());

		// Add device to the node
		sat->AddDevice (dev);

		// Set device queue
		Ptr<Queue<Packet> > queue = m_queueFactory.Create<Queue<Packet>>();
		dev->SetQueue (queue);

		// Aggregate NetDeviceQueueInterface objects to connect
		// the device queue to the interface (used by traffic control layer)
		Ptr<NetDeviceQueueInterface> ndqi = CreateObject<NetDeviceQueueInterface>();
		ndqi->GetTxQueue (0)->ConnectQueueTraces (queue);
		dev->AggregateObject (ndqi);

		// Aggregate MPI receivers // TODO: Why?
		//Ptr<MpiReceiver> mpiRec = CreateObject<MpiReceiver> ();
		//mpiRec->SetReceiveCallback (MakeCallback (&SAGLinkLayerGSL::Receive, dev));
		//dev->AggregateObject(mpiRec);


		#ifdef NS3_MPI
			if (MpiInterface::IsEnabled () && (m_basicSimulation->GetNodeAssignmentAlogirthm() == "algorithm1" || m_basicSimulation->GetNodeAssignmentAlogirthm() == "customize")) {
				Ptr<MpiReceiver> mpiRecA = CreateObject<MpiReceiver> ();
				mpiRecA->SetReceiveCallback (MakeCallback (&SAGLinkLayerGSL::Receive, dev));
				dev->AggregateObject (mpiRecA);
			}
		#endif


		// channel attaching to the accessing satellite
		//dev->Attach (channel);

		allNetDevices.Add(dev);

	}

	return allNetDevices;

}

void
SAGPhysicalGSLHelper::EnablePcapInternal (std::string prefix, Ptr<NetDevice> nd, bool promiscuous, bool explicitFilename)
{
  //
  // All of the Pcap enable functions vector through here including the ones
  // that are wandering through all of devices on perhaps all of the nodes in
  // the system.  We can only deal with devices of type PointToPointNetDevice.
  //
  Ptr<LoopbackNetDevice> device1 = nd->GetObject<LoopbackNetDevice> ();
  if (device1 != 0)
    {
      //NS_LOG_INFO ("PointToPointLaserHelper::EnablePcapInternal(): Device " << device << " not of type ns3::PointToPointLaserNetDevice");
      return;
    }

  Ptr<NetDevice> device = nd;
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
    	    pcapHelper.HookDefaultSink<NetDevice> (device, "PromiscSniffer", file);
    }
    else{
    	  Ptr<PcapFileWrapper> file = pcapHelper.CreateFile (filename, std::ios::out,
    	                                                     PcapHelper::DLT_PPP);
    	  pcapHelper.HookDefaultSink<NetDevice> (device, "PromiscSniffer", file);
    }
}

void
SAGPhysicalGSLHelper::EnableAsciiInternal (
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
  Ptr<SAGLinkLayerGSL> device = nd->GetObject<SAGLinkLayerGSL> ();
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
      asciiTraceHelper.HookDefaultReceiveSinkWithoutContext<SAGLinkLayerGSL> (device, "MacRx", theStream);

      //
      // The "+", '-', and 'd' events are driven by trace sources actually in the
      // transmit queue.
      //
      Ptr<Queue<Packet> > queue = device->GetQueue ();
      asciiTraceHelper.HookDefaultEnqueueSinkWithoutContext<Queue<Packet> > (queue, "Enqueue", theStream);
      asciiTraceHelper.HookDefaultDropSinkWithoutContext<Queue<Packet> > (queue, "Drop", theStream);
      asciiTraceHelper.HookDefaultDequeueSinkWithoutContext<Queue<Packet> > (queue, "Dequeue", theStream);

      // PhyRxDrop trace source for "d" event
      asciiTraceHelper.HookDefaultDropSinkWithoutContext<SAGLinkLayerGSL> (device, "PhyRxDrop", theStream);

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

  oss << "/NodeList/" << nd->GetNode ()->GetId () << "/DeviceList/" << deviceid << "/$ns3::PointToPointLaserNetDevice/MacRx";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultReceiveSinkWithContext, stream));

  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::PointToPointLaserNetDevice/TxQueue/Enqueue";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultEnqueueSinkWithContext, stream));

  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::PointToPointLaserNetDevice/TxQueue/Dequeue";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultDequeueSinkWithContext, stream));

  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::PointToPointLaserNetDevice/TxQueue/Drop";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultDropSinkWithContext, stream));

  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::PointToPointLaserNetDevice/PhyRxDrop";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultDropSinkWithContext, stream));
}


} // namespace ns3




