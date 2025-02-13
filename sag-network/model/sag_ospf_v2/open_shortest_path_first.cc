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
#define NS_LOG_APPEND_CONTEXT                                   \
   std::clog << "[node " << m_nodeId << " " << m_systemId << "] ";
//#define NS_LOG_CONDITION    if (m_nodeId==9)

#include "open_shortest_path_first.h"
#include "ns3/log.h"
#include "ns3/route_trace_tag.h"
#include "ns3/sag_physical_layer_gsl.h"
#include "ns3/output-stream-wrapper.h"
#include <iomanip>


namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("OpenShortestPathFirst");

namespace ospf {
NS_OBJECT_ENSURE_REGISTERED (Open_Shortest_Path_First);
/// IP Protocol Number for OSPF
const uint8_t Open_Shortest_Path_First::OSPF_PROTOCOL = 89;

TypeId
Open_Shortest_Path_First::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::ospf::Open_Shortest_Path_First")
		  .SetParent<SAGRoutingProtocal>()
		  .SetGroupName("OSPF")
		  .AddConstructor<Open_Shortest_Path_First>()
		  .AddAttribute ("HelloInterval", "HELLO messages interval.",
				   	    TimeValue (Seconds (10)),
						MakeTimeAccessor (&Open_Shortest_Path_First::m_helloInterval),
						MakeTimeChecker ())
		  .AddAttribute ("RouterDeadInterval", "Neighbor dead interval.",
						TimeValue (Seconds (40)),
						MakeTimeAccessor (&Open_Shortest_Path_First::m_rtrDeadInterval),
						MakeTimeChecker ())
		  .AddAttribute ("RetransmitInterval", "Message retransmit interval.",
						TimeValue (Seconds (3)),
						MakeTimeAccessor (&Open_Shortest_Path_First::m_rxmtInterval),
						MakeTimeChecker ())
		  .AddAttribute ("LSRefreshTime", "Interval at which LSAs are periodically generated.",
						TimeValue (Seconds (30000)),
						MakeTimeAccessor (&Open_Shortest_Path_First::m_LSRefreshTime),
						MakeTimeChecker ())
  	  	  .AddAttribute ("EnableHello", "Indicates whether a hello messages enable.",
						 BooleanValue (true),
						 MakeBooleanAccessor (&Open_Shortest_Path_First::SetHelloEnable,
											  &Open_Shortest_Path_First::GetHelloEnable),
						 MakeBooleanChecker ())
			.AddAttribute ("PromptMode", "Indicates whether to enable prompt mode.",
						 BooleanValue (false),
						 MakeBooleanAccessor (&Open_Shortest_Path_First::m_promptMode),
						 MakeBooleanChecker ())
  	  	  .AddAttribute ("UniformRv",
						"Access to the underlying UniformRandomVariable",
						StringValue ("ns3::UniformRandomVariable"),
						MakePointerAccessor (&Open_Shortest_Path_First::m_uniformRandomVariable),
						MakePointerChecker<UniformRandomVariable> ());
	return tid;
}

Open_Shortest_Path_First::Open_Shortest_Path_First()
{
	m_routingTable = CreateObject<SAGRoutingTable>();
	m_routeBuild = CreateObject<OspfBuildRouting>();
	m_routeBuild->SetRouter(m_routingTable);
	m_routeBuild->SetRoutingCalculationCallback(MakeCallback(&SAGRoutingProtocal::CalculationTimeLog, this));
}

Open_Shortest_Path_First::~Open_Shortest_Path_First()
{
	NS_LOG_FUNCTION(this);
}




