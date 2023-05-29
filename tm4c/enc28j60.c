#include <stdint.h>
#include <stdbool.h>
#include "enc28j60.h"
#include "driverlib/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/ssi.h"
#include "driverlib/sysctl.h"

#define PART_TM4C123GH6PM
#include "driverlib/pin_map.h"

#define RCR_OPCODE 0
#define RBM_OPCODE 0x20
#define WCR_OPCODE 0x40
#define WBM_OPCODE 0x60
#define BFS_OPCODE 0x80
#define BFC_OPCODE 0xA0
#define SRC_OPCODE 0xE0
#define NOP 0xF0  // any opcode not mapped to an instruction

#define RBM_ARG0 0x1A
#define WBM_ARG0 0x1A
#define SRC_ARG0 0x1F

#define ERDPTL 0
#define ERDPTH 0x01
#define EWRPTL 0x02
#define EWRPTH 0x03
#define ETXSTL 0x04
#define ETXSTH 0x05
#define ETXNDL 0x06
#define ETXNDH 0x07
#define ERXSTL 0x08
#define ERXSTH 0x09
#define ERXNDL 0xA
#define ERXNDH 0xB
#define ERXRDPTL 0xC
#define ERXRDPTH 0xD
#define ERXWRPTL 0xE
#define ERXWRPTH 0xF
#define EDMASTL 0x10
#define EDMASTH 0x11
#define EDMANDL 0x12
#define EDMANDH 0x13
#define EDMADSTL 0x14
#define EDMADSTH 0x15
#define EDMACSL 0x16
#define EDMACSH 0x17
#define EIE 0x1B
#define EIR 0x1C
#define ESTAT 0x1D
#define ECON2 0x1E
#define ECON1 0x1F

#define EHT0 0
#define EHT1 0x01
#define EHT2 0x02
#define EHT3 0x03
#define EHT4 0x04
#define EHT5 0x05
#define EHT6 0x06
#define EHT7 0x07
#define EPMM0 0x08
#define EPMM1 0x09
#define EPMM2 0xA
#define EPMM3 0xB
#define EPMM4 0xC
#define EPMM5 0xD
#define EPMM6 0xE
#define EPMM7 0xF
#define EPMMCSL 0x10
#define EPMMCSH 0x11
#define EPMOL 0x14
#define EPMOH 0x15
#define ERXFCON 0x18
#define EPKTCNT 0x19

#define MACON1 0
#define MACON3 0x02
#define MACON4 0x03
#define MABBIPG 0x04
#define MAIPGL 0x06
#define MAIPGH 0x07
#define MACLCON1 0x08
#define MACLCON2 0x09
#define MAMXFLL 0xA
#define MAMXFLH 0xB
#define MICMD 0x12
#define MIREGADR 0x14
#define MIWRL 0x16
#define MIWRH 0x17
#define MIRDL 0x18
#define MIRDH 0x19

#define MAADR5 0
#define MAADR6 0x01
#define MAADR3 0x02
#define MAADR4 0x03
#define MAADR1 0x04
#define MAADR2 0x05
#define EBSTSD 0x06
#define EBSTCON 0x07
#define EBSTCSL 0x08
#define EBSTCSH 0x09
#define MISTAT 0xA
#define EREVID 0x12
#define ECOCON 0x15
#define EFLOCON 0x17
#define EPAUSL 0x18
#define EPAUSH 0x19

#define PHCON1 0
#define PHSTAT1 0x01
#define PHID1 0x02
#define PHID2 0x03
#define PHCON2 0x10
#define PHSTAT2 0x11
#define PHIE 0x12
#define PHIR 0x13
#define PHLCON 0x14

#define ENC28J60_MAX_FRAME_LEN 1518
#define ENC28J60_TIMEOUT 0xFFFFFFFF  // This corresponds to ~54 seconds when running at 80 MHz.

#define LEN(x) (sizeof(x) / sizeof(x[0]))

static uint8_t enc28j60_rx_buffer[ENC28J60_MAX_FRAME_LEN];
static uint8_t enc28j60_tx_buffer[ENC28J60_MAX_FRAME_LEN];

