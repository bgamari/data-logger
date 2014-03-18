#include "sensors/tcs3472_sensor.h"

static void
tcs_sample_finish(enum i2c_result result, void *cbdata)
{
        struct sensor *sensor = cbdata;
        struct tcs_sensor_data *sd = sensor->sensor_data;
        struct tcs_sample *d = &sd->sample;
        if (result != I2C_RESULT_SUCCESS) {
                sensor_sample_failed(sensor);
                return;
        }
        sd->values[0] = 1.0k * d->cdata;
        sd->values[1] = 1.0k * d->rdata;
        sd->values[2] = 1.0k * d->gdata;
        sd->values[3] = 1.0k * d->bdata;
        sensor_new_sample(sensor, sd->values);
}

static void
tcs_sample_stop(uint8_t *buf, enum i2c_result result, void *cbdata)
{
        struct sensor *sensor = cbdata;
        struct tcs_sensor_data *sd = sensor->sensor_data;
        if (result != I2C_RESULT_SUCCESS) {
                sensor_sample_failed(sensor);
                return;
        }
        tcs_write_reg(&sd->tcs, TCS_REG_ENABLE, 0x00, tcs_sample_finish, sensor);
}

static void
tcs_sample_waiting(uint8_t *value, enum i2c_result result, void *cbdata)
{
        struct sensor *sensor = cbdata;
        struct tcs_sensor_data *sd = sensor->sensor_data;
        if (result != I2C_RESULT_SUCCESS) {
                sensor_sample_failed(sensor);
                return;
        }

        if (value[0] & 0x1) {
                tcs_read_data(&sd->tcs, &sd->sample, tcs_sample_stop, sensor);
        } else {
                tcs_read_reg(&sd->tcs, TCS_REG_STATUS, tcs_sample_waiting, sensor);
        }
}

static void
tcs_sample_begin_wait(enum i2c_result result, void *cbdata)
{
        struct sensor *sensor = cbdata;
        if (result != I2C_RESULT_SUCCESS) {
                sensor_sample_failed(sensor);
                return;
        }
        tcs_sample_waiting(0x0, I2C_RESULT_SUCCESS, sensor);
}

static void
tcs_sample_rgbc_en(void *cbdata)
{
        struct sensor *sensor = cbdata;
        struct tcs_sensor_data *sd = sensor->sensor_data;
        tcs_write_reg(&sd->tcs, TCS_REG_ENABLE, 0x03, tcs_sample_begin_wait, sensor);
}
        
static void
tcs_sample_enabled(enum i2c_result result, void *cbdata)
{
        struct sensor *sensor = cbdata;
        struct tcs_sensor_data *sd = sensor->sensor_data;
        if (result != I2C_RESULT_SUCCESS) {
                sensor_sample_failed(sensor);
                return;
        }
        timeout_add(&sd->timeout, 3, tcs_sample_rgbc_en, sensor);
}

static void
tcs_sample(struct sensor *sensor)
{
        struct tcs_sensor_data *sd = sensor->sensor_data;
        tcs_write_reg(&sd->tcs, TCS_REG_ENABLE, 0x1, tcs_sample_enabled, sensor);
}

const char intensity[] = "intensity";

struct measurable tcs3472_measurables[] = {
        {.id = 0, .name = "clear", .unit = "intensity"},
        {.id = 1, .name = "red", .unit = "intensity"},
        {.id = 2, .name = "green", .unit = "intensity"},
        {.id = 3, .name = "blue", .unit = "intensity"},
};

struct sensor_type tcs3472_sensor_type = {
        .sample_fn = tcs_sample,
        // Needs I2C
        .no_stop = true,
        .n_measurables = 4,
        .measurables = tcs3472_measurables
};
