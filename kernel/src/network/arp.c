/**
 *author: Anmol Vatsa<anvatsa@cs.utah.edu>
 *
 *kernel code to send recv arp request responses
 */

#include "network/arp.h"

extern void sys_sleep(int ticks);

static int block_until_arp_reply(struct ethr_hdr *arpReply, const char *myIpAddr, bool expected_reply) {
    while (1) {
        // Sleep for a short duration to simulate waiting for a network interrupt
        sys_sleep(100);

        // Check if an ARP reply has been received
        int status = e1000_receive((char *)arpReply, sizeof(struct ethr_hdr) - 2);
        if (status < 0) {
            // Log error if the receive call fails
            // printf("Error: failed to receive ARP packet\n");
            return -1;
        } else if (status >= 0) {
            return 0;
        }

        // If no valid ARP reply is received, continue waiting
    }
    // This point will not be reached, but is included for completeness
    return -1;
}



int send_arp(char* interface, const char *myIpAddr, uint8_t *dst_mac, char* ipAddr, bool is_reply, uint8_t *macMsg) {
  // printf("Create arp request for ip:%s over Interface:%s\n", ipAddr, interface);

  struct nic_device *nd;
  if(get_device(interface, &nd) < 0) {
    printf("ERROR:send_arpRequest:Device not loaded\n");
    return -1;
  }

  struct ethr_hdr eth;
  create_eth_arp_frame(nd->mac_addr, myIpAddr, is_reply ? dst_mac : NULL, ipAddr, is_reply, macMsg, &eth);
  e1000_transmit((char *)&eth, sizeof(eth)-2);

  return 0;
}


int recv_arp(const char *myIpAddr, bool expected_reply, uint8_t *received_from_mac, char *received_from_ip, char *arpMsgMac) {
    // printf("Wait for arp over Interface:%s\n", interface);
    struct ethr_hdr eth;

    while (block_until_arp_reply(&eth, myIpAddr, expected_reply) != 0);
    
    if (parse_arp_packet(&eth, myIpAddr, expected_reply, received_from_mac, received_from_ip, (uint8_t *)arpMsgMac) < 0) {
      return -1; // parse_arp_packet fails
    }
  
    if (arpMsgMac && received_from_mac) {
      unpack_mac(received_from_mac, arpMsgMac);
      arpMsgMac[17] = '\0';
    }

    return 0;
}

int send_arpRequest(char *interface, char *ipAddr, char *arpResp) {
  const char *sender_ip = "192.168.1.1";
  uint8_t emptymac[6];
  
  assert(ipAddr);
  // printf("Sending ARP request\n");
  send_arp(interface, sender_ip, NULL, ipAddr, false, emptymac);

  // printf("Sent ARP request\n");

  //struct ethr_hdr arpResponse;
  
  if(recv_arp(sender_ip, true, NULL, NULL, arpResp) < 0) {
    printf("ERROR:send_arpRequest:Failed to recv ARP response over the NIC\n");
    return -3;
  }
  return 0;
}

int recv_arpRequest(char *interface) {
  const char *receiver_ip = "192.168.2.1";

  struct nic_device *nd;
  if(get_device(interface, &nd) < 0) {
    printf("ERROR:recv_arp:Device not loaded\n");
    return -1;
  }

  // printf("Receiving ARP request\n");

  uint8_t received_from_mac[6];
  char received_from_ip[16];

  if(recv_arp(receiver_ip, false, received_from_mac, received_from_ip, NULL) < 0) {
    printf("ERROR:recv_arpRequest:Failed to recv ARP request over the NIC\n");
    return -3;
  }

  send_arp(interface, receiver_ip, received_from_mac, received_from_ip, true, nd->mac_addr);
  return 0;
}
