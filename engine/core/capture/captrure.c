#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include "./capture.h"
#include <unistd.h>

/**
 * REFERENCES I WILL NEED :
 * ETHERNER HEADERS : /usr/include/net/ethernet.h
 * IP HEADERS       : /usr/include/netinet/ip.h
 * PROTOCOLS        : /usr/include/netinet/in.h
 */
volatile sig_atomic_t running = 1;
int ret;


void handle_sigint(int sig) {
    (void)sig;
    running = 0;
}

/**
 * INIT_PCAP: this function initiates a pcap capture 
 * with a ring buffer of size 64 * 1024 * 1024 
 * and it allows buffering by setting the `pcap_set_immediate_mode` 
 * second param to 0 and sets a timeout of 10 seconds
 * so we don't wast CPU cycles doing shallow work
 */
pcap_t *INIT_PCAP(char *interface_name){
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *pcap;
    

    signal(SIGINT, handle_sigint);

    // init handler
    pcap = pcap_create(interface_name, errbuf);
    if (!pcap) {
        fprintf(stderr, "pcap_create failed: %s\n", errbuf);
        return NULL;
    }

    // allocate ring buffe
    pcap_set_buffer_size(pcap, 64 * 1024 * 1024);

    // buffer it outtt (this might bite me in the ass)
    pcap_set_immediate_mode(pcap, 0); // 0 allow buffering 

    // my trafic is my trafic your trafic is my trafic :)
    pcap_set_promisc(pcap, 1);

    // set timeout 10s
    pcap_set_timeout(pcap, 10000);

    // activate the capture
    if (pcap_activate(pcap) != 0) {
        fprintf(stderr, "pcap_activate failed: %s\n", pcap_geterr(pcap));
        pcap_close(pcap);
        return NULL;
    }
    return pcap;
}
