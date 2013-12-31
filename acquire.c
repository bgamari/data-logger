#include <mchck.h>
#include "blink.h"
#include "acquire.h"
#include "sample_store.h"
#include "config.h"

bool acquire_running = false;
static unsigned int sample_period = 30; // seconds

static struct timeout_ctx timeout;

static void
on_sample_cb(struct sensor *sensor, accum value, void *cbdata)
{
        struct sample s = {
                        .time = rtc_get_time(),
                        .sensor_id = sensor->sensor_id,
                        .value = value
        };
        sample_store_push(s);
}

void
take_sample()
{
        start_blink(1, 50, 50);
        for (struct sensor **s = &sensors[0]; *s != NULL; s++) {
                (*s)->sample(*s);
        }
}

static void
timeout_cb(void *data)
{
        take_sample();
        timeout_add(&timeout, 1000*sample_period, timeout_cb, NULL);
}

void
set_sample_period(unsigned int seconds)
{
        timeout_cancel(&timeout);
        sample_period = seconds;
        timeout_cb(NULL);
}

unsigned int
get_sample_period()
{
        return sample_period;
}

void start_acquire()
{
        if (acquire_running) return;
        start_blink(2, 50, 50);
        timeout_add(&timeout, 1000, timeout_cb, NULL);
        acquire_running = true;
}

void stop_acquire()
{
        if (!acquire_running) return;
        start_blink(3, 50, 50);
        timeout_cancel(&timeout);
        acquire_running = false;
}

static struct sensor_listener listener;

void acquire_init()
{
        sensor_listen(&listener, on_sample_cb, NULL);
}
