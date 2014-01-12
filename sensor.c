#include "sensor.h"

static struct sensor_listener *listeners;

int
sensor_start_sample(struct sensor *sensor)
{
        if (sensor->busy)
                return 1;
        sensor->busy = true;
        sensor->sample(sensor);
        return 0;
}

void
sensor_new_sample(struct sensor *sensor, accum value)
{
        struct sensor_listener *l = listeners;
        crit_enter();
        sensor->last_sample_time = rtc_get_time();
        sensor->last_sample = value;
        sensor->busy = false;
        crit_exit();
        while (l) {
                l->new_sample(sensor, value, l->cbdata);
                l = l->next;
        }
}

void
sensor_listen(struct sensor_listener *listener,
              new_sample_cb new_sample, void *cbdata)
{
        listener->new_sample = new_sample;
        listener->cbdata = cbdata;
        crit_enter();
        listener->next = listeners;
        listeners = listener;
        crit_exit();
}
