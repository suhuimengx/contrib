/*
 * sag_tlv_content.h
 *
 *  Created on: 2023年10月13日
 *      Author: kolimn
 */

#ifndef CONTRIB_PROTOCOLS_MODEL_SAG_LLDP_SAG_TLV_CONTENT_H_
#define CONTRIB_PROTOCOLS_MODEL_SAG_LLDP_SAG_TLV_CONTENT_H_
/*******************************************************************
 *
 * OpenLLDP TLV
 *
 * See LICENSE file for more info.
 *
 * File: lldp_tlv.c
 *
 * Authors: Terry Simons (terry.simons@gmail.com)
 *
 *******************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sag_lldp_port.h"
#include "ns3/node.h"
#include "sag_tlv_content.h"
#include "sag_tlv_struct.h"
//#include "lldp_neighbor.h"

namespace ns3 {
/* There are a max of 128 TLV validators (types 0 through 127), so we'll stick them in a static array indexed by their tlv type */
uint8_t (*validate_tlv[128])(struct lldp_tlv *tlv) = {
    validate_end_of_lldpdu_tlv,        /* 0 End of LLDPU TLV        */
    validate_chassis_id_tlv,          /* 1 Chassis ID TLV          */
    validate_port_id_tlv,             /* 2 Port ID TLV             */
    validate_ttl_tlv,                 /* 3 Time To Live TLV        */
    validate_port_description_tlv,    /* 4 Port Description TLV    */
    validate_system_name_tlv,         /* 5 System Name TLV         */
    validate_system_description_tlv,  /* 6 System Description TLV  */
    validate_system_capabilities_tlv, /* 7 System Capabilities TLV */
    validate_management_address_tlv,  /* 8 Management Address TLV  */
    /* 9 - 126 are reserved and set to NULL in lldp_tlv_validator_init()                        */
    /* 127 is populated for validate_organizationally_specific_tlv in lldp_tlv_validator_init() */
};




/*char *capability_name(uint16_t capability)
{
    switch(capability)
    {
        case SYSTEM_CAPABILITY_OTHER:
            return "Other";
        case SYSTEM_CAPABILITY_REPEATER:
            return "Repeater/Hub";
        case SYSTEM_CAPABILITY_BRIDGE:
            return "Bridge/Switch";
        case SYSTEM_CAPABILITY_WLAN:
            return "Wireless LAN";
        case SYSTEM_CAPABILITY_ROUTER:
            return "Router";
        case SYSTEM_CAPABILITY_TELEPHONE:
            return "Telephone";
        case SYSTEM_CAPABILITY_DOCSIS:
            return "DOCSIS/Cable Modem";
        case SYSTEM_CAPABILITY_STATION:
            return "Station";
        default:
            return "Unknown";
    };
}

*/

std::string tlv_typetoname(uint8_t tlv_type)
{
    switch(tlv_type)
    {
        case CHASSIS_ID_TLV:
            return "Chassis ID";
            break;
        case PORT_ID_TLV:
            return "Port ID";
            break;
        case TIME_TO_LIVE_TLV:
            return "Time To Live";
            break;
        case PORT_DESCRIPTION_TLV:
            return "Port Description";
            break;
        case SYSTEM_NAME_TLV:
            return "System Name";
            break;
        case SYSTEM_DESCRIPTION_TLV:
            return "System Description";
            break;
        case SYSTEM_CAPABILITIES_TLV:
            return "System Capabiltiies";
            break;
        case MANAGEMENT_ADDRESS_TLV:
            return "Management Address";
            break;
        case ORG_SPECIFIC_TLV:
            return "Organizationally Specific";
            break;
        case END_OF_LLDPDU_TLV:
            return "End Of LLDPDU";
            break;
        default:
            return "Unknown";
    };
}


void tlvCleanupLLDP()
{

}


uint8_t initializeTLVFunctionValidators()
{
    int index = 0;

    /* Set all of the reserved TLVs to NULL validator functions */
    /* so they're forced to go through the generic validator    */
    for(index = LLDP_BEGIN_RESERVED_TLV; index < LLDP_END_RESERVED_TLV; index++)
    {

        validate_tlv[index] = NULL;
    }

    //std::cout<<"Setting TLV Validator"<<ORG_SPECIFIC_TLV<<"  - it's the organizational specific TLV validator..."<<std::endl;

    validate_tlv[ORG_SPECIFIC_TLV] = validate_organizationally_specific_tlv;

    return 0;
}

struct lldp_tlv *initialize_tlv() {
  struct lldp_tlv *tlv = new struct lldp_tlv;

  return tlv;
}

struct lldp_tlv *create_end_of_lldpdu_tlv(struct lldp_port *lldp_port) {

