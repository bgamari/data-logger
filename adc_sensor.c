#include <stdbool.h>
#include "config.h"
#include "adc_sensor.h"

static bool adc_busy = false;
static struct sample_queue *queue_head;

static void adc_schedule(void);

struct sensor_type adc_sensor = {
        .sample_fn = &adc_sensor_sample,
        /* We are using the bus clock therefore we must no enter STOP */
        .no_stop = true
};

static void
adc_sample_done(uint16_t codepoint, int error, void* cbdata)
{
        struct sensor *sensor = cbdata;
        struct adc_sensor_data *sensor_data = sensor->sensor_data;
        accum value = (*sensor_data->map)(codepoint, sensor_data->map_data);
        sensor_new_sample(sensor, value);
        adc_busy = false;
        adc_schedule();
}

// ADC requests need to be serialized. This is where we do this.
static void
adc_schedule()
{
        if (adc_busy)
                return;

        crit_enter();
        if (queue_head) {
                struct adc_sensor_data *sensor_data = queue_head->sensor->sensor_data;
                adc_busy = true;
                adc_sample_start(sensor_data->channel, adc_sample_done, queue_head->sensor);
                queue_head = queue_head->next;
        }
        crit_exit();
}

void
adc_sensor_sample(struct sensor *sensor)
{
        struct adc_sensor_data *sensor_data = sensor->sensor_data;
        crit_enter();
        sensor_data->queue.sensor = sensor;
        sensor_data->queue.next = queue_head;
        queue_head = &sensor_data->queue;
        crit_exit();

        adc_schedule();
}

accum
adc_map_raw(uint16_t codepoint, void *map_data)
{
        return 1. * codepoint;
}