void
Open_Shortest_Path_First::NotifyInterfaceUp(uint32_t i)
{
	NS_LOG_FUNCTION(this);
	Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
	// One IP address per interface
	if (m_ipv4->GetNAddresses(i) != 1) {
		throw std::runtime_error("Each interface is permitted exactly one IP address.");
	}

	// Get interface single IP's address and mask
	Ipv4Address if_addr = m_ipv4->GetAddress(i, 0).GetLocal();
	Ipv4Mask if_mask = m_ipv4->GetAddress(i, 0).GetMask();

	// Loopback interface must be 0
	if (i == 0) {
		if (if_addr != Ipv4Address("127.0.0.1") || if_mask != Ipv4Mask("255.0.0.0")) {
			throw std::runtime_error("Loopback interface 0 must have IP 127.0.0.1 and mask 255.0.0.0");
		}

	} else { // Towards another interface

//		// Check that the subnet mask is maintained
//		if (if_mask.Get() != Ipv4Mask("255.255.255.0").Get()) {
//			throw std::runtime_error("Each interface must have a subnet mask of 255.255.255.0");
//		}
	}

	// Skip loopback
	if(i == 0){
		return;
	}
	Ptr<NetDevice> netDevice = l3->GetNetDevice(i);
	Ptr<Channel> channel = netDevice->GetChannel();
	// Initialize or update hello firing time
	if(netDevice->GetInstanceTypeId() == TypeId::LookupByName ("ns3::PointToPointLaserNetDevice")){
		Time startTime = Simulator::Now();
		m_helloTimeExpireRecord.push_back(std::make_pair(if_addr, startTime));
	}
	else{
		// Get the switch information
		if(channel->GetDevice(0)->GetNode()->GetId() == m_ipv4->GetObject<Node>()->GetId()){
			// myself -> satellite
			std::vector<Ipv4Address> gsAdrs;
			for(uint32_t i = 1; i < uint32_t(channel->GetNDevices()); i++){
				Ptr<NetDevice> gsNetDevice = channel->GetDevice(i);
				Ptr<Ipv4> gsIpv4 = gsNetDevice->GetNode()->GetObject<Ipv4>();
				uint32_t gsItf = gsIpv4->GetInterfaceForDevice(gsNetDevice);
				Ipv4Address gsAdr = gsIpv4->GetAddress(gsItf, 0).GetLocal();
				gsAdrs.push_back(gsAdr);
			}
			// update: just delete older neighbors, new neighbors should be found by hello
			m_nb.DeleteNeighborsPMPPTriggering(if_addr, gsAdrs);
			Time startTime = Simulator::Now();
			for(uint32_t k = 0; k < m_helloTimeExpireRecord.size(); k++){
				if(m_helloTimeExpireRecord[k].first == if_addr){
					m_helloTimeExpireRecord.erase(m_helloTimeExpireRecord.begin() + k);
					break;
				}
			}
			m_helloTimeExpireRecord.push_back(std::make_pair(if_addr, startTime));
		}
		else{
			// myself -> ground station
			Time startTime = Simulator::Now();
			m_helloTimeExpireRecord.push_back(std::make_pair(if_addr, startTime));
		}

	}
	// right now trigger the timer
	if(Simulator::Now() > Seconds(0)){
		m_htimer.Cancel ();
		m_htimer.Schedule (Seconds(0));
	}


}

void
Open_Shortest_Path_First::NotifyInterfaceDown (uint32_t i){

	NS_LOG_FUNCTION(this);
	Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
	// One IP address per interface
	if (m_ipv4->GetNAddresses(i) != 1) {
		throw std::runtime_error("Each interface is permitted exactly one IP address.");
	}

	// Get interface single IP's address and mask
	Ipv4Address if_addr = m_ipv4->GetAddress(i, 0).GetLocal();
	Ipv4Mask if_mask = m_ipv4->GetAddress(i, 0).GetMask();
	// Loopback interface must be 0
	if (i == 0) {
		if (if_addr != Ipv4Address("127.0.0.1") || if_mask != Ipv4Mask("255.0.0.0")) {
			throw std::runtime_error("Loopback interface 0 must have IP 127.0.0.1 and mask 255.0.0.0");
		}

	} else { // Towards another interface

//		// Check that the subnet mask is maintained
//		if (if_mask.Get() != Ipv4Mask("255.255.255.0").Get()) {
//			throw std::runtime_error("Each interface must have a subnet mask of 255.255.255.0");
//		}
	}

	// Skip loopback
	if(i == 0){
		return;
	}


	m_nb.DeleteNeighbor(/*routerID=*/ Ipv4Address(m_ipv4->GetObject<Node>()->GetId()), /*localInterfaceAddress=*/if_addr);

	for(uint32_t k = 0; k < m_helloTimeExpireRecord.size(); k++){
		if(m_helloTimeExpireRecord[k].first == if_addr){
			m_helloTimeExpireRecord.erase(m_helloTimeExpireRecord.begin() + k);
			break;
		}
	}

}

void
Open_Shortest_Path_First::NotifyAddAddress(uint32_t interface, Ipv4InterfaceAddress address)
{
	NS_LOG_FUNCTION(this);
	Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
	if (!l3->IsUp (interface)){
		return;
	}
	if(interface == 0){
		return;
		throw std::runtime_error("Not permitted to add IP addresses after the loopback interface has gone up.");
	}
	// One IP address per interface
	if (m_ipv4->GetNAddresses(interface) != 1) {
		throw std::runtime_error("Each interface is permitted exactly one IP address.");
	}

	// Get interface single IP's address and mask
	Ipv4Address if_addr = address.GetLocal();
//	Ipv4Mask if_mask = address.GetMask();

//	// Check that the subnet mask is maintained
//	if (if_mask.Get() != Ipv4Mask("255.255.255.0").Get()) {
//		throw std::runtime_error("Each interface must have a subnet mask of 255.255.255.0");
//	}

	Time startTime = Simulator::Now();
	m_helloTimeExpireRecord.push_back(std::make_pair(if_addr, startTime));

	// right now trigger the timer
	if(Simulator::Now() > Seconds(0)){
		m_htimer.Cancel ();
		m_htimer.Schedule (Seconds(0));
	}

}

