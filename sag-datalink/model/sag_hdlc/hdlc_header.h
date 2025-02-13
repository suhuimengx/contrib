/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Author: Yuru Liu    November 2023
 *
 */

#ifndef HDLC_HEADER_H_
#define HDLC_HEADER_H_
#include "ns3/header.h"

namespace ns3 {
enum SS_t {RR=0, REJ=1, RNR=2, SREJ=3};
enum COMMAND_t {SNRM,DISC, UA, DM};
enum HDLCFrameType{ HDLC_S_frame, HDLC_I_frame, HDLC_U_frame };

struct HDLCControlFrame {
	int recv_seq;
	int send_seq;
	char frame_command;
	bool poll_final_bit;
	SS_t stype;
	COMMAND_t utype;
};

class HdlcHeader : public Header
{
public:

  HdlcHeader();
  virtual ~HdlcHeader();

  void SetHDLCControlFrame(const HDLCControlFrame& controlFrame);
  HDLCControlFrame GetHDLCControlFrame() const;

  void SetHDLCFrameUType(COMMAND_t utype_);
  COMMAND_t GetHDLCFrameUType() const;

  void SetHDLCFrameSType(SS_t utype_);
  SS_t GetHDLCFrameSType() const;

  void SetHDLCFrameType(HDLCFrameType frameType);
  HDLCFrameType GetHDLCFrameType() const;

  int GetHDLCReceseq() const;
  void SetHDLCReceseq(int recv);
  int GetHDLCSendseq() const;
  void SetHDLCSendseq(int send);

  static TypeId GetTypeId();
  virtual TypeId GetInstanceTypeId() const;
  virtual uint32_t GetSerializedSize() const;
  /**
  * Serialize address field
  * Serialize control field based on HDLC frame type
  * For HDLC U-frame, set control field based on utype
  * For HDLC I-frame, set control field with send_seq and recv_seq
  * For HDLC S-frame, set control field with recv_seq
  * Serialize protocol field
     */
  virtual void Serialize(Buffer::Iterator start) const;
  virtual uint32_t Deserialize(Buffer::Iterator start);

  void SetProtocol (uint16_t protocol);
  uint16_t GetProtocol (void);
  void Print(std::ostream &os) const {
      os << "HDLC Header Information";
  };

private:
  int m_saddr;
  int m_daddr;
  HDLCControlFrame m_controlFrame;
  HDLCFrameType m_frameType;
  uint16_t m_protocol;
  uint8_t recv_seq;
  uint8_t send_seq;
  COMMAND_t utype;
  SS_t stype;
};
}
#endif
