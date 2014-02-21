#include <mchck.h>
#include "sensors/bmp085_sensor.h"

static void
bmp085_sample_done(uint16_t pressure, void *cbdata)
{
        struct sensor *sensor = cbdata;
        struct bmp085_sensor_data *sd = sensor->sensor_data;
        sd->last_pressure = pressure;
        accum real_temperature = bmp085_correct_temperature(&sd->ctx,
                                                            sd->last_temperature);
        accum real_pressure = bmp085_correct_pressure(&sd->ctx,
                                                      sd->last_temperature,
                                                      sd->last_pressure);
        sensor_new_sample(sensor, 0, real_pressure, 1, real_temperature);
}

static void
bmp085_sample2(uint16_t temperature, void *cbdata)
{
        struct sensor *sensor = cbdata;
        struct bmp085_sensor_data *sd = sensor->sensor_data;
        sd->last_temperature = temperature;
        bmp085_sample_pressure(&sd->ctx, bmp085_sample_done, sensor);
}

static void
bmp085_sample(struct sensor *sensor)
{
        struct bmp085_sensor_data *sd = sensor->sensor_data;
        bmp085_sample_temperature(&sd->ctx, bmp085_sample2, sensor);
}

static struct measurable bmp085_measurables[] = {
        {.id = 0, .name = "pressure", .unit = "Pascals"},
        {.id = 1, .name = "temperature", .unit = "Celcius"},
};
        
struct sensor_type bmp085_sensor_type = {
        .sample_fn = bmp085_sample,
        .no_stop = false, // TODO?
        .n_measurables = 2,
        .measurables = bmp085_measurables
};
