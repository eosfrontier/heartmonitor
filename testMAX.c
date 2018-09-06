#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "MAX30102.h"

int main(int argc, char *argv[])
{
    max30102_t *mx = max_init();
    if (!mx) {
        exit(1);
    }

    while (1) {
        if (max_update(mx) < 0) exit(1);
        fprintf(stderr, "Red: %15.8f IR: %15.8f Raw: 0x%06x 0x%06x\n", mx->red, mx->ir, mx->rawred, mx->rawir);
        usleep(1000000 / 100);
    }
    max_fini(mx);
    exit(0);
}

/* vim: ai:si:expandtab:ts=4:sw=4
 */
