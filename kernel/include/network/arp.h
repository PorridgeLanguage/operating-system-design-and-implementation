#ifndef __ARP_H__
#define __ARP_H__

#include "fs.h"
#include "network/arp_frame.h"
#include "network/e1000.h"
#include "network/nic.h"

int send_arpRequest(char *interface, char *ipAddr, char *arpResp);
int recv_arpRequest(char *interface);

#endif