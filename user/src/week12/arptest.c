#include "ulib.h"

int main(void) {
  int MAC_SIZE = 18;
  char* ip = "192.168.2.1";
  char* mac = malloc(MAC_SIZE);
  if(arp_create("mynet0", ip, mac, MAC_SIZE) < 0) {
    fprintf(1, "ARP for IP:%s Failed.\n", ip);
  }
  char macstr[30];
  memset(macstr, 0, 30);
  sprintf(macstr, "%x:%x:%x:%x:%x:%x", (uint8_t)mac[0], (uint8_t)mac[1], (uint8_t)mac[2],
  (uint8_t)mac[3], (uint8_t)mac[4], (uint8_t)mac[5]);
  fprintf(1, "IP %s has mac %s\n", ip, macstr);
  exit(0);
}