    struct lldp_tlv* tlv = initialize_tlv();

    tlv->type = END_OF_LLDPDU_TLV; // Constant defined in lldp_tlv.h
    tlv->length = 0;     // The End of LLDPDU TLV is length 0.

    tlv->info_string = NULL;

    return tlv;
}

uint8_t validate_end_of_lldpdu_tlv(struct lldp_tlv *tlv)
{
    if(tlv->length != 0)
    {
        std::cout<<"[ERROR] TLV type is 'End of LLDPDU' (0), but TLV length is %d when it should be 0!"<<std::endl;
        return XEINVALIDTLV;
    }

    return XVALIDTLV;
}

uint8_t validate_length_max_255(struct lldp_tlv *tlv)
{
    //Length will never be below 0 because the variable used is unsigned...
    if(tlv->length > 255)
    {
    	std::cout<<"[ERROR] TLV has invalid length"<<tlv->length<< std::endl;
    	std::cout<<"It should be between 0 and 255 inclusive!"<<std::endl;

        return XEINVALIDTLV;
    }

    return XVALIDTLV;
}

uint8_t validate_length_max_256(struct lldp_tlv *tlv)
{
    if(tlv->length < 2 || tlv->length > 256)
    {
    	std::cout<<"[ERROR] TLV has invalid length"<<tlv->length<< std::endl;
    	std::cout<<"It should be between 0 and 256 inclusive!"<<std::endl;

        return XEINVALIDTLV;
    }

    return XVALIDTLV;
}


struct lldp_tlv *create_chassis_id_tlv(struct lldp_port *lldp_port) {

    struct lldp_tlv* tlv = initialize_tlv();

    tlv->type = CHASSIS_ID_TLV; // Constant defined in lldp_tlv.h
    tlv->length = 7; //The size of a MAC + the size of the subtype (1 byte)

    tlv->info_string = new uint8_t[tlv->length];

    // CHASSIS_ID_MAC_ADDRESS is a 1-byte value - 4 in this case. Defined in lldp_tlv.h
    tlv->info_string[0] = CHASSIS_ID_MAC_ADDRESS;

    // We need to start copying at the 2nd byte, so we use [1] here...
    // This reads "memory copy to the destination at the address of tlv->info_string[1] with the source my_mac for 6 bytes" (the size of a MAC address)
    memcpy(&tlv->info_string[1], &lldp_port->source_mac[0], 6);

    return tlv;
}

uint8_t validate_chassis_id_tlv(struct lldp_tlv *tlv)
{
    // Several TLVs have this requirement.
    return validate_length_max_256(tlv);
}

struct lldp_tlv *create_port_id_tlv(struct lldp_port *lldp_port) {

    struct lldp_tlv* tlv = initialize_tlv();

    tlv->type = PORT_ID_TLV; // Constant defined in lldp_tlv.h
    if(strlen(lldp_port->if_name) == 0)
    {
    	Ptr<NetDevice> netdevice = lldp_port->netdevice;
    	Ptr<Node> node = netdevice->GetNode();
		char name[20];
		std::sprintf(name,"%s_%d","node",node->GetId());
		char real_name[20 + 10];
		std::sprintf(real_name, "%s_%d", name, lldp_port -> if_index);
		lldp_port -> if_name = real_name;
    	std::cout<<"the interface name is "<<lldp_port->if_name<<std::endl;
    }

    tlv->length = 1 + strlen(lldp_port->if_name); //The length of the interface name + the size of the subtype (1 byte)


    tlv->info_string = new uint8_t[tlv->length];

    // PORT_ID_INTERFACE_NAME is a 1-byte value - 5 in this case. Defined in lldp_tlv.h
    tlv->info_string[0] = PORT_ID_INTERFACE_NAME;


    // We need to start copying at the 2nd byte, so we use [1] here...
    // This reads "memory copy to the destination at the address of tlv->info_string[1] with the source lldp_port->if_name for strlen(lldp_port->if_name) bytes"
    memcpy(&tlv->info_string[1], lldp_port->if_name, strlen(lldp_port->if_name));

    return tlv;
}

uint8_t validate_port_id_tlv(struct lldp_tlv *tlv)
{
    // Several TLVs have this requirement.
    return validate_length_max_256(tlv);
}

struct lldp_tlv *create_ttl_tlv(struct lldp_port *lldp_port) {

    struct lldp_tlv* tlv = initialize_tlv();
    //Mengy's::modify
    uint16_t ttl = lldp_port->tx.txTTL;

    tlv->type = TIME_TO_LIVE_TLV; // Constant defined in lldp_tlv.h
    tlv->length = 2; // Static length defined by IEEE 802.1AB section 9.5.4

