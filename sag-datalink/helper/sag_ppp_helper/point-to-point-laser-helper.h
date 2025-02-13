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
 * 
 */


#ifndef POINT_TO_POINT_LASER_HELPER_H
#define POINT_TO_POINT_LASER_HELPER_H

#include "ns3/object-factory.h"
#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/trace-helper.h"
#include "ns3/basic-simulation.h"

namespace ns3 {

class NetDevice;
class Node;

class PointToPointLaserHelper: public PcapHelperForDevice,
								public AsciiTraceHelperForDevice
{
public:

  // Constructors
  PointToPointLaserHelper ();
  PointToPointLaserHelper (Ptr<ns3::BasicSimulation> basicSimulation);
  void SetBasicSimulation (Ptr<ns3::BasicSimulation> basicSimulation);

  // Set point-to-point laser device and channel attributes
  void SetQueue (std::string type,
                 std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
                 std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
                 std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
                 std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue ());
  void SetDeviceAttribute (std::string name, const AttributeValue &value);
  void SetChannelAttribute (std::string name, const AttributeValue &value);

  // Installers
  NetDeviceContainer Install (NodeContainer c);
  NetDeviceContainer Install (Ptr<Node> a, Ptr<Node> b);

private:
  virtual void EnablePcapInternal (std::string prefix, Ptr<NetDevice> nd, bool promiscuous, bool explicitFilename);

  virtual void EnableAsciiInternal (
    Ptr<OutputStreamWrapper> stream,
    std::string prefix,
    Ptr<NetDevice> nd,
    bool explicitFilename);

  ObjectFactory m_queueFactory;         //!< Queue Factory
  ObjectFactory m_channelFactory;       //!< Channel Factory
  ObjectFactory m_remoteChannelFactory; //!< Remote Channel Factory
  ObjectFactory m_deviceFactory;        //!< Device Factory
  Ptr<ns3::BasicSimulation> m_basicSimulation;
};

} // namespace ns3

#endif /* POINT_TO_POINT_LASER_HELPER_H */
