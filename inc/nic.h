#ifndef __NIC_H__
#define __NIC_H__

#include <stdint.h>

int nic_init(void);
int nic_read(uint8_t *buf);
void nic_write(uint8_t *buf, int size);

#endif /* __NIC_H__ */

