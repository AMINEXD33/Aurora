#ifndef PROTO_HEADERS
#define PROTO_HEADERS
#include <stdint.h>


#define ETH_HEADER_SIZE_PLAIN 14
#define ETH_HEADER_SIZE_802Q  
typedef struct{
    u_int16_t tci;// PCP(3 bit) | DEI(1 bit) | VID (12 bits)
}vlan_tci;

typedef struct{
    // version 4bits priority 8bits 20bits flow
    u_int32_t ver_prio_flow;
    u_int32_t lenght_nheader_hoplimit;
    u_int64_t source_address[2];
    u_int64_t destin_address[2];
}IPV6;


void protocol_mapper(struct ip *iph);







#endif