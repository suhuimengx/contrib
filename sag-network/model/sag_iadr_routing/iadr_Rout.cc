/*
 * open_shortest_path_first.cc
 *
 *  Created on: 2023年1月5日
 *      Author: root
 */
#define NS_LOG_APPEND_CONTEXT                                   \
   std::clog << "[node " << m_nodeId << "] ";

#include "iadr_Rout.h"
#include "ns3/log.h"
#include "ns3/route_trace_tag.h"
#include "ns3/sag_physical_layer_gsl.h"
#include "ns3/sag_link_layer_gsl.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("IadrRout");

namespace iadr{
NS_OBJECT_ENSURE_REGISTERED (Iadr_Rout);
/// IP Protocol Number for IADR
const uint8_t Iadr_Rout::IADR_PROTOCOL = 89;

TypeId
Iadr_Rout::GetTypeId(void)
{
	static TypeId tid = TypeId("ns3::iadr::Iadr_Rout")
			  .SetParent<SAGRoutingProtocal>()
			  .SetGroupName("IADR")
			  .AddConstructor<Iadr_Rout>()
			  .AddAttribute ("HelloInterval", "HELLO messages interval.",
					   	    TimeValue (Seconds (5)),
							MakeTimeAccessor (&Iadr_Rout::m_helloInterval),
							MakeTimeChecker ())
			  .AddAttribute ("RoutCalculateInterval", "RoutCalculate interval.",
							TimeValue (Seconds (20)),
							MakeTimeAccessor (&Iadr_Rout::m_routcalInterval),
							MakeTimeChecker ())
			  .AddAttribute ("RouterDeadInterval", "Neighbor dead interval.",
							TimeValue (Seconds (10)),
							MakeTimeAccessor (&Iadr_Rout::m_rtrDeadInterval),
							MakeTimeChecker ())
			  .AddAttribute ("RetransmitInterval", "Message retransmit interval.",
							TimeValue (Seconds (5)),
							MakeTimeAccessor (&Iadr_Rout::m_rxmtInterval),
							MakeTimeChecker ())
			  .AddAttribute ("LSRefreshTime", "Interval at which LSAs are periodically generated.",
							TimeValue (Seconds (31)),
							MakeTimeAccessor (&Iadr_Rout::m_LSRefreshTime),
							MakeTimeChecker ())
			  .AddAttribute("ContrllerNo", "The Seq of Goundstation Controller in the simulation topology.",
							UintegerValue(1),
							MakeUintegerAccessor(&Iadr_Rout::m_ContrllerNo),
							MakeUintegerChecker<uint16_t>())
			  .AddAttribute ("EnableHello", "Indicates whether a hello messages enable.",
							 BooleanValue (true),
							 MakeBooleanAccessor (&Iadr_Rout::SetHelloEnable,
												  &Iadr_Rout::GetHelloEnable),
							 MakeBooleanChecker ())
			 .AddAttribute("ServerComputingPowerMultiples", "The multiple of the computing power of the server relative to the running simulation CPU.",
							   UintegerValue(25),
							   MakeUintegerAccessor(&Iadr_Rout::m_serverComputingPower),
							   MakeUintegerChecker<uint16_t>())
	  	  	  .AddAttribute ("UniformRv",
							"Access to the underlying UniformRandomVariable",
							StringValue ("ns3::UniformRandomVariable"),
							MakePointerAccessor (&Iadr_Rout::m_uniformRandomVariable),
							MakePointerChecker<UniformRandomVariable> ());
		return tid;
}

Iadr_Rout::Iadr_Rout()
  : m_enableHello (false)
{
	m_routingTable = CreateObject<SAGRoutingTable>();
	m_routeBuild = CreateObject<IadrBuildRouting>();
	m_routeBuild->SetRouter(m_routingTable);
	m_routeBuild->SetRoutingCalculationCallback(MakeCallback(&SAGRoutingProtocal::CalculationTimeLog, this));
	m_queue = IADRServerQueue(100000000);
	//convergence_record.resize(m_satellitesNumber + m_groundStationNumber,1);
}

Iadr_Rout::~Iadr_Rout()
{
	NS_LOG_FUNCTION(this);
}

void
Iadr_Rout::NotifyInterfaceUp(uint32_t i)
{
	NS_LOG_FUNCTION(this);
	Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
	// One IP address per interface
	if (m_ipv4->GetNAddresses(i) != 1) {
		throw std::runtime_error("Each interface is permitted exactly one IoP address.");
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

		// Check that the subnet mask is maintained
		if (if_mask.Get() != Ipv4Mask("255.255.255.0").Get()) {
			throw std::runtime_error("Each interface must have a subnet mask of 255.255.255.0");
		}

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
Iadr_Rout::NotifyInterfaceDown (uint32_t i){

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

		// Check that the subnet mask is maintained
		if (if_mask.Get() != Ipv4Mask("255.255.255.0").Get()) {
			throw std::runtime_error("Each interface must have a subnet mask of 255.255.255.0");
		}

	}

	// Skip loopback
	if(i == 0){
		return;
	}

	//std::cout<< " Time: "<< Simulator::Now().GetSeconds()<<" ISL Interuption: "<< m_ipv4->GetObject<Node>()->GetId() <<std::endl;

	m_nb.DeleteNeighbor(/*routerID=*/ Ipv4Address(m_ipv4->GetObject<Node>()->GetId()), /*localInterfaceAddress=*/if_addr);

	for(uint32_t k = 0; k < m_helloTimeExpireRecord.size(); k++){
		if(m_helloTimeExpireRecord[k].first == if_addr){
			m_helloTimeExpireRecord.erase(m_helloTimeExpireRecord.begin() + k);
			break;
		}
	}
}

void
Iadr_Rout::NotifyAddAddress(uint32_t interface, Ipv4InterfaceAddress address)
{
	NS_LOG_FUNCTION(this);
	Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
	if (!l3->IsUp (interface)){
		return;
	}
	if(interface == 0){
		throw std::runtime_error("Not permitted to add IP addresses after the loopback interface has gone up.");
	}
	// One IP address per interface
	if (m_ipv4->GetNAddresses(interface) != 1) {
		throw std::runtime_error("Each interface is permitted exactly one IP address.");
	}

	// Get interface single IP's address and mask
	Ipv4Address if_addr = address.GetLocal();
	Ipv4Mask if_mask = address.GetMask();

	// Check that the subnet mask is maintained
	if (if_mask.Get() != Ipv4Mask("255.255.255.0").Get()) {
		throw std::runtime_error("Each interface must have a subnet mask of 255.255.255.0");
	}

	Time startTime = Simulator::Now();
	m_helloTimeExpireRecord.push_back(std::make_pair(if_addr, startTime));

	// right now trigger the timer
	if(Simulator::Now() > Seconds(0)){
		m_htimer.Cancel ();
		m_htimer.Schedule (Seconds(0));
	}

}

void
Iadr_Rout::NotifyRemoveAddress(uint32_t interface, Ipv4InterfaceAddress address)
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
	Ipv4Mask if_mask = address.GetMask();

	// Check that the subnet mask is maintained
	if (if_mask.Get() != Ipv4Mask("255.255.255.0").Get()) {
		throw std::runtime_error("Each interface must have a subnet mask of 255.255.255.0");
	}

	//std::cout<< " Time: "<< Simulator::Now().GetSeconds()<<" ISL Interuption: "<< m_ipv4->GetObject<Node>()->GetId() <<std::endl;

	m_nb.DeleteNeighbor(/*routerID=*/ Ipv4Address(m_ipv4->GetObject<Node>()->GetId()), /*localInterfaceAddress=*/if_addr);

	for(uint32_t k = 0; k < m_helloTimeExpireRecord.size(); k++){
		if(m_helloTimeExpireRecord[k].first == if_addr){
			m_helloTimeExpireRecord.erase(m_helloTimeExpireRecord.begin() + k);
			break;
		}
	}
}

void
Iadr_Rout::SendHello (Ipv4Address ad){
	NS_LOG_FUNCTION (this);
	Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
	int32_t i = l3->GetInterfaceForAddress(ad);

	if(i == int32_t(-1)){
		throw std::runtime_error ("Wrong interface address.");
	}

	i = (uint32_t)i;

	if (l3->GetNAddresses (i) > 1){
		throw std::runtime_error ("IADR does not work with more then one address per each interface.");
	}
	// not loopback ensured
	Ipv4InterfaceAddress ifaceAddress = l3->GetAddress (i, 0);
	if (ifaceAddress.GetLocal () == Ipv4Address ("127.0.0.1")){
		throw std::runtime_error("Must be not loopback");
	}

	// prepare hello message
	Ptr<Packet> packet = Create<Packet> ();
	IadrHeader iadrHeader (/*packetLength=*/ 0, /*routerID=*/ Ipv4Address(m_ipv4->GetObject<Node>()->GetId()), /*areaID=*/ Ipv4Address::GetAny(), /*checkSum=*/ 0,
					/*auType=*/ 0, /*authentication=*/ 0);
	HelloHeader helloHeader (/*mask=*/ ifaceAddress.GetMask().Get(), /*helloInterval=*/ m_helloInterval, /*options=*/ uint8_t(0), /*rtrPri=*/ uint8_t(1),
			/*rtrDeadInterval=*/ m_rtrDeadInterval, /*dr=*/ Ipv4Address::GetAny(), /*bdr=*/ Ipv4Address::GetAny(), /*neighors=*/ m_nb.GetNeighbors());

	TypeHeader tHeader (IADRTYPE_HELLO);
	SocketIpTtlTag tag;
	tag.SetTtl (1);
	packet->AddPacketTag (tag);
	packet->AddHeader (iadrHeader);
	packet->AddHeader (helloHeader);
	packet->AddHeader (tHeader);
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
	SendRoutingProtocalPacket(packet, ifaceAddress.GetLocal(), destination, IADR_PROTOCOL, route);
}


void
Iadr_Rout::SendDD (Ipv4Address ad, Ipv4Address nb, uint8_t flags, uint32_t seqNum, std::vector<std::pair<LSAHeader,LSAPacket>> lsas)
{
	NS_LOG_FUNCTION (this);
	Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
	uint32_t i = (uint32_t)l3->GetInterfaceForAddress(ad);
	// prepare empty DD message
	Ptr<Packet> packet = Create<Packet> ();
	IadrHeader iadrHeader (/*packetLength=*/ 0, /*routerID=*/ Ipv4Address(m_ipv4->GetObject<Node>()->GetId()), /*areaID=*/ Ipv4Address::GetAny(), /*checkSum=*/ 0,
					/*auType=*/ 0, /*authentication=*/ 0);
	TypeHeader tHeader (IADRTYPE_DBD);

	//borrow LSU
	LSUHeader lsuHeader(lsas);

	SocketIpTtlTag tag;
	tag.SetTtl (1);

	uint16_t mtu = l3->GetMtu(i); // 1500 by default

	std::vector<LSAHeader> lsaHeaders={};
	//uint32_t LSUPacketSize = 24;
	for(auto lsa : lsas){
		lsaHeaders.push_back(lsa.first);
	}
	DDHeader ddHeader(mtu, 0, flags, seqNum, lsaHeaders); // option no use

	packet->AddPacketTag(tag);
	packet->AddHeader(iadrHeader);
	packet->AddHeader(ddHeader);

	packet->AddHeader(lsuHeader);
	packet->AddHeader(tHeader);

	// Unicast
	Ipv4Address destination = nb;
	Ptr<Ipv4Route> route = Create<Ipv4Route>();
	route->SetDestination(destination);
	route->SetGateway(destination);
	route->SetSource(ad);
	route->SetOutputDevice(l3->GetNetDevice(i));
	SendRoutingProtocalPacket(packet, ad, destination, IADR_PROTOCOL, route);

}

void
Iadr_Rout::SendRTS (Ipv4Address ad, Ipv4Address nb, std::pair<RTSHeader,RTSPacket> rts)
{
	NS_LOG_FUNCTION (this);
	Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();

	uint32_t i = (uint32_t)l3->GetInterfaceForAddress(ad);
	// prepare empty RTS message
	Ptr<Packet> packet = Create<Packet> ();
	IadrHeader iadrHeader (/*packetLength=*/ 0, /*routerID=*/ Ipv4Address(m_ipv4->GetObject<Node>()->GetId()), /*areaID=*/ Ipv4Address::GetAny(), /*checkSum=*/ 0,
					/*auType=*/ 0, /*authentication=*/ 0);
	TypeHeader tHeader (IADRTYPE_RTS);

	//
	RTSPHeader rtspHeader(rts);

	SocketIpTtlTag tag;
	tag.SetTtl (1);
	packet->AddPacketTag(tag);
	packet->AddHeader(iadrHeader);
	packet->AddHeader(rtspHeader);
	packet->AddHeader(tHeader);


	// Unicast
	Ipv4Address destination = nb;
	Ptr<Ipv4Route> route = Create<Ipv4Route>();
	route->SetDestination(destination);
	route->SetGateway(destination);
	route->SetSource(ad);

	//if (l3->GetInterfaceForAddress(ad)!= -1){
	route->SetOutputDevice(l3->GetNetDevice(i));
	DoSendRoutingProtocalPacket(packet, ad, destination, IADR_PROTOCOL, route);
	//SendRoutingProtocalPacket(packet, ad, destination, IADR_PROTOCOL, route);

}

static bool cmp(const std::pair<Ipv4Address,Time>& a, const std::pair<Ipv4Address,Time>& b)
{
	return a.second < b.second;
}

void
Iadr_Rout::HelloTimerExpire ()
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
Iadr_Rout::HelloTriggeringBy1WayReceived (Ipv4Address ad){
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
Iadr_Rout::DDTriggeringBy2WayReceived (Ipv4Address ad, Ipv4Address nb, uint8_t flags, uint32_t seqNum, std::vector<std::pair<LSAHeader,LSAPacket>> lsas){
	NS_LOG_FUNCTION (this);
	bool found = false;
	for(uint8_t i = 0; i < m_helloTimeExpireRecord.size(); i++){
		if(m_helloTimeExpireRecord.at(i).first == ad){
			SendDD (m_helloTimeExpireRecord.at(i).first, nb, flags, seqNum, lsas);
			found = true;
			break;
		}
	}
	if(!found){
		throw std::runtime_error ("Wrong interface address.");
	}
}



void
Iadr_Rout::RTSTriggeringAftRouterCalculator (Ipv4Address ad, Ipv4Address nb, std::pair<RTSHeader,RTSPacket> rts){

	NS_LOG_FUNCTION (this);
	SendRTS (ad, nb, rts);
}


void
Iadr_Rout::InitialRTSTriggeringAftRouterCalculator (std::pair<RTSHeader,RTSPacket> rts){

	NS_LOG_FUNCTION (this);
	m_queue.Enqueue(QueueEntry(rts));

}

void Iadr_Rout::RTSTimerExpire(){

	if(m_queue.GetSize() == 0){
		return;
	}
	if(m_rtsSendFromQueueEvent.IsRunning()){
	  return;
	 }
	QueueEntry qEntry;
	m_queue.Dequeue(qEntry);
	SendInitialRTS(qEntry.GetRTS());

	Ptr<SAGLinkLayerGSL> myDec =m_ipv4->GetNetDevice(1)->GetObject<SAGLinkLayerGSL>();
	double q = myDec->GetQueueOccupancyRate();
	q = q / 100 * myDec->GetMaxsize();
	uint64_t cap = myDec->GetDataRate();
	double avepacketlength = 1500*8; //set average packet length = 1500byte
	double expected_queuingdelay = q * avepacketlength / (1.0 * cap);
	//std::cout<<"expected_queuingdelay"<<expected_queuingdelay<<std::endl;
	m_rtsSendFromQueueEvent = Simulator::Schedule(Seconds(expected_queuingdelay), &Iadr_Rout::RTSTimerExpire, this);

}


void
Iadr_Rout::SendInitialRTS (std::pair<RTSHeader,RTSPacket> rts)
{
	NS_LOG_FUNCTION (this);

	Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
	uint32_t i = 1;
	// not loopback ensured
	Ipv4InterfaceAddress ifaceAddress = l3->GetAddress (i, 0);
	if (ifaceAddress.GetLocal () == Ipv4Address ("127.0.0.1")){
		throw std::runtime_error("Must be not loopback");
	}

	// prepare empty RTS message
	Ptr<Packet> packet = Create<Packet> ();
	IadrHeader iadrHeader (/*packetLength=*/ 0, /*routerID=*/ Ipv4Address(m_ipv4->GetObject<Node>()->GetId()), /*areaID=*/ Ipv4Address::GetAny(), /*checkSum=*/ 0,
					/*auType=*/ 0, /*authentication=*/ 0);
	TypeHeader tHeader (IADRTYPE_RTS);
	RTSPHeader rtspHeader(rts);
	SocketIpTtlTag tag;
	tag.SetTtl (1);
	packet->AddPacketTag(tag);
	packet->AddHeader(iadrHeader);
	packet->AddHeader(rtspHeader);
	packet->AddHeader(tHeader);


	Ipv4Address destination;
	if (ifaceAddress.GetMask () == Ipv4Mask::GetOnes ())
	{
		destination = Ipv4Address ("255.255.255.255");
	}
	else
	{
		destination = ifaceAddress.GetBroadcast ();
	}
	Ptr<Ipv4Route> route = 0; //this can be nullptr for an auto search of forwarding table, or you can give it
	if(Simulator::Now()>= Seconds(25)){
		//std::cout<<"Simulator::Now() "<< Simulator::Now()<<"node "<< ifaceAddress.GetLocal()<<"sends rts to "<< destination <<std::endl;
	}
	DoSendRoutingProtocalPacket(packet, ifaceAddress.GetLocal(), destination, IADR_PROTOCOL, route);
	//SendRoutingProtocalPacket(packet, ifaceAddress.GetLocal(), destination, IADR_PROTOCOL, route);
}

Ptr<Ipv4Route>
Iadr_Rout::RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr){
	NS_LOG_FUNCTION (this << header << (oif ? oif->GetIfIndex () : 0));

	sockerr = Socket::ERROR_NOTERROR;
	Ptr<Ipv4Route> route;
	Ipv4Address dst = header.GetDestination ();
	SAGRoutingTableEntry rtEntry;
	bool found = m_routingTable->LookupRoute(dst, rtEntry);
	if (found)
	{
		route = rtEntry.GetRoute();
		NS_ASSERT (route != 0);
		NS_LOG_DEBUG ("Exist route to " << route->GetDestination () << " from interface " << route->GetSource ());
		if (oif != 0 && route->GetOutputDevice () != oif)
		{
		  NS_LOG_DEBUG ("Output device doesn't match. Dropped.");
		  sockerr = Socket::ERROR_NOROUTETOHOST;
		  return Ptr<Ipv4Route> ();
		}

		//RouteTrace(p);

		return route;
	}
	return Ptr<Ipv4Route> ();


}

bool
Iadr_Rout::RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
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
		if(header.GetProtocol() == IADR_PROTOCOL)
		{
			Ptr<Packet> packet = p->Copy ();
			RecvIADR(packet, localInterfaceAdr, origin);
			return true;
		}
		else
		{
			if (lcb.IsNull()) {
				throw std::runtime_error("Local callback cannot be null");
			}
			// Info: If you want to decide that a packet should not be delivered (dropped),
			//       you can decide that here by not calling lcb(), but still returning true.

			// tag route
			Ptr<Packet> packet = p->Copy ();// Because the tag needs to be modified, we need a non-const packet
			//RouteTrace(packet);
			lcb(packet, header, iif);

			return true;
		}
	}

	// Check if input device supports IP forwarding
	if (m_ipv4->IsForwarding(iif) == false) {
		throw std::runtime_error("Forwarding must be enabled for every interface");
	}

	// Uni-cast delivery
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
		Ptr<Packet> packet = p->Copy ();// Because the tag needs to be modified, we need a non-const packet
		//RouteTrace(packet);
		ucb(route, packet, header);
		return true;
	}
}