void
Open_Shortest_Path_First::NotifyRemoveAddress(uint32_t interface, Ipv4InterfaceAddress address)
{
	NS_LOG_FUNCTION(this);
	if(interface == 0){
		throw std::runtime_error("Not permitted to remove IP addresses after the loopback interface has gone up.");
	}
	Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
	// One IP address per interface
	if (m_ipv4->GetNAddresses(interface) != 0) {
		throw std::runtime_error("Each interface is permitted exactly one IP address.");
	}

	// Get interface single IP's address and mask
	Ipv4Address if_addr = address.GetLocal();
//	Ipv4Mask if_mask = address.GetMask();

//	// Check that the subnet mask is maintained
//	if (if_mask.Get() != Ipv4Mask("255.255.255.0").Get()) {
//		throw std::runtime_error("Each interface must have a subnet mask of 255.255.255.0");
//	}

	m_nb.DeleteNeighbor(/*routerID=*/ Ipv4Address(m_ipv4->GetObject<Node>()->GetId()), /*localInterfaceAddress=*/if_addr);

	for(uint32_t k = 0; k < m_helloTimeExpireRecord.size(); k++){
		if(m_helloTimeExpireRecord[k].first == if_addr){
			m_helloTimeExpireRecord.erase(m_helloTimeExpireRecord.begin() + k);
			break;
		}
	}



}

void
Open_Shortest_Path_First::PrintRoutingTable(Ptr<OutputStreamWrapper> stream, Time::Unit unit) const
{
	NS_LOG_FUNCTION (this << stream);
	std::ostream* os = stream->GetStream ();
	// Copy the current ostream state
	std::ios oldState (nullptr);
	oldState.copyfmt (*os);

	*os << std::resetiosflags (std::ios::adjustfield) << std::setiosflags (std::ios::left);

	*os << "Node: " << m_ipv4->GetObject<Node> ()->GetId ()
	  << ", Time: " << Now().As (unit)
	  << ", Local time: " << m_ipv4->GetObject<Node> ()->GetLocalTime ().As (unit)
	  << ", Ipv4OspfRouting table" << std::endl;

	if (m_routingTable->GetNRoute() > 0)
	{
	  *os << "Destination     Gateway         Genmask         Flags Metric Ref    Use Iface" << std::endl;
	  for (uint32_t j = 0; j < m_routingTable->GetNRoute(); j++)
		{
		  std::ostringstream dest, gw, mask, flags;
		  SAGRoutingTableEntry route = m_routingTable->GetRoute (j);
		  dest << route.GetDestination ();
		  //std::cout<<"!!!!!!!!!!!!!!!"<<route.GetDestination ()<<std::endl;
		  *os << std::setw (16) << dest.str ();
		  gw << route.GetNextHop ();
		  *os << std::setw (16) << gw.str ();
//		  mask << route.GetDestNetworkMask ();
//		  *os << std::setw (16) << mask.str ();
//		  flags << "U";
//		  if (route.IsHost ())
//			{
//			  flags << "HS";
//			}
//		  else if (route.IsGateway ())
//			{
//			  flags << "GS";
//			}
//		  *os << std::setw (6) << flags.str ();
//		  *os << std::setw (7) << GetMetric (j);
//		  // Ref ct not implemented
//		  *os << "-" << "      ";
//		  // Use not implemented
//		  *os << "-" << "   ";
//		  if (Names::FindName (m_ipv4->GetNetDevice (route.GetInterface ())) != "")
//			{
//			  *os << Names::FindName (m_ipv4->GetNetDevice (route.GetInterface ()));
//			}
//		  else
//			{
//			  *os << route.GetInterface ();
//			}
		  *os << std::endl;
		}
	}
	*os << std::endl;
	// Restore the previous ostream state
	(*os).copyfmt (oldState);
}



void
Open_Shortest_Path_First::SendHello (Ipv4Address ad){
	NS_LOG_FUNCTION (this);
	Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
	int32_t i = l3->GetInterfaceForAddress(ad);

	if(i == int32_t(-1)){
		throw std::runtime_error ("Wrong interface address.");
	}

	i = (uint32_t)i;

	if (l3->GetNAddresses (i) > 1){
		throw std::runtime_error ("OSPF does not work with more then one address per each interface.");
	}
	// not loopback ensured
	Ipv4InterfaceAddress ifaceAddress = l3->GetAddress (i, 0);
	if (ifaceAddress.GetLocal () == Ipv4Address ("127.0.0.1")){
		throw std::runtime_error("Must be not loopback");
	}

	// prepare hello message
	Ptr<Packet> packet = Create<Packet> ();
	// routerId -> lookback address all the same: l3->GetAddress (0, 0).GetLocal()
	OspfHeader ospfHeader (/*packetLength=*/ 0, OSPFTYPE_HELLO, /*routerID=*/ Ipv4Address(m_ipv4->GetObject<Node>()->GetId()), /*areaID=*/ Ipv4Address::GetAny(), /*checkSum=*/ 0,
					/*auType=*/ 0, /*authentication=*/ 0);
	HelloHeader helloHeader (/*mask=*/ ifaceAddress.GetMask().Get(), /*helloInterval=*/ m_helloInterval, /*options=*/ uint8_t(0), /*rtrPri=*/ uint8_t(1),
			/*rtrDeadInterval=*/ m_rtrDeadInterval, /*dr=*/ Ipv4Address::GetAny(), /*bdr=*/ Ipv4Address::GetAny(), /*neighors=*/ m_nb.GetNeighbors());

	int size = ospfHeader.GetSerializedSize() + helloHeader.GetSerializedSize();
	ospfHeader.SetOspfLength((uint16_t)size);

	SocketIpTtlTag tag;
	tag.SetTtl (1);
	packet->AddPacketTag (tag);
	packet->AddHeader (helloHeader);
	packet->AddHeader (ospfHeader);

	//packet->AddHeader (tHeader);
	// Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
	Ipv4Address destination;
	if (ifaceAddress.GetMask () == Ipv4Mask::GetOnes ())
	{
		destination = Ipv4Address ("255.255.255.255");
	}
	else
	{
		destination = ifaceAddress.GetBroadcast ();
	}
	//Time jitter = Time (MilliSeconds (m_uniformRandomVariable->GetInteger (0, 10)));
	//Simulator::Schedule (jitter, &RoutingProtocol::SendTo, this, socket, packet, destination);
	Ptr<Ipv4Route> route = 0; //this can be nullptr for an auto search of forwarding table, or you can give it
	SendRoutingProtocalPacket(packet, ifaceAddress.GetLocal(), destination, OSPF_PROTOCOL, route);

}

