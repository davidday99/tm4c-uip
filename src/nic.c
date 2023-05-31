#include <stdint.h>
#include "nic.h"
#include "enc28j60.h"
#include "uip_arp.h"

#define BUF ((struct uip_eth_hdr *)&uip_buf[0])
#define ETH_SENDER_MAC_ADDR_OFFSET 6
#define ARP_SENDER_HW_ADDR_OFFSET 22

static uint8_t mac[6];

static struct ENC28J60 *pENC = &ENC28J60;

int nic_init(void) {
    if (!ENC28J60_init(pENC))
        return 0;
    ENC28J60_get_mac_address(pENC, mac);
    ENC28J60_enable_receive(pENC);
    return 0;
}

int nic_read(uint8_t *buf) {
    int size = 0;
    if (ENC28J60_get_packet_count(pENC) > 0) {
        size = ENC28J60_read_frame_blocking(pENC, buf);
        ENC28J60_decrement_packet_count(pENC);
    }
    return size;
}

void nic_write(uint8_t *buf, int size) {
    /* uIP expects the NIC to insert the MAC, so we
     * always insert it in the sender field of the ethernet frame,
     * and if the buffer contains an ARP packet we insert it 
     * into the sender HW address field too.
     */
    memcpy(buf + ETH_SENDER_MAC_ADDR_OFFSET, mac, sizeof(mac));
    if (BUF->type == htons(UIP_ETHTYPE_ARP))
        memcpy(buf + ARP_SENDER_HW_ADDR_OFFSET, mac, sizeof(mac));
    ENC28J60_write_frame_blocking(pENC, buf, size);
}