void
Iadr_Rout::DoInitialize (void)
{
	NS_LOG_FUNCTION (this);
	//uint16_t ContrllerNo=578;
	m_nb = Neighbors(m_helloInterval, m_rtrDeadInterval, m_rxmtInterval, m_LSRefreshTime, m_routcalInterval, m_ContrllerNo+m_satellitesNumber);
	m_nb.SetHelloTriggeringCallback (MakeCallback (&Iadr_Rout::HelloTriggeringBy1WayReceived, this));
	m_nb.SetDDTriggeringCallback(MakeCallback (&Iadr_Rout::DDTriggeringBy2WayReceived, this));
	m_nb.SetRTSTriggeringCallback(MakeCallback (&Iadr_Rout::RTSTriggeringAftRouterCalculator, this));
	//m_nb.SetIntialRTSTriggeringCallback(MakeCallback (&Iadr_Rout::InitialRTSTriggeringAftRouterCalculator, this));
	//m_routingTable = CreateObject<SAGRoutingTable>();
	//m_routeBuild = CreateObject<IadrBuildRouting>();
	//m_routeBuild= IadrBuildRouting();
	m_nb.SetRouterBuildCallback(MakeCallback(&IadrBuildRouting::RouterCalculate, m_routeBuild));
	m_nb.SetRouterId(Ipv4Address(m_ipv4->GetObject<Node>()->GetId()));
	m_nb.SetIpv4(m_ipv4);


	//m_routeBuild->SetRouter(m_routingTable);
	m_routeBuild->SetIpv4(m_ipv4);
	m_routeBuild->SetIntialRTSTriggeringCallback(MakeCallback (&Iadr_Rout::InitialRTSTriggeringAftRouterCalculator, this));
	m_routeBuild->SetSendFromServerQueueCallback(MakeCallback (&Iadr_Rout::RTSTimerExpire, this));
	m_routeBuild->SetRtrCalTimeEnable(m_rtrCalTimeConsidered);
	m_routeBuild->SetSatNum(m_satellitesNumber);
	m_routeBuild->SetGndNum(m_groundStationNumber);
	m_routeBuild->SetServerComputPower(m_serverComputingPower);

	uint32_t startTime = 0;
	if (m_enableHello)
	{
		m_htimer.SetFunction (&Iadr_Rout::HelloTimerExpire, this);
	    NS_LOG_DEBUG ("Starting at time " << startTime << "ms");
	    m_htimer.Schedule (MilliSeconds (startTime));
	}
	Ipv4RoutingProtocol::DoInitialize ();
}

