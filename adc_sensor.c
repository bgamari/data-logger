#include <stdbool.h>
#include "config.h"
#include "adc_sensor.h"

/*
 * Whether the ADC is in a usable state.
 *
 * This is reset upon leaving low-power mode.
 */
bool adc_initialized = false;

struct sensor_type adc_sensor = {
        .sample_fn = &adc_sensor_sample,
        /* We are using the bus clock therefore we must no enter STOP */
        .no_stop = true
};

static void
adc_sensor_sample_done(uint16_t codepoint, int error, void* cbdata)
{
        struct sensor *sensor = cbdata;
        struct adc_sensor_data *sensor_data = sensor->sensor_data;
        accum value = (*sensor_data->map)(codepoint, sensor_data->map_data);
        sensor_new_sample(sensor, value);
}

void
adc_sensor_sample(struct sensor *sensor)
{
        struct adc_sensor_data *sd = sensor->sensor_data;
        adc_queue_sample(&sd->ctx, sd->channel, sd->mode,
                         adc_sensor_sample_done, sensor);
}

accum
adc_map_raw(uint16_t codepoint, void *map_data)
{
        return 1. * codepoint;
}

