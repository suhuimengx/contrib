
/*
 * traffic_light_based_routing.cc
 *
 *  Created on: 
 *      Author:
 */
#define NS_LOG_APPEND_CONTEXT                                   \
   std::clog << "[node " << m_nodeId << "] ";
//#define NS_LOG_CONDITION    if (m_nodeId==9)

#include "traffic_light_based_routing.h"
#include "ns3/log.h"
#include "ns3/route_trace_tag.h"
#include "ns3/sag_physical_layer_gsl.h"


namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("TrafficLightBasedRouting");

namespace tlr {
NS_OBJECT_ENSURE_REGISTERED (Traffic_Light_Based_Routing);
/// IP Protocol Number for TLR
const uint8_t Traffic_Light_Based_Routing::TLR_PROTOCOL = 159;

TypeId
Traffic_Light_Based_Routing::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::tlr::Traffic_Light_Based_Routing")
		  .SetParent<SAGRoutingProtocal>()
		  .SetGroupName("TLR")
		  .AddConstructor<Traffic_Light_Based_Routing>()
		  .AddAttribute ("HelloInterval", "HELLO messages interval.",
				   	    TimeValue (Seconds (250)),
						MakeTimeAccessor (&Traffic_Light_Based_Routing::m_helloInterval),
						MakeTimeChecker ())
		  .AddAttribute ("RouterDeadInterval", "Neighbor dead interval.",
						TimeValue (Seconds (250)),
						MakeTimeAccessor (&Traffic_Light_Based_Routing::m_rtrDeadInterval),
						MakeTimeChecker ())
		  .AddAttribute ("RetransmitInterval", "Message retransmit interval.",
						TimeValue (Seconds (5)),
						MakeTimeAccessor (&Traffic_Light_Based_Routing::m_rxmtInterval),
						MakeTimeChecker ())
		  .AddAttribute ("LSRefreshTime", "Interval at which LSAs are periodically generated.",
						TimeValue (Seconds (250)),
						MakeTimeAccessor (&Traffic_Light_Based_Routing::m_LSRefreshTime),
						MakeTimeChecker ())
  	  	  .AddAttribute ("EnableHello", "Indicates whether a hello messages enable.",
						 BooleanValue (true),
						 MakeBooleanAccessor (&Traffic_Light_Based_Routing::SetHelloEnable,
											  &Traffic_Light_Based_Routing::GetHelloEnable),
						 MakeBooleanChecker ())
  	  	  .AddAttribute ("UniformRv",
						"Access to the underlying UniformRandomVariable",
						StringValue ("ns3::UniformRandomVariable"),
						MakePointerAccessor (&Traffic_Light_Based_Routing::m_uniformRandomVariable),
						MakePointerChecker<UniformRandomVariable> ())
         .AddAttribute ("CheckNowTrafficColorInterval", "Check CurHop Traffic Color interval.",
				   	    TimeValue (MilliSeconds(10)),
						MakeTimeAccessor (&Traffic_Light_Based_Routing::m_checktrafficcolorInterval),
						MakeTimeChecker ())
		 .AddAttribute("CheckWaitingQueueInterval", "Check  Public Waiting List Interval.",
				        TimeValue(MilliSeconds(15)),
				        MakeTimeAccessor(&Traffic_Light_Based_Routing::m_checkwaitinglistInterval),
				        MakeTimeChecker());
	return tid;
}

Traffic_Light_Based_Routing::Traffic_Light_Based_Routing()
	: m_enableHello(false),
	  m_lasttrafficlightcolor(0),
	  T1(0.1),//Traffic Light Color Setting Rules
	  T2(0.6),
	  Tgy(0.1),
	  Tyr(0.6),
	  PWQueue(100)// maxnum packet of public waiting queue
//	m_helloInterval(Seconds(5)),
//	m_rtrDeadInterval(Seconds(10)),
//	m_rxmtInterval(Seconds(5)),
//	m_LSRefreshTime(Seconds(300))

{
	m_trafficlightcolor.clear();//not safe?

	m_routingTable = CreateObject<TLRRoutingTable>();
	m_routeBuild = CreateObject<TlrBuildRouting>();
	m_routeBuild->SetRouter(m_routingTable);
	m_routeBuild->SetRoutingCalculationCallback(MakeCallback(&SAGRoutingProtocal::CalculationTimeLog, this));
}

Traffic_Light_Based_Routing::~Traffic_Light_Based_Routing()
{
	NS_LOG_FUNCTION(this);
}




void
Traffic_Light_Based_Routing::NotifyInterfaceUp(uint32_t i)
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
Traffic_Light_Based_Routing::NotifyInterfaceDown (uint32_t i){

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


	m_nb.DeleteNeighbor(/*routerID=*/ Ipv4Address(m_ipv4->GetObject<Node>()->GetId()), /*localInterfaceAddress=*/if_addr);

	for(uint32_t k = 0; k < m_helloTimeExpireRecord.size(); k++){
		if(m_helloTimeExpireRecord[k].first == if_addr){
			m_helloTimeExpireRecord.erase(m_helloTimeExpireRecord.begin() + k);
			break;
		}
	}

}

void
Traffic_Light_Based_Routing::NotifyAddAddress(uint32_t interface, Ipv4InterfaceAddress address)
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
Traffic_Light_Based_Routing::NotifyRemoveAddress(uint32_t interface, Ipv4InterfaceAddress address)
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

	m_nb.DeleteNeighbor(/*routerID=*/ Ipv4Address(m_ipv4->GetObject<Node>()->GetId()), /*localInterfaceAddress=*/if_addr);

	for(uint32_t k = 0; k < m_helloTimeExpireRecord.size(); k++){
		if(m_helloTimeExpireRecord[k].first == if_addr){
			m_helloTimeExpireRecord.erase(m_helloTimeExpireRecord.begin() + k);
			break;
		}
	}



}