static uint8_t read_control_register(struct ENC28J60 *enc28j60, uint8_t reg, uint8_t ethreg);
static void write_control_register(struct ENC28J60 *enc28j60, uint8_t reg, uint8_t data);
static uint16_t read_phy_register(struct ENC28J60 *enc28j60, uint8_t phy_addr);
static void write_phy_register(struct ENC28J60 *enc28j60, uint8_t phy_addr, int16_t value);
static void read_buffer_memory(struct ENC28J60 *enc28j60, uint32_t *data, uint16_t bytes);
static void write_buffer_memory(struct ENC28J60 *enc28j60, uint32_t *data, uint16_t bytes);
static void bit_field_set(struct ENC28J60 *enc28j60, uint8_t reg, uint8_t bitfield);
static void bit_field_clear(struct ENC28J60 *enc28j60, uint8_t reg, uint8_t bitfield);
static void system_reset(struct ENC28J60 *enc28j60);
static void init_peripherals(struct ENC28J60 *enc28j60);
static void init_buffers(struct ENC28J60 *enc28j60);
static void init_receive_filters(struct ENC28J60 *enc28j60);
static void init_interrupts(struct ENC28J60 *enc28j60);
static void init_mac_registers(struct ENC28J60 *enc28j60);
static void init_phy_registers(struct ENC28J60 *enc28j60);
static uint8_t init_success(struct ENC28J60 *enc28j60);

struct ENC28J60 ENC28J60 = {
    {SYSCTL_PERIPH_SSI0, SYSCTL_PERIPH_GPIOA, SYSCTL_PERIPH_GPIOB},
    SSI0_BASE,
    GPIO_PORTA_BASE, 
    GPIO_PIN_5,
    GPIO_PA5_SSI0TX,
    GPIO_PIN_4,
    GPIO_PA4_SSI0RX,
    GPIO_PIN_2,
    GPIO_PA2_SSI0CLK,
    GPIO_PORTB_BASE,
    GPIO_PIN_1,
    GPIO_PORTB_BASE,
    GPIO_PIN_0,
    enc28j60_rx_buffer,
    enc28j60_tx_buffer,
    0
};

uint8_t ENC28J60_init(struct ENC28J60 *enc28j60) {
    enc28j60->_nf_ptr = 0;

    init_peripherals(enc28j60);
    system_reset(enc28j60);
    init_buffers(enc28j60);
    init_receive_filters(enc28j60);
    init_interrupts(enc28j60);

    while ((read_control_register(enc28j60, ESTAT, 1) & 1) == 0)  // wait for OST
        ;

    init_mac_registers(enc28j60);
    init_phy_registers(enc28j60);

    bit_field_clear(enc28j60, ECON1, 0xC0);
    /*start_timer(enc28j60->timeout_clk);*/

    return init_success(enc28j60);
}

uint8_t ENC28J60_enable_receive(struct ENC28J60 *enc28j60) {
    bit_field_set(enc28j60, ECON1, 4);

    return (read_control_register(enc28j60, ECON1, 1) & 4) != 0; 
}

uint8_t ENC28J60_disable_receive(struct ENC28J60 *enc28j60) {
    bit_field_clear(enc28j60, ECON1, 4);

    return (read_control_register(enc28j60, ECON1, 1) & 4) == 0; 
}

/*void ENC28J60_enable_dma(struct ENC28J60 *enc28j60) {*/
    /*enable_ssi_tx_dma(enc28j60->ssi);*/
    /*enable_ssi_rx_dma(enc28j60->ssi);*/
/*}*/

/*void ENC28J60_disable_dma(struct ENC28J60 *enc28j60) {*/
    /*disable_ssi_tx_dma(enc28j60->ssi);*/
    /*disable_ssi_rx_dma(enc28j60->ssi);*/
/*}*/

void ENC28J60_disable_interrupts(struct ENC28J60 *enc28j60) {
    bit_field_clear(enc28j60, EIE, 0x80);
}

void ENC28J60_enable_interrupts(struct ENC28J60 *enc28j60) {
    bit_field_set(enc28j60, EIE, 0x80);
}

uint8_t ENC28J60_get_interrupt_requests(struct ENC28J60 *enc28j60) {
    return read_control_register(enc28j60, EIR, 1);
}

void ENC28J60_decrement_packet_count(struct ENC28J60 *enc28j60) {
    bit_field_set(enc28j60, ECON2, 0x40);  // decrement packet count
}

