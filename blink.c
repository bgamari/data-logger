#include <stdbool.h>
#include <mchck.h>
#include "blink.h"

struct led_blink {
        int nblinks; // number of transitions remaining, -1 for continuous blinking
        unsigned int on_time; // in milliseconds
        unsigned int off_time; // in milliseconds
        struct timeout_ctx timeout;
        int state;
        bool running;
};

struct led_blink blink;

static void
blink_timeout_cb(void *cbdata)
{
        struct led_blink *blink = cbdata;
        if (blink->nblinks > 0)
                blink->nblinks--;
        if (blink->nblinks == 0)
                blink->running = false;

        if (blink->running) {
                blink->state = !blink->state;
                unsigned int timeout = blink->state ? blink->on_time : blink->off_time;
                timeout_add(&blink->timeout, timeout, blink_timeout_cb, blink);
        } else {
                blink->state = false;
        }

        onboard_led(blink->state);
}

void
start_blink(int nblinks, unsigned int on_time, unsigned int off_time)
{
        if (blink.running) return;
        blink.nblinks = 2*nblinks;
        blink.on_time = on_time;
        blink.off_time = off_time;
        blink.state = 0;
        blink.running = true;
        blink_timeout_cb((void*) &blink);
}

void
stop_blink()
{
        blink.running = false;
}