void
Open_Shortest_Path_First::SendDD (Ipv4Address ad, Ipv4Address nb, uint8_t flags, uint32_t seqNum, std::vector<LSAHeader> lsaHeaders)
{
	NS_LOG_FUNCTION (this);
	Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
	uint32_t i = (uint32_t)l3->GetInterfaceForAddress(ad);
	// prepare empty DD message
	Ptr<Packet> packet = Create<Packet> ();
	OspfHeader ospfHeader (/*packetLength=*/ 0, OSPFTYPE_DBD, /*routerID=*/ Ipv4Address(m_ipv4->GetObject<Node>()->GetId()), /*areaID=*/ Ipv4Address::GetAny(), /*checkSum=*/ 0,
					/*auType=*/ 0, /*authentication=*/ 0);
	//TypeHeader tHeader (OSPFTYPE_DBD);
	SocketIpTtlTag tag;
	tag.SetTtl (1);


	uint16_t mtu = l3->GetMtu(i); // 1500 by default

	for(size_t x = 0; x < lsaHeaders.size(); x++){
		lsaHeaders[x].SetPacketLength((uint16_t)24);
	}
	//ddHeader.SetLSAHeaders(lsaHeaders);
	DDHeader ddHeader(mtu, 0, flags, seqNum, lsaHeaders); // option no use
	int size = ospfHeader.GetSerializedSize() + ddHeader.GetSerializedSize();

	//int size = ospfHeader.GetSerializedSize() + ddHeader.GetSerializedSize();
	ospfHeader.SetOspfLength((uint16_t)size);

	packet->AddPacketTag(tag);
	packet->AddHeader(ddHeader);
	packet->AddHeader(ospfHeader);
	//packet->AddHeader(tHeader);

	// Unicast
	Ipv4Address destination = nb;
	Ptr<Ipv4Route> route = Create<Ipv4Route>();
	route->SetDestination(destination);
	route->SetGateway(destination);
	route->SetSource(ad);
	route->SetOutputDevice(l3->GetNetDevice(i));
	SendRoutingProtocalPacket(packet, ad, destination, OSPF_PROTOCOL, route);


}

void
Open_Shortest_Path_First::SendLSR (Ipv4Address ad, Ipv4Address nb,std::vector <LSAHeader> lsaHeaders)
{
	NS_LOG_FUNCTION (this);
	Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
	uint32_t i = (uint32_t)l3->GetInterfaceForAddress(ad);
	// prepare empty LSR message
	Ptr<Packet> packet = Create<Packet> ();
	OspfHeader ospfHeader (/*packetLength=*/ 0, OSPFTYPE_LSR, /*routerID=*/ Ipv4Address(m_ipv4->GetObject<Node>()->GetId()), /*areaID=*/ Ipv4Address::GetAny(), /*checkSum=*/ 0,
					/*auType=*/ 0, /*authentication=*/ 0);
	//TypeHeader tHeader (OSPFTYPE_LSR);
	SocketIpTtlTag tag;
	tag.SetTtl (1);


	std::vector<LSRPacket> LSRs = {};

	for (std::vector<LSAHeader>::iterator i = lsaHeaders.begin(); i != lsaHeaders.end(); ++i)
	{
		 LSRPacket LSR (i->GetLinkStateID(),i->GetAdvertisiongRouter());
		 LSRs.push_back(LSR);
	}

	LSRHeader lsrHeader(LSRs);

	int size = ospfHeader.GetSerializedSize() + lsrHeader.GetSerializedSize();
	ospfHeader.SetOspfLength((uint16_t)size);

	packet->AddPacketTag(tag);
	packet->AddHeader(lsrHeader);
	packet->AddHeader(ospfHeader);
	//packet->AddHeader(tHeader);


	// Unicast
	Ipv4Address destination = nb;
	Ptr<Ipv4Route> route = Create<Ipv4Route>();
	route->SetDestination(destination);
	route->SetGateway(destination);
	route->SetSource(ad);
	route->SetOutputDevice(l3->GetNetDevice(i));
	SendRoutingProtocalPacket(packet, ad, destination, OSPF_PROTOCOL, route);
}