void
Traffic_Light_Based_Routing::SendHello (Ipv4Address ad){
	NS_LOG_FUNCTION (this);
	Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
	int32_t i = l3->GetInterfaceForAddress(ad);

	if(i == int32_t(-1)){
		throw std::runtime_error ("Wrong interface address.");
	}

	i = (uint32_t)i;

	if (l3->GetNAddresses (i) > 1){
		throw std::runtime_error ("TLR does not work with more then one address per each interface.");
	}
	// not loopback ensured
	Ipv4InterfaceAddress ifaceAddress = l3->GetAddress (i, 0);
	if (ifaceAddress.GetLocal () == Ipv4Address ("127.0.0.1")){
		throw std::runtime_error("Must be not loopback");
	}

	// prepare hello message
	Ptr<Packet> packet = Create<Packet> ();
	// routerId -> lookback address all the same: l3->GetAddress (0, 0).GetLocal()
	TlrHeader tlrHeader (/*packetLength=*/ 0, /*routerID=*/ Ipv4Address(m_ipv4->GetObject<Node>()->GetId()), /*areaID=*/ Ipv4Address::GetAny(), /*checkSum=*/ 0,
					/*auType=*/ 0, /*authentication=*/ 0);
	HelloHeader helloHeader (/*mask=*/ ifaceAddress.GetMask().Get(), /*helloInterval=*/ m_helloInterval, /*options=*/ uint8_t(0), /*rtrPri=*/ uint8_t(1),
			/*rtrDeadInterval=*/ m_rtrDeadInterval, /*dr=*/ Ipv4Address::GetAny(), /*bdr=*/ Ipv4Address::GetAny(), /*neighors=*/ m_nb.GetNeighbors());

	TypeHeader tHeader (TLRTYPE_HELLO);
	SocketIpTtlTag tag;
	tag.SetTtl (1);
	packet->AddPacketTag (tag);
	packet->AddHeader (tlrHeader);
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
	SendRoutingProtocalPacket(packet, ifaceAddress.GetLocal(), destination, TLR_PROTOCOL, route);

}
void
Traffic_Light_Based_Routing::SendNotify(Ipv4Address ad, Ipv4Address nb, uint32_t id,uint8_t color) {
	NS_LOG_FUNCTION(this);
	Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol>();
	uint32_t i = (uint32_t)l3->GetInterfaceForAddress(ad);
	// prepare empty Notify message
	Ptr<Packet> packet = Create<Packet>();
	TlrHeader tlrHeader(/*packetLength=*/ 0, /*routerID=*/ Ipv4Address(m_ipv4->GetObject<Node>()->GetId()), /*areaID=*/ Ipv4Address::GetAny(), /*checkSum=*/ 0,
		/*auType=*/ 0, /*authentication=*/ 0);
	TypeHeader tHeader(TLRTYPE_NOTIFY);
	SocketIpTtlTag tag;
	tag.SetTtl(1);

	NotifyHeader notifyHeader(id,color);

	packet->AddPacketTag(tag);
	packet->AddHeader(tlrHeader);
	packet->AddHeader(notifyHeader);
	packet->AddHeader(tHeader);

	// Unicast
	Ipv4Address destination = nb;
	Ptr<Ipv4Route> route = Create<Ipv4Route>();
	route->SetDestination(destination);
	route->SetGateway(destination);
	route->SetSource(ad);
	route->SetOutputDevice(l3->GetNetDevice(i));
	SendRoutingProtocalPacket(packet, ad, destination, TLR_PROTOCOL, route);

}
void
Traffic_Light_Based_Routing::SendDD (Ipv4Address ad, Ipv4Address nb, uint8_t flags, uint32_t seqNum, std::vector<LSAHeader> lsaHeaders)
{
	NS_LOG_FUNCTION (this);
	Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
	uint32_t i = (uint32_t)l3->GetInterfaceForAddress(ad);
	// prepare empty DD message
	Ptr<Packet> packet = Create<Packet> ();
	TlrHeader tlrHeader (/*packetLength=*/ 0, /*routerID=*/ Ipv4Address(m_ipv4->GetObject<Node>()->GetId()), /*areaID=*/ Ipv4Address::GetAny(), /*checkSum=*/ 0,
					/*auType=*/ 0, /*authentication=*/ 0);
	TypeHeader tHeader (TLRTYPE_DBD);
	SocketIpTtlTag tag;
	tag.SetTtl (1);


	uint16_t mtu = l3->GetMtu(i); // 1500 by default
	DDHeader ddHeader(mtu, 0, flags, seqNum, lsaHeaders); // option no use

	packet->AddPacketTag(tag);
	packet->AddHeader(tlrHeader);
	packet->AddHeader(ddHeader);
	packet->AddHeader(tHeader);

	// Unicast
	Ipv4Address destination = nb;
	Ptr<Ipv4Route> route = Create<Ipv4Route>();
	route->SetDestination(destination);
	route->SetGateway(destination);
	route->SetSource(ad);
	route->SetOutputDevice(l3->GetNetDevice(i));
	SendRoutingProtocalPacket(packet, ad, destination, TLR_PROTOCOL, route);


}

void
Traffic_Light_Based_Routing::SendLSR (Ipv4Address ad, Ipv4Address nb,std::vector <LSAHeader> lsaHeaders)
{
	NS_LOG_FUNCTION (this);
	Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
	uint32_t i = (uint32_t)l3->GetInterfaceForAddress(ad);
	// prepare empty LSR message
	Ptr<Packet> packet = Create<Packet> ();
	TlrHeader tlrHeader (/*packetLength=*/ 0, /*routerID=*/ Ipv4Address(m_ipv4->GetObject<Node>()->GetId()), /*areaID=*/ Ipv4Address::GetAny(), /*checkSum=*/ 0,
					/*auType=*/ 0, /*authentication=*/ 0);
	TypeHeader tHeader (TLRTYPE_LSR);
	SocketIpTtlTag tag;
	tag.SetTtl (1);


	std::vector<LSRPacket> LSRs = {};

	for (std::vector<LSAHeader>::iterator i = lsaHeaders.begin(); i != lsaHeaders.end(); ++i)
	{
		 LSRPacket LSR (i->GetLinkStateID(),i->GetAdvertisiongRouter());
		 LSRs.push_back(LSR);
	}

	LSRHeader lsrHeader(LSRs);

	packet->AddPacketTag(tag);
	packet->AddHeader(tlrHeader);
	packet->AddHeader(lsrHeader);
	packet->AddHeader(tHeader);


	// Unicast
	Ipv4Address destination = nb;
	Ptr<Ipv4Route> route = Create<Ipv4Route>();
	route->SetDestination(destination);
	route->SetGateway(destination);
	route->SetSource(ad);
	route->SetOutputDevice(l3->GetNetDevice(i));
	SendRoutingProtocalPacket(packet, ad, destination, TLR_PROTOCOL, route);
}

void
Traffic_Light_Based_Routing::SendLSU (Ipv4Address ad, Ipv4Address nb,std::vector<std::pair<LSAHeader,LSAPacket>> lsas)
{
	NS_LOG_FUNCTION (this);
	Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
	uint32_t i = (uint32_t)l3->GetInterfaceForAddress(ad);
	// prepare empty LSR message
	Ptr<Packet> packet = Create<Packet> ();
	TlrHeader tlrHeader (/*packetLength=*/ 0, /*routerID=*/ Ipv4Address(m_ipv4->GetObject<Node>()->GetId()), /*areaID=*/ Ipv4Address::GetAny(), /*checkSum=*/ 0,
					/*auType=*/ 0, /*authentication=*/ 0);
	TypeHeader tHeader (TLRTYPE_LSU);
	SocketIpTtlTag tag;
	tag.SetTtl (1);




	LSUHeader lsuHeader(lsas); // option no use

	packet->AddPacketTag(tag);
	packet->AddHeader(tlrHeader);
	packet->AddHeader(lsuHeader);
	packet->AddHeader(tHeader);


	// Unicast
	Ipv4Address destination = nb;
	Ptr<Ipv4Route> route = Create<Ipv4Route>();
	route->SetDestination(destination);
	route->SetGateway(destination);
	route->SetSource(ad);
	route->SetOutputDevice(l3->GetNetDevice(i));
	SendRoutingProtocalPacket(packet, ad, destination, TLR_PROTOCOL, route);

}

void
Traffic_Light_Based_Routing::SendLSAack (Ipv4Address ad, Ipv4Address nb,std::vector<LSAHeader> lsaheaders)
{
	NS_LOG_FUNCTION (this);
	Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
	uint32_t i = (uint32_t)l3->GetInterfaceForAddress(ad);
	// prepare empty LSAack message
	Ptr<Packet> packet = Create<Packet> ();
	TlrHeader tlrHeader (/*packetLength=*/ 0, /*routerID=*/ Ipv4Address(m_ipv4->GetObject<Node>()->GetId()), /*areaID=*/ Ipv4Address::GetAny(), /*checkSum=*/ 0,
					/*auType=*/ 0, /*authentication=*/ 0);
	TypeHeader tHeader (TLRTYPE_LSAck);
	SocketIpTtlTag tag;
	tag.SetTtl (1);



	// There is no delayed acknowledgment, that is, it is only limited by the number of LSAs in the LSU,
	// so the number of LSAs confirmed at this time can meet the requirements of forming an LSAACK
	LSAackHeader lsaack (lsaheaders); // option no use

	packet->AddPacketTag(tag);
	packet->AddHeader(tlrHeader);
	packet->AddHeader(lsaack);
	packet->AddHeader(tHeader);


	// Unicast
	Ipv4Address destination = nb;
	Ptr<Ipv4Route> route = Create<Ipv4Route>();
	route->SetDestination(destination);
	route->SetGateway(destination);
	route->SetSource(ad);
	route->SetOutputDevice(l3->GetNetDevice(i));
	SendRoutingProtocalPacket(packet, ad, destination, TLR_PROTOCOL, route);

}

static bool cmp(const std::pair<Ipv4Address,Time>& a, const std::pair<Ipv4Address,Time>& b)
{
	return a.second < b.second;
}

