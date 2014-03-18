#include "sensor.h"

static struct sensor_listener *listeners;

static unsigned int busy_count = 0;

static const enum gpio_pin_id sensor_enable_pin = PIN_PTD3;

int
sensor_start_sample(struct sensor *sensor)
{
        if (sensor->busy)
                return 1;
        busy_count++;
        gpio_write(sensor_enable_pin, GPIO_LOW);
        sensor->busy = true;
        sensor->type->sample_fn(sensor);
        return 0;
}

void
sensor_sample_failed(struct sensor *sensor)
{
        sensor->busy = false;
}

void
sensor_new_sample(struct sensor *sensor, accum *values)
{
        uint32_t time = rtc_get_time();
        crit_enter();
        sensor->last_sample_time = time;
        sensor->values = values;
        for (unsigned int i=0; i < sensor->type->n_measurables; i++) {
                accum value = sensor->values[i];
                struct sensor_listener *l = listeners;
                while (l) {
                        l->new_sample(sensor, time, i, value, l->cbdata);
                        l = l->next;
                }
        }
        sensor->busy = false;

        busy_count--;
        if (busy_count == 0)
                gpio_write(sensor_enable_pin, GPIO_HIGH);
        crit_exit();
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

accum
sensor_get_value(struct sensor *sensor, unsigned int measurable)
{
        if (measurable < sensor->type->n_measurables
            && sensor->values != NULL)
                return sensor->values[measurable];
        else
                return 0;
}