void
Open_Shortest_Path_First::SendLSU (Ipv4Address ad, Ipv4Address nb,std::vector<std::pair<LSAHeader,LSAPacket>> lsas)
{
	NS_LOG_FUNCTION (this);
	Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
	uint32_t i = (uint32_t)l3->GetInterfaceForAddress(ad);
	// prepare empty LSR message
	Ptr<Packet> packet = Create<Packet> ();
	OspfHeader ospfHeader (/*packetLength=*/ 0, OSPFTYPE_LSU, /*routerID=*/ Ipv4Address(m_ipv4->GetObject<Node>()->GetId()), /*areaID=*/ Ipv4Address::GetAny(), /*checkSum=*/ 0,
					/*auType=*/ 0, /*authentication=*/ 0);
	//TypeHeader tHeader (OSPFTYPE_LSU);
	SocketIpTtlTag tag;
	tag.SetTtl (1);



	for(size_t x = 0; x < lsas.size(); x++){
		int lsaSize = lsas[x].first.GetSerializedSize() + lsas[x].second.GetSerializedSize();
		//int lsaSize = lsas[x].first.GetSerializedSize() + lsas[x].second.GetSerializedSize();
		lsas[x].first.SetPacketLength((uint16_t)lsaSize);
	}
	//lsuHeader.SetLSAs(lsas);
	LSUHeader lsuHeader(lsas); // option no use
	int size = ospfHeader.GetSerializedSize() + lsuHeader.GetSerializedSize();

	ospfHeader.SetOspfLength((uint16_t)size);

	packet->AddPacketTag(tag);
	packet->AddHeader(lsuHeader);
	packet->AddHeader(ospfHeader);
	//packet->AddHeader(tHeader);


	// Unicast
	Ipv4Address destination = nb;
	Ptr<Ipv4Route> route = Create<Ipv4Route>();
	route->SetDestination(destination);
	route->SetGateway(destination);
	route->SetSource(ad);
	route->SetOutputDevice(l3->GetNetDevice(i));
	SendRoutingProtocalPacket(packet, ad, destination, OSPF_PROTOCOL, route);

}

void
Open_Shortest_Path_First::SendLSAack (Ipv4Address ad, Ipv4Address nb,std::vector<LSAHeader> lsaheaders)
{
	NS_LOG_FUNCTION (this);
	Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
	uint32_t i = (uint32_t)l3->GetInterfaceForAddress(ad);
	// prepare empty LSAack message
	Ptr<Packet> packet = Create<Packet> ();
	OspfHeader ospfHeader (/*packetLength=*/ 0, OSPFTYPE_LSAck, /*routerID=*/ Ipv4Address(m_ipv4->GetObject<Node>()->GetId()), /*areaID=*/ Ipv4Address::GetAny(), /*checkSum=*/ 0,
					/*auType=*/ 0, /*authentication=*/ 0);
	//TypeHeader tHeader (OSPFTYPE_LSAck);
	SocketIpTtlTag tag;
	tag.SetTtl (1);



	// There is no delayed acknowledgment, that is, it is only limited by the number of LSAs in the LSU,
	// so the number of LSAs confirmed at this time can meet the requirements of forming an LSAACK
	for(size_t x = 0; x < lsaheaders.size(); x++){
		lsaheaders[x].SetPacketLength((uint16_t)24);
	}
	//lsaack.SetLSAHeaders(lsaheaders);

	LSAackHeader lsaack (lsaheaders); // option no use
	int size = ospfHeader.GetSerializedSize() + lsaack.GetSerializedSize();

	ospfHeader.SetOspfLength((uint16_t)size);

	packet->AddPacketTag(tag);
	packet->AddHeader(lsaack);
	packet->AddHeader(ospfHeader);
	//packet->AddHeader(tHeader);


	// Unicast
	Ipv4Address destination = nb;
	Ptr<Ipv4Route> route = Create<Ipv4Route>();
	route->SetDestination(destination);
	route->SetGateway(destination);
	route->SetSource(ad);
	route->SetOutputDevice(l3->GetNetDevice(i));
	SendRoutingProtocalPacket(packet, ad, destination, OSPF_PROTOCOL, route);

}

static bool cmp(const std::pair<Ipv4Address,Time>& a, const std::pair<Ipv4Address,Time>& b)
{
	return a.second < b.second;
}

