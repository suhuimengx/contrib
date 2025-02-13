/*
 * sag_lldp.cc
 *
 *  Created on: 2023年9月21日
 *      Author: kolimn
 */
#include "sag_lldp.h"
#include "sag_lldp_port.h"
#include "sag_rx_sm.h"
#include "sag_tx_sm.h"
#include "sag_packet.h"
#include "sag_rx_sm.h"
#include "sag_tlv_content.h"

#include "ns3/assert.h"
#include "ns3/node.h"
#include "ns3/boolean.h"
#include "ns3/log.h"
#include "ns3/callback.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv4-interface.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("SAGLLDP");

NS_OBJECT_ENSURE_REGISTERED (SAGLLDP);

const uint16_t SAGLLDP::PROT_NUMBER = 0x88cc;
struct lldp_port;

TypeId
SAGLLDP::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SAGLLDP")
	.SetParent<Object> ()
	.SetGroupName ("GSL")
	.AddConstructor<SAGLLDP> ()
/*    .AddAttribute ("IpForward", "Globally enable or disable IP forwarding for all current and future Ipv4 devices.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&Ipv4::SetIpForward,
                                        &Ipv4::GetIpForward),
                   MakeBooleanChecker ())*/
  ;
  return tid;
}

SAGLLDP::SAGLLDP ()
{
	m_node = 0;
	m_if_no = 4;
    NS_LOG_FUNCTION (this);
    InitializeLLDP();
}

SAGLLDP::SAGLLDP (Ptr<Node> node, int if_no)
{
    NS_LOG_FUNCTION (this);
    m_node = node;
    m_if_no = if_no;
    InitializeLLDP();
}

SAGLLDP::~SAGLLDP ()
{

  NS_LOG_FUNCTION (this);
}

void
SAGLLDP::DoSend(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber, uint32_t device_index)
{
	  if (m_node == 0)
		{
		  Ptr<Node>node = this->GetObject<Node>();
		  if (node != 0)
			{
			  this->SetNode (node);
			}
		}
	  Ptr<Ipv4L3Protocol> ipv4 = m_node->GetObject<Ipv4L3Protocol>();

	  uint32_t device_no = ipv4->GetNInterfaces();
	  if(device_index == 999)
	  {
		  for(uint32_t i = 0; i<device_no; i++)
		  {
			  Ptr< NetDevice > device = ipv4->GetInterface(i)->GetDevice();
			  device->Send(packet, dest, protocolNumber);
		  }
	  }
	  else
	  {
		  Ptr< NetDevice > device = ipv4->GetInterface(device_index)->GetDevice();
		  device->Send(packet, dest, protocolNumber);
/*		  std::cout<<"The Sender Address is "<<device->GetAddress()<<std::endl;
		  std::cout<<"The node id is "<<m_node->GetId()<<std::endl;*/
	  }
}

void
SAGLLDP::Send(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber, struct lldp_port* lldp_port)
{

	mibConstrInfoLLDPDU(lldp_port,packet);

	uint32_t device_index = lldp_port->if_index;
	struct lldp_port* lldp_port_temp = m_last_lldp_port;
	while(m_last_lldp_port -> next != NULL)
	{
		if (m_last_lldp_port -> if_index == device_index)
		{
			break;
		}
		m_last_lldp_port = m_last_lldp_port -> next;
	}

	std::cout<<"Node_"<<lldp_port->netdevice->GetNode()->GetId()<<"_Dev_"<<lldp_port->netdevice->GetIfIndex()<<":Send a Packet!"<<std::endl;
	SAGLLDP::DoSend(packet, dest, protocolNumber, device_index);

	m_last_lldp_port = lldp_port_temp;

}

void
SAGLLDP::RunTx(struct lldp_port* lldp_port)
{
	txStatemachineRun(lldp_port,this);
	Simulator::Schedule(Seconds(1.0),
						&SAGLLDP::RunTx,this, lldp_port);
}

