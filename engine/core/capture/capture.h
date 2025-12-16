#ifndef CAPTURE_HEADERS
#define CAPTURE_HEADERS
#include <pcap.h>
typedef unsigned char u_char;

pcap_t *INIT_PCAP(char *interface_name);
void packet_handler(
    u_char *user,
    const struct pcap_pkthdr *h,
    const u_char *bytes
);
#endif