    tlv->info_string = new uint8_t[tlv->length];

    memcpy(tlv->info_string, &ttl, tlv->length);


    return tlv;
}

uint8_t validate_ttl_tlv(struct lldp_tlv *tlv)
{
    if(tlv->length != 2)
    {
        std::cout<< "[ERROR] TLV has invalid length"<<tlv->length<<std::endl;
        std::cout<<"Length should be 2"<<std::endl;

        return XEINVALIDTLV;
    }

    return XVALIDTLV;
}

struct lldp_tlv *create_port_description_tlv(struct lldp_port *lldp_port) {

    struct lldp_tlv* tlv = initialize_tlv();

    tlv->type = PORT_DESCRIPTION_TLV; // onstant defined in lldp_tlv.h
    tlv->length = strlen(lldp_port->if_name);

    tlv->info_string = new uint8_t[tlv->length];

    memcpy(&tlv->info_string[0], lldp_port->if_name, strlen(lldp_port->if_name));

    return tlv;

}


uint8_t validate_port_description_tlv(struct lldp_tlv *tlv)
{
    // Several TLVs have this requirement.
    return validate_length_max_255(tlv);
}

struct lldp_tlv *create_system_name_tlv(struct lldp_port *lldp_port)
{

    struct lldp_tlv* tlv = initialize_tlv();

    tlv->type = SYSTEM_NAME_TLV; // Constant defined in lldp_tlv.h

    char lldp_systemname[] = "SAG_Ubantu_20.04";
    tlv->length = strlen(lldp_systemname);

    tlv->info_string = new uint8_t[tlv->length];

    memcpy(tlv->info_string, lldp_systemname, tlv->length);

    return tlv;
}

uint8_t validate_system_name_tlv(struct lldp_tlv *tlv)
{
    // Several TLVs have this requirement.
    return validate_length_max_255(tlv);
}

struct lldp_tlv *create_system_description_tlv(struct lldp_port *lldp_port)
{

    struct lldp_tlv* tlv = initialize_tlv();

    tlv->type = SYSTEM_DESCRIPTION_TLV; // Constant defined in lldp_tlv.h

    char lldp_systemdesc[]="SAG_Ubantu_Info";
    tlv->length = strlen(lldp_systemdesc);

    tlv->info_string = new uint8_t[tlv->length];

    memcpy(tlv->info_string, lldp_systemdesc, tlv->length);

    return tlv;

}

uint8_t validate_system_description_tlv(struct lldp_tlv *tlv)
{
    // Several TLVs have this requirement.
    return validate_length_max_255(tlv);
}

struct lldp_tlv *create_system_capabilities_tlv(struct lldp_port *lldp_port) {
    struct lldp_tlv* tlv = initialize_tlv();
    // Tell it we're a station for now... bit 7
    uint16_t capabilities = 128;

    tlv->type = SYSTEM_CAPABILITIES_TLV; // Constant defined in lldp_tlv.h

    tlv->length = 4;

    tlv->info_string = new uint8_t[tlv->length];

    memcpy(&tlv->info_string[0], &capabilities, sizeof(uint16_t));
    memcpy(&tlv->info_string[2], &capabilities, sizeof(uint16_t));

    return tlv;
}

uint8_t validate_system_capabilities_tlv(struct lldp_tlv *tlv)
{
    if(tlv->length != 4)
    {
        std::cout<< "[ERROR] TLV has invalid length"<<tlv->length<<std::endl;
        std::cout<<"Length should be 4"<<std::endl;

        return XEINVALIDTLV;
    }

    return XVALIDTLV;
}

// NB: Initial deployment will do IPv4 only...
//
struct lldp_tlv *create_management_address_tlv(struct lldp_port *lldp_port) {
   struct lldp_tlv *tlv = initialize_tlv();
   //uint32_t if_index = lldp_port->if_index;

    tlv->type = MANAGEMENT_ADDRESS_TLV; // Constant defined in lldp_tlv.h

#define MGMT_ADDR_STR_LEN 1
#define MGMT_ADDR_SUBTYPE 1
#define IPV4_LEN 4
#define IF_NUM_SUBTYPE 1
#define IF_NUM 4
#define OID 1
#define OBJ_IDENTIFIER 0

    // management address string length (1 octet)
    // management address subtype (1 octet)
    // management address (4 bytes for IPv4)
    // interface numbering subtype (1 octet)
    // interface number (4 bytes)
    // OID string length (1 byte)
    // object identifier (0 to 128 octets)
    tlv->length = MGMT_ADDR_STR_LEN + MGMT_ADDR_SUBTYPE + IPV4_LEN + IF_NUM_SUBTYPE + IF_NUM + OID + OBJ_IDENTIFIER ;

