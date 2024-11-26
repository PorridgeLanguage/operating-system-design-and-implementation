#include "ulib.h"

char *ips[10] = {
    "192.168.1.1",
    "192.168.2.1",
    "192.168.3.1",
    "192.168.4.1",
    "192.168.5.1",
    "192.168.6.1",
    "192.168.7.1",
    "192.168.8.1",
    "192.168.9.1",
    "192.168.10.1",
};

int main(void) {
  for(int i = 1; i < 10; i++){
    int MAC_SIZE = 18;
    char* ip = ips[i];
    char* mac = malloc(MAC_SIZE);
    if(arp_create("mynet0", ip, mac, MAC_SIZE) < 0) {
      fprintf(1, "ARP for IP:%s Failed.\n", ip);
      exit(-1);
    }
    char macstr[30];
    memset(macstr, 0, 30);
    sprintf(macstr, "%x:%x:%x:%x:%x:%x", (uint8_t)mac[0], (uint8_t)mac[1], (uint8_t)mac[2],
    (uint8_t)mac[3], (uint8_t)mac[4], (uint8_t)mac[5]);
    fprintf(1, "IP %s has mac %s\n", ip, macstr);
  }
  
  exit(0);
}