/*
 * sag_lldp.h
 *
 *  Created on: 2023年9月21日
 *      Author: kolimn
 */

#ifndef CONTRIB_PROTOCOLS_MODEL_SAG_LLDP_SAG_LLDP_H_
#define CONTRIB_PROTOCOLS_MODEL_SAG_LLDP_SAG_LLDP_H_

#include <stdint.h>
#include "ns3/object.h"
#include "ns3/socket.h"
#include "ns3/callback.h"
#include "ns3/node.h"
#include "ns3/node-container.h"
#include "ns3/net-device.h"
#include "ns3/address.h"
#include "ns3/event-id.h"

#include "sag_lldp_port.h"

#define IF_NAME_SIZE 20

namespace ns3 {

class Node;
class NetDevice;
class Packet;


class SAGLLDP : public Object
{
public:
  /**
   * \brief Get the TypeId
   *
   * \return The TypeId for this class
   */
  static TypeId GetTypeId (void);

  static const uint16_t PROT_NUMBER;

  /**
   * Construct a SAGLLDP
   *
   * This is the constructor for the SAGLinkLayerGSL
   */
  SAGLLDP();

  SAGLLDP(Ptr<Node> node, int if_no);

  /**
   * Destroy a SAGLinkLayerGSL
   *
   * This is the destructor for the SAGLinkLayerGSL.
   */
  virtual ~SAGLLDP ();

  /**
   * Send LLDP packet
   *
   * \param packet, the packet to send
   * \param dest, the destination MAC address
   * \param protocolNumber, the LLDP protocol number 0x88cc
   * \param lldp_port, the port to send this packet
   */
  void Send(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber, struct lldp_port* lldp_port);

  /**
   * Receive LLDP packet
   *
   * \param device, the netdevice which received the packet
   * \param p, the packet
   * \param protocol, protocol number
   * \param from, the source MAC address
   * \param to, the destination MAC address
   * \param packetType, packet type
   */
  void Receive ( Ptr<NetDevice> device, Ptr<const Packet> p, uint16_t protocol, const Address &from,
          const Address &to, NetDevice::PacketType packetType);

  void RunTx(struct lldp_port* lldp_port);

protected:

  /**
   * Set the lldp instance to node
   *
   * \param node, the node to set
   */
  void SetNode(Ptr<Node> node);

  /**
   * Register the protocol number and receive handler to a node
   */
  void SetLoopBack();

  void NotifyNewAggregate ();

  /**
   * Do send the  packet from dest by broadcast
   *
   */
  void DoSend(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber, uint32_t device_num);

  /**
   * Initialize the lldp instance and send a LLDP packet by Broadcast
   *
   */
  void InitializeLLDP();




private:

  //Node m_node;
  uint32_t m_if_no;
  struct lldp_port* m_last_lldp_port;
  Ptr<Node> m_node;



};










}
#endif /* CONTRIB_PROTOCOLS_MODEL_SAG_LLDP_SAG_LLDP_H_ */
