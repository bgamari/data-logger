#include <mchck.h>
#include "blink.h"
#include "acquire.h"
#include "sample_store.h"
#include "config.h"

bool acquire_running = false;

static struct timeout_ctx timeout;

static void
on_sample_cb(struct sensor *sensor, accum value, void *cbdata)
{
        sample_store_push((struct sample) {
                        .time = rtc_get_time(),
                        .sensor_id = sensor->sensor_id,
                        .value = value});
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
        timeout_add(&timeout, 500, timeout_cb, NULL);
}

void start_acquire()
{
        start_blink(2, 50, 50);
        timeout_add(&timeout, 1000, timeout_cb, NULL);
        acquire_running = true;
}

void stop_acquire()
{
        start_blink(3, 50, 50);
        timeout_cancel(&timeout);
        acquire_running = false;
}

static struct sensor_listener listener;

void acquire_init()
{
        sensor_listen(&listener, on_sample_cb, NULL);
}
