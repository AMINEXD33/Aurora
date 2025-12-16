#ifndef PROTO_HEADERS
#define PROTO_HEADERS
#include <stdint.h>


#define ETH_HEADER_SIZE_PLAIN 14
#define ETH_HEADER_SIZE_802Q  
typedef struct{
    u_int16_t tci;// PCP(3 bit) | DEI(1 bit) | VID (12 bits)
}vlan_tci;

void protocol_mapper(struct ip *iph);







#endif