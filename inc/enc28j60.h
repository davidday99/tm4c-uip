#ifndef _ENC28J60_H_
#define _ENC28J60_H_

#include <stdint.h>

struct ENC28J60 {
    uint32_t sysctl_peripherals[4];
    uint32_t ssi_base;
    uint32_t ssi_gpio_base;
    uint32_t tx_pin;
    uint32_t tx_pin_mode;
    uint32_t rx_pin; 
    uint32_t rx_pin_mode;
    uint32_t clk_pin;
    uint32_t clk_pin_mode;
    uint32_t cs_pin_base;
    uint32_t cs_pin;
    uint32_t intr_pin_base;
    uint32_t intr_pin;
    uint8_t *rx_buf;
    uint8_t *tx_buf;
    uint16_t _nf_ptr;
};

extern struct ENC28J60 ENC28J60;

uint8_t ENC28J60_init(struct ENC28J60 *enc28j60);
uint8_t ENC28J60_enable_receive(struct ENC28J60 *enc28j60);
uint8_t ENC28J60_disable_receive(struct ENC28J60 *enc28j60);
uint16_t ENC28J60_read_frame_blocking(struct ENC28J60 *enc28j60, uint8_t *data);
void ENC28J60_write_frame_blocking(struct ENC28J60 *enc28j60, uint8_t *data, uint16_t size);
uint16_t ENC28J60_read_frame_dma(struct ENC28J60 *enc28j60);
void ENC28J60_enable_dma(struct ENC28J60 *enc28j60);
void ENC28J60_disable_dma(struct ENC28J60 *enc28j60);
void ENC28J60_get_tx_status_vec(struct ENC28J60 *enc28j60, uint8_t *tsv);
uint8_t ENC28J60_get_packet_count(struct ENC28J60 *enc28j60);
void ENC28J60_disable_interrupts(struct ENC28J60 *enc28j60);
void ENC28J60_enable_interrupts(struct ENC28J60 *enc28j60);
uint8_t ENC28J60_get_interrupt_requests(struct ENC28J60 *enc28j60);
void ENC28J60_advance_rdptr(struct ENC28J60 *enc28j60);
void ENC28J60_decrement_packet_count(struct ENC28J60 *enc28j60);
void ENC28J60_get_mac_address(struct ENC28J60 *enc28j60, uint8_t *buf);

#endif /* _ENC28J60_H_ */
