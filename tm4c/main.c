#include "enc28j60.h"

void uip_log(char *m)
{
  printf("uIP log message: %s\n", m);
}

uint8_t rxbuf[1000];
uint8_t txbuf[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x08, 0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x04,
    0x05,
    0x06
};

int main() {

    uint8_t mac[6];

    int status = ENC28J60_init(&ENC28J60);
    if (!status)
        goto error;

    ENC28J60_get_mac_address(&ENC28J60, mac);
    ENC28J60_enable_receive(&ENC28J60);

    ENC28J60_write_frame_blocking(&ENC28J60, txbuf, sizeof(txbuf));
    int count = ENC28J60_get_packet_count(&ENC28J60);
    ENC28J60_write_frame_blocking(&ENC28J60, txbuf, sizeof(txbuf));
    count = ENC28J60_get_packet_count(&ENC28J60);
    int size = ENC28J60_read_frame_blocking(&ENC28J60, rxbuf);

error:
    while (1)
        ;


	return 0;
}

