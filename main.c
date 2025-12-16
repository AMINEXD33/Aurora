#define _GNU_SOURCE
#include "./engine/core/init/init.h"
#include "./engine/core/jobs/jobs.h"
#include "./engine/helpers/helpers.h"
#include "./engine/core/config/config.h"
#include "./engine/core/clientserver/clientserver.h"
#include <stdio.h>    
#include <stdlib.h>    
#include <unistd.h>    
#include <sys/wait.h> 
#include <stdatomic.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <stdatomic.h>

// for parsing 
#include <netinet/if_ether.h>   // struct ether_header
#include <netinet/ip.h>         // struct ip, struct iphdr
#include <netinet/tcp.h>        // struct tcphdr
#include <netinet/udp.h>        // struct udphdr
#include <netinet/ip_icmp.h>    // struct icmphdr
#include <netinet/ip6.h>        // struct ip6_hdr (IPv6)
#include <arpa/inet.h>          // ntohs(), ntohl(), inet_ntoa()
#include <net/ethernet.h>       // ETHERTYPE_* constants
#include "./engine/core/capture/protocols/protoheaders.h"
void sigchld_handler(int signum) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (WIFSIGNALED(status)) {
            printf("[MASTER] Worker %d died due to signal %d (%s)\n",
                   pid, WTERMSIG(status), strsignal(WTERMSIG(status)));
        } else if (WIFEXITED(status)) {
            printf("[MASTER] Worker %d exited normally with code %d\n",
                   pid, WEXITSTATUS(status));
        }
    }
}

#define MAX_BATCH 1024
#define PACKET_SIZE 2048

typedef struct {
    sem_t batch_ready;     // signals workers
    sem_t batch_done;      // signals sniffer
    atomic_int workers_done;
    int count;
    size_t lengths[MAX_BATCH];
    u_char packets[MAX_BATCH][PACKET_SIZE];
} shared_batch_t;

shared_batch_t *shared_batch;




void packet_handler(u_char *user, const struct pcap_pkthdr *hdr, const u_char *packet) {
    shared_batch_t *batch = (shared_batch_t*)user;

    if (batch->count >= MAX_BATCH) return; // simple overflow protection

    memcpy(batch->packets[batch->count], packet, hdr->caplen);
    batch->lengths[batch->count] = hdr->caplen;
    batch->count++;
}

void sniffer(pcap_t *initiated_pcap, int workers_count){
    while (1) {
        shared_batch->count = 0;
        int res = pcap_dispatch(initiated_pcap, MAX_BATCH, packet_handler, (u_char*)shared_batch);
        
        if (res < 0) {
            fprintf(stderr, "[x] pcap error: %s\n", pcap_geterr(initiated_pcap));
            break;
        }
        
        if (res == 0) {
            // No packets captured, continue waiting
            continue;
        }

        printf("[SNIFFER] Captured %d packets\n", shared_batch->count);
        atomic_store(&shared_batch->workers_done, workers_count);
        // mark batch ready
        for (int i = 0; i < workers_count; i++) {
            sem_post(&shared_batch->batch_ready);
        }

        // wait until workers consume
        sem_wait(&shared_batch->batch_done);
    }
}