uint16_t ENC28J60_read_frame_blocking(struct ENC28J60 *enc28j60, uint8_t *data) {
    uint16_t len;
    uint8_t next_frame[2];
    uint8_t rsv[4];


    uint8_t bank = read_control_register(enc28j60, ECON1, 1) & 3;
    bit_field_clear(enc28j60, ECON1, 3); // switch to bank 0
    bit_field_set(enc28j60, ECON1, 0);

    read_buffer_memory(enc28j60, next_frame, 2);
    read_buffer_memory(enc28j60, rsv, 4);

    len = (rsv[0] & 0xFF) | (rsv[1] << 8);

    /* If we ever enter here an error has occurred. */
    if (len > ENC28J60_MAX_FRAME_LEN) {
        ENC28J60_disable_receive(enc28j60);
        return len;
    }

    read_buffer_memory(enc28j60, data, len);

    write_control_register(enc28j60, ERXRDPTL, next_frame[0]);
    write_control_register(enc28j60, ERXRDPTH, next_frame[1]);
    
    bit_field_clear(enc28j60, ECON1, 3);  // restore bank to previous value
    bit_field_set(enc28j60, ECON1, bank);

    return len;
}

void ENC28J60_write_frame_blocking(struct ENC28J60 *enc28j60, uint8_t *data, uint16_t size) {
    uint8_t control = 7;
    uint16_t start_addr;
    
    uint8_t bank = read_control_register(enc28j60, ECON1, 1) & 3;
    bit_field_clear(enc28j60, ECON1, 3); // switch to bank 0
    bit_field_set(enc28j60, ECON1, 0);

    /* ensure current transmission is complete, only necessary right now since
        transmission buffer is not currently designed to handle more than one
        frame at a time.
    */
    while (read_control_register(enc28j60, ECON1, 1) & 8)
        ;

    start_addr = read_control_register(enc28j60, ETXSTL, 1) |
                    (read_control_register(enc28j60, ETXSTH, 1) << 8);

    write_control_register(enc28j60, EWRPTL, start_addr & 0xFF);
    write_control_register(enc28j60, EWRPTH, (start_addr & 0xFF00) >> 8);

    write_control_register(enc28j60, ETXNDL, (start_addr + size) & 0xFF);
    write_control_register(enc28j60, ETXNDH, ((start_addr + size) & 0xFF00) >> 8);

    write_buffer_memory(enc28j60, &control, 1);
    write_buffer_memory(enc28j60, data, size);

    bit_field_set(enc28j60, ECON1, 0x08);  // start transmission process

    bit_field_clear(enc28j60, ECON1, 3);  // restore bank to previous value
    bit_field_set(enc28j60, ECON1, bank);
}

/*uint16_t ENC28J60_read_frame_dma(struct ENC28J60 *enc28j60) {*/
    /*ENC28J60_disable_dma(enc28j60);*/

    /*uint16_t len;*/
    /*uint16_t rdptr;*/
    /*uint8_t next_frame[2];*/
    /*uint8_t rsv[4];*/

    /*uint8_t bank = read_control_register(enc28j60, ECON1, 1) & 3;*/
    /*bit_field_clear(enc28j60, ECON1, 3); // switch to bank 0*/
    /*bit_field_set(enc28j60, ECON1, 0);*/

    /*rdptr = (read_control_register(enc28j60, ERDPTL, 1) & 0xFF) | */
            /*(read_control_register(enc28j60, ERDPTH, 1) << 8);*/

    /* RDPTR may be a few locatiosn behind due to padding from the receiver.
        In this case it's necessary to advance it to the actual start of the next frame. */
    /*if (rdptr != enc28j60->_nf_ptr) {*/
        /*write_control_register(enc28j60, ERDPTL, enc28j60->_nf_ptr & 0xFF);*/
        /*write_control_register(enc28j60, ERDPTH, (enc28j60->_nf_ptr & 0xFF00) >> 8);*/
    /*}*/

    /*read_buffer_memory(enc28j60, next_frame, 2);*/
    /*read_buffer_memory(enc28j60, rsv, 4);*/
    
    /*len = (rsv[0] & 0xFF) | (rsv[1] << 8);*/
    /*uint16_t nf = (next_frame[0] & 0xFF) | (next_frame[1] << 8);*/
    /*enc28j60->_nf_ptr = nf;*/

    /*[> If we ever enter here an error has occurred. <]*/
    /*if (len > ENC28J60_MAX_FRAME_LEN) {*/
        /*ENC28J60_disable_receive(enc28j60);*/
        /*return len;*/
    /*}*/

    /*uint8_t read_buf_cmd = RBM_OPCODE | RBM_ARG0;*/
    /*enc28j60->tx_buf[0] = read_buf_cmd;*/
    /*for (uint16_t i = 1; i < len; i++) {*/
        /*enc28j60->tx_buf[i] = NOP;*/

    /*}*/

    /*bit_field_clear(enc28j60, ECON1, 3);  // restore bank to previous value*/
    /*bit_field_set(enc28j60, ECON1, bank);*/

    /*enable_ssi_interrupts(enc28j60->ssi);*/

    /*ENC28J60_enable_dma(enc28j60);*/
        
    /*set_gpio_pin_low(enc28j60->cs);*/
    /*start_dma_transfer_peripheral_rx(enc28j60->dmarx, (uint8_t *) &SSI1_DR_R, enc28j60->rx_buf + len, len + 1);*/
    /*start_dma_transfer_peripheral_tx(enc28j60->dmatx, enc28j60->tx_buf + len, (uint8_t *) &SSI1_DR_R, len + 1);*/
    
    /*return len;*/
