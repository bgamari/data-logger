#include "adc_sensor.h"

static void
adc_sample_done(uint16_t codepoint, int error, void* cbdata)
{
        struct sensor *sensor = cbdata;
        struct adc_sensor_data *sensor_data = sensor->sensor_data;
        accum value = (*sensor_data->map)(codepoint, sensor_data->map_data);
        sensor_new_sample(sensor, value);
}

void
adc_sensor_sample(struct sensor *sensor)
{
        struct adc_sensor_data *sensor_data = sensor->sensor_data;
        adc_sample_start(sensor_data->channel, adc_sample_done, sensor);
}