void worker(int id){

    char filename[64];
    snprintf(filename, sizeof(filename), "worker_%d.log", id);
    freopen(filename, "w", stdout);
    while (1) {
        sem_wait(&shared_batch->batch_ready);
        printf("[Worker %d] Processing %d packets\n", id, shared_batch->count);
        fflush(stdout);
        for (int i = 0; i < shared_batch->count; i++) {
            const u_char *pkt = shared_batch->packets[i];
            size_t len = shared_batch->lengths[i];
            
            // point to start of eth
            struct ether_header *eth = (struct ether_header *)pkt;
            struct ip *iph = NULL;
            switch (ntohs(eth->ether_type))
            {
                case ETHERTYPE_IP:
                    iph = (struct ip *)(pkt + ETH_HEADER_SIZE_PLAIN);
                    // ip dest and source
                    protocol_mapper(iph);
                    fflush(stdout);
                    break;
                case ETHERTYPE_VLAN:
                    // advance pointer to point at type
                    pkt += sizeof(struct ether_header);
                    uint16_t next_header = ntohs(eth->ether_type);
                    
                    while(next_header == ETHERTYPE_VLAN){
                        // get the tci , the struct is is just simplifying things
                        vlan_tci *vlantci = (vlan_tci *) pkt;
                        // get tci
                        uint16_t tci = ntohs(vlantci->tci);
                        // get vid
                        uint16_t vid = tci & 0X0FFF; 
                        
                        printf("vlan[%u] ", vid);
                        
                        // skip 2bytes of tci
                        pkt += sizeof(uint16_t);
                        // get the next header 2bytes
                        next_header = ntohs(*(uint16_t*)pkt);
                        // now point at the next 2 headers 
                        // it's either the next tci ot the start of 
                        // the payload if next header is not a vlan
                        pkt += sizeof(uint16_t);
                    }
                    // get ip packet
                    iph = (struct ip *)(pkt);
                    // pass it to the mapper
                    protocol_mapper(iph);
                    // flush
                    fflush(stdout);
                    break;

                case ETHERTYPE_LOOPBACK:
                    break;
                    
                default:
                    break;
            }
            // check if this is an ipv4 eth packer
            if (ntohs(eth->ether_type) == ETHERTYPE_IP) {

            }
            // check if this is an ipv6 tagged packet
            // well not now 
        }

        // signal done
        
        if (atomic_fetch_sub(&shared_batch->workers_done, 1) == 1) {
            sem_post(&shared_batch->batch_done);
        }
    }
}

int main (int argc, char **argv){
    signal(SIGCHLD, sigchld_handler);
    cJSON *core_config =  INIT_CORE_CONFIG();
    int thread_count = GET_THREAD_COUNT(core_config);
    int core_count = GET_CORE_COUNT(core_config);

    // just create an annonymous shared mempry (THIS is temp , 
    // i think i will switch with zero copy from the ring directly , 
    // or double batching but for now this will do)
    shared_batch = mmap(NULL, sizeof(shared_batch_t), 
                        PROT_READ | PROT_WRITE, 
                        MAP_SHARED | MAP_ANONYMOUS,  // ← no file descriptor needed
                        -1, 0);

    // init semaphore for batch ready
    sem_init(&shared_batch->batch_ready, 1, 0);
    // init semaphore for batch done
    sem_init(&shared_batch->batch_done, 1, 0);
    // init atomic counter for workers to track if they are done
    atomic_init(&shared_batch->workers_done, 0);


    // print some config info
    printf("---------LOADED CONFIG-----------\n");
    printf("[@] thread count = %d\n", thread_count);
    printf("[@] core count = %d\n", core_count);
    printf("---------------------------------\n");


    // initiat the pcap capturing , for now  ill keep this wlan0 hardcoded here
    pcap_t *initiated_pcap = INIT_PCAP("wlan0");
    if (!initiated_pcap){
        printf("[x] can't initiat pcap\n");
        return -1;
    }
    // keep track of the workers pids
    pid_t *pids = calloc(1 ,sizeof(pid_t) * core_count);
    // track how many fork
    int forked_count = 0;
    
    // fork sniffer
    pid_t sniffer_pid = fork();
    if (sniffer_pid == 0) {
        sniffer(initiated_pcap, core_count);
        exit(0);
    } else if (sniffer_pid > 0) {
        // if parent , fork workers
        for (int i = 0; i < core_count; i++) {
            pid_t p = fork();
            if (p == 0) {
                worker(i);
                exit(0);
            }
            pids[i] = p;
            forked_count++;
        }
    }
    // cactch if a child didn't even start
    if (forked_count != core_count){
        printf("[!] can't start all workers \n");
        // do something here
    }

    // wait for children
    int remaining = core_count;
    while (remaining > 0) {
        sleep(1); // can also do other work
        for (int i = 0; i < core_count; i++) {
            if (pids[i] == 0) continue; // already handled
            if (waitpid(pids[i], NULL, WNOHANG) > 0) {
                pids[i] = 0;
                remaining--;
            }
        }
    }
    return 1;
}