void
Traffic_Light_Based_Routing::HelloTimerExpire ()
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
Traffic_Light_Based_Routing::HelloTriggeringBy1WayReceived (Ipv4Address ad){
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
Traffic_Light_Based_Routing::DDTriggeringBy2WayReceived (Ipv4Address ad, Ipv4Address nb, uint8_t flags, uint32_t seqNum, std::vector<LSAHeader> lsa){
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
Traffic_Light_Based_Routing::LSUTriggeringByLSRReceived(Ipv4Address ad, Ipv4Address nb,std::vector<std::pair<LSAHeader,LSAPacket>> lsas)
{
	NS_LOG_FUNCTION (this);
	SendLSU(ad,nb,lsas);
}

void
Traffic_Light_Based_Routing::LSAaackTriggeringByLSUReceived(Ipv4Address ad, Ipv4Address nb,std::vector<LSAHeader> lsaheaders)
{
	NS_LOG_FUNCTION (this);
	SendLSAack(ad,nb,lsaheaders);
}

void
Traffic_Light_Based_Routing::LSRTriggering(Ipv4Address ad, Ipv4Address nb,std::vector <LSAHeader> lsaHeaders)
{
	NS_LOG_FUNCTION (this);
	SendLSR(ad, nb, lsaHeaders);
}

void
Traffic_Light_Based_Routing::NOTIFYTriggering(Ipv4Address ad, Ipv4Address dst, uint32_t id,uint8_t color)
{
 NS_LOG_FUNCTION(this);
 SendNotify(ad, dst, id, color);

}

Ptr<Ipv4Route>
Traffic_Light_Based_Routing::RouteOutput(Ptr<Packet> p, const Ipv4Header& header, Ptr<NetDevice> oif, Socket::SocketErrno& sockerr) {
	NS_LOG_FUNCTION(this << header << (oif ? oif->GetIfIndex() : 0));

	sockerr = Socket::ERROR_NOTERROR;
	Ptr<Ipv4Route> route;
	Ipv4Address dst = header.GetDestination();
	TLRRoutingTableEntry rtEntry;
	bool found = m_routingTable->LookupRoute(dst, rtEntry);

	if (found)
	{
		route = rtEntry.GetRoute();
		 NS_ASSERT(route != 0);
		 NS_LOG_DEBUG("Exist route to " << route->GetDestination() << " from interface " << route->GetSource());
			if (oif != 0 && route->GetOutputDevice() != oif)
			{
				NS_LOG_DEBUG("Output device doesn't match. Dropped.");
				sockerr = Socket::ERROR_NOROUTETOHOST;
				return Ptr<Ipv4Route>();
			}

			RouteTrace(p);

			return route;
		}

	return Ptr<Ipv4Route>();

}

int
Traffic_Light_Based_Routing::JudgeTrafficLightColor(Ipv4Address InterfaceAdr, uint32_t NextHop)
{
	uint32_t itface = m_ipv4->GetInterfaceForAddress(InterfaceAdr);
	//uint32_t num = m_ipv4->GetNInterfaces();
	// // std::cout<<" num "<<num<<"itface "<<itface<<" InterfaceAdr "<<InterfaceAdr<<" id "<<m_ipv4->GetObject<Node>()->GetId()<<std::endl;
	int ans = 0;
	if(m_ipv4->GetNetDevice(itface)->GetInstanceTypeId() == TypeId::LookupByName ("ns3::PointToPointLaserNetDevice")){
	if (itface <= 4 && itface != 0) //
		{
		  // // std::cout<<" num "<<num<<"itface "<<itface<<" InterfaceAdr "<<InterfaceAdr<<" id "<<m_ipv4->GetObject<Node>()->GetId()<<std::endl;
		   Ptr<PointToPointLaserNetDevice> myDec = m_ipv4->GetNetDevice(itface)->GetObject<PointToPointLaserNetDevice>();
		   double q = myDec->GetQueueOccupancyRate();
		   q = q / 100;
		   if (q<T1) ans = m_trafficlightcolor[NextHop];
		   else if (q >= T1 && q < T2) {
			   if (m_trafficlightcolor[NextHop]<=1) ans = 1;
			    else ans = 2;
		      }
		   else ans = 2;
		}
	}
 //// std::cout<<"ans "<<ans<<" InterfaceAdr "<<InterfaceAdr<<" NextHop "<<NextHop<<std::endl;
 return ans;
}


Ptr<Ipv4Route>
Traffic_Light_Based_Routing::GetOptimizedRoute(Ptr<const Packet> p, const Ipv4Header& header, Ipv4Address InterfaceAdr,
		                                       UnicastForwardCallback ucb,LocalDeliverCallback lcb, int32_t iif) {
	NS_LOG_FUNCTION(this << header );
	//// std::cout<<"GetOptimizedRoute "<<m_ipv4->GetObject<Node>()->GetId()<<std::endl;
	Ptr<Ipv4Route> route;
	Ipv4Address dst = header.GetDestination();
	TLRRoutingTableEntry rtEntry;
	bool found = m_routingTable->LookupRoute(dst, rtEntry);

	if (found)
	{
		route = rtEntry.GetRoute();
		uint32_t BRnxtHopid = rtEntry.GetNextHopId();
		uint32_t SBRnxtHopid = rtEntry.GetSecNextHopId();
		if (BRnxtHopid == SBRnxtHopid)
		{
			NS_ASSERT(route != 0);
			NS_LOG_DEBUG("Exist route to " << route->GetDestination() << " from interface " << route->GetSource());
			return route;
		}
		else {
			NS_ASSERT(route != 0);
			Ipv4Address dst = header.GetDestination();
			if (rtEntry.GetNextHop() == dst )
			{
				// std::cout<<"rtEntry.GetNextHop() == dst "<<dst<<std::endl;
				return route;
			}
			else
			{
				Ptr<Ipv4Route> Newroute  = rtEntry.GetSecRoute();
				Ipv4Address secInterfaceAdr=rtEntry.GetSecInterface().GetLocal();
				//return Newroute means send packet to second best next hop
				RouteTraceTag rtTrTag;
				int flagfind1 = 0, flagfind2 = 0;
				std::vector<uint32_t>::iterator it1;
				std::vector<uint32_t>::iterator it2;
				std::vector<uint32_t> nodesPassed;
				if (p->PeekPacketTag(rtTrTag))
					//flagfind1=0:no found BRnxthopid in  router path  1:the opposite
				    //flagfind2=0:no found SBRnxthopid in  router path 1:the opposite
				{
					nodesPassed = rtTrTag.GetRouteTrace();
					it1 = find(nodesPassed.begin(), nodesPassed.end(), BRnxtHopid);
					it2 = find(nodesPassed.begin(), nodesPassed.end(), SBRnxtHopid);
					if (it1 != nodesPassed.end()) flagfind1 = 1;
					if (it2 != nodesPassed.end()) flagfind2 = 1;

				}

				if (flagfind1 == 0 && flagfind2 == 0)
				{
					if (JudgeTrafficLightColor(InterfaceAdr,BRnxtHopid) == 0)
					{
						return route;
					}
				//	// std::cout<<"herebug "<<InterfaceAdr<<" "<<BRnxtHopid<<" "<<secInterfaceAdr<<" "<<SBRnxtHopid<<std::endl;
					if (JudgeTrafficLightColor(InterfaceAdr, BRnxtHopid) == 1)
					{

						if (JudgeTrafficLightColor(secInterfaceAdr, SBRnxtHopid) != 2)
						{
							uint32_t random_num = (BRnxtHopid & SBRnxtHopid) % 2;
							if (random_num) return route;
							else
								{
								// // std::cout<<"NewRoute"<<rtEntry.GetSecondNextHop()<<" "<<rtEntry. GetSecNextHopId()<<std::endl;
						          return Newroute;
							  	}


						}
						else return route;
					}
					if (JudgeTrafficLightColor(InterfaceAdr, BRnxtHopid) == 2)
					{
						if (JudgeTrafficLightColor(secInterfaceAdr, SBRnxtHopid) != 2)
						{
						//	// std::cout<<"NewRoute"<<rtEntry.GetSecondNextHop()<<" "<<rtEntry. GetSecNextHopId()<<std::endl;
						  return Newroute;
						}
						else
						{
							QueueEntry newEntry(p, header, rtEntry.GetNextHop(), BRnxtHopid, ucb, lcb, iif);
							PWQueue.Enqueue(newEntry);
							return Ptr<Ipv4Route>();
						}

					}
				}
				if (flagfind1 == 0 && flagfind2 == 1)
				{
					if (JudgeTrafficLightColor(InterfaceAdr, BRnxtHopid) != 2)
						return route;
					else
					{
						QueueEntry newEntry(p, header, rtEntry.GetNextHop(),BRnxtHopid, ucb, lcb, iif);
						PWQueue.Enqueue(newEntry);
						return Ptr<Ipv4Route>();
					}
				}
				if (flagfind1 == 1 && flagfind2 == 0)
				{
					if (JudgeTrafficLightColor(secInterfaceAdr, SBRnxtHopid) != 2)
					{
					//	// std::cout<<"NewRoute"<<rtEntry.GetSecondNextHop()<<" "<<rtEntry. GetSecNextHopId()<<std::endl;
					 return Newroute;
					}
					else
					{
						QueueEntry newEntry(p, header, rtEntry.GetSecondNextHop(),SBRnxtHopid, ucb, lcb, iif);
						PWQueue.Enqueue(newEntry);
						return Ptr<Ipv4Route>();
					}
				}
				if (flagfind1 == 1 && flagfind2 == 1)
				{
					uint32_t curNodeid = m_ipv4->GetObject<Node>()->GetId();
					std::vector<uint32_t>::iterator it3;
					it3 = find(nodesPassed.begin(), nodesPassed.end(), curNodeid);
					uint32_t preNodeid;
					if (it3 - nodesPassed.begin() - 1 >= 0)
						preNodeid = nodesPassed[it3 - nodesPassed.begin() - 1];
					else return Ptr<Ipv4Route>();
					Ipv4Address preNodeAddress;
					TLRRoutingTableEntry rt;
					bool found2 = m_routingTable->LookupInterfaceFromNode(preNodeid,preNodeAddress);
					if (found2)
					{
					  bool foundroute = m_routingTable->LookupRoute(preNodeAddress, rt);
					  if (foundroute)
					  {
					  Ptr<Ipv4Route> Backroute = rt.GetRoute();
					  // std::cout<<"BackRoute"<<rt.GetSecondNextHop()<<" "<<rt. GetSecNextHopId()<<std::endl;
					  return Backroute;
					  }
					  else return Ptr<Ipv4Route>();;
				    }
					else return Ptr<Ipv4Route>();
				}
			}
		}
	}
	return Ptr<Ipv4Route>();
}

bool
Traffic_Light_Based_Routing::RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
			   	   	   	   	   	   	  UnicastForwardCallback ucb, MulticastForwardCallback mcb,
									  LocalDeliverCallback lcb, ErrorCallback ecb)
{
	NS_LOG_FUNCTION(this << p->GetUid() << header.GetDestination() << idev->GetAddress());
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
		if(header.GetProtocol() == TLR_PROTOCOL)
		{
			Ptr<Packet> packet = p->Copy ();
			RecvTLR(packet, localInterfaceAdr, origin);
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
			RouteTrace(packet);
		    lcb(packet, header, iif);

			return true;
		}
	}

	// Check if input device supports IP forwarding
	if (m_ipv4->IsForwarding(iif) == false) {
		throw std::runtime_error("Forwarding must be enabled for every interface");
	}

	// Uni-cast delivery
	TLRRoutingTableEntry rtEntry;
	bool found = m_routingTable->LookupRoute(dst, rtEntry);
	if (!found) {
		// Lookup failed, so we did not find a route
		// If there are no other routing protocols, this will lead to a drop
		return false;
	} else {
		// Lookup succeeded in producing a route
		// So we perform the unicast callback to forward there
		//Ptr<NetDevice> netDevice = l3->GetNetDevice(itface);
		Ptr<Ipv4Route> route={};
		//static int cnt=0;
		//if(idev->GetInstanceTypeId() == TypeId::LookupByName ("ns3::PointToPointLaserNetDevice"))
		//uint32_t nxtitface = m_ipv4->GetInterfaceForAddress(rtEntry.GetNextHop());
		if (m_ipv4->GetObject<Node>()->GetId() <= SatelliteMaxid)
	   // if(m_ipv4->GetNetDevice(nxtitface)->GetInstanceTypeId() == TypeId::LookupByName ("ns3::PointToPointLaserNetDevice"))
		{
		 //// std::cout<<m_ipv4->GetObject<Node>()->GetId()<<std::endl;
		 route = GetOptimizedRoute(p->Copy (), header, rtEntry.GetInterface().GetLocal() , ucb, lcb, iif);
		// if (++cnt==1)
		// // std::cout<<"GetOptimizedRoute "<<m_ipv4->GetObject<Node>()->GetId()<<std::endl;
		}
	     else route = rtEntry.GetRoute();

		//Ptr<Ipv4Route> route={};
		//NS_ASSERT (route != 0);
		if (route !=0 )
		{
		 Ptr<Packet> packet = p->Copy ();// Because the tag needs to be modified, we need a non-const packet
		 RouteTrace(packet);
	     ucb(route, packet, header);
		}
		return true;
	}

}
void
Traffic_Light_Based_Routing::CheckWaitingList(void)
{
	NS_LOG_FUNCTION(this);
//	// std::cout<<"CheckWaitingList"<<"    "<<Simulator::Now()<<std::endl;
	std::vector<Ipv4Address> nownb = m_nb.GetNeighbors();
    // #define m_queue.mqueue mqueue
	    //std::vector<QueueEntry> nowq=&(m_queue.m_queue);
		for (std::vector<QueueEntry>::iterator i = PWQueue.m_queue.begin(); i != PWQueue.m_queue.end();  )
		{
			bool flag = false;
			QueueEntry nowcheckpacket= *i;
			Ipv4Address flagadd;
			for (std::vector<Ipv4Address>::iterator it = nownb.begin(); it != nownb.end(); ++it)
			{
				if (i->GetNextHop() == nownb[it-nownb.begin()])
				{
				    flagadd = nownb[it-nownb.begin()];
					flag = true;
					break;
			    }
			}
			// std::cout<<"flag is "<<flag<<" flagadd "<<flagadd<<" "<<i->GetNextHopId()<<std::endl;
			if (flag)
			{
				if (m_trafficlightcolor[i->GetNextHopId()] != 2)
				{

					TLRRoutingTableEntry rtEntry;
					Ptr<Ipv4Route> route;
					bool found = m_routingTable->LookupRoute(nowcheckpacket.GetIpv4Header().GetDestination(), rtEntry);
					if (found) {
					route = rtEntry.GetRoute();
					NS_ASSERT(route != 0);
					Ptr<Packet> packet = ConstCast<Packet> (nowcheckpacket.GetPacket());// Because the tag needs to be modified, we need a non-const packet
					//// std::cout<<packet<<" my id "<<m_ipv4->GetObject<Node>()->GetId()<<" nxt id "<< rtEntry.GetNextHopId() <<"  "<<nowcheckpacket.GetIpv4Header().GetDestination()<<std::endl;
					RouteTraceTag rtTrTag;
					std::vector<uint32_t> nodesPassed;
					bool flagfind = 0;
					if (packet->PeekPacketTag(rtTrTag)){
					nodesPassed = rtTrTag.GetRouteTrace();
					std::vector<uint32_t>::iterator  it1 = find(nodesPassed.begin(), nodesPassed.end(), m_ipv4->GetObject<Node>()->GetId());
					if (it1 != nodesPassed.end()) flagfind = 1;
					}
					if (flagfind == 0){
					RouteTrace(packet);
//					// std::cout<<route<<std::endl;
//
//					RouteTraceTag rtTrTag;
//					std::vector<uint32_t> nodesPassed;
//					if (packet->PeekPacketTag(rtTrTag)){
//					nodesPassed = rtTrTag.GetRouteTrace();
//					for (std::vector<uint32_t> :: iterator tt = nodesPassed.begin(); tt != nodesPassed.end();tt++)
//						// std::cout<<"/"<<*tt;
//					    // std::cout<<""<<std::endl;
//				}
					UnicastForwardCallback ucb = nowcheckpacket.GetUnicastForwardCallback ();
				    if (route != 0) ucb(route, packet, nowcheckpacket.GetIpv4Header());
				    // std::cout<<"SendPacketFromQueueUCB "<<std::endl;
					}

					i = PWQueue.m_queue.erase(i);
					continue;
					}
				}
				else
				{  uint8_t t = nowcheckpacket.GetTTW();
		    	   t=t-1;
		    	   if (t == 0)
		    		   {
		    		    i = PWQueue.m_queue.erase(i);
		    		    continue;
		    		   }
		    	     else nowcheckpacket.SetTTW(t);
				}
			}
			else {
				Ptr<Packet> packet = nowcheckpacket.GetPacket()->Copy();// Because the tag needs to be modified, we need a non-const packet
				RouteTrace(packet);
				LocalDeliverCallback lcb = nowcheckpacket.GetLocalDeliverCallback ();
				lcb( nowcheckpacket.GetPacket(), nowcheckpacket.GetIpv4Header(),nowcheckpacket.GetIIF());
				 // std::cout<<"SendPacketFromQueueLCB"<<std::endl;
				//m_queue.Drop(nowq[i-nowq.begin()], " forward pkt to curHop again ");
				 i = PWQueue.m_queue.erase(i);
				 continue;
			  }
              i++;
			}

		m_TriggingCheckWaitingListEvent=Simulator::Schedule(m_checkwaitinglistInterval,&Traffic_Light_Based_Routing::CheckWaitingList,this);

}

