#include "sensor.h"

static struct sensor_listener *listeners;

int
sensor_start_sample(struct sensor *sensor)
{
        if (sensor->busy)
                return 1;
        sensor->busy = true;
        sensor->type->sample_fn(sensor);
        return 0;
}

static void
sensor_new_measurement(struct sensor *sensor, uint32_t time,
                       uint8_t measurable, accum value)
{
        struct sensor_listener *l = listeners;
        crit_enter();
        sensor->last_sample_time = time;
        sensor->last_sample = value;
        crit_exit();
        while (l) {
                l->new_sample(sensor, time, measurable, value, l->cbdata);
                l = l->next;
        }
}

void
sensor_new_sample_list(struct sensor *sensor, size_t elems, ...)
{
        va_list ap;
        uint32_t time = rtc_get_time();
        
        va_start(ap, elems);
        for ( ; elems > 0; elems--) {
                uint8_t measurable = va_arg(ap, unsigned int);
                accum value = va_arg(ap, accum);
                sensor_new_measurement(sensor, time, measurable, value);
        }
        va_end(ap);
        sensor->busy = false;
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