void
Iadr_Rout::RecvIADR (Ptr<Packet> p, Ipv4Address receiver, Ipv4Address src)
{
	NS_LOG_FUNCTION (this);
	TypeHeader tHeader (IADRTYPE_HELLO);
	p->RemoveHeader (tHeader);
	if (!tHeader.IsValid ())
	{
	  NS_LOG_DEBUG ("IADR message " << p->GetUid () << " with unknown type received: " << tHeader.Get () << ". Drop");
	  return; // drop
	}
	switch (tHeader.Get ())
	{
		case IADRTYPE_HELLO:
		{
			RecvHELLO (p, receiver, src);
			break;
		}
		case IADRTYPE_DBD:
		{
			RecvDD (p, receiver, src);
			break;
		}
		//recieive RTS
		case IADRTYPE_RTS:
		{
			RecvRTS (p, receiver, src);
			break;
		}
	}

}


void
Iadr_Rout::RecvHELLO (Ptr<Packet> p, Ipv4Address receiver, Ipv4Address src)
{
	NS_LOG_FUNCTION (this << " src " << src);
	HelloHeader helloHeader;
	p->RemoveHeader (helloHeader);
	IadrHeader iadrHeader;
	p->RemoveHeader (iadrHeader);
	// Next, the values of the Network Mask, HelloInterval, and RouterDeadInterval fields in the received Hello packet must
	// be checked against the values configured for the receiving
	// However, there is one exception to the above rule: on point-to-point networks and on virtual links,
	// the Network Mask in the received Hello Packet should be ignored.
	if(helloHeader.GetHelloInterval() == m_helloInterval && helloHeader.GetRouterDeadInterval() == m_rtrDeadInterval){
		// judge whether this is the first time the neighbor has been detected, create a new data structure or update expireTime
		m_nb.HelloReceived (Ipv4Address(m_ipv4->GetObject<Node>()->GetId()),iadrHeader.GetAreaID(), receiver, iadrHeader.GetRouterID(), src, 0,
				Ipv4Address::GetZero(), Ipv4Address::GetZero(), m_rtrDeadInterval, helloHeader.GetNeighbors());
	}

}