void
Traffic_Light_Based_Routing::CheckNowTrafficColor(void)
{
 NS_LOG_FUNCTION(this);
 uint8_t newest_color = 0;
 double qsum = 0;
 int sum = 0;
 //Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
 //Ptr<NetDevice> netDevice = l3->GetNetDevice(1);
 //if(netDevice->GetInstanceTypeId() == TypeId::LookupByName ("ns3::PointToPointLaserNetDevice")){
 uint32_t num = m_ipv4->GetNInterfaces();
 //// std::cout<<"num"<<num<<std::endl;

 if (num<=2) return;  // skip ground stations, 2 interfaces by default for ground station, 0: lookback 1: gsl interface
 //// std::cout<<"num"<<num<<std::endl;
 //NS_ASSERT (num == 6); // 6 interfaces by default for satellite, 0: lookback 1 2 3 4: isl interfaces 5: gsl interface
 num = num - 2;
 for (uint32_t i = 1; i <= num; i++)
 {
  Ptr<PointToPointLaserNetDevice> myDec = m_ipv4->GetNetDevice(i)->GetObject<PointToPointLaserNetDevice>();
  double q = myDec->GetQueueOccupancyRate();
//  if(q > 0){
//   // std::cout<<q<<std::endl;
//  }
  qsum = qsum + q;
  sum++;
 }
 qsum /= sum;
 qsum /= 100;//now maxsize packet
 if (qsum<=Tgy) newest_color=0;
  else if (qsum > Tgy && qsum < Tyr) newest_color = 1;
  else newest_color = 2;
 //// std::cout<<"CheckNowTrafficColor"<<"    "<<qsum<<std::endl;
 //// std::cout<<"CheckNowTrafficColor"<<"    "<<newest_color<<"      "<<m_lasttrafficlightcolor<<std::endl;
 if (newest_color != m_lasttrafficlightcolor)
 {
  m_lasttrafficlightcolor = newest_color;
  Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
  for (uint32_t i = 1; i <= num; i++)//uint32_t num = m_ipv4->GetNInterfaces()-2;
  {
   Ipv4Address nowaddress = l3->GetAddress (i, 0).GetLocal();
   Ipv4Address dst = l3->GetAddress (i, 0).GetBroadcast();
   // std::cout<<"NotifyTriggering "<<nowaddress<<" "<<dst<<" "<<m_ipv4->GetObject<Node>()->GetId()<<" color "<<int(newest_color)<<std::endl;
   NOTIFYTriggering(nowaddress, dst, m_ipv4->GetObject<Node>()->GetId(), newest_color);
  }
 }

 m_TriggingCheckTrafficColorEvent=Simulator::Schedule(m_checktrafficcolorInterval,&Traffic_Light_Based_Routing::CheckNowTrafficColor,this);


}



