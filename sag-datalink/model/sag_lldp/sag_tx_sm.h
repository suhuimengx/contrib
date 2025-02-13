/*
 * sag_tx_sm.h
 *
 *  Created on: 2023年9月21日
 *      Author: kolimn
 */

#ifndef CONTRIB_PROTOCOLS_MODEL_SAG_LLDP_SAG_TX_SM_H_
#define CONTRIB_PROTOCOLS_MODEL_SAG_LLDP_SAG_TX_SM_H_
#include <stdint.h>
#include "sag_lldp_port.h"
#include "sag_packet.h"


namespace ns3 {
class SAGLLDP;
// 发送状态
#define TX_LLDP_INITIALIZE 0 /**< Initialize state from IEEE 802.1AB 10.5.4.3 */
#define TX_IDLE            1 /**< Idle state from IEEE 802.1AB 10.5.4.3 */
#define TX_SHUTDOWN_FRAME  2 /**< Shutdown state from IEEE 802.1AB 10.5.4.3 */
#define TX_INFO_FRAME      3 /**< Transmit state from IEEE 802.1AB 10.5.4.3 */


/**
 * Initialize the send port
 * \param lldp_port, lldp port
 *
 */
uint8_t txInitializeLLDP(struct lldp_port *lldp_port);

/**
 * Construct a LLDP PDU from lldp port
 * \param lldp_port, lldp port
 * \param p, the packet
 *
 */
void mibConstrInfoLLDPDU(struct lldp_port *lldp_port, Ptr<Packet> p);

/**
 * Construct a shutdown LLDP PDU
 * \param lldp_port, lldp port
 * \param p, the pacekt
 *
 */
void mibConstrShutdownLLDPDU(struct lldp_port *lldp_port, Ptr<Packet> p);



/**
 * Change send state to a specific state
 * \param lldp_port, lldp port
 * \param state, the new state
 *
 */
void txChangeToState(struct lldp_port *lldp_port, uint8_t state);
std::string txStateFromID(uint8_t state);

/**
 * Run send state machine
 * \param lldp port
 *
 */
void txGlobalStatemachineRun(struct lldp_port *lldp_port);
void txStatemachineRun(struct lldp_port *lldp_port, SAGLLDP* lldp);


void tx_do_tx_lldp_initialize(struct lldp_port *lldp_port,SAGLLDP* lldp);
void tx_do_update_timers(struct lldp_port *lldp_port);
void tx_do_tx_idle(struct lldp_port *lldp_port,SAGLLDP* lldp);
void tx_do_tx_shutdown(struct lldp_port *lldp_port,SAGLLDP* lldp);
void tx_do_tx_info_frame(struct lldp_port *lldp_port, SAGLLDP* lldp);
void tx_display_timers(struct lldp_port *lldp_port);
void tx_do_tx_shutdown_frame(struct lldp_port *lldp_port,SAGLLDP* lldp);

void tx_decrement_timer(uint16_t *timer);




#endif /* CONTRIB_PROTOCOLS_MODEL_SAG_LLDP_SAG_TX_SM_H_ */
}
