/*
 * sag_rx_sm.cc
 *
 *  Created on: 2023年9月21日
 *      Author: kolimn
 */

#include <strings.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "sag_lldp_port.h"
#include "sag_rx_sm.h"
#include "sag_packet.h"
#include "sag_tlv_struct.h"
#include "sag_tlv_content.h"
#include "ns3/simulator.h"
#include "ns3/node.h"

namespace ns3 {

uint8_t rxInitializeLLDP(struct lldp_port *lldp_port) {
    /* As per IEEE 802.1AB section 10.5.5.3 */
    lldp_port->rx.rcvFrame        = 0;

    /* As per IEEE 802.1AB section 10.1.2 */
    lldp_port->rx.tooManyNeighbors = 0;

    lldp_port->rx.rxInfoAge = 0;

    lldp_port -> rx.frame = new uint8_t[lldp_port->mtu];

    lldp_port->rx.timers.update_time = (uint16_t)Simulator::Now().GetSeconds();

    //mibDeleteObjects(lldp_port);

    return 0;
}

std::string rxStateFromID(uint8_t state)
{
    switch(state)
    {
        case LLDP_WAIT_PORT_OPERATIONAL:
            return "LLDP_WAIT_PORT_OPERATIONAL";
        case DELETE_AGED_INFO:
            return "DELETE_AGED_INFO";
        case RX_LLDP_INITIALIZE:
            return "RX_LLDP_INITIALIZE";
        case RX_WAIT_FOR_FRAME:
            return "RX_WAIT_FOR_FRAME";
        case RX_FRAME:
            return "RX_FRAME";
        case DELETE_INFO:
            return "DELETE_INFO";
        case UPDATE_INFO:
            return "UPDATE_INFO";
    };

    return "Unknown";
}

//该函数改变了lldp_port的状态，并且在状态转换时打印出一些信息
void rxChangeToState(struct lldp_port *lldp_port, uint8_t state) {

    if(lldp_port->rx.state != state)
    {
    	std::cout<<"Node_"<<lldp_port->netdevice->GetNode()->GetId()<<"_Dev_"<<lldp_port->netdevice->GetIfIndex()<<":RX from "<<rxStateFromID(lldp_port->rx.state)<<" to "<<rxStateFromID(state)<<std::endl;
    }
    lldp_port->rx.state = state;

}


int ProcessLLDPHeader(struct lldp_port* lldp_port,Ptr<Packet> p)
{

	LLDPHeader LLDPHeader;
	p->RemoveHeader(LLDPHeader);
	LLDPPDU LLDPPDU;
	p->RemoveHeader(LLDPPDU);
	int ret = rxProcessFrame(lldp_port, LLDPHeader,LLDPPDU);
	return ret;
}

int rxProcessFrame(struct lldp_port *lldp_port,LLDPHeader header, LLDPPDU pdu) {
	    uint8_t badFrame = 0;

	    /* Keep track of the last TLV so we can adhere to the specification */
	    /* Which requires the first 3 TLVs to be in the correct order       */
	    uint16_t last_tlv_type = 0;
	    uint16_t num_tlvs      = 0;//tlv的数量
	    uint8_t tlv_type       = 0;//tlv 类型
	    uint16_t tlv_length    = 0;//tlv长度
	    uint16_t tlv_offset    = 0;

		// The current TLV and respective helper variables
		struct lldp_tlv *tlv     = NULL;
		uint8_t *tlv_info_string = NULL;

		// The TLV to be cached for this frame's MSAP
		struct lldp_tlv *cached_tlv = NULL;

	    /* TLV 1 and TLV 2. */
	    uint8_t *msap_id           = NULL;
	    uint32_t msap_length       = 0;
	    uint8_t have_msap          = 0;
	    struct lldp_tlv *msap_tlv1 = NULL;
	    struct lldp_tlv *msap_tlv2 = NULL;
	    struct lldp_tlv *msap_ttl_tlv = NULL;

	    /* The TLV list for this frame */
	    /* This list will be added to the MSAP cache */
	    struct lldp_tlv_list *tlv_list = NULL;

	    /* The MSAP cache for this frame */
	    struct lldp_msap *msap_cache = NULL;


	    std::cout<<"Valiadate LLDP Header!"<<std::endl;
	    //1. 验证Destination MAC
	    uint8_t* des_mac = header.GetDesMacAddress();
	    if(des_mac[0] != 0x01 || des_mac[1] != 0x80 || des_mac[2] != 0xc2 || des_mac[3] != 0x00 ||des_mac[4] != 0x00 || des_mac[5] != 0x0e)
	    {
	    	std::cout<<"The Destination MAC Address is Error!"<<std::endl;
	    	badFrame ++;
	    }

	    //2. 验证以太类型
	    std::cout<<"Valiadate Ethertype!"<<std::endl;
	    uint16_t ethertype = header.GetEtherType();
	    if(ethertype != 0x88cc)
	    {
	    	std::cout<<"The Ethertype is Error!"<<std::endl;
	    	badFrame ++;
	    }

	    if(!badFrame)
	    {
	    	lldp_port->rx.statistics.statsFramesInTotal ++;
	    }

	    //3. 处理各个TLV
	    std::cout<<"Process received TLV!"<<std::endl;
	    tlv_list = pdu.GetTLVList();
	    do{
	    	num_tlvs ++;//处理的tlv数量加1
	    	if(tlv_offset > pdu.GetSerializedSize())
	    	{
	    		std::cout<<"Error! Offset is Larger than Receive Size!"<<std::endl;
	    		badFrame++;
	    		break;
	    	}

	    	struct lldp_tlv * tlv_tem = tlv_list->tlv;
	    	tlv_type = tlv_tem->type;
	    	tlv_length = tlv_tem->length;
	    	tlv_info_string = tlv_tem->info_string;
	    	//是否具有三个强制性的tlv
	    	if(num_tlvs<=3)
	    	{
	    		if(num_tlvs != tlv_type) {
	    			std::cout<<"Error! TLV number "<<num_tlvs<<" should have tlv_type" <<num_tlvs<<" , but is actually "<<tlv_type<<std::endl;
	                lldp_port->rx.statistics.statsFramesDiscardedTotal++;
	                lldp_port->rx.statistics.statsFramesInErrorsTotal++;
	                badFrame++;
	    		}
	    	}
	    	std::cout<<"TLV type: "<<tlv_typetoname(tlv_type) <<", Length: "<<tlv_length<<std::endl;

	    	//提取这个tlv
	    	tlv = initialize_tlv();
	    	if(!tlv)
	    	{
	    		std::cout<<"Unable to malloc buffer for struct tlv!"<<std::endl;
	    	}

	    	tlv ->type = tlv_type;
	    	tlv ->length = tlv_length;
	    	tlv -> info_string = new uint8_t[tlv ->length];
	    	if(tlv->info_string)
	    	{
	            memset(tlv->info_string, 0x0, tlv_length);
	            memcpy(tlv->info_string, tlv_info_string, tlv_length);
	    	}

	    	//处理Time To Live TLV
	    	if(tlv_type == TIME_TO_LIVE_TLV)
	    	{
	    		if(tlv_length != 2)
	    		{
	    			std::cout<<"The TTL TlV Should Have Length: 2, But is "<<tlv_length<<std::endl;
	    		}
	    		else
	    		{
	    			lldp_port->rx.timers.rxTTL = tlv_info_string[0];
	    			std::cout<<"the TTL info is "<<lldp_port->rx.timers.rxTTL<<std::endl;
	    			msap_ttl_tlv = tlv;
	    		}
	    	}

	    	//TODO 这个validate函数要看一下
	    	//验证这个tlv
	    	if(validate_tlv[tlv_type]!=NULL)
	    	{
	    		//std::cout<<"Find a Specific Validator for TLV Type: "<<tlv_typetoname(tlv_type)<<std::endl;
	    		if(validate_tlv[tlv_type](tlv)!=XVALIDTLV)
	    		{
	    			badFrame++;
	    		}
	    	}
	    	else
	    	{
	    		//std::cout<<"Use Generic Validator for TLV Type: "<<(int)tlv_type<<std::endl;
	            if(validate_generic_tlv(tlv) != XVALIDTLV) {
	                badFrame++;
	            }
	    	}

	    	//将这个tlv加入到msap维护的tlv_list中
	    	cached_tlv = initialize_tlv();
	    	if(tlvcpy(cached_tlv, tlv) != 0) {
	    	  std::cout<<"Error copying TLV for MSAP cache!"<<std::endl;
	    	  }
	    	add_tlv(cached_tlv,&tlv_list);

	    	//如果是chassis_id_tlv或者port_id_tlv，需要从中提取信息组成msap_id
	    	if(tlv_type == CHASSIS_ID_TLV) {
	    	  //std::cout<<"Copying TLV1 for MSAP Processing..."<<std::endl;
	    	  msap_tlv1 = initialize_tlv();
	    	  tlvcpy(msap_tlv1, tlv);
	    	}
	    	else if(tlv_type == PORT_ID_TLV) {
	           // std::cout<<"Copying TLV2 for MSAP Processing..."<<std::endl;
	    	    msap_tlv2 = initialize_tlv();
	    	    tlvcpy(msap_tlv2, tlv);

				msap_id = new uint8_t[msap_tlv1->length - 1  + msap_tlv2->length - 1];
				if(!msap_id)
				{
					std::cout<<"Error！Unable to malloc buffer for masp_id!"<<std::endl;
				}

				memcpy(msap_id, &msap_tlv1->info_string[1], msap_tlv1->length - 1);
				memcpy(&msap_id[msap_tlv1->length - 1], &msap_tlv2->info_string[1], msap_tlv2->length - 1);

				msap_length = (msap_tlv1->length - 1) + (msap_tlv2->length - 1);

				//释放msap_tlv1 msap_tlv2的内存
				destroy_tlv(&msap_tlv1);
				destroy_tlv(&msap_tlv2);
				msap_tlv1 = NULL;
				msap_tlv2 = NULL;

				have_msap = 1;
	    	}

	        tlv_offset += sizeof(tlv_type) + sizeof(tlv_length) + tlv->length;
	        last_tlv_type = tlv_type;
	        //释放tlv占用的内存
	        destroy_tlv(&tlv);

	        if(last_tlv_type!=0)
	        {
	        	tlv_list = tlv_list->next;
	        }
	    }
	    //当tlv_type = 0时，那么说明是last tlv
	    while(last_tlv_type != 0);

	    //处理新的msap
	    if(have_msap)
	    {
	    	lldp_port->rxChanges = true;
	    	//std::cout<< "We have a "<<msap_length<< " byte MSAP!" <<std::endl;

	    	msap_cache = new lldp_msap;
			msap_cache->id = msap_id;
			msap_cache->length = msap_length;
			msap_cache->tlv_list = tlv_list;
			msap_cache->next = NULL;

			msap_cache->ttl_tlv = msap_ttl_tlv;
			msap_ttl_tlv = NULL;
			//std::cout<<"Setting rxInfoTTL to: "<<lldp_port->rx.timers.rxTTL<<std::endl;
			msap_cache->rxInfoTTL = lldp_port->rx.timers.rxTTL;
			msap_cache->update_time = (uint16_t)Simulator::Now().GetSeconds();

			//Mengy's::
			update_msap_cache(lldp_port, msap_cache);

			if(msap_tlv1 != NULL) {
			  delete(msap_tlv1);
			  msap_tlv1 = NULL;
									}
			if(msap_tlv2 != NULL) {
			  delete(msap_tlv2);
			  msap_tlv2 = NULL;
									}
	      }
	      else
	      {
	    		std::cout<< "ERROR! No MSAP for TLVs in Frame!"<<std::endl;
	      }

	    /* Report frame errors */
	    if(badFrame) {
	        rxBadFrameInfo(badFrame);
	    }

	    //std::cout<<"Process Done!"<<std::endl;
	    return badFrame;
}

void rxBadFrameInfo(uint8_t frameErrors) {
    std::cout<<"WARNING! This frame had "<<frameErrors<<" errors!"<<std::endl;
}

uint8_t mibUpdateObjects(struct lldp_port *lldp_port) {
    return 0;
}

//删除lldp_port中过期的邻居信息msap
uint8_t mibDeleteObjects(struct lldp_port *lldp_port) {
  struct lldp_msap *current = lldp_port->msap_cache;
  struct lldp_msap *tail    = NULL;
  struct lldp_msap *tmp     = NULL;

  while(current != NULL) {
    if(current->rxInfoTTL <= 0) {

      // If the top list is expired, then adjust the list
      // before we delete the node.
      if(current == lldp_port->msap_cache) {
	lldp_port->msap_cache = current->next;
      } else {
	tail->next = current->next;
      }

      tmp = current;
      current = current->next;

      if(tmp->id != NULL) {
	   delete(tmp->id);
      }

      //Mengy's::删除这个msap维护的所有tlv信息
      destroy_tlv_list(&tmp->tlv_list);
      delete(tmp);
    } else {
      tail = current;
      current = current->next;
    }
  }

  return 0;
}

//这里是一些计时器、端口使能等改变的状态
uint8_t rxGlobalStatemachineRun(struct lldp_port *lldp_port)
{
  if((lldp_port->rx.rxInfoAge == false) && (lldp_port->portEnabled == false))
    {
      rxChangeToState(lldp_port, LLDP_WAIT_PORT_OPERATIONAL);
    }

  switch(lldp_port->rx.state)
    {

    case LLDP_WAIT_PORT_OPERATIONAL:
      {
	if(lldp_port->rx.rxInfoAge == true)
		  rxChangeToState(lldp_port, DELETE_AGED_INFO);
		if(lldp_port->portEnabled == true)
		  rxChangeToState(lldp_port, RX_LLDP_INITIALIZE);
      }break;

/*    case DELETE_AGED_INFO:
      {
    	  std::cout<<"坏了"<<std::endl<<std::endl;
    	  	  rxChangeToState(lldp_port, LLDP_WAIT_PORT_OPERATIONAL);
      }break;*/

    case RX_LLDP_INITIALIZE:
      {
			if((lldp_port->adminStatus == enabledRxTx) || (lldp_port->adminStatus == enabledRxOnly))
			  rxChangeToState(lldp_port, RX_WAIT_FOR_FRAME);
      }break;

    case RX_WAIT_FOR_FRAME:
      {
			if(lldp_port->rx.rxInfoAge == true)
				rxChangeToState(lldp_port, DELETE_INFO);
			else if((lldp_port->adminStatus == disabled) || (lldp_port->adminStatus == enabledTxOnly))
				rxChangeToState(lldp_port, RX_LLDP_INITIALIZE);
			else if(lldp_port->rx.rcvFrame == true)
				rxChangeToState(lldp_port, RX_FRAME);
      }break;

/*    case DELETE_INFO:
      {
    	  	  std::cout<<"坏了"<<std::endl<<std::endl;
    	  	 rxChangeToState(lldp_port, RX_WAIT_FOR_FRAME);
      }break;*/

    case RX_FRAME:
      {
    	  std::cout<<"坏了"<<std::endl<<std::endl;
			if(lldp_port->rx.timers.rxTTL == 0)
				rxChangeToState(lldp_port, DELETE_INFO);
			if((lldp_port->rx.timers.rxTTL != 0) && (lldp_port->rxChanges == true))
			  {
				rxChangeToState(lldp_port, UPDATE_INFO);
			  }
      }break;

/*    case UPDATE_INFO:
      {
    	  std::cout<<"坏了"<<std::endl<<std::endl;
    	  rxChangeToState(lldp_port, RX_WAIT_FOR_FRAME);
      }break;*/

    default:
            std::cout<<"The RX Global State Machine is broken!"<<std::endl;
    };

  return 0;
}
void rxStatemachineRun(struct lldp_port *lldp_port,Ptr<ns3::Packet> packet)
{
    rx_do_update_timers(lldp_port);
    uint8_t state = lldp_port->rx.state;
    rxGlobalStatemachineRun(lldp_port);


    if(state!=lldp_port->rx.state)
    {
    	//std::cout<<"Global State Run From "<<rxStateFromID(state) <<" Into "<<rxStateFromID(lldp_port->rx.state)<<std::endl;
		switch(lldp_port->rx.state)
		  {
		  case LLDP_WAIT_PORT_OPERATIONAL:
		{
		  rx_do_lldp_wait_port_operational(lldp_port,packet);

		}break;
		  case DELETE_AGED_INFO:
		{
		  rx_do_delete_aged_info(lldp_port,packet);
		}break;
		  case RX_LLDP_INITIALIZE:
		{
		  rx_do_rx_lldp_initialize(lldp_port,packet);
		}break;
		  case RX_WAIT_FOR_FRAME:
		{
		  rx_do_rx_wait_for_frame(lldp_port,packet);
		}break;
		  case RX_FRAME:
		{
		  rx_do_rx_frame(lldp_port,packet);
		}break;
		  case DELETE_INFO: {
		rx_do_rx_delete_info(lldp_port,packet);
		  }break;
		  case UPDATE_INFO: {
		rx_do_rx_update_info(lldp_port,packet);
		  }break;
		  default:
		std::cout<< "The RX State Machine is broken!"<<std::endl;
		};
    }

}

void rx_do_lldp_wait_port_operational(struct lldp_port *lldp_port,Ptr<ns3::Packet> packet)
{
	if(lldp_port->rx.rxInfoAge == true)
		  {
			rxChangeToState(lldp_port, DELETE_AGED_INFO);
			rx_do_delete_aged_info(lldp_port,packet);
		  }
	else if(lldp_port->portEnabled == true)
	{
		  rxChangeToState(lldp_port, RX_LLDP_INITIALIZE);
		  rx_do_rx_lldp_initialize(lldp_port,packet);
	}
}

void rx_do_delete_aged_info(struct lldp_port *lldp_port,Ptr<ns3::Packet> packet)
{
	//std::cout<<"do delete aged info"<<std::endl;
    lldp_port->rx.somethingChangedRemote = false;
    mibDeleteObjects(lldp_port);
    lldp_port->rx.rxInfoAge = false;
    lldp_port->rx.somethingChangedRemote = true;
    rxChangeToState(lldp_port, LLDP_WAIT_PORT_OPERATIONAL);
    rx_do_lldp_wait_port_operational(lldp_port,packet);
}
void rx_do_rx_lldp_initialize(struct lldp_port *lldp_port,Ptr<ns3::Packet> packet)
{
    rxInitializeLLDP(lldp_port);
    lldp_port->rx.rcvFrame = false;
	if((lldp_port->adminStatus == enabledRxTx) || (lldp_port->adminStatus == enabledRxOnly))
	{
	  rxChangeToState(lldp_port, RX_WAIT_FOR_FRAME);
	  rx_do_rx_wait_for_frame(lldp_port,packet);
	}
}
void rx_do_rx_wait_for_frame(struct lldp_port *lldp_port,Ptr<ns3::Packet> packet)
{
    lldp_port->rx.badFrame = false;
    lldp_port->rx.rxInfoAge = false;
    lldp_port->rx.somethingChangedRemote = false;
	if(lldp_port->rx.rxInfoAge == true)
	{
		rxChangeToState(lldp_port, DELETE_INFO);
		rx_do_rx_delete_info(lldp_port,packet);
	}
	if((lldp_port->adminStatus == disabled) || (lldp_port->adminStatus == enabledTxOnly))
	{
		rxChangeToState(lldp_port, RX_LLDP_INITIALIZE);
		rx_do_rx_lldp_initialize(lldp_port,packet);
	}
	else if(lldp_port->rx.rcvFrame == true)
	{
		rxChangeToState(lldp_port, RX_FRAME);
		rx_do_rx_frame(lldp_port,packet);
	}
}
void rx_do_rx_frame(struct lldp_port *lldp_port,Ptr<ns3::Packet> packet)
{
    lldp_port->rxChanges = false;
    lldp_port->rx.rcvFrame = false;
    int ret = ProcessLLDPHeader(lldp_port,packet);


	if(lldp_port->rx.timers.rxTTL == 0)
		{
			rxChangeToState(lldp_port, DELETE_INFO);
			rx_do_delete_aged_info(lldp_port,packet);
		}
	else if((lldp_port->rx.timers.rxTTL != 0) && (lldp_port->rxChanges == true))
	  {
			rxChangeToState(lldp_port, UPDATE_INFO);
			rx_do_rx_update_info(lldp_port,packet);
	  }

	else if(ret!=0||(lldp_port->rx.timers.rxTTL != 0 && lldp_port->rxChanges == false))
	{
		  rxChangeToState(lldp_port, RX_WAIT_FOR_FRAME);
		  rx_do_rx_wait_for_frame(lldp_port,packet);
	}

}
void rx_do_rx_delete_info(struct lldp_port *lldp_port,Ptr<ns3::Packet> packet)
{
    mibDeleteObjects(lldp_port);
    lldp_port->rx.somethingChangedRemote = true;
    rxChangeToState(lldp_port, RX_WAIT_FOR_FRAME);
    rx_do_rx_wait_for_frame(lldp_port,packet);
}
void rx_do_rx_update_info(struct lldp_port *lldp_port,Ptr<ns3::Packet> packet)
{
    mibUpdateObjects(lldp_port);
    lldp_port->rx.somethingChangedRemote = true;
	  rxChangeToState(lldp_port, RX_WAIT_FOR_FRAME);
	  rx_do_rx_wait_for_frame(lldp_port,packet);
}

void rx_decrement_timer(uint16_t *timer) {
    if((*timer) > 0) {
        (*timer)--;
    }
}

void rx_do_update_timers(struct lldp_port *lldp_port) {
  struct lldp_msap *msap_cache = lldp_port->msap_cache;

  // Here's where we update the IEEE 802.1AB RX timers:
  while(msap_cache != NULL) {

	uint16_t current_time  = (uint16_t)Simulator::Now().GetSeconds();
	uint16_t update_time = msap_cache->update_time;

	for(uint16_t i = 0; i<current_time-update_time; i++)
	{
		rx_decrement_timer(&msap_cache->rxInfoTTL);
	}
	msap_cache->update_time =(uint16_t)Simulator::Now().GetSeconds();



    // We're going to potenetially break the state machine here for a performance bump.
    // The state machine isn't clear (to me) how rxInfoAge is supposed to be set (per MSAP or per port)
    // and it seems to me that having a single tag that gets set if at least 1 MSAP is out of date
    // is much more efficient than traversing the entire cache every state machine loop looking for an
    // expired MSAP...
    if(msap_cache->rxInfoTTL <= 0)
      lldp_port->rx.rxInfoAge = true;

    msap_cache = msap_cache->next;
  }

	for(uint16_t i = 0; i<(uint16_t)Simulator::Now().GetSeconds()-lldp_port->rx.timers.update_time; i++)
	{
		rx_decrement_timer(&lldp_port->rx.timers.tooManyNeighborsTimer);
	}
	lldp_port->rx.timers.update_time = (uint16_t)Simulator::Now().GetSeconds();


  //rx_display_timers(lldp_port);
}


}
