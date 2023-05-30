#include "enc28j60.h"
#include "uip.h"
#include "uip_arp.h"
#include "timer.h"

#define BUF ((struct uip_eth_hdr *)&uip_buf[0])

void uip_log(char *m)
{
  printf("uIP log message: %s\n", m);
}

int main() {

    uint8_t mac[6];
    struct timer periodic_timer, arp_timer;

    int status = ENC28J60_init(&ENC28J60);
    if (!status)
        goto error;

    ENC28J60_get_mac_address(&ENC28J60, mac);
    ENC28J60_enable_receive(&ENC28J60);

    /*ENC28J60_write_frame_blocking(&ENC28J60, txbuf, sizeof(txbuf));*/
    /*int count = ENC28J60_get_packet_count(&ENC28J60);*/
    /*ENC28J60_write_frame_blocking(&ENC28J60, txbuf, sizeof(txbuf));*/
    /*count = ENC28J60_get_packet_count(&ENC28J60);*/
    /*int size = ENC28J60_read_frame_blocking(&ENC28J60, rxbuf);*/

    int i;
    uip_ipaddr_t ipaddr;
    
    uip_init();

    uip_ipaddr(ipaddr, 192,168,1,150);
    uip_sethostaddr(ipaddr);
    uip_ipaddr(ipaddr, 192,168,1,254);
    uip_setdraddr(ipaddr);
    uip_ipaddr(ipaddr, 255,255,255,0);
    uip_setnetmask(ipaddr);

    hello_world_init();

    while(1) {
        if (ENC28J60_get_packet_count(&ENC28J60) == 0) {
            continue;
        } 
        uip_len = ENC28J60_read_frame_blocking(&ENC28J60, uip_buf);
        ENC28J60_decrement_packet_count(&ENC28J60);
        if(uip_len > 0) {
            if(BUF->type == htons(UIP_ETHTYPE_IP)) {
                uip_arp_ipin();
                uip_input();
                /* If the above function invocation resulted in data that
                should be sent out on the network, the global variable
                uip_len is set to a value > 0. */
                if(uip_len > 0) {
                    uip_arp_out();
                    memcpy(uip_buf + 6, mac, 6);
                    ENC28J60_write_frame_blocking(&ENC28J60, uip_buf, uip_len);
                }
            } else if(BUF->type == htons(UIP_ETHTYPE_ARP)) {
                uip_arp_arpin();
                /* If the above function invocation resulted in data that
                should be sent out on the network, the global variable
                uip_len is set to a value > 0. */
                if(uip_len > 0) {
                    memcpy(uip_buf + 6, mac, 6);
                    memcpy(uip_buf + 22, mac, 6);
                    ENC28J60_write_frame_blocking(&ENC28J60, uip_buf, uip_len);
                }
            }

        } else if(timer_expired(&periodic_timer)) {
            timer_reset(&periodic_timer);
            for(i = 0; i < UIP_CONNS; i++) {
                uip_periodic(i);
                /* If the above function invocation resulted in data that
                should be sent out on the network, the global variable
                uip_len is set to a value > 0. */
                if(uip_len > 0) {
                    uip_arp_out();
                    memcpy(uip_buf + 6, mac, 6);
                    ENC28J60_write_frame_blocking(&ENC28J60, uip_buf, uip_len);
                }
            }

#if UIP_UDP
            for(i = 0; i < UIP_UDP_CONNS; i++) {
                uip_udp_periodic(i);
                /* If the above function invocation resulted in data that
                should be sent out on the network, the global variable
                uip_len is set to a value > 0. */
                if(uip_len > 0) {
                    uip_arp_out();
                    ENC28J60_write_frame_blocking(&ENC28J60, uip_buf, uip_len);
                }
            }
#endif /* UIP_UDP */

        /* Call the ARP timer function every 10 seconds. */
            if(timer_expired(&arp_timer)) {
                timer_reset(&arp_timer);
                uip_arp_timer();
            }
        }
    }

error:
    while (1)
        ;
}