void
Traffic_Light_Based_Routing::DoInitialize (void)
{
	NS_LOG_FUNCTION (this);
	//// std::cout<<m_LSRefreshTime<<std::endl;
	m_nb = Neighbors(m_helloInterval, m_rtrDeadInterval, m_rxmtInterval, m_LSRefreshTime);
	m_nb.SetHelloTriggeringCallback (MakeCallback (&Traffic_Light_Based_Routing::HelloTriggeringBy1WayReceived, this));
	m_nb.SetDDTriggeringCallback(MakeCallback (&Traffic_Light_Based_Routing::DDTriggeringBy2WayReceived, this));
	m_nb.SetLSUTriggeringCallback(MakeCallback(&Traffic_Light_Based_Routing::LSUTriggeringByLSRReceived,this));
	m_nb.SetLSATriggeringCallback(MakeCallback(&Traffic_Light_Based_Routing::LSAaackTriggeringByLSUReceived,this));
	m_nb.SetLSRTriggeringCallback(MakeCallback(&Traffic_Light_Based_Routing::LSRTriggering,this));
	m_lasttrafficlightcolor = 0;
	m_nb.SetRouterBuildCallback(MakeCallback(&TlrBuildRouting::RouterCalculate, m_routeBuild));
	m_routeBuild->SetRtrCalTimeEnable(m_rtrCalTimeConsidered);

	SatelliteMaxid = m_satellitesNumber-1;
	m_routeBuild->SetMaxSatelliteId(m_satellitesNumber-1);



	m_nb.SetRouterId(Ipv4Address(m_ipv4->GetObject<Node>()->GetId()));
	m_routeBuild->SetIpv4(m_ipv4);
	m_nb.SetIpv4(m_ipv4);
	uint32_t startTime = 0;
	if (m_enableHello)
	{
	  m_htimer.SetFunction (&Traffic_Light_Based_Routing::HelloTimerExpire, this);
	  NS_LOG_DEBUG ("Starting at time " << startTime << "ms");
	  m_htimer.Schedule (MilliSeconds (startTime));
	}
	m_TriggingCheckTrafficColorEvent=Simulator::Schedule(m_checktrafficcolorInterval,&Traffic_Light_Based_Routing::CheckNowTrafficColor,this);
	m_TriggingCheckWaitingListEvent=Simulator::Schedule(m_checkwaitinglistInterval,&Traffic_Light_Based_Routing::CheckWaitingList,this);
	Ipv4RoutingProtocol::DoInitialize ();
}

