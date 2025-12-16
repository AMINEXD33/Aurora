#define _GNU_SOURCE
#include <netinet/if_ether.h>   // struct ether_header
#include <netinet/ip.h>         // struct ip, struct iphdr
#include <netinet/tcp.h>        // struct tcphdr
#include <netinet/udp.h>        // struct udphdr
#include <netinet/ip_icmp.h>    // struct icmphdr
#include <netinet/ip6.h>        // struct ip6_hdr (IPv6)
#include <arpa/inet.h>          // ntohs(), ntohl(), inet_ntoa()
#include <net/ethernet.h>  
#include <string.h>
#include <stdio.h>
void protocol_mapper(struct ip *iph){
    if (!iph)
        return;
    char src_ip[INET_ADDRSTRLEN];
    char dst_ip[INET_ADDRSTRLEN];
    strcpy(src_ip, inet_ntoa(iph->ip_src));
    strcpy(dst_ip, inet_ntoa(iph->ip_dst));
    char protocol_str[200]; 
    switch (iph->ip_p)
    {
        case 0:// IP
            /* code */
            break;
        case 1:// ICMP
            strcpy(protocol_str, "ICMP");
            break;
        case 2:// IGMP
            strcpy(protocol_str, "IGMP");
            break;
        case 4:// IPIP
            strcpy(protocol_str, "IPIP");
            break;
        case 6:// TCP
            strcpy(protocol_str, "TCP");
            break;
        case 8:// EGP
            strcpy(protocol_str, "EGP");
            break;
        case 12:// PUP
            strcpy(protocol_str, "PUP");
            break;
        case 17:// UDP
            strcpy(protocol_str, "UDP");
            break;
        case 22:// IDP
            strcpy(protocol_str, "IDP");
            break;
        case 29:// TP
            strcpy(protocol_str, "TP");
            break;
        case 33:// DCCP
            strcpy(protocol_str, "DCCP");
            break;
        case 41:// IPV6
            strcpy(protocol_str, "IPV6");
            break;
        case 46:
            break;
        case 47:
            break;
        case 50:
            break;
        case 51:
            break;
        case 92:
            break;
        case 94:
            break;
        case 98:
            break;
        case 103:
            break;
        case 108:
            break;
        case 115:
            break;
        case 132:
            break;
        case 136:
            break;
        case 137:
            break;
        case 143:
            break;
        case 255:
            break;
    default:
        break;
    }
    printf(
    "[%s]%s -> %s\n", 
    protocol_str,
    src_ip,
    dst_ip
    );

}