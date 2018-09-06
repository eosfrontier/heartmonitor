#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "MAX30102.h"

static int running = 1;

static void ctrl_c_handler(int signum)
{
    running = 0;
}

static void setup_handlers(void)
{
    struct sigaction sa = { .sa_handler = ctrl_c_handler };
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

int main(int argc, char *argv[])
{
    max30102_t *mx = max_init();
    if (!mx) {
        exit(1);
    }

    setup_handlers();

    while (running) {
        if (max_update(mx) < 0) exit(1);
        int len = (int)(((mx->ir + 3000) / 6000) * 80);
        if (len <   0) len = 0;
        if (len >= 80) len = 79;
        char *str = "################################################################################                                                                                ";
        printf("Red: %18.4f IR: %18.4f %80.80s\n", mx->red, mx->ir, str+len);
        fflush(stdout);
        usleep(1000000 / 50);
    }
    max_fini(mx);
    printf("Exiting\n");
    exit(0);
}

/* vim: ai:si:expandtab:ts=4:sw=4
 */
