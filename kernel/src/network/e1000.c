#include "network/e1000.h"
#include "network/nic.h"
#include "vme.h"

uint32_t* e1000;
struct e1000_tx_desc tx_desc_buf[TXRING_LEN] __attribute((aligned(PGSIZE)));
struct e1000_data tx_data_buf[TXRING_LEN] __attribute((aligned(PGSIZE)));
//
struct e1000_rx_desc rx_desc_buf[RXRING_LEN] __attribute((aligned(PGSIZE)));
struct e1000_data rx_data_buf[RXRING_LEN] __attribute((aligned(PGSIZE)));

extern uint32_t emulator_e1000_tdh;
extern uint32_t emulator_e1000_tdt;
extern uint32_t emulator_e1000_rdh;
extern uint32_t emulator_e1000_rdt;
extern uint32_t emulator_e1000_txlen;
extern uint32_t emulator_e1000_rxlen;

static void
emulator_init() {
  emulator_e1000_tdh = e1000[E1000_TDH];
  emulator_e1000_tdt = e1000[E1000_TDT];
  emulator_e1000_rdh = e1000[E1000_RDH];
  emulator_e1000_rdt = e1000[E1000_RDT];
  emulator_e1000_txlen = TXRING_LEN;
  emulator_e1000_rxlen = RXRING_LEN;
}

/**
 * @brief Function that does the initial descriptions for the queue and registers.
 * This method of type void initilizes the sending and recieveing buffers.
 * There are two for loops which set the addresses of each spot in the buffer.
 * init_desc() cycles through data up to TXRING_LEN (64) & adds the
 * appropriate buffer address containing the physical addr.
 * @param none
 * @see e1000_init()
 */
static void
init_desc() {
  int i;
  for (i = 0; i < TXRING_LEN; ++i) {
    // tx_desc_buf[i].buffer_addr = V2P(&tx_data_buf[i]);
    tx_desc_buf[i].buffer_addr = (uint32_t)&tx_data_buf[i];
    tx_desc_buf[i].status = E1000_TXD_STAT_DD;
  }
  //
  for (i = 0; i < RXRING_LEN; ++i) {
    // rx_desc_buf[i].buffer_addr = V2P(&rx_data_buf[i]);
    rx_desc_buf[i].buffer_addr = (int)&rx_data_buf[i];
  }
}

/**
 * @brief helper method to initlize hardware/registers
 * This method initlizes the E1000 card with specific buffer values.
 * @param none
 * @return none
 */
static void
e1000_init() {
  // assert(e1000[E1000_STATUS] == 0x80080783);
  // e1000[E1000_TDBAL] = V2P(tx_desc_buf);
  e1000[E1000_TDBAL] = (int)tx_desc_buf;
  e1000[E1000_TDBAH] = 0x0;
  e1000[E1000_TDH] = 0x0;
  e1000[E1000_TDT] = 0x0;
  e1000[E1000_TDLEN] = TXRING_LEN * sizeof(struct e1000_tx_desc);
  e1000[E1000_TCTL] = VALUEATMASK(1, E1000_TCTL_EN) |
                      VALUEATMASK(1, E1000_TCTL_PSP) |
                      VALUEATMASK(0x10, E1000_TCTL_CT) |
                      VALUEATMASK(0x40, E1000_TCTL_COLD);
  e1000[E1000_TIPG] = VALUEATMASK(10, E1000_TIPG_IPGT) |
                      VALUEATMASK(8, E1000_TIPG_IPGR1) |
                      VALUEATMASK(6, E1000_TIPG_IPGR2);
  e1000[E1000_RAL] = 0x12005452;
  e1000[E1000_RAH] = 0x00005634 | E1000_RAH_AV;
  // e1000[E1000_RDBAL] = V2P(rx_desc_buf);
  e1000[E1000_RDBAL] = (int)rx_desc_buf;
  e1000[E1000_RDBAH] = 0x0;
  e1000[E1000_RDLEN] = RXRING_LEN * sizeof(struct e1000_rx_desc);
  e1000[E1000_RDH] = 0x0;
  e1000[E1000_RDT] = RXRING_LEN;
  e1000[E1000_RCTL] = E1000_RCTL_EN |
                      !E1000_RCTL_LPE |
                      E1000_RCTL_LBM_NO |
                      E1000_RCTL_RDMTS_HALF |
                      E1000_RCTL_MO_0 |
                      E1000_RCTL_BAM |
                      E1000_RCTL_BSEX |
                      E1000_RCTL_SZ_4096 |
                      E1000_RCTL_SECRC;
}

/**
 * @brief This method attaches the network card's function to the kernel.
 * enables the pci_func, initializes its descriptors &
 * maps memory for its initialization
 *
 * @return 0 on successfull completion
 */
int e1000_attach(struct pci_func* pcif) {
  pci_func_enable(pcif);
  //
  init_desc();
  //
  e1000 = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
  printf("e1000: bar0: %x; size0: %x\n", pcif->reg_base[0], pcif->reg_size[0]);
  printf("e1000: status %x\n", ((uint32_t*)e1000)[STATUS / 4]);

  // 获取 MAC 地址
  uint32_t mac_lower = e1000[E1000_RAL];                  // 低 4 字节
  uint32_t mac_upper = e1000[E1000_RAH] & ~E1000_RAH_AV;  // 高 2 字节，去掉 E1000_RAH_AV 标志

  // 将 MAC 地址从两个 32 位寄存器拼接到 nic_devices[0].mac_addr
  nic_devices[0].mac_addr[0] = (mac_lower) & 0xFF;
  nic_devices[0].mac_addr[1] = (mac_lower >> 8) & 0xFF;
  nic_devices[0].mac_addr[2] = (mac_lower >> 16) & 0xFF;
  nic_devices[0].mac_addr[3] = (mac_lower >> 24) & 0xFF;
  nic_devices[0].mac_addr[4] = (mac_upper) & 0xFF;
  nic_devices[0].mac_addr[5] = (mac_upper >> 8) & 0xFF;

  printf("MAC Address: %02x:%02x:%02x:%02x:%02x:%02x\n",
         nic_devices[0].mac_addr[0], nic_devices[0].mac_addr[1],
         nic_devices[0].mac_addr[2], nic_devices[0].mac_addr[3],
         nic_devices[0].mac_addr[4], nic_devices[0].mac_addr[5]);

  e1000_init();
  emulator_init();

  return 0;
}

int e1000_transmit(const char* buf, unsigned int len) {
  // TODO: WEEK12-network, handle transmitting net message
  TODO();

  return 0;  // Return success
}

int e1000_receive(char* buf, unsigned int len) {
  // TODO: WEEK12-network, handle receiving net message
  TODO();
  return len;  // Return the length of the received data
}