/*}*/

void ENC28J60_advance_rdptr(struct ENC28J60 *enc28j60) {
    uint16_t rxrdptr;
    if (enc28j60->_nf_ptr == 0) {
        rxrdptr = 0x17ff;
    } else {
        rxrdptr = enc28j60->_nf_ptr - 1;
    }   
    write_control_register(enc28j60, ERXRDPTL, rxrdptr & 0xFF);
    write_control_register(enc28j60, ERXRDPTH, (rxrdptr >> 8) & 0xFF);
}

void ENC28J60_get_tx_status_vec(struct ENC28J60 *enc28j60, uint8_t *tsv) {
    uint8_t bank = read_control_register(enc28j60, ECON1, 1) & 3;
    bit_field_clear(enc28j60, ECON1, 3); // switch to bank 0
    bit_field_set(enc28j60, ECON1, 0);

    while (read_control_register(enc28j60, ECON1, 1) & 8)  // ensure current transmission is complete
        ;

    uint16_t current_ptr = read_control_register(enc28j60, ERDPTL, 1) |
                            (read_control_register(enc28j60, ERDPTH, 1) << 8);

    uint16_t tx_end_ptr = (read_control_register(enc28j60, ETXNDL, 1) |
                            (read_control_register(enc28j60, ETXNDH, 1) << 8)) + 1;

    write_control_register(enc28j60, ERDPTL, tx_end_ptr & 0xFF);
    write_control_register(enc28j60, ERDPTH, (tx_end_ptr & 0xFF00) >> 8);

    read_buffer_memory(enc28j60, tsv, 7);

    write_control_register(enc28j60, ERDPTL, current_ptr & 0xFF);  // restore ERDPT
    write_control_register(enc28j60, ERDPTH, (current_ptr & 0xFF00) >> 8);

    bit_field_clear(enc28j60, ECON1, 3);  // restore bank to previous value
    bit_field_set(enc28j60, ECON1, bank);
}

uint8_t ENC28J60_get_packet_count(struct ENC28J60 *enc28j60) {
    uint8_t bank = read_control_register(enc28j60, ECON1, 1) & 3;
    bit_field_clear(enc28j60, ECON1, 3); // switch to bank 1
    bit_field_set(enc28j60, ECON1, 1);
    uint8_t count = read_control_register(enc28j60, EPKTCNT, 1);
    bit_field_clear(enc28j60, ECON1, 3);  // restore bank to previous value
    bit_field_set(enc28j60, ECON1, bank);
    return count;
}

