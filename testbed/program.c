#include <stdio.h>

extern int magic_number(void);

int main(void) {
    printf("the magic number is %d!\n", magic_number());
    return 0;
}
