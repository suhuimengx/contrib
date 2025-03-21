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


#ifndef SAG_PHYSICAL_GSL_HELPER_H
#define SAG_PHYSICAL_GSL_HELPER_H

#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/sag_physical_layer_gsl.h"
#include "ns3/sag_link_layer_gsl.h"
#include "ns3/basic-simulation.h"
namespace ns3 {

class NetDevice;
class Node;

class SAGPhysicalGSLHelper: public PcapHelperForDevice,
							public AsciiTraceHelperForDevice
{
public:
	SAGPhysicalGSLHelper (Ptr<ns3::BasicSimulation> basicSimulation);
	virtual ~SAGPhysicalGSLHelper () {}

	// Set device and channel attributes
	void SetQueue (std::string type,
				 std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
				 std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
				 std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
				 std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue ());
	void SetDeviceAttribute (std::string name, const AttributeValue &value);
	void SetChannelAttribute (std::string name, const AttributeValue &value);

	// Installers
//	NetDeviceContainer Install (NodeContainer satellites, NodeContainer gss, std::vector<std::tuple<int32_t, double>>& node_gsl_if_info);
//	Ptr<SAGLinkLayerGSL> Install (Ptr<Node> node, Ptr<SAGPhysicalLayerGSL> channel);

	NetDeviceContainer SatelliteInstall (NodeContainer satellites);
	NetDeviceContainer GroundStationInstall (NodeContainer groundStations);
	//NetDeviceContainer GSLSwitch (Ptr<Node> satOutOfSight, Ptr<Node> satInSight, Ptr<Node> gs);

private:
  virtual void EnablePcapInternal (std::string prefix, Ptr<NetDevice> nd, bool promiscuous, bool explicitFilename);

  virtual void EnableAsciiInternal (
    Ptr<OutputStreamWrapper> stream,
    std::string prefix,
    Ptr<NetDevice> nd,
    bool explicitFilename);

private:
	Ptr<ns3::BasicSimulation> m_basicSimulation;
	ObjectFactory m_queueFactory;         //!< Queue Factory
	ObjectFactory m_channelFactory;       //!< Channel Factory
	ObjectFactory m_deviceFactory;        //!< Device Factory

};

} // namespace ns3

#endif /* GSL_HELPER_H */

