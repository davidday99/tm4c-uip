#include <stdio.h>

int nic_recv(char *buf, int count) {
    for (int i = 0; i < count; i++) {
        buf[i] = i;
    }
    return count;
}

void nic_send(char *buf, int count) {
    return;
}
