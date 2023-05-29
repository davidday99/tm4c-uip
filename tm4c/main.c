#include "enc28j60.h"

void uip_log(char *m)
{
  printf("uIP log message: %s\n", m);
}

int main() {

    int status = ENC28J60_init(&ENC28J60);

    while (1)
        ;

	return 0;
}

