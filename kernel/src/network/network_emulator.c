#include "network/arp.h"
#include "network/arp_frame.h"
#include "network/e1000.h"

extern uint32_t* e1000;
uint32_t emulator_e1000_tdh;
uint32_t emulator_e1000_tdt;
uint32_t emulator_e1000_rdh;
uint32_t emulator_e1000_rdt;
uint32_t emulator_e1000_txlen;
uint32_t emulator_e1000_rxlen;

extern struct e1000_tx_desc tx_desc_buf[TXRING_LEN] __attribute((aligned(PGSIZE)));
extern struct e1000_data tx_data_buf[TXRING_LEN] __attribute((aligned(PGSIZE)));
//
extern struct e1000_rx_desc rx_desc_buf[RXRING_LEN] __attribute((aligned(PGSIZE)));
extern struct e1000_data rx_data_buf[RXRING_LEN] __attribute((aligned(PGSIZE)));

uint16_t htons(uint16_t v);

static char *ips[11] = {
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

static uint8_t macs[10][6] = {
    {0x56, 0x34, 0x12, 0x00, 0x54, 0x52},
    {0x1A, 0x2B, 0x3C, 0x4D, 0x5E, 0x6F},
    {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF},
    {0x78, 0x56, 0x34, 0x12, 0xAB, 0xCD},
    {0x00, 0x1A, 0xB2, 0x3C, 0x4D, 0x5E},
    {0x9E, 0x7F, 0xBC, 0x21, 0x34, 0x43},
    {0x02, 0x00, 0x5E, 0x1A, 0x2B, 0x3C},
    {0x8C, 0x8D, 0xAC, 0x11, 0x22, 0x33},
    {0x5E, 0x00, 0x11, 0x22, 0x33, 0x44},
    {0x6B, 0x3A, 0x2D, 0x1E, 0x4F, 0x10},
};

static void sip2str(uint32_t sip, char *ipstr) __attribute__((unused));
static void sip2str(uint32_t sip, char *ipstr) {
    // Convert the source IP address from uint32_t to a string
    sprintf(ipstr, "%d.%d.%d.%d",
        sip & 0xFF,              // First byte
        (sip >> 8) & 0xFF,       // Second byte
        (sip >> 16) & 0xFF,      // Third byte
        (sip >> 24) & 0xFF);     // Fourth byte
}

static void dip2str(uint32_t dip, char *ipstr) __attribute__((unused));
static void dip2str(uint32_t dip , char *ipstr) {
    // Combine dip and dip_2 to represent a 4-byte IP address and convert it to a string
    sprintf(ipstr, "%d.%d.%d.%d",
            dip & 0xFF, (dip >> 8) & 0xFF,
            (dip >> 16) & 0xFF, (dip >> 24) & 0xFF);
}

static int ip2mac(char *ip) __attribute__((unused));
static int ip2mac(char *ip) {
    // Iterate through the global `ips` array to find a match
    for (int i = 0; i < 10; i++) {
        if (strcmp(ip, ips[i]) == 0) {
            return i; // Return the index if a match is found
        }
    }
    return -1; // Return -1 if no match is found
}

static uint16_t ntohs(uint16_t netshort) {
    return ((netshort & 0xFF00) >> 8) | ((netshort & 0x00FF) << 8);
}

static void parse_ethr_hdr(struct ethr_hdr *ethr_ptr) __attribute__((unused));
void parse_ethr_hdr(struct ethr_hdr *ethr_ptr){
    printf("--- ethrnet Frame ---\n");
    printf("Destination MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
        ethr_ptr->dmac[0], ethr_ptr->dmac[1], ethr_ptr->dmac[2],
        ethr_ptr->dmac[3], ethr_ptr->dmac[4], ethr_ptr->dmac[5]);

    printf("Source MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
        ethr_ptr->smac[0], ethr_ptr->smac[1], ethr_ptr->smac[2],
        ethr_ptr->smac[3], ethr_ptr->smac[4], ethr_ptr->smac[5]);

    printf("ethrType: 0x%04x\n", ntohs(ethr_ptr->ethr_type));

    // Check if the ethrType indicates an ARP packet
    if (ntohs(ethr_ptr->ethr_type) == 0x0806) {
        printf("--- ARP Packet ---\n");
        printf("Hardware Type: 0x%04x\n", ntohs(ethr_ptr->hwtype));
        printf("Protocol Type: 0x%04x\n", ntohs(ethr_ptr->protype));
        printf("Hardware Size: %d\n", ethr_ptr->hwsize);
        printf("Protocol Size: %d\n", ethr_ptr->prosize);
        printf("Opcode: %d\n", ntohs(ethr_ptr->opcode));

        printf("ARP Source MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
            ethr_ptr->arp_smac[0], ethr_ptr->arp_smac[1], ethr_ptr->arp_smac[2],
            ethr_ptr->arp_smac[3], ethr_ptr->arp_smac[4], ethr_ptr->arp_smac[5]);
        printf("ARP Source IP: %d.%d.%d.%d\n",
            ethr_ptr->sip & 0xFF, (ethr_ptr->sip >> 8) & 0xFF,
            (ethr_ptr->sip >> 16) & 0xFF, (ethr_ptr->sip >> 24) & 0xFF);

        printf("ARP Destination MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
            ethr_ptr->arp_dmac[0], ethr_ptr->arp_dmac[1], ethr_ptr->arp_dmac[2],
            ethr_ptr->arp_dmac[3], ethr_ptr->arp_dmac[4], ethr_ptr->arp_dmac[5]);

        printf("ARP Destination IP: %d.%d.%d.%d\n",
            ethr_ptr->dip & 0xFF, (ethr_ptr->dip >> 8) & 0xFF,
            (ethr_ptr->dip >> 16) & 0xFF, (ethr_ptr->dip >> 24) & 0xFF);
            
    } else {
        printf("Unsupported ethrType\n");
    }
}

int check_arp(struct ethr_hdr *ethr_ptr){
    if(ntohs(ethr_ptr->ethr_type) != 0x0806) goto bad;
    if(ntohs(ethr_ptr->hwtype) != 0x001) goto bad;
    if(ntohs(ethr_ptr->protype) != 0x0800) goto bad;
    if(ethr_ptr->hwsize != 6) goto bad;
    if(ethr_ptr->protype != 4) goto bad;
    if(ethr_ptr->opcode != 1) goto bad;
    return 1;
bad:
    return -1;
}

void e1000_emulator_handle(){
    // This network emulator simulate the transmit and receive functionality.

    // check whethr there are messages to transmit.
    emulator_e1000_tdt = e1000[E1000_TDT];
    int index = emulator_e1000_tdh;
    struct ethr_hdr *ethr_ptr = NULL;
    while(index != emulator_e1000_tdt){
        if(!(tx_desc_buf[emulator_e1000_tdh].cmd & E1000_TXD_CMD_RS) ||
        !(tx_desc_buf[emulator_e1000_tdh].cmd & E1000_TXD_CMD_EOP)) return;
        ethr_ptr = (struct ethr_hdr *)&tx_data_buf[emulator_e1000_tdh];
        assert(ethr_ptr);
        
        if(!check_arp(ethr_ptr)){
            printf("Get the wrong type of ARP packet in ethernet phrame.\n");
            return;
        }
        tx_desc_buf[emulator_e1000_tdt].status |= E1000_TXD_STAT_DD;
        index = (index + 1) % emulator_e1000_txlen;

        // answer by handling emulator_e1000_rdh
        emulator_e1000_rdt = e1000[E1000_RDT];

        if(emulator_e1000_rdh == emulator_e1000_rdt){
            printf("RXD buffer is full!\n");
            return; // The receive buffer is empty.
        }

        // 填充TODO部分以构造ARP应答报文并包装在以太网帧中
        char dipstr[30];
        memset(dipstr, 0, 30);
        dip2str(ethr_ptr->dip, dipstr);
        int macindex = ip2mac(dipstr);
        if(macindex == -1){
            printf("The destination IP %s doesn't exist.\n", dipstr);
            return;
        }

        // 构造ARP应答报文
        char response_buf[sizeof(struct ethr_hdr)];
        struct ethr_hdr *response_hdr = (struct ethr_hdr *)response_buf;

        // 设置以太网头部
        memcpy(response_hdr->dmac, ethr_ptr->smac, 6); // 设置目的MAC为请求者的MAC
        memcpy(response_hdr->smac, macs[macindex], 6);          // 设置源MAC为本机的MAC
        response_hdr->ethr_type = htons(0x0806);        // ethrType为ARP

        // 设置ARP报文头部
        response_hdr->hwtype = htons(1);                // 硬件类型为以太网（1）
        response_hdr->protype = htons(0x0800);          // 协议类型为IPv4（0x0800）
        response_hdr->hwsize = 6;                       // 硬件地址长度为6（MAC地址长度）
        response_hdr->prosize = 4;                      // 协议地址长度为4（IPv4地址长度）
        response_hdr->opcode = htons(2);                // 操作码为2（ARP应答）

        memcpy(response_hdr->arp_smac, macs[macindex], 6);      // 源MAC地址为本机MAC
        response_hdr->sip = ethr_ptr->dip;             // 源IP地址为请求中的目的IP

        memcpy(response_hdr->arp_dmac, ethr_ptr->arp_smac, 6); // 目的MAC地址为请求中的源MAC
        response_hdr->dip = ethr_ptr->sip;                      // 目的IP地址为请求中的源IP

        // 拷贝应答ARP报文到接收缓冲区
        memcpy(&rx_data_buf[emulator_e1000_rdh], response_buf, sizeof(struct ethr_hdr));
        rx_desc_buf[emulator_e1000_rdh].length = sizeof(struct ethr_hdr);
        rx_desc_buf[emulator_e1000_rdh].status |= E1000_RXD_STAT_DD; // 更新描述符状态为已完成接收
        emulator_e1000_rdh = (emulator_e1000_rdh + 1) % RXRING_LEN;   // 更新RDH指针
    }
    emulator_e1000_tdh = emulator_e1000_tdt;
    
}