void
Iadr_Rout::RecvDD (Ptr<Packet> p, Ipv4Address my,Ipv4Address src)
{

	NS_LOG_FUNCTION(this << " src " << src);
	Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
	LSUHeader lsu;
	p->RemoveHeader(lsu);
	DDHeader ddHeader;
	p->RemoveHeader(ddHeader);
	IadrHeader iadrHeader;
	p->RemoveHeader(iadrHeader);

	if(ddHeader.GetMTU() == l3->GetMtu((uint32_t)l3->GetInterfaceForAddress(my))){
		std::vector<std::pair<LSAHeader,LSAPacket>> lsas;
		lsas = lsu.GetLSAs();
		//std::cout<<"Received lsa number is"<<lsas.begin()->second.Getlinkn()<<std::endl;
		m_nb.DDReceived(Ipv4Address(m_ipv4->GetObject<Node>()->GetId()), src, ddHeader.GetFlags(), ddHeader.GetSeqNum(), lsas/*ddHeader.GetLSAHeaders()*/);
	}
	else{
		// the Database Description packet is rejected
	}

}

void
Iadr_Rout::RecvRTS (Ptr<Packet> p, Ipv4Address my, Ipv4Address src)
{
	//convergence_record.resize(m_satellitesNumber + m_groundStationNumber,1);
	//std::cout<<"recv rts"<<std::endl;
	NS_LOG_FUNCTION(this << " src " << src);
	Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
	RTSPHeader rtsp;
	p->RemoveHeader(rtsp);
	/*IadrHeader iadrHeader;
	p->RemoveHeader(iadrHeader);
	RTSHeader rtsHeader;
	p->RemoveHeader(rtsHeader);*/
	std::pair<RTSHeader, RTSPacket> rts;
	rts=rtsp.GetRTSs();
	if(m_ipv4->GetObject<Node>()->GetId()==149){
		//std::cout<<m_ipv4->GetObject<Node>()->GetId()<<"recvs rts for"<<rts.first.GetLocal_RouterID ()<<std::endl;
	}
	//if(m_ipv4->GetObject<Node>()->GetId()==6){std::cout<<"6 recv rts src:"<<rts.first.GetLocal_RouterID ().Get()<<std::endl;}
	//if(m_ipv4->GetObject<Node>()->GetId()==11&&rts.first.GetLocal_RouterID ().Get()==11){std::cout<<"recv rts size: "<<rts.second.Getsrcip().size()<<std::endl;}
	if (rts.first.GetLocal_RouterID ()== Ipv4Address(m_ipv4->GetObject<Node>()->GetId())){
		std::vector<Ipv4Address> srcips; srcips= rts.second.Getsrcip();
		std::vector<Ipv4Address> dsts; dsts= rts.second.Getdst();
		std::vector<Ipv4Address> nexthops; nexthops= rts.second.Getnexthop();
		for(uint32_t i=0;i< srcips.size();i++){
			if(m_ipv4->GetInterfaceForAddress(srcips[i])!=-1){
				//std::cout<< m_ipv4->GetInterfaceForAddress(srcips[i]) << " test interface"<<std::endl;
				Ptr<NetDevice> dec = m_ipv4->GetNetDevice(m_ipv4->GetInterfaceForAddress(srcips[i]));
				Ipv4InterfaceAddress itrAddress = m_ipv4->GetAddress(m_ipv4->GetInterfaceForAddress(srcips[i]),0);
				//uint32_t gateway = (uint32_t)l3->GetInterfaceForAddress(nexthops[i]);

				SAGRoutingTableEntry rtEntry(dec, dsts[i], itrAddress, nexthops[i]);
				if(m_ipv4->GetObject<Node>()->GetId()==40){
					//std::cout<<" dec:"<<dec<<" dsts[i]:"<<dsts[i]<</*"itrAddress:"<<itrAddress<<*/" nexthops[i]:"<<nexthops[i]<<std::endl;
				}
				m_routingTable->AddRoute(rtEntry);
			}
		}
		m_routeBuild->SetRouter(m_routingTable);
		//std::cout <<m_ipv4->GetObject<Node>()->GetId()<<" convergence time:"<<Simulator::Now().GetNanoSeconds() << std::endl;
		char currentDir[200];
		getcwd(currentDir,200);
		std::string currentDir_s(currentDir);
		std::string run_dir = currentDir_s+"/scratch/main_satnet/test_data/logs_ns3/";
		std::ofstream ofs;
		ofs.open(run_dir + format_string("route_convergence_node_%" PRIu64, m_ipv4->GetObject<Node>()->GetId()), std::ofstream::out | std::ofstream::app);
		ofs << m_ipv4->GetObject<Node>()->GetId() << "," << Simulator::Now().GetNanoSeconds() << std::endl;
		ofs.close();
	}

	else{
		m_nb.RTSReceived(Ipv4Address(m_ipv4->GetObject<Node>()->GetId()),src,rts);
	}

}

}
}


