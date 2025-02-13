/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Author: Yuru Liu    November 2023
 *
 */
#include<ns3/hdlc_header.h>
#include "ns3/address-utils.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (HdlcHeader);

TypeId
HdlcHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::HdlcHeader")
    .SetParent<Header> ()
    .SetGroupName ("HDLC")
    .AddConstructor<HdlcHeader> ()
  ;
  return tid;
}

TypeId
HdlcHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

HdlcHeader::HdlcHeader()
{
	recv_seq=0;
	send_seq=0;
	m_protocol=0x0800;
}

HdlcHeader::~HdlcHeader()
{
  // 析构函数
}


void HdlcHeader::SetHDLCControlFrame(const HDLCControlFrame& controlFrame)
{
  m_controlFrame = controlFrame;
}

HDLCControlFrame HdlcHeader::GetHDLCControlFrame() const
{
  return m_controlFrame;
}

void HdlcHeader::SetHDLCFrameUType(COMMAND_t utype_)
{
	utype =utype_;
}

COMMAND_t HdlcHeader::GetHDLCFrameUType() const
{
  return utype;
}

void HdlcHeader::SetHDLCFrameSType(SS_t stype_)
{
	stype =stype_;
}

SS_t HdlcHeader::GetHDLCFrameSType() const
{
  return stype;
}

void HdlcHeader::SetHDLCFrameType(HDLCFrameType frameType)
{
	m_frameType =frameType;
}

HDLCFrameType HdlcHeader::GetHDLCFrameType() const
{
  return m_frameType;
}

int HdlcHeader::GetHDLCReceseq() const
{
	return recv_seq;
}

void HdlcHeader::SetHDLCReceseq(int recv)
{
	recv_seq=recv;
}

int HdlcHeader::GetHDLCSendseq() const
{
	return send_seq;
}

void HdlcHeader::SetHDLCSendseq(int send)
{
	send_seq=send;
}

uint32_t
HdlcHeader::GetSerializedSize (void) const
{
  return 4;
}

void
HdlcHeader::Serialize (Buffer::Iterator start) const
{
	//address field unicast
	start.WriteU8(0x0F);
	//control field
	//uframe  11 mn(S) p/f mn(r)
	if(m_frameType==HDLC_U_frame){
		switch (utype) {
		case 0:
			start.WriteU8(0b11001001);
			break;
		case 1:
			start.WriteU8(0b11001010);
			break;
		case 2:
			start.WriteU8(0b11001110);
			break;
		case 3:
			start.WriteU8(0b11110000);
			break;
	}}
	if(m_frameType==HDLC_I_frame){
	    uint8_t bits_1_to_3 = send_seq;
	    uint8_t bits_4 = 0b1;
	    uint8_t bits_5_to_7 = recv_seq;
	    uint8_t bits_8 = 0b0;
	    uint8_t data = 0;
	    data |= bits_1_to_3;
	    data |= ( bits_4 << 3);
	    data |= (bits_5_to_7 << 4);
	    data |= (bits_8 << 7);
	    start.WriteU8(data);
	}
	if(m_frameType==HDLC_S_frame){
		if(stype==RR){
		//RR 10001recv_seq
	    uint8_t bits_1_to_3 = recv_seq;
	    uint8_t bits_4 = 0b1;
	    uint8_t bits_5_to_7 = 0;
	    uint8_t bits_8 = 0b1;
	    uint8_t data = 0;
	    data |= bits_1_to_3;
	    data |= ( bits_4 << 3);
	    data |= (bits_5_to_7 << 4);
	    data |= (bits_8 << 7);
	    start.WriteU8(data);}
		if(stype==RNR){
		//RNR 10101recv_seq
	    uint8_t bits_1_to_3 = recv_seq;
	    uint8_t bits_4 = 0b1;
	    uint8_t bits_5_to_7 = 0b010;
	    uint8_t bits_8 = 0b1;
	    uint8_t data = 0;
	    data |= bits_1_to_3;
	    data |= ( bits_4 << 3);
	    data |= (bits_5_to_7 << 4);
	    data |= (bits_8 << 7);
	    start.WriteU8(data);}
	}
	start.WriteHtonU16(m_protocol);
}

uint32_t
HdlcHeader::Deserialize (Buffer::Iterator start)
{
	uint8_t addressValue=start.ReadU8();
	if(addressValue){}
	uint8_t receivedValue=start.ReadU8();
	switch(receivedValue){
	case 0b11001001:
		utype=SNRM;
		m_frameType=HDLC_U_frame;
		break;
	case 0b11110000:
		utype=DM;
		m_frameType=HDLC_U_frame;
		break;
	case 0b11001010:
		utype=DISC;
		m_frameType=HDLC_U_frame;
		break;
	case 0b11001110:
		utype=UA;
		m_frameType=HDLC_U_frame;
		break;
		}

	uint8_t bit_1_3 = receivedValue & 0b00000111;
	uint8_t bit_4 = (receivedValue >> 3) & 0b00000001;
	uint8_t bit_5_7 = (receivedValue >> 4) & 0b00000111;
	uint8_t bit_8 = (receivedValue >> 7) & 0b00000001;
	if(bit_8==0&&bit_4==1){
		m_frameType=HDLC_I_frame;
		send_seq=bit_1_3;
		recv_seq=bit_5_7;
	}
	if(bit_8==1&&bit_5_7==0&&bit_4==1){
		m_frameType=HDLC_S_frame;
		stype=RR;
		recv_seq=bit_1_3;
	}
	if(bit_8==1&&bit_5_7==0b010 &&bit_4==1){
		m_frameType=HDLC_S_frame;
		stype=RNR;
		recv_seq=bit_1_3;
	}
	uint16_t protocolvalue=start.ReadNtohU16();
	if(protocolvalue){}
	 return GetSerializedSize ();
}

void
HdlcHeader::SetProtocol (uint16_t protocol)
{
  m_protocol=protocol;
}

uint16_t
HdlcHeader::GetProtocol (void)
{
  return m_protocol;
}


}

