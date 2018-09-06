#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MAX30102.h"

int main(int argc, char *argv[])
{
    max30102_t *mx = max_init();
    if (!mx) {
        exit(1);
    }

    while (1) {
        max_update(mx);
        fprintf(stderr, "Red: %12.4f IR: %12.4f\n", mx.red, mx.ir);
        usleep(1000000 / 100);
    }
    exit(0);
}

/* vim: ai:si:expandtab:ts=4:sw=4
 */
