/*
 * sag_rx_sm.h
 *
 *  Created on: 2023年9月21日
 *      Author: kolimn
 */

#ifndef CONTRIB_PROTOCOLS_MODEL_SAG_LLDP_SAG_RX_SM_H_
#define CONTRIB_PROTOCOLS_MODEL_SAG_LLDP_SAG_RX_SM_H_

#include <stdint.h>
#include "sag_lldp_port.h"
#include "sag_packet.h"

namespace ns3 {

#define LLDP_WAIT_PORT_OPERATIONAL 4
#define DELETE_AGED_INFO           5
#define RX_LLDP_INITIALIZE         6
#define RX_WAIT_FOR_FRAME          7
#define RX_FRAME                   8
#define DELETE_INFO                9
#define UPDATE_INFO                10


/**
 * Delete MIB Object
 *
 * \param lldp, the lldp port to delete MIB
 * \retrun 0
 */
uint8_t mibDeleteObjects(struct lldp_port *lldp);

/**
 * Update MIB objects
 *
 * \param lldp, lldp port
 */
uint8_t mibUpdateObjects(struct lldp_port *lldp);

/**
 * Initialize the receive lldp port
 * \param lldp, lldp port
 *
 */
uint8_t rxInitializeLLDP(struct lldp_port *lldp);

/**
 * Process a LLDP Header packet
 *
 * \param lldp_port, port
 * \param p, the pacekt
 *
 * \return 1
 */
int ProcessLLDPHeader(struct lldp_port* lldp_port,Ptr<Packet> p);

/**
 * Process a LLDP frame
 * \param lldp_port, lldp port
 * \param header, LLDP Header packet
 * \param pdu, LLDP PDU packet
 *
 * \return 1
 *
 */
int rxProcessFrame(struct lldp_port *lldp_port, LLDPHeader header,LLDPPDU pdu);


/**
 * Change receive machine state
 *
 * \param lldp_port, lldp port
 * \param state, receive state
 */
void rxChangeToState(struct lldp_port *lldp_port, uint8_t state);
void rxBadFrameInfo(uint8_t frameErrors);
std::string rxStateFromID(uint8_t state);

/**
 * Run receive machine state
 *
 * \param lldp_port, lldp port
 * \param packet, packet
 *
 */
void rxStatemachineRun(struct lldp_port *lldp_port,Ptr<ns3::Packet> packet);


void rx_decrement_timer(uint16_t *timer);

// The following function are the do function when in a new recive state
void rx_do_lldp_wait_port_operational(struct lldp_port *lldp_port,Ptr<ns3::Packet> packet);
void rx_do_delete_aged_info(struct lldp_port *lldp_port,Ptr<ns3::Packet> packet);
void rx_do_rx_lldp_initialize(struct lldp_port *lldp_port,Ptr<ns3::Packet> packet);
void rx_do_rx_wait_for_frame(struct lldp_port *lldp_port,Ptr<ns3::Packet> packet);
void rx_do_rx_frame(struct lldp_port *port,Ptr<ns3::Packet> packet);
void rx_do_rx_delete_info(struct lldp_port *lldp_port,Ptr<ns3::Packet> packet);
void rx_do_rx_update_info(struct lldp_port *lldp_port,Ptr<ns3::Packet> packet);
void rx_do_update_timers(struct lldp_port *lldp_port);



void rx_display_timers(struct lldp_port *lldp_port);

}
#endif /* CONTRIB_PROTOCOLS_MODEL_SAG_LLDP_SAG_RX_SM_H_ */