void ENC28J60_get_mac_address(struct ENC28J60 *enc28j60, uint8_t *buf) {
    uint8_t bank = read_control_register(enc28j60, ECON1, 1) & 3;
    bit_field_clear(enc28j60, ECON1, 3); // switch to bank 3
    bit_field_set(enc28j60, ECON1, 3);

    buf[0] = read_control_register(enc28j60, MAADR1, 0);
    buf[1] = read_control_register(enc28j60, MAADR2, 0);
    buf[2] = read_control_register(enc28j60, MAADR3, 0);
    buf[3] = read_control_register(enc28j60, MAADR4, 0);
    buf[4] = read_control_register(enc28j60, MAADR5, 0);
    buf[5]= read_control_register(enc28j60, MAADR6, 0);

    bit_field_clear(enc28j60, ECON1, 3);  // restore bank to previous value
    bit_field_set(enc28j60, ECON1, bank);
}

static uint8_t read_control_register(struct ENC28J60 *enc28j60, uint8_t reg, uint8_t ethreg) {
    reg = (reg & 0x1F) | RCR_OPCODE;
    uint32_t data[2];
    GPIOPinWrite(enc28j60->cs_pin_base, enc28j60->cs_pin, 0);

    SSIDataPut(enc28j60->ssi_base, reg);
    /*while (SSIBusy(enc28j60->ssi_base))*/
        /*;*/
    SSIDataGet(enc28j60->ssi_base, data);  // discard trash first byte
    if (ethreg) {
        SSIDataPut(enc28j60->ssi_base, NOP);
        SSIDataGet(enc28j60->ssi_base, data);
    } else {
        SSIDataPut(enc28j60->ssi_base, NOP);
        SSIDataPut(enc28j60->ssi_base, NOP);
        SSIDataGet(enc28j60->ssi_base, &data[0]);
        SSIDataGet(enc28j60->ssi_base, &data[1]);
    }
    GPIOPinWrite(enc28j60->cs_pin_base, enc28j60->cs_pin, enc28j60->cs_pin);
    return ethreg ? data[0] : data[1];
}

static void write_control_register(struct ENC28J60 *enc28j60, uint8_t reg, uint8_t data) {
    uint32_t trash;
    reg = (reg & 0x1F) | WCR_OPCODE;
    GPIOPinWrite(enc28j60->cs_pin_base, enc28j60->cs_pin, 0);
    SSIDataPut(enc28j60->ssi_base, reg);
    SSIDataPut(enc28j60->ssi_base, data);
    while (SSIBusy(enc28j60->ssi_base))
        ;
    GPIOPinWrite(enc28j60->cs_pin_base, enc28j60->cs_pin, enc28j60->cs_pin);
    while (SSIDataGetNonBlocking(enc28j60->ssi_base, &trash))
        ;
}

static uint16_t read_phy_register(struct ENC28J60 *enc28j60, uint8_t phy_addr) {
    uint8_t bank = read_control_register(enc28j60, ECON1, 1) & 3;
    bit_field_clear(enc28j60, ECON1, 3); // switch to bank 2
    bit_field_set(enc28j60, ECON1, 2);

    write_control_register(enc28j60, MIREGADR, phy_addr);
    write_control_register(enc28j60, MICMD, 1);

    while (read_control_register(enc28j60, MISTAT, 0) & 1)  // poll MIISTAT.BUSY bit
        ;

    write_control_register(enc28j60, MICMD, 0);

    uint16_t data = read_control_register(enc28j60, MIRDL, 0) | (read_control_register(enc28j60, MIRDH, 0) << 8);

    bit_field_clear(enc28j60, ECON1, 3);  // restore bank to previous value
    bit_field_set(enc28j60, ECON1, bank);

    return data;
}

static void write_phy_register(struct ENC28J60 *enc28j60, uint8_t phy_addr, int16_t value) {
    uint8_t bank = read_control_register(enc28j60, ECON1, 1) & 3;
    bit_field_clear(enc28j60, ECON1, 3); // switch to bank 2
    bit_field_set(enc28j60, ECON1, 2);

    write_control_register(enc28j60, MIREGADR, phy_addr);
    write_control_register(enc28j60, MIWRL, value & 0xFF);
    write_control_register(enc28j60, MIWRH, (value & 0xFF00) >> 8);


    while (read_control_register(enc28j60, MISTAT, 0) & 1)  // poll MIISTAT.BUSY bit
        ;

    bit_field_clear(enc28j60, ECON1, 3);  // restore bank to previous value
    bit_field_set(enc28j60, ECON1, bank);
}

