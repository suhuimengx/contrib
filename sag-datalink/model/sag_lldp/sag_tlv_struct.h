/*
 * sag_tlv_struct.h
 *
 *  Created on: 2023年9月22日
 *      Author: kolimn
 */

#ifndef CONTRIB_PROTOCOLS_MODEL_SAG_LLDP_SAG_TLV_STRUCT_H_
#define CONTRIB_PROTOCOLS_MODEL_SAG_LLDP_SAG_TLV_STRUCT_H_

#include <stdint.h>
#include <iostream>

namespace ns3 {
//tlv架构的轻量版
struct lldp_flat_tlv {
    uint16_t size;
    uint8_t *tlv;
};

//tlv的具体内容
struct lldp_tlv {
    uint8_t  type;
    uint16_t length;
    uint8_t  *info_string;

    lldp_tlv(){
    	type = 0;
    	length = 0;
    	info_string=NULL;
    }
};

struct lldp_organizational_tlv {
    uint8_t oui[3];
    uint8_t oui_subtype;
};

//tlv列表
struct lldp_tlv_list {
    struct lldp_tlv_list *next;
    struct lldp_tlv *tlv;

    lldp_tlv_list(){
    	next = NULL;
    	tlv = NULL;
    }
};

/**
 * Add a tlv to tlv list
 *
 * \param tlv, the new tlv to add
 * \param tlv_list, tlv list
 *
 */
void add_tlv(struct lldp_tlv *tlv, struct lldp_tlv_list **tlv_list);

struct lldp_flat_tlv *flatten_tlv(struct lldp_tlv *tlv);
struct lldp_tlv *explode_tlv(struct lldp_flat_tlv *flat_tlv);

/**
 * Use generic funtion to validate a tlv
 * \param tlv, tlv
 *
 */
uint8_t validate_generic_tlv(struct lldp_tlv *tlv);

void destroy_tlv(struct lldp_tlv **tlv);
void destroy_flattened_tlv(struct lldp_flat_tlv **tlv);

void destroy_tlv_list(struct lldp_tlv_list **tlv_list);

/**
 * Use the new masp to update the old msap of lldp port
 *
 * \param lldp_port, lldp port
 * \param msap_cache, the new msap
 *
 */
void update_msap_cache(struct lldp_port *lldp_port, struct lldp_msap* msap_cache);

}

#endif /* CONTRIB_PROTOCOLS_MODEL_SAG_LLDP_SAG_TLV_STRUCT_H_ */