void
Traffic_Light_Based_Routing::RecvTLR (Ptr<Packet> p, Ipv4Address receiver, Ipv4Address src)
{
	NS_LOG_FUNCTION (this);
	TypeHeader tHeader (TLRTYPE_HELLO);
	p->RemoveHeader (tHeader);
	if (!tHeader.IsValid ())
	{
	  NS_LOG_DEBUG ("TLR message " << p->GetUid () << " with unknown type received: " << tHeader.Get () << ". Drop");
	  return; // drop
	}
	switch (tHeader.Get ())
	{
		case TLRTYPE_HELLO:
		{
			RecvHELLO (p, receiver, src);
			break;
		}
		case TLRTYPE_DBD:
		{
			RecvDD (p, receiver, src);
			break;
		}
		case TLRTYPE_LSR:
		{
			RecvLSR (p, receiver, src);
			break;
		}
		case TLRTYPE_LSU:
		{
			RecvLSU (p, receiver, src);
			break;
		}
		case TLRTYPE_LSAck:
		{
			RecvLSAck(p, receiver, src);
			break;
		}
		case TLRTYPE_NOTIFY:
		{

			RecvNOTIFY(p, receiver, src);
			break;
		}
	}

}
void 
Traffic_Light_Based_Routing::RecvNOTIFY(Ptr<Packet> p, Ipv4Address receiver, Ipv4Address src)
{
   NS_LOG_FUNCTION(this << " src " << src);
   NotifyHeader notifyHeader;
   p->RemoveHeader(notifyHeader);
   TlrHeader tlrHeader;
   p->RemoveHeader(tlrHeader);
   // std::cout<<"RECVNOTIFY "<<"curID "<<m_ipv4->GetObject<Node>()->GetId()<<"id "<<notifyHeader.GetId()<<" color "<<notifyHeader.GetColor()<<std::endl;
   // std::cout<<"RECVNOTIFY receiver"<<receiver<<" src "<<src<<" intcolor "<<int(notifyHeader.GetColor())<<std::endl;
   m_trafficlightcolor[notifyHeader.GetId()]=notifyHeader.GetColor();

}
void
Traffic_Light_Based_Routing::RecvHELLO (Ptr<Packet> p, Ipv4Address receiver, Ipv4Address src)
{
	NS_LOG_FUNCTION (this << " src " << src);
	HelloHeader helloHeader;
	p->RemoveHeader (helloHeader);
	TlrHeader tlrHeader;
	p->RemoveHeader (tlrHeader);

	//// std::cout<<helloHeader.GetHelloInterval()<<std::endl;

	// Next, the values of the Network Mask, HelloInterval, and RouterDeadInterval fields in the received Hello packet must
	// be checked against the values configured for the receiving
	// However, there is one exception to the above rule: on point-to-point networks and on virtual links,
	// the Network Mask in the received Hello Packet should be ignored.
	if(helloHeader.GetHelloInterval() == m_helloInterval && helloHeader.GetRouterDeadInterval() == m_rtrDeadInterval){
		// judge whether this is the first time the neighbor has been detected, create a new data structure or update expireTime
		m_nb.HelloReceived (Ipv4Address(m_ipv4->GetObject<Node>()->GetId()),tlrHeader.GetAreaID(), receiver, tlrHeader.GetRouterID(), src, 0,
				Ipv4Address::GetZero(), Ipv4Address::GetZero(), m_rtrDeadInterval, helloHeader.GetNeighbors());
	}

	//delete p;


}