static void read_buffer_memory(struct ENC28J60 *enc28j60, uint32_t *data, uint16_t bytes) {
    uint8_t cmd = RBM_OPCODE | RBM_ARG0;
    GPIOPinWrite(enc28j60->cs_pin_base, enc28j60->cs_pin, 0);
    SSIDataPut(enc28j60->ssi_base, cmd);
    SSIDataGet(enc28j60->ssi_base, data);
    for (int i = 0; i < bytes; i++) {
        SSIDataPut(enc28j60->ssi_base, NOP);
        SSIDataGet(enc28j60->ssi_base, &data[i]);
    }
    GPIOPinWrite(enc28j60->cs_pin_base, enc28j60->cs_pin, enc28j60->cs_pin);
}

static void write_buffer_memory(struct ENC28J60 *enc28j60, uint32_t *data, uint16_t bytes) {
    uint32_t trash;
    uint8_t cmd = WBM_OPCODE | WBM_ARG0;
    GPIOPinWrite(enc28j60->cs_pin_base, enc28j60->cs_pin, 0);
    SSIDataPut(enc28j60->ssi_base, cmd);
    for (int i = 0; i < bytes; i++) {
        SSIDataPut(enc28j60->ssi_base, data[i]);
    }
    while (SSIBusy(enc28j60->ssi_base))
        ;
    GPIOPinWrite(enc28j60->cs_pin_base, enc28j60->cs_pin, enc28j60->cs_pin);
    while (SSIDataGetNonBlocking(enc28j60->ssi_base, &trash))
        ;
}

static void bit_field_set(struct ENC28J60 *enc28j60, uint8_t reg, uint8_t bitfield) {
    uint32_t trash;
    reg = (reg & 0x1F) | BFS_OPCODE;
    GPIOPinWrite(enc28j60->cs_pin_base, enc28j60->cs_pin, 0);
    SSIDataPut(enc28j60->ssi_base, reg);
    SSIDataPut(enc28j60->ssi_base, bitfield);
    while (SSIBusy(enc28j60->ssi_base))
        ;
    GPIOPinWrite(enc28j60->cs_pin_base, enc28j60->cs_pin, enc28j60->cs_pin);
    while (SSIDataGetNonBlocking(enc28j60->ssi_base, &trash))
        ;
}

static void bit_field_clear(struct ENC28J60 *enc28j60, uint8_t reg, uint8_t bitfield) {
    uint32_t trash;
    reg = (reg & 0x1F) | BFC_OPCODE;
    GPIOPinWrite(enc28j60->cs_pin_base, enc28j60->cs_pin, 0);
    SSIDataPut(enc28j60->ssi_base, reg);
    SSIDataPut(enc28j60->ssi_base, bitfield);
    while (SSIBusy(enc28j60->ssi_base))
        ;
    GPIOPinWrite(enc28j60->cs_pin_base, enc28j60->cs_pin, enc28j60->cs_pin);
    while (SSIDataGetNonBlocking(enc28j60->ssi_base, &trash))
        ;
}

static void system_reset(struct ENC28J60 *enc28j60) {
    uint32_t trash;
    uint8_t cmd = SRC_OPCODE | SRC_ARG0;
    GPIOPinWrite(enc28j60->cs_pin_base, enc28j60->cs_pin, 0);
    SSIDataPut(enc28j60->ssi_base, cmd);
    while (SSIBusy(enc28j60->ssi_base))
        ;
    GPIOPinWrite(enc28j60->cs_pin_base, enc28j60->cs_pin, enc28j60->cs_pin);
    while (SSIDataGetNonBlocking(enc28j60->ssi_base, &trash))
        ;
}