    //uint64_t tlv_offset = 0;

    tlv->info_string = new uint8_t[tlv->length];

    // Management address string length
    // subtype of 1 byte + management address length, so 5 for IPv4
    tlv->info_string[0] = 5;

    // 1 for IPv4 as per http://www.iana.org/assignments/address-family-numbers
    tlv->info_string[1] = 1;

    // Copy in our IP
    memcpy(&tlv->info_string[2], lldp_port->source_ipaddr, 4);

    // Interface numbering subtype... system port number in our case.
    tlv->info_string[6] = 3;

    // Interface number... 4 bytes long, or uint32_t
    memcpy(&tlv->info_string[7], &lldp_port->if_index, sizeof(uint32_t));

    //std::cout<<"Would stuff interface #: "<<if_index<<std::endl;

    // OID - 0 for us
    tlv->info_string[11] = 0;

    // object identifier... doesn't exist for us because it's not required, and we don't have an OID.

    return tlv;
}

uint8_t validate_management_address_tlv(struct lldp_tlv *tlv)
{
    if(tlv->length < 9 || tlv->length > 167)
    {
        std::cout<<"[ERROR] TLV has invalid length "<<tlv->length<<std::endl;
        std::cout<<"It should be between 9 and 167 inclusive!"<<std::endl;

        return XEINVALIDTLV;
    }

    return XVALIDTLV;
}





uint8_t validate_organizationally_specific_tlv(struct lldp_tlv *tlv)
{
    if(tlv->length < 4 || tlv->length > 511)
    {
        std::cout<<"[ERROR] TLV has invalid length "<<tlv->length<<std::endl;
        std::cout<<"It should be between 4 and 511 inclusive!"<<std::endl;

        return XEINVALIDTLV;
    }

    return XVALIDTLV;
}

uint8_t validate_generic_tlv(struct lldp_tlv *tlv)
{
	std::cout<<"Generic TLV Validation for TLV type: "<<tlv->type<<std::endl;
    std::cout<<"TLV Info String Length: "<<tlv->length<<std::endl;
    std::cout<<"TLV Info String: "<<tlv->info_string<<" "<<tlv->length;

    // Length will never fall below 0 because it's an unsigned variable
    if(tlv->length > 511)
    {
    	std::cout<<"[ERROR] TLV has invalid length "<<tlv->length<<std::endl;
    	std::cout<<"It should be between 0 and 511 inclusive"<<std::endl;

        return XEINVALIDTLV;
    }

    return XVALIDTLV;
}



// A helper function to explode a flattened TLV.
struct lldp_tlv *explode_tlv(struct lldp_flat_tlv *flat_tlv) {

    uint16_t type_and_length = 0;
    struct lldp_tlv *tlv   = NULL;

    tlv = new struct lldp_tlv;

    if(tlv) {

        // Suck the type and length out...
        type_and_length = *(uint16_t *)&tlv[0];

        tlv->length     = type_and_length & 511;
        type_and_length = type_and_length >> 9;
        tlv->type       = (uint8_t)type_and_length;

        tlv->info_string = new uint8_t[tlv->length];

        if(tlv->info_string) {
            // Copy the info string into our TLV...
            memcpy(&tlv->info_string[0], &flat_tlv->tlv[sizeof(type_and_length)], tlv->length);
        } else { // tlv->info_string == NULL
            std::cout<<"[ERROR] Unable to malloc buffer"<< std::endl;
        }
    } else { // tlv == NULL
    	std::cout<<"[ERROR] Unable to malloc buffer"<< std::endl;
    }

    return tlv;
}

uint8_t tlvcpy(struct lldp_tlv *dst, struct lldp_tlv *src)
{
  if(src == NULL)
    return -1;

  if(dst == NULL)
    return -1;

  dst->type = src->type;
  dst->length = src->length;
  dst->info_string = new uint8_t[dst->length];

  if(((dst->info_string != NULL) && (src->info_string != NULL)))
    {
      memcpy(dst->info_string, src->info_string, dst->length);
    }
  else
    {
      std::cout<<"[ERROR] Couldn't allocate memory!"<<std::endl;

      return -1;
    }

  return 0;
}



char *tlv_info_string_to_cstr(struct lldp_tlv *tlv)
{
	char *cstr = new char[tlv->length + 1];

  if(tlv == NULL)
    return NULL;

  if(tlv->length <= 0)
    return NULL;

  if(tlv->info_string == NULL)
    return NULL;


  memcpy(cstr, tlv->info_string, tlv->length);

  return cstr;
}
}



#endif /* CONTRIB_PROTOCOLS_MODEL_SAG_LLDP_SAG_TLV_CONTENT_H_ */