void
Open_Shortest_Path_First::HelloTimerExpire ()
{
	NS_LOG_FUNCTION (this);
	if(m_helloTimeExpireRecord.empty()){
		return;
	}
	sort(m_helloTimeExpireRecord.begin(),m_helloTimeExpireRecord.end(),cmp);
	if(m_helloTimeExpireRecord.at(0).second.GetMicroSeconds() != Simulator::Now ().GetMicroSeconds()){
		m_htimer.Cancel ();
		m_htimer.Schedule (m_helloTimeExpireRecord.at(0).second - Simulator::Now ());
		return;
	}
	SendHello (m_helloTimeExpireRecord.at(0).first);
	m_helloTimeExpireRecord.at(0).second += m_helloInterval;
	sort(m_helloTimeExpireRecord.begin(),m_helloTimeExpireRecord.end(),cmp);
	m_htimer.Cancel ();
	m_htimer.Schedule (m_helloTimeExpireRecord.at(0).second - Simulator::Now ());
}

void
Open_Shortest_Path_First::HelloTriggeringBy1WayReceived (Ipv4Address ad){
	NS_LOG_FUNCTION (this);
	bool found = false;
	for(uint8_t i = 0; i < m_helloTimeExpireRecord.size(); i++){
		if(m_helloTimeExpireRecord.at(i).first == ad){
			SendHello (m_helloTimeExpireRecord.at(i).first);
			// delay its next trigger time
			if(m_helloTimeExpireRecord.at(i).second > Simulator::Now()){
				m_helloTimeExpireRecord.at(i).second = m_helloInterval + Simulator::Now();
			}
			else{
				m_helloTimeExpireRecord.at(i).second += m_helloInterval;
			}
			found = true;
			break;
		}
	}
	if(!found){
		throw std::runtime_error ("Wrong interface address.");
	}
	sort(m_helloTimeExpireRecord.begin(),m_helloTimeExpireRecord.end(),cmp);


}

void
Open_Shortest_Path_First::DDTriggeringBy2WayReceived (Ipv4Address ad, Ipv4Address nb, uint8_t flags, uint32_t seqNum, std::vector<LSAHeader> lsa){
	NS_LOG_FUNCTION (this);
	bool found = false;
	for(uint8_t i = 0; i < m_helloTimeExpireRecord.size(); i++){
		if(m_helloTimeExpireRecord.at(i).first == ad){
			SendDD (m_helloTimeExpireRecord.at(i).first, nb, flags, seqNum,lsa);
			found = true;
			break;
		}
	}
	if(!found){
		throw std::runtime_error ("Wrong interface address.");
	}
}

void
Open_Shortest_Path_First::LSUTriggeringByLSRReceived(Ipv4Address ad, Ipv4Address nb,std::vector<std::pair<LSAHeader,LSAPacket>> lsas)
{
	NS_LOG_FUNCTION (this);
	SendLSU(ad,nb,lsas);
}

void
Open_Shortest_Path_First::LSAaackTriggeringByLSUReceived(Ipv4Address ad, Ipv4Address nb,std::vector<LSAHeader> lsaheaders)
{
	NS_LOG_FUNCTION (this);
	SendLSAack(ad,nb,lsaheaders);
}

void
Open_Shortest_Path_First::LSRTriggering(Ipv4Address ad, Ipv4Address nb,std::vector <LSAHeader> lsaHeaders)
{
	NS_LOG_FUNCTION (this);
	SendLSR(ad, nb, lsaHeaders);
}

Ptr<Ipv4Route>
Open_Shortest_Path_First::RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr){
	NS_LOG_FUNCTION (this << header << (oif ? oif->GetIfIndex () : 0));

	sockerr = Socket::ERROR_NOTERROR;
	Ptr<Ipv4Route> route;
	Ipv4Address dst = header.GetDestination ();

//modify{
// for bgp loopback address
	Ipv4Mask mask = Ipv4Mask("255.0.0.0");
	if(mask.IsMatch(dst,Ipv4Address("128.0.0.0"))){
		dst = Ipv4Address(dst.Get() - Ipv4Address("128.0.0.0").Get() - 1);
	}
//}modify

	SAGRoutingTableEntry rtEntry;
	bool found = m_routingTable->LookupRoute(dst, rtEntry);
	if (found)
	{	
		route = rtEntry.GetRoute();
		NS_ASSERT (route != 0);
//		std::stringstream stream;
//		Ptr<OutputStreamWrapper> routingstream = Create<OutputStreamWrapper> (&std::cout);
//		PrintRoutingTable(routingstream);
		NS_LOG_DEBUG ("Exist route to " << route->GetDestination () << " from interface " << route->GetSource ());
		if (oif != 0 && route->GetOutputDevice () != oif)
		{
		  NS_LOG_DEBUG ("Output device doesn't match. Dropped.");
		  sockerr = Socket::ERROR_NOROUTETOHOST;
		  return Ptr<Ipv4Route> ();
		}

		return route;
	}
	return Ptr<Ipv4Route> ();
}