static void init_peripherals(struct ENC28J60 *enc28j60) {
    uint32_t trashbuf;
    for (int i = 0; i < LEN(enc28j60->sysctl_peripherals) && enc28j60->sysctl_peripherals[i] != 0; i++) {
        SysCtlPeripheralEnable(enc28j60->sysctl_peripherals[i]);
    } 
    
    GPIOPinConfigure(enc28j60->clk_pin_mode);
    GPIOPinConfigure(enc28j60->rx_pin_mode);
    GPIOPinConfigure(enc28j60->tx_pin_mode);

    GPIOPinTypeSSI(enc28j60->ssi_gpio_base, 
                   enc28j60->tx_pin | enc28j60->rx_pin | enc28j60->clk_pin);
    SSIConfigSetExpClk(enc28j60->ssi_base, SysCtlClockGet(), SSI_FRF_MOTO_MODE_0,
                       SSI_MODE_MASTER, 1000000, 8);

    SSIEnable(enc28j60->ssi_base);
    while(SSIDataGetNonBlocking(enc28j60->ssi_base, &trashbuf))
        ;

    /* initialize GPIO pin to high for CS function */
    GPIOPinTypeGPIOOutput(enc28j60->cs_pin_base, enc28j60->cs_pin);
    GPIOPinWrite(enc28j60->cs_pin_base, enc28j60->cs_pin, enc28j60->cs_pin);

    GPIOPinTypeGPIOInput(enc28j60->intr_pin_base, enc28j60->intr_pin);
}

static void init_buffers(struct ENC28J60 *enc28j60) {
    uint8_t bank = read_control_register(enc28j60, ECON1, 1) & 3;
    bit_field_clear(enc28j60, ECON1, 3); // switch to bank 0
    bit_field_set(enc28j60, ECON1, 0);

    write_control_register(enc28j60, ERXSTL, 0);  // Rx buffer start
    write_control_register(enc28j60, ERXSTH, 0);
    write_control_register(enc28j60, ERXNDL, 0xFF);  // Rx buffer end
    write_control_register(enc28j60, ERXNDH, 0x17);

    write_control_register(enc28j60, ERDPTL, 0);  // Rx read pointer
    write_control_register(enc28j60, ERDPTH, 0);

    // set rx read ptr to start; data cannot be written past this, and it must be incremented manually
    write_control_register(enc28j60, ERXRDPTL, 0xFF);  
    write_control_register(enc28j60, ERXRDPTH, 0x17);

    // tx buffer is above rx buffer
    write_control_register(enc28j60, ETXSTL, 0);
    write_control_register(enc28j60, ETXSTH, 0x19);
    write_control_register(enc28j60, ETXNDL, 0xF8);
    write_control_register(enc28j60, ETXNDH, 0x1F);  // leave 7 bytes for status vector at end of buffer
    write_control_register(enc28j60, EWRPTL, 0);
    write_control_register(enc28j60, EWRPTH, 0x19);

    bit_field_clear(enc28j60, ECON1, 3);  // restore bank to previous value
    bit_field_set(enc28j60, ECON1, bank);
}

static void init_receive_filters(struct ENC28J60 *enc28j60) {
    uint8_t bank = read_control_register(enc28j60, ECON1, 1) & 3;
    bit_field_clear(enc28j60, ECON1, 3); // switch to bank 1
    bit_field_set(enc28j60, ECON1, 1);
    write_control_register(enc28j60, ERXFCON, 0x81);  // accept only unicast and broadcast frames
    bit_field_clear(enc28j60, ECON1, 3);  // restore bank to previous value
    bit_field_set(enc28j60, ECON1, bank);
}

static void init_interrupts(struct ENC28J60 *enc28j60) {
    bit_field_set(enc28j60, EIE, 0xC0);
}

static void init_mac_registers(struct ENC28J60 *enc28j60) {
    uint8_t bank = read_control_register(enc28j60, ECON1, 1) & 3;
    bit_field_clear(enc28j60, ECON1, 3); // switch to bank 2
    bit_field_set(enc28j60, ECON1, 2);

    write_control_register(enc28j60, MACON1, 0xF);  // enable receiving of all frames + flow control
    
    // enable padding, append CRC, no proprietary header, no large frames, check frame length, full-duplex
    write_control_register(enc28j60, MACON3, 0x33);
    write_control_register(enc28j60, MACON4, 0x40);  // init defer and backoff settings, applies to half-duplex only? needed?

    write_control_register(enc28j60, MAMXFLL, 0xEE);  // max frame size is 1518 bytes
    write_control_register(enc28j60, MAMXFLH, 0x05);

    write_control_register(enc28j60, MABBIPG, 0x15);  // back-to-back inter-packet gap of IEEE specified 9.6us

    write_control_register(enc28j60, MAIPGL, 0x12);  // non-back-to-back interpacket gap, datasheet says 12h is typical

    bit_field_clear(enc28j60, ECON1, 3); // switch to bank 3
    bit_field_set(enc28j60, ECON1, 3);

    // init MAC address to A0-CD-EF-01-23-45, 0 in LSB if first byte indicates unicast address
    write_control_register(enc28j60, MAADR1, 0xA0);
    write_control_register(enc28j60, MAADR2, 0xCD);
    write_control_register(enc28j60, MAADR3, 0xEF);
    write_control_register(enc28j60, MAADR4, 0x01);
    write_control_register(enc28j60, MAADR5, 0x23);
    write_control_register(enc28j60, MAADR6, 0x45);

    bit_field_clear(enc28j60, ECON1, 3);  // restore bank to previous value
    bit_field_set(enc28j60, ECON1, bank);
}

