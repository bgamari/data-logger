#include <mchck.h>
#include "power.h"
#include "blink.h"
#include "acquire.h"
#include "sample_store.h"
#include "config.h"

#define MIN_SAMPLE_PERIOD 50

bool acquire_running = false;
static unsigned int sample_period = 30*1000; // milliseconds

static struct timeout_ctx timeout; // for < 10 seconds
static struct rtc_alarm_ctx alarm; // for >= 10 seconds

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
        if (!low_power_mode)
                start_blink(1, 50, 50);

        for (struct sensor **s = &sensors[0]; *s != NULL; s++) {
                sensor_start_sample(*s);
        }
}

/* Take a sample and schedule next sample */
static void
alarm_cb(void *data)
{
        take_sample();
        if (sample_period < 10000) {
                timeout_add(&timeout, sample_period, alarm_cb, NULL);
        } else {
                rtc_alarm_add(&alarm, rtc_get_time() + sample_period / 1000,
                              alarm_cb, NULL);
        }
}

void
set_sample_period(unsigned int milliseconds)
{
        if (milliseconds < MIN_SAMPLE_PERIOD)
                return;
        
        crit_enter();
        if (acquire_running) {
                timeout_cancel(&timeout);
                rtc_alarm_cancel(&alarm);
        }
        sample_period = milliseconds;
        if (acquire_running)
                alarm_cb(NULL);
        crit_exit();
}

unsigned int
get_sample_period()
{
        return sample_period;
}

void acquire_blink_state()
{
        if (acquire_running) {
                start_blink(2, 50, 50);
        } else {
                start_blink(3, 50, 50);
        }
}

void start_acquire()
{
        if (acquire_running) return;
        alarm_cb(NULL);
        acquire_running = true;
        acquire_blink_state();
}

void stop_acquire()
{
        if (!acquire_running) return;
        rtc_alarm_cancel(&alarm);
        timeout_cancel(&timeout);
        acquire_running = false;
        acquire_blink_state();
}

bool acquire_can_stop()
{
        for (struct sensor **s = &sensors[0]; *s != NULL; s++) {
                if ((*s)->busy && (*s)->type->no_stop)
                        return false;
        }
        return true;
}

static struct sensor_listener listener;

void acquire_init()
{
        sensor_listen(&listener, on_sample_cb, NULL);
}