bool
Open_Shortest_Path_First::RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
			   	   	   	   	   	   	  UnicastForwardCallback ucb, MulticastForwardCallback mcb,
									  LocalDeliverCallback lcb, ErrorCallback ecb)
{
	NS_LOG_FUNCTION (this << p->GetUid () << header.GetDestination () << idev->GetAddress ());
	NS_ASSERT(m_ipv4 != 0);
	NS_ASSERT (p != 0);

	// Check if input device supports IP
	NS_ASSERT (m_ipv4->GetInterfaceForDevice (idev) >= 0);
	int32_t iif = m_ipv4->GetInterfaceForDevice (idev);

	Ipv4Address dst = header.GetDestination ();
	Ipv4Address origin = header.GetSource ();
	Ipv4Address localInterfaceAdr = m_ipv4->GetAddress(iif, 0).GetLocal();

	// Multi-cast, waiting to be optimised
	// we use directed-broadcast currently
	if (dst.IsMulticast()) {
		throw std::runtime_error("Multi-cast not supported.");
	}

	// Local delivery
	if (m_ipv4->IsDestinationAddress(dst, iif)) {

		// Determine whether a protocol packet or a data packet
		if(header.GetProtocol() == OSPF_PROTOCOL)
		{
			Ptr<Packet> packet = p->Copy ();
			RecvOSPF(packet, localInterfaceAdr, origin);
			return true;
		}
		else
		{
			if (lcb.IsNull()) {
				throw std::runtime_error("Local callback cannot be null");
			}
			// Info: If you want to decide that a packet should not be delivered (dropped),
			//       you can decide that here by not calling lcb(), but still returning true.
			//Ptr<Packet> packet = p->Copy ();// Because the tag needs to be modified, we need a non-const packet
		    lcb(p, header, iif);

			return true;
		}
	}

	// Check if input device supports IP forwarding
	if (m_ipv4->IsForwarding(iif) == false) {
		throw std::runtime_error("Forwarding must be enabled for every interface");
	}

	// Uni-cast delivery
//modify{
// for bgp loopback address
		Ipv4Mask mask = Ipv4Mask("255.0.0.0");
		if(mask.IsMatch(dst,Ipv4Address("128.0.0.0"))){
			dst = Ipv4Address(dst.Get() - Ipv4Address("128.0.0.0").Get() - 1);
		}
//}modify
	SAGRoutingTableEntry rtEntry;
	bool found = m_routingTable->LookupRoute(dst, rtEntry);
	if (!found) {
		// Lookup failed, so we did not find a route
		// If there are no other routing protocols, this will lead to a drop
		return false;
	} else {
		// Lookup succeeded in producing a route
		// So we perform the unicast callback to forward there
		Ptr<Ipv4Route> route = rtEntry.GetRoute();
		NS_ASSERT (route != 0);
		//Ptr<Packet> packet = p->Copy ();// Because the tag needs to be modified, we need a non-const packet
		ucb(route, p, header);

		return true;
	}

}






void
Open_Shortest_Path_First::DoInitialize (void)
{
	NS_LOG_FUNCTION (this);
	//std::cout<<m_LSRefreshTime<<std::endl;
	m_nb = Neighbors(m_helloInterval, m_rtrDeadInterval, m_rxmtInterval, m_LSRefreshTime);
	m_nb.SetHelloTriggeringCallback (MakeCallback (&Open_Shortest_Path_First::HelloTriggeringBy1WayReceived, this));
	m_nb.SetDDTriggeringCallback(MakeCallback (&Open_Shortest_Path_First::DDTriggeringBy2WayReceived, this));
	m_nb.SetLSUTriggeringCallback(MakeCallback(&Open_Shortest_Path_First::LSUTriggeringByLSRReceived,this));
	m_nb.SetLSATriggeringCallback(MakeCallback(&Open_Shortest_Path_First::LSAaackTriggeringByLSUReceived,this));
	m_nb.SetLSRTriggeringCallback(MakeCallback(&Open_Shortest_Path_First::LSRTriggering,this));

	m_nb.SetRouterBuildCallback(MakeCallback(&OspfBuildRouting::RouterCalculate, m_routeBuild));
	m_routeBuild->SetRtrCalTimeEnable(m_rtrCalTimeConsidered);
	m_routeBuild->SetBaseDir(m_baseDir);
	m_routeBuild->SetNodeNum(m_satellitesNumber, m_groundStationNumber);


	m_nb.SetRouterId(Ipv4Address(m_ipv4->GetObject<Node>()->GetId()));
	//m_nb.SetRouterId(m_ipv4->GetAddress(0,1).GetLocal());

	m_routeBuild->SetIpv4(m_ipv4);
	m_routeBuild->SetPromptMode(m_promptMode);
	m_nb.SetIpv4(m_ipv4);
	m_nb.SetNodeNum(m_satellitesNumber, m_groundStationNumber);
	uint32_t startTime = 0;
	if (m_enableHello)
	{
	  m_htimer.SetFunction (&Open_Shortest_Path_First::HelloTimerExpire, this);
	  NS_LOG_DEBUG ("Starting at time " << startTime << "ms");
	  m_htimer.Schedule (MilliSeconds (startTime));
	}
	Ipv4RoutingProtocol::DoInitialize ();
}

