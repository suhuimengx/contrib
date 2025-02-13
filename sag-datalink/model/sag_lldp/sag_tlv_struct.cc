/*
 * sag_tlv_struct.cc
 *
 *  Created on: 2023年9月22日
 *      Author: kolimn
 */



#include <stdlib.h>
#include <string.h>
#include <iostream>

#include "sag_tlv_struct.h"
#include "sag_lldp_port.h"
#include "sag_tlv_content.h"


namespace ns3 {
// 将tlv变成一个轻量化的flatten_tlv
struct lldp_flat_tlv *flatten_tlv(struct lldp_tlv *tlv) {
	uint16_t type_and_length       = 0;
	struct lldp_flat_tlv *flat_tlv = NULL;

    if(!tlv) {
        return NULL;
    }

    type_and_length = tlv->type;
    type_and_length = type_and_length << 9;
    type_and_length |= tlv->length;


    //Mengy's::modify
    flat_tlv = new struct lldp_flat_tlv;

    if(flat_tlv) {
        // We malloc for the size of the entire TLV, plus the 2 bytes for type and length.
        flat_tlv->size = tlv->length + 2;

        flat_tlv->tlv = new uint8_t[flat_tlv->size];
        memset(&flat_tlv->tlv[0], 0x0, flat_tlv->size);
        memcpy(&flat_tlv->tlv[0], &type_and_length, sizeof(type_and_length));
        memcpy(&flat_tlv->tlv[sizeof(type_and_length)], tlv->info_string, tlv->length);
    }

    return flat_tlv;
}

void destroy_tlv(struct lldp_tlv **tlv) {
	if((tlv != NULL && (*tlv) != NULL))
	{
		if((*tlv)->info_string != NULL)
		{
			delete((*tlv)->info_string);
			((*tlv)->info_string) = NULL;
		}

		delete(*tlv);
		(*tlv) = NULL;
	}
}

void destroy_flattened_tlv(struct lldp_flat_tlv **tlv) {
	if((tlv != NULL && (*tlv) != NULL))
	{
		if((*tlv)->tlv != NULL)
		{
			delete((*tlv)->tlv);
			(*tlv)->tlv = NULL;

			delete(*tlv);
			(*tlv) = NULL;
		}
	}
}

/** */
void destroy_tlv_list(struct lldp_tlv_list **tlv_list) {
    struct lldp_tlv_list *current  = *tlv_list;

    //std::cout<<"Destroy TLV List"<<std::endl;

    if(current == NULL) {
        std::cout<< "[WARNING] Asked to delete empty list!"<<std::endl;
    }

    //std::cout<< "Entering Destroy loop"<<std::endl;

    while(current != NULL) {

    	/*std::cout<< "current = "<<current<<std::endl;
    	std::cout<< "current->next = "<<current->next<<std::endl;*/

        current = current->next;

        //std::cout<< "deleteing TLV Info String."<<std::endl;

        delete((*tlv_list)->tlv->info_string);

        //std::cout<< "deleteing TLV."<<std::endl;
        delete((*tlv_list)->tlv);

	//std::cout<<  "deleteing TLV List Node."<<std::endl;
        delete(*tlv_list);

        (*tlv_list) = current;
    }
    //std::cout<<"Succeed in Destroying TLV_List, Space is Free!"<<std::endl;
}

void add_tlv(struct lldp_tlv *tlv, struct lldp_tlv_list **tlv_list) {
	//std::cout<<"Add TLV: "<<tlv_typetoname(tlv->type)<<" to Packet!"<<std::endl;
  struct lldp_tlv_list *tail = NULL;
  struct lldp_tlv_list *tmp = *tlv_list;

  if(tlv != NULL) {
    tail = new struct lldp_tlv_list;

    tail->tlv  = tlv;
    tail->next = NULL;

    if((*tlv_list) == NULL) {
        (*tlv_list) = tail;
    } else {
        while(tmp->next != NULL) {
            tmp = tmp->next;
        }

        //std::cout<<"Setting temp->next to "<<tail<<std::endl;

        tmp->next = tail;
    }
  }

}

//根据收到的msap替换id相同的msap
void update_msap_cache(struct lldp_port *lldp_port, struct lldp_msap* msap_cache) {
  struct lldp_msap *old_cache = lldp_port->msap_cache;
  struct lldp_msap *new_cache = msap_cache;

  while(old_cache != NULL) {

    if(old_cache->length == new_cache->length) {

      if(memcmp(old_cache->id, new_cache->id, new_cache->length) == 0) {

    	  	  destroy_tlv_list(&old_cache->tlv_list);
    	  	  old_cache->tlv_list = new_cache->tlv_list;
    	  	  delete(new_cache->id);
    	  	  delete(new_cache);

    	  	  return;
      }
    }

    old_cache = old_cache->next;
  }


  new_cache->next = lldp_port->msap_cache;
  lldp_port->msap_cache = new_cache;

}




}
