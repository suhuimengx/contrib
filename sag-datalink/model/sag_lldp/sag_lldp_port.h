/*
 * sag_lldp_port.h
 *
 *  Created on: 2023年9月21日
 *      Author: kolimn
 */

#ifndef CONTRIB_PROTOCOLS_MODEL_SAG_LLDP_SAG_LLDP_PORT_H_
#define CONTRIB_PROTOCOLS_MODEL_SAG_LLDP_SAG_LLDP_PORT_H_

#include <stdint.h>
#include <time.h>
#include "ns3/ptr.h"
#include "ns3/object.h"
#include "ns3/net-device.h"



/**
 * 结构:lldp_port ->(lldp_tx_port,lldp_rx_port,lldp_masp)->(statistic)
 */
namespace ns3 {

struct lldp_tx_port_statistics {
    uint64_t statsFramesOutTotal; //tx_port发出frame的统计数量
};

//这个用于存储source_mac des_mac和ethertype
struct eth_hdr {
    uint8_t dst[6];
    uint8_t src[6];
    uint16_t ethertype;
};


//tx_port的计时器
struct lldp_tx_port_timers {
    uint16_t reinitDelay;   /**< IEEE 802.1AB 10.5.3 */
    uint16_t msgTxHold;     /**< IEEE 802.1AB 10.5.3 */
    uint16_t msgTxInterval; /**< IEEE 802.1AB 10.5.3 */
    uint16_t txDelay;       /**< IEEE 802.1AB 10.5.3 */

    uint16_t txTTR;         /**< IEEE 802.1AB 10.5.3 - transmit on expire. */

    /* Not sure what this is for */
    uint16_t txShutdownWhile;
    uint16_t txDelayWhile;

    //Mengy's::
    uint16_t update_time;
};


//tx_port
struct lldp_tx_port {
    uint8_t *frame;    /**< The tx frame buffer */
    uint64_t sendsize; /**< The size of our tx frame */
    uint8_t state;     /**< The tx state for this interface */
    uint8_t somethingChangedLocal; /**< IEEE 802.1AB var (from where?) */
    uint16_t txTTL;/**< IEEE 802.1AB var (from where?) */
    struct lldp_tx_port_timers timers; /**< The lldp tx state machine timers for this interface */
    struct lldp_tx_port_statistics statistics; /**< The lldp tx statistics for this interface */
};


//tx_port的统计数据
struct lldp_rx_port_statistics {
    uint64_t statsAgeoutsTotal;
    uint64_t statsFramesDiscardedTotal;
    uint64_t statsFramesInErrorsTotal;
    uint64_t statsFramesInTotal;
    uint64_t statsTLVsDiscardedTotal;
    uint64_t statsTLVsUnrecognizedTotal;
};

//rx_port的计时器
struct lldp_rx_port_timers {
    uint16_t tooManyNeighborsTimer;
    uint16_t rxTTL;
    uint16_t update_time;
};

//rx_port
struct lldp_rx_port {
    uint8_t *frame;
    ssize_t recvsize;
    uint8_t state;
    uint8_t badFrame;
    uint8_t rcvFrame;
    //uint8_t rxChanges; /* This belongs in the MSAP cache */
    uint8_t rxInfoAge;//当有msap过期时，这个设置为true
    uint8_t somethingChangedRemote;
    uint8_t tooManyNeighbors;
    struct lldp_rx_port_timers timers;
    struct lldp_rx_port_statistics statistics;
  //    struct lldp_msap_cache *msap;
};

//msap结构，用于存储邻居的设备信息
struct lldp_msap {
  struct lldp_msap *next;
  uint8_t *id;
  uint8_t length;
  struct lldp_tlv_list *tlv_list;

  struct lldp_tlv *ttl_tlv;

  /* IEEE 802.1AB MSAP-specific counters */
  uint16_t rxInfoTTL;
  uint16_t update_time;
};

enum portAdminStatus {
    disabled,
    enabledTxOnly,
    enabledRxOnly,
    enabledRxTx,
};

struct lldp_port {
  struct lldp_port *next;
  char *if_name;     // The interface name.
  uint32_t if_index; // The interface index.
  uint32_t mtu;      // The interface MTU.
  uint8_t source_mac[6];
  uint8_t source_ipaddr[4];
  struct lldp_rx_port rx;
  struct lldp_tx_port tx;
  uint8_t portEnabled;
  uint8_t adminStatus;

  /* I'm not sure where this goes... the state machine indicates it's per-port */
  uint8_t rxChanges;

  // I'm really unsure about the best way to handle this...
  uint8_t tick;
  time_t last_tick;

  struct lldp_msap *msap_cache;


  // 802.1AB Appendix G flag variables.
  uint8_t  auto_neg_status;
  uint16_t auto_neg_advertized_capabilities;
  uint16_t operational_mau_type;

  //Mengy's::modify
  Ptr<NetDevice> netdevice;
};


}
#endif /* CONTRIB_PROTOCOLS_MODEL_SAG_LLDP_SAG_LLDP_PORT_H_ */
