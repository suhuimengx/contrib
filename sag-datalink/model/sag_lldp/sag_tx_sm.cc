/*
 * sag_tx_sm.cc
 *
 *  Created on: 2023年9月21日
 *      Author: kolimn
 */


#include <stdint.h>
#include <strings.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include "ns3/sag_link_layer_gsl.h"
#include "sag_tx_sm.h"
#include "sag_lldp_port.h"
#include "sag_tlv_content.h"
#include "sag_tlv_struct.h"
#include "sag_packet.h"
#include "sag_lldp.h"
#include "ns3/simulator.h"

namespace ns3 {
void mibConstrInfoLLDPDU(struct lldp_port *lldp_port, Ptr<Packet> p)
{
    struct lldp_tlv_list *tlv_list = NULL;
	struct lldp_tlv_list *tmp      = 0;

	//********************1. 初始化源目节点的mac地址及以太类型，存入tx.frame
    // 这个函数用于初始化lldp_port的ip和mac
    //refreshInterfaceData(lldp_port);

    //这里destination的mac地址是固定的，用于在同一个LAN中进行传输。
	//每次lldp_frame都是用广播的方式进行传输，所以不需要特定于目的节点的mac

    uint8_t dst[6];
    dst[0] = 0x01;
    dst[1] = 0x80;
    dst[2] = 0xc2;
    dst[3] = 0x00;
    dst[4] = 0x00;
    dst[5] = 0x0e;

    uint16_t ethertype = 0x88cc;

    //std::cout<<"Add LLDP Header To Packet!"<<std::endl;



    //std::cout<<"Start to Add TLV in this Frame！"<<std::endl;
    //********************2. 加入强制性的tlv
    add_tlv(create_chassis_id_tlv(lldp_port), &tlv_list);
    add_tlv(create_port_id_tlv(lldp_port), &tlv_list);
    add_tlv(create_ttl_tlv(lldp_port), &tlv_list);

    //********************3. 加入可选tlv
    add_tlv(create_port_description_tlv(lldp_port), &tlv_list);
    add_tlv(create_system_name_tlv(lldp_port), &tlv_list);
    add_tlv(create_system_description_tlv(lldp_port), &tlv_list);
    add_tlv(create_system_capabilities_tlv(lldp_port), &tlv_list);
    add_tlv(create_management_address_tlv(lldp_port), &tlv_list);

    //********************4. 加入last_tlv
    // This TLV *MUST* be last.
    add_tlv(create_end_of_lldpdu_tlv(lldp_port), &tlv_list);
    //std::cout<<"End in Adding TLV"<<std::endl;


    //********************5. 将tlv进行轻量化，然后写入frame
    tmp = tlv_list;
    LLDPPDU LLDPPDU(tmp);
    p->AddHeader(LLDPPDU);

    LLDPHeader LLDPHeader(dst,lldp_port->source_mac,ethertype);
    p->AddHeader(LLDPHeader);

    //********************6. 进行padding

    if(LLDPHeader.GetSerializedSize()+LLDPPDU.GetSerializedSize() < 64) {
        lldp_port->tx.sendsize = 64;
    } else {
        lldp_port->tx.sendsize = LLDPHeader.GetSerializedSize()+LLDPPDU.GetSerializedSize();
    }
    destroy_tlv_list(&tlv_list);
}

uint8_t txInitializeLLDP(struct lldp_port *lldp_port)
{
    /* As per IEEE 802.1AB section 10.1.1 */
    lldp_port->tx.somethingChangedLocal = 0;

    /* Defined in 10.5.2.1 */
    lldp_port->tx.statistics.statsFramesOutTotal = 0;

    lldp_port->tx.timers.reinitDelay   = 2;  // Recommended minimum by 802.1AB 10.5.3.3
    lldp_port->tx.timers.msgTxHold     = 4;  // Recommended minimum by 802.1AB 10.5.3.3
    //Mengy's:: TODO 下面这个参数修改以下
    lldp_port->tx.timers.msgTxInterval = 4; // Recommended minimum by 802.1AB 10.5.3.3 原来为30
    lldp_port->tx.timers.txDelay       = 2;  // Recommended minimum by 802.1AB 10.5.3.3

    // Unsure what to set these to...
    lldp_port->tx.timers.txShutdownWhile = 0;

    lldp_port->tx.frame = new uint8_t[lldp_port->mtu];

    //Mengy's::TODO 这里需要获取设备的mac地址，但是我不知道怎么将netdevice转换为PointToPointNetDevice然后获取MACaddress
    Address address = lldp_port->netdevice->GetAddress();
    address.CopyFrom(lldp_port->source_mac,6);
    std::cout<<"the address is "<<(int)lldp_port->source_mac[0]<<(int)lldp_port->source_mac[1]<<lldp_port->source_mac[2]<<std::endl;



    lldp_port->source_ipaddr[0] = 1;
    lldp_port->source_ipaddr[1] = 1;
    lldp_port->source_ipaddr[2] = 1;
    lldp_port->source_ipaddr[3] = 1;

    lldp_port->tx.timers.update_time = (uint16_t)Simulator::Now().GetSeconds();
    return 0;
}

uint16_t min(uint16_t value1, uint16_t value2)
{
    if(value1 < value2)
    {
        return value1;
    }

    return value2;
}

void txChangeToState(struct lldp_port *lldp_port, uint8_t state) {

    if(lldp_port->tx.state != state)
    {
    	//std::cout<<"Node_"<<lldp_port->netdevice->GetNode()->GetId()<<"_Dev_"<<lldp_port->netdevice->GetIfIndex()<<":TX from "<<txStateFromID(lldp_port->tx.state)<<" to "<<txStateFromID(state)<<std::endl;
    }
    lldp_port->tx.state = state;
}

void mibConstrShutdownLLDPDU(struct lldp_port *lldp_port, Ptr<Packet> p)
{
    struct lldp_tlv_list *tlv_list = NULL;
	struct lldp_tlv_list *tmp      = 0;

    uint8_t dst[6];
    dst[0] = 0x01;
    dst[1] = 0x80;
    dst[2] = 0xc2;
    dst[3] = 0x00;
    dst[4] = 0x00;
    dst[5] = 0x0e;

    uint16_t ethertype = 0x88cc;

    LLDPHeader LLDPHeader(dst,lldp_port->source_mac,ethertype);
    p->AddHeader(LLDPHeader);


    //********************2. 加入强制性的tlv
    add_tlv(create_chassis_id_tlv(lldp_port), &tlv_list);
    add_tlv(create_port_id_tlv(lldp_port), &tlv_list);
    add_tlv(create_ttl_tlv(lldp_port), &tlv_list);

    //********************3. 加入last_tlv
    // This TLV *MUST* be last.
    add_tlv(create_end_of_lldpdu_tlv(lldp_port), &tlv_list);
    //std::cout<<"End in Adding TLV"<<std::endl;


    //********************5. 将tlv进行轻量化，然后写入frame
    tmp = tlv_list;
    LLDPPDU LLDPPDU(tmp);
    p->AddHeader(LLDPPDU);


    //********************6. 进行padding

    if(LLDPHeader.GetSerializedSize()+LLDPPDU.GetSerializedSize() < 64) {
        lldp_port->tx.sendsize = 64;
    } else {
        lldp_port->tx.sendsize = LLDPHeader.GetSerializedSize()+LLDPPDU.GetSerializedSize();
    }
    destroy_tlv_list(&tlv_list);
}

void txGlobalStatemachineRun(struct lldp_port *lldp_port) {
    /* Sit in TX_LLDP_INITIALIZE until the next initialization */
    if(lldp_port->portEnabled == false) {
        lldp_port->portEnabled = true;

        txChangeToState(lldp_port, TX_LLDP_INITIALIZE);
    }

    switch(lldp_port->tx.state) {
        case TX_LLDP_INITIALIZE: {
                                     if((lldp_port->adminStatus == enabledRxTx) || (lldp_port->adminStatus == enabledTxOnly)) {
                                         txChangeToState(lldp_port, TX_IDLE);
                                     }
                                 }break;
        case TX_IDLE: {
                          // It's time to send a shutdown frame...
                          if((lldp_port->adminStatus == disabled) || (lldp_port->adminStatus == enabledRxOnly)) {
                              txChangeToState(lldp_port, TX_SHUTDOWN_FRAME);
                              break;
                          }

                          // It's time to send a frame...
                          if((lldp_port->tx.timers.txDelayWhile == 0) && ((lldp_port->tx.timers.txTTR == 0) || (lldp_port->tx.somethingChangedLocal))) {
                              txChangeToState(lldp_port, TX_INFO_FRAME);
                          }
                      }break;
        case TX_SHUTDOWN_FRAME: {
                                    if(lldp_port->tx.timers.txShutdownWhile == 0)
                                        txChangeToState(lldp_port, TX_LLDP_INITIALIZE);
                                }break;
        case TX_INFO_FRAME: {
                                txChangeToState(lldp_port, TX_IDLE);
                            }break;
        default:
                            std::cout<<"[ERROR] The TX State Machine is broken!"<<std::endl;
    };
}


void txStatemachineRun(struct lldp_port *lldp_port,SAGLLDP* lldp)
{
    tx_do_update_timers(lldp_port);

    uint8_t state = lldp_port->tx.state;
    txGlobalStatemachineRun(lldp_port);

    //只有进入下一状态，才进行do_state，保留原来的状态不进行do_state
    if(state != lldp_port->tx.state)
    {
		switch(lldp_port->tx.state)
		{
			case TX_LLDP_INITIALIZE:
				{
					tx_do_tx_lldp_initialize(lldp_port,lldp);
				}break;
			case TX_IDLE:
				{
					tx_do_tx_idle(lldp_port,lldp);
				}break;
			case TX_SHUTDOWN_FRAME:
				{
					tx_do_tx_shutdown_frame(lldp_port,lldp);
				}break;
			case TX_INFO_FRAME:
				{
					tx_do_tx_info_frame(lldp_port,lldp);
				}break;
			default:
				std::cout<<" The TX State Machine is broken!"<<std::endl;
		};
    }

}

void tx_decrement_timer(uint16_t *timer) {
  if((*timer) > 0)
    (*timer)--;
}

void tx_do_update_timers(struct lldp_port *lldp_port) {

	uint16_t current_time = (uint16_t)Simulator::Now().GetSeconds();
	uint16_t delta = current_time - lldp_port->tx.timers.update_time;
	for(uint16_t i = 0; i < delta; i++)
	{
	    tx_decrement_timer(&lldp_port->tx.timers.txShutdownWhile);
	    tx_decrement_timer(&lldp_port->tx.timers.txDelayWhile);
	    tx_decrement_timer(&lldp_port->tx.timers.txTTR);
	}

	lldp_port->tx.timers.update_time = (uint16_t)Simulator::Now().GetSeconds();
    //tx_display_timers(lldp_port);
}

void tx_display_timers(struct lldp_port *lldp_port) {
   std::cout<<"[TIMER] "<<(char*)lldp_port->if_name<<" txTTL: "<<lldp_port->tx.txTTL<<std::endl;
   std::cout<<"[TIMER] "<<(char*)lldp_port->if_name<<" txTTR: "<<lldp_port->tx.timers.txTTR<<std::endl;
   std::cout<<"[TIMER] "<<(char*)lldp_port->if_name<<" txDelayWhile: "<<lldp_port->tx.timers.txDelayWhile<<std::endl;
   std::cout<<"[TIMER] "<<(char*)lldp_port->if_name<<" txShutdownWhile: "<<lldp_port->tx.timers.txShutdownWhile<<std::endl;
}

void tx_do_tx_lldp_initialize(struct lldp_port *lldp_port,SAGLLDP* lldp) {
    /* As per 802.1AB 10.5.4.3 */
    txInitializeLLDP(lldp_port);
    if((lldp_port->adminStatus == enabledRxTx) || (lldp_port->adminStatus == enabledTxOnly)) {
        txChangeToState(lldp_port, TX_IDLE);
        tx_do_tx_idle(lldp_port,lldp);
    }
}

void tx_do_tx_idle(struct lldp_port *lldp_port,SAGLLDP* lldp) {
    lldp_port->tx.txTTL = min(65535, (lldp_port->tx.timers.msgTxInterval * lldp_port->tx.timers.msgTxHold));
    lldp_port->tx.timers.txTTR = lldp_port->tx.timers.msgTxInterval;
    lldp_port->tx.somethingChangedLocal = 0;
    lldp_port->tx.timers.txDelayWhile = lldp_port->tx.timers.txDelay;

    if((lldp_port->adminStatus == disabled) || (lldp_port->adminStatus == enabledRxOnly)) {
        txChangeToState(lldp_port, TX_SHUTDOWN_FRAME);
        tx_do_tx_shutdown_frame(lldp_port,lldp);
    }

    else if((lldp_port->tx.timers.txDelayWhile == 0) && ((lldp_port->tx.timers.txTTR == 0) || (lldp_port->tx.somethingChangedLocal))) {
        txChangeToState(lldp_port, TX_INFO_FRAME);
        tx_do_tx_info_frame(lldp_port,lldp);
    }
}

//TODO
void tx_do_tx_shutdown_frame(struct lldp_port *lldp_port,SAGLLDP* lldp) {
    /* As per 802.1AB 10.5.4.3 */
	Ptr<Packet> packet = Create<Packet>();
    mibConstrShutdownLLDPDU(lldp_port,packet);
    lldp->Send(packet, lldp_port->netdevice->GetBroadcast(), SAGLLDP::PROT_NUMBER, lldp_port);
    lldp_port->tx.timers.txShutdownWhile = lldp_port->tx.timers.reinitDelay;
    if(lldp_port->tx.timers.txShutdownWhile == 0)
    {
    	txChangeToState(lldp_port, TX_LLDP_INITIALIZE);
    	tx_do_tx_lldp_initialize(lldp_port,lldp);
    }
}



void tx_do_tx_info_frame(struct lldp_port *lldp_port,SAGLLDP* lldp) {
    /* As per 802.1AB 10.5.4.3 */
	Ptr<Packet> packet = Create<Packet>();
	lldp->Send(packet, lldp_port->netdevice->GetBroadcast(), SAGLLDP::PROT_NUMBER, lldp_port);
    txChangeToState(lldp_port, TX_IDLE);
    tx_do_tx_idle(lldp_port,lldp);
}

std::string txStateFromID(uint8_t state) {
    switch(state) {
        case TX_LLDP_INITIALIZE:
            return "TX_LLDP_INITIALIZE";
        case TX_IDLE:
            return "TX_IDLE";
        case TX_SHUTDOWN_FRAME:
            return "TX_SHUTDOWN_FRAME";
        case TX_INFO_FRAME:
            return "TX_INFO_FRAME";
    };

    return "Unknown";
}


}