static void init_phy_registers(struct ENC28J60 *enc28j60) {
    write_phy_register(enc28j60, PHCON1, 0x0100);
}

static uint8_t init_success(struct ENC28J60 *enc28j60) {
    uint8_t bank = read_control_register(enc28j60, ECON1, 1) & 3;
    bit_field_clear(enc28j60, ECON1, 3); // switch to bank 0
    bit_field_set(enc28j60, ECON1, 0);

    uint8_t success = ((uint16_t) (read_control_register(enc28j60, ERXSTL, 1) | 
                        (read_control_register(enc28j60, ERXSTH, 1) << 8))) == 0;
    success &= ((uint16_t) (read_control_register(enc28j60, ERXNDL, 1) |
                        (read_control_register(enc28j60, ERXNDH, 1) << 8))) == 0x17FF;
    success &= ((uint16_t) read_control_register(enc28j60, ERXRDPTL, 1) |
                        (read_control_register(enc28j60, ERXRDPTH, 1) << 8)) == 0x17FF;
    success &= ((uint16_t) read_control_register(enc28j60, ERDPTL, 1) |
                        (read_control_register(enc28j60, ERDPTH, 1) << 8)) == 0;

    success &= ((uint16_t) read_control_register(enc28j60, ETXSTL, 1) |
                        (read_control_register(enc28j60, ETXSTH, 1) << 8)) == 0x1900;
    success &= ((uint16_t) read_control_register(enc28j60, ETXNDL, 1) |
                        (read_control_register(enc28j60, ETXNDH, 1) << 8)) == 0x1FF8;
    success &= ((uint16_t) read_control_register(enc28j60, EWRPTL, 1) |
                        (read_control_register(enc28j60, EWRPTH, 1) << 8)) == 0x1900;

    bit_field_clear(enc28j60, ECON1, 3); // switch to bank 2
    bit_field_set(enc28j60, ECON1, 2);                    

    success &= read_control_register(enc28j60, MACON1, 0) == 0xF;
    success &= read_control_register(enc28j60, MACON3, 0) == 0x33;
    success &= read_control_register(enc28j60, MACON4, 0) == 0x40;
    success &= ((uint16_t) (read_control_register(enc28j60, MAMXFLL, 0) |
                (read_control_register(enc28j60, MAMXFLH, 0) << 8))) == 0x05EE;
    success &= read_control_register(enc28j60, MABBIPG, 0) == 0x15;
    success &= read_control_register(enc28j60, MAIPGL, 0) == 0x12;

    bit_field_clear(enc28j60, ECON1, 3); // switch to bank 3
    bit_field_set(enc28j60, ECON1, 3);   

    success &= read_control_register(enc28j60, MAADR1, 0) == 0xA0;
    success &= read_control_register(enc28j60, MAADR2, 0) == 0xCD;
    success &= read_control_register(enc28j60, MAADR3, 0) == 0xEF;
    success &= read_control_register(enc28j60, MAADR4, 0) == 0x01;
    success &= read_control_register(enc28j60, MAADR5, 0) == 0x23;
    success &= read_control_register(enc28j60, MAADR6, 0) == 0x45;

    success &= read_phy_register(enc28j60, PHCON1) == 0x0100;

    success &= read_control_register(enc28j60, EIE, 1) == 0xC0;

    bit_field_clear(enc28j60, ECON1, 3);  // restore bank to previous value
    bit_field_set(enc28j60, ECON1, bank);

    return success;
} 
