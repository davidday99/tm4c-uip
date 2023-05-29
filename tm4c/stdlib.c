void memset(void *dst, char c, int n) {
    while (n--)
      *((char *) dst++) = c; 
}

void memcpy(void *dst, const void *src, int n) {
    while (n--)
      *((char *) dst++) = *((char *) src++);
}

int strlen(char *s) {
    int n = 0;
    while (*s++)
        n++;
    return n;
}

char *strncpy(char *dst, char *src, int sz) {
    while (*src && sz--)
      *((char *) dst++) = *((char *) src++);
    return dst;
}

int printf() {
    return 0;
}