void
Open_Shortest_Path_First::RecvOSPF (Ptr<Packet> p, Ipv4Address receiver, Ipv4Address src)
{
	NS_LOG_FUNCTION (this);
//	TypeHeader tHeader (OSPFTYPE_HELLO);
//	p->RemoveHeader (tHeader);
//	if (!tHeader.IsValid ())
//	{
//	  NS_LOG_DEBUG ("OSPF message " << p->GetUid () << " with unknown type received: " << tHeader.Get () << ". Drop");
//	  return; // drop
//	}

	OspfHeader ospfHeader;
	p->RemoveHeader (ospfHeader);

	switch (ospfHeader.GetType())
	{
		case OSPFTYPE_HELLO:
		{
			RecvHELLO (p, receiver, src, ospfHeader);
			break;
		}
		case OSPFTYPE_DBD:
		{
			RecvDD (p, receiver, src, ospfHeader);
			break;
		}
		case OSPFTYPE_LSR:
		{
			RecvLSR (p, receiver, src, ospfHeader);
			break;
		}
		case OSPFTYPE_LSU:
		{
			RecvLSU (p, receiver, src, ospfHeader);
			break;
		}
		case OSPFTYPE_LSAck:
		{
			RecvLSAck(p, receiver, src, ospfHeader);
			break;
		}
	}

}

void
Open_Shortest_Path_First::RecvHELLO (Ptr<Packet> p, Ipv4Address receiver, Ipv4Address src, OspfHeader ospfHeader)
{
	NS_LOG_FUNCTION (this << " src " << src);
	HelloHeader helloHeader;
	p->RemoveHeader (helloHeader);

	//std::cout<<helloHeader.GetHelloInterval()<<std::endl;

	// Next, the values of the Network Mask, HelloInterval, and RouterDeadInterval fields in the received Hello packet must
	// be checked against the values configured for the receiving
	// However, there is one exception to the above rule: on point-to-point networks and on virtual links,
	// the Network Mask in the received Hello Packet should be ignored.
	if(helloHeader.GetHelloInterval() == m_helloInterval && helloHeader.GetRouterDeadInterval() == m_rtrDeadInterval){
		// judge whether this is the first time the neighbor has been detected, create a new data structure or update expireTime
		m_nb.HelloReceived (Ipv4Address(m_ipv4->GetObject<Node>()->GetId()),ospfHeader.GetAreaID(), receiver, ospfHeader.GetRouterID(), src, 0,
				Ipv4Address::GetZero(), Ipv4Address::GetZero(), m_rtrDeadInterval, helloHeader.GetNeighbors());
	}

	//delete p;


}

void
Open_Shortest_Path_First::RecvDD (Ptr<Packet> p, Ipv4Address my,Ipv4Address src, OspfHeader ospfHeader)
{


	NS_LOG_FUNCTION(this << " src " << src);
	Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
	DDHeader ddHeader;
	p->RemoveHeader(ddHeader);

	if(ddHeader.GetMTU() == l3->GetMtu((uint32_t)l3->GetInterfaceForAddress(my))){
		m_nb.DDReceived(Ipv4Address(m_ipv4->GetObject<Node>()->GetId()), src, ddHeader.GetFlags(), ddHeader.GetSeqNum(), ddHeader.GetLSAHeaders());
	}
	else{
		// the Database Description packet is rejected
	}


}
void
Open_Shortest_Path_First::RecvLSR (Ptr<Packet> p, Ipv4Address my,Ipv4Address src, OspfHeader ospfHeader)
{
	NS_LOG_FUNCTION(this << " src " << src);
	Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
	LSRHeader lsrHeader;
	p->RemoveHeader(lsrHeader);

	m_nb.LSRReceived(Ipv4Address(m_ipv4->GetObject<Node>()->GetId()),src,lsrHeader);
/*	if(ddHeader.GetMTU() == l3->GetMtu((uint32_t)l3->GetInterfaceForAddress(my))){
		m_nb.DDReceived(Ipv4Address(m_ipv4->GetObject<Node>()->GetId()), src, ddHeader.GetFlags(), ddHeader.GetSeqNum(), ddHeader.GetLSAHeaders());
	}*/
}

void
Open_Shortest_Path_First::RecvLSU (Ptr<Packet> p, Ipv4Address my,Ipv4Address src, OspfHeader ospfHeader)
{
	NS_LOG_FUNCTION(this << " src " << src);
	Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
	LSUHeader lsu;
	p->RemoveHeader(lsu);

	m_nb.LSUReceived(Ipv4Address(m_ipv4->GetObject<Node>()->GetId()),src,lsu);
/*	if(ddHeader.GetMTU() == l3->GetMtu((uint32_t)l3->GetInterfaceForAddress(my))){
		m_nb.DDReceived(Ipv4Address(m_ipv4->GetObject<Node>()->GetId()), src, ddHeader.GetFlags(), ddHeader.GetSeqNum(), ddHeader.GetLSAHeaders());
	}*/
}

void
Open_Shortest_Path_First::RecvLSAck (Ptr<Packet> p, Ipv4Address my,Ipv4Address src, OspfHeader ospfHeader)
{
	NS_LOG_FUNCTION(this << " src " << src);
	Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
	LSAackHeader lsack;
	p->RemoveHeader(lsack);

	m_nb.LSAckReceived(Ipv4Address(m_ipv4->GetObject<Node>()->GetId()),src,lsack);
}




}
}


