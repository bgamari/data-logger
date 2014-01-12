#include <mchck.h>
#include "blink.h"
#include "acquire.h"
#include "sample_store.h"
#include "config.h"

bool acquire_running = false;
static unsigned int sample_period = 30; // seconds

static struct rtc_alarm_ctx alarm;

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
                sensor_start_sample(*s);
        }
}

static void
alarm_cb(void *data)
{
        take_sample();
        rtc_alarm_add(&alarm, rtc_get_time() + sample_period,
                      alarm_cb, NULL);
}

void
set_sample_period(unsigned int seconds)
{
        crit_enter();
        if (acquire_running)
                rtc_alarm_cancel(&alarm);
        sample_period = seconds;
        if (acquire_running)
                alarm_cb(NULL);
        crit_enter();
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
        alarm_cb(NULL);
        acquire_running = true;
}

void stop_acquire()
{
        if (!acquire_running) return;
        start_blink(3, 50, 50);
        rtc_alarm_cancel(&alarm);
        acquire_running = false;
}

static struct sensor_listener listener;

void acquire_init()
{
        sensor_listen(&listener, on_sample_cb, NULL);
}
