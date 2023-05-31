#include <stdint.h>
#include <stdbool.h>
#include "uip.h"
#include "uip_arp.h"
#include "uip-fw.h"
#include "timer.h"
#include "nic.h"

#define BUF ((struct uip_eth_hdr *)&uip_buf[0])

void uip_log(char *m) {
    return;
}

int main(void){
    struct timer periodic_timer, arp_timer;


    int i;
    uip_ipaddr_t ipaddr;
    
    nic_init();
    uip_init();
    
    /* modify this per your LAN configuration */
    uip_ipaddr(ipaddr, 192,168,1,150);  
    uip_sethostaddr(ipaddr);
    uip_ipaddr(ipaddr, 192,168,1,254);
    uip_setdraddr(ipaddr);
    uip_ipaddr(ipaddr, 255,255,255,0);
    uip_setnetmask(ipaddr);

    hello_world_init();

    while(1) {
        uip_len = nic_read(uip_buf);
        if(uip_len > 0) {
            if(BUF->type == htons(UIP_ETHTYPE_IP)) {
                uip_arp_ipin();
                uip_input();
                /* If the above function invocation resulted in data that
                should be sent out on the network, the global variable
                uip_len is set to a value > 0. */
                if(uip_len > 0) {
                    uip_arp_out();
                    nic_write(uip_buf, uip_len);
                }
            } else if(BUF->type == htons(UIP_ETHTYPE_ARP)) {
                uip_arp_arpin();
                /* If the above function invocation resulted in data that
                should be sent out on the network, the global variable
                uip_len is set to a value > 0. */
                if(uip_len > 0) {
                    nic_write(uip_buf, uip_len);
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
                    nic_write(uip_buf, uip_len);
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
                    nic_write(uip_buf, uip_len);
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
}

