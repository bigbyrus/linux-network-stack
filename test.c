#include <stdio.h>
#include <stdlib.h>

typedef struct {
	int a;
	short s[2];
} MSG;

int main() {
    /* create an MSG struct, memory allocated in the STACK */
    MSG m = {4, {1, 2}};

    printf("Address of m: %p\n", (void*)&m);
    printf("Address of m.a: %p\n", (void*)&m.a);
    printf("Address of m.s[0]:   %p\n", (void*)&m.s[0]);
    printf("Address of m.s[1]:   %p\n", (void*)&m.s[1]);

    printf("\nSize of MSG: %zu bytes\n\n", sizeof(MSG));

    /* UNSIGNED CHAR data type represents the RAW BYTES */
    unsigned char *bytes = (unsigned char*)&m;

    printf("Raw bytes of m:\n");
    for (int i = 0; i < sizeof(MSG); i++) {
        printf("%02x ", bytes[i]);
    }
    printf("\n\n");
    
    /* this struct is allocated on the HEAP */
    MSG *mp = malloc(sizeof(MSG));

    /* SRC points to data in the STACK */
    char *src = (char*)&m;

    /* DST points to allocated memory in the HEAP */
    char *dst = (char*)mp;

    /* copy memory from STACK to HEAP */
    for (int i = 0; i < sizeof(MSG); i++) {
        dst[i] = src[i];
    }

    /* print the new data we stored in the heap */
    printf("Copied struct:\n");
    printf("mp->a = %d\n", mp->a);
    printf("mp->s[0] = %d\n", mp->s[0]);
    printf("mp->s[1] = %d\n", mp->s[1]);

    /* deallocate the memory in the heap (no memory leakage) */
    free(mp);
    return 0;
}