void
SAGLLDP::InitializeLLDP()
{
	//为该节点的每一个interface都维护一个lldp_port结构
	//这里i = 1开始，避免了回环端口也初始化一个lldp数据结构
/*	for(uint32_t i = 1; i < m_if_no; i++)
	{*/

	//Only for Test
	uint32_t i = m_if_no;

		//1. 声明一个临时的lldp_port结构
		//Mengy's::modify
		struct lldp_port* lldp_port = new struct lldp_port;

		//2. 初始化名字、索引等
		lldp_port -> if_index = i;
		//char name[] = "node";
		char name[IF_NAME_SIZE];
		std::sprintf(name,"%s_%d","node",m_node->GetId());
		char real_name[IF_NAME_SIZE + 10];
		std::sprintf(real_name, "%s_%d", name, i);

		lldp_port -> if_name = real_name;
		lldp_port -> portEnabled = 1;
		lldp_port -> mtu = 1500;

		//Mengy's::modify
		Ptr< NetDevice > device = m_node->GetObject<Ipv4>()->GetNetDevice(i);
		lldp_port->netdevice = device;

		int32_t interface_index = m_node->GetObject<Ipv4>()->GetInterfaceForDevice(device);
		m_node->GetObject<Ipv4>()->GetAddress(interface_index,0).GetLocal().Serialize(lldp_port->source_ipaddr);

		//3. 初始化端口的发送、接收状态及
		lldp_port -> tx.state = TX_LLDP_INITIALIZE;
		lldp_port->rx.state = LLDP_WAIT_PORT_OPERATIONAL;
		lldp_port->msap_cache = NULL;
		txInitializeLLDP(lldp_port);
		rxInitializeLLDP(lldp_port);
		//Mengy's::记得把下面的注释取消了
		//lldp_port -> portEnabled = 0;
		lldp_port->adminStatus  = enabledRxTx;


		//initializeTLVFunctionValidators();

		//5. 更新链表
		if(i == 0)
		{
			lldp_port -> next = NULL;
		}
		else
		{
			lldp_port -> next = m_last_lldp_port;
		}
		m_last_lldp_port = lldp_port;

		//6. 发送数据包
		Ptr<Packet> packet = Create<Packet>();
		SAGLLDP::Send(packet, device->GetBroadcast(), PROT_NUMBER, lldp_port);

		//7. 驱动发送状态机以及时出发定时器。在这里面设置每一秒进行一次定时器更新（及tick的周期为1s）
		RunTx(lldp_port);

		initializeTLVFunctionValidators();
	//}
}



void
SAGLLDP::Receive ( Ptr<NetDevice> device, Ptr<const Packet> p, uint16_t protocol, const Address &from,
        const Address &to, NetDevice::PacketType packetType)
{
	NS_LOG_FUNCTION (this << p);


	struct lldp_port* lldp_port =  m_last_lldp_port;

	bool stop = false;
	while(!stop)
	{
		if(lldp_port->netdevice == device)
		{

			break;
		}

		if(lldp_port->next != NULL)
		{
			lldp_port = lldp_port->next;
		}
		else
		{
			stop = true;
			std::cout<<"no lldp_port match with the device!"<<std::endl;
		}

	}

	NS_LOG_FUNCTION (this);

	lldp_port->rx.rcvFrame = 1;

	std::cout<<"Node_"<<lldp_port->netdevice->GetNode()->GetId()<<"_Dev_"<<lldp_port->netdevice->GetIfIndex()<<":Receive a Packet!"<<std::endl;;

	ns3::Ptr<ns3::Packet> packet = ns3::ConstCast<ns3::Packet>(p);
	rxStatemachineRun(lldp_port,packet);


}

void
SAGLLDP::SetNode(Ptr<Node> node)
{
	NS_LOG_FUNCTION (this);
	m_node = node;
}

void
SAGLLDP::SetLoopBack()
{
	NS_LOG_FUNCTION (this);
	Ptr<Node> node = m_node;
	node->RegisterProtocolHandler (MakeCallback (&SAGLLDP::Receive, this),
	                                 SAGLLDP::PROT_NUMBER, 0);

}


void
SAGLLDP::NotifyNewAggregate ()
{
  NS_LOG_FUNCTION (this);
  if (m_node == 0)
	{
	  Ptr<Node>node = this->GetObject<Node>();
	  if (node != 0)
		{
		  this->SetNode (node);
		}
	}
  SetLoopBack();
}


}