void
Traffic_Light_Based_Routing::RecvDD (Ptr<Packet> p, Ipv4Address my,Ipv4Address src)
{


	NS_LOG_FUNCTION(this << " src " << src);
	Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
	DDHeader ddHeader;
	p->RemoveHeader(ddHeader);
	TlrHeader tlrHeader;
	p->RemoveHeader(tlrHeader);

	if(ddHeader.GetMTU() == l3->GetMtu((uint32_t)l3->GetInterfaceForAddress(my))){
		m_nb.DDReceived(Ipv4Address(m_ipv4->GetObject<Node>()->GetId()), src, ddHeader.GetFlags(), ddHeader.GetSeqNum(), ddHeader.GetLSAHeaders());
	}
	else{
		// the Database Description packet is rejected
	}


}
void
Traffic_Light_Based_Routing::RecvLSR (Ptr<Packet> p, Ipv4Address my,Ipv4Address src)
{
	NS_LOG_FUNCTION(this << " src " << src);
	Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
	LSRHeader lsrHeader;
	p->RemoveHeader(lsrHeader);
	TlrHeader tlrHeader;
	p->RemoveHeader(tlrHeader);

	m_nb.LSRReceived(Ipv4Address(m_ipv4->GetObject<Node>()->GetId()),src,lsrHeader);
/*	if(ddHeader.GetMTU() == l3->GetMtu((uint32_t)l3->GetInterfaceForAddress(my))){
		m_nb.DDReceived(Ipv4Address(m_ipv4->GetObject<Node>()->GetId()), src, ddHeader.GetFlags(), ddHeader.GetSeqNum(), ddHeader.GetLSAHeaders());
	}*/
}

void
Traffic_Light_Based_Routing::RecvLSU (Ptr<Packet> p, Ipv4Address my,Ipv4Address src)
{
	NS_LOG_FUNCTION(this << " src " << src);
	Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
	LSUHeader lsu;
	p->RemoveHeader(lsu);
	TlrHeader tlrHeader;
	p->RemoveHeader(tlrHeader);

	m_nb.LSUReceived(Ipv4Address(m_ipv4->GetObject<Node>()->GetId()),src,lsu);
/*	if(ddHeader.GetMTU() == l3->GetMtu((uint32_t)l3->GetInterfaceForAddress(my))){
		m_nb.DDReceived(Ipv4Address(m_ipv4->GetObject<Node>()->GetId()), src, ddHeader.GetFlags(), ddHeader.GetSeqNum(), ddHeader.GetLSAHeaders());
	}*/
}

void
Traffic_Light_Based_Routing::RecvLSAck (Ptr<Packet> p, Ipv4Address my,Ipv4Address src)
{
	NS_LOG_FUNCTION(this << " src " << src);
	Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
	LSAackHeader lsack;
	p->RemoveHeader(lsack);
	TlrHeader tlrHeader;
	p->RemoveHeader(tlrHeader);

	m_nb.LSAckReceived(Ipv4Address(m_ipv4->GetObject<Node>()->GetId()),src,lsack);
}




}
}


