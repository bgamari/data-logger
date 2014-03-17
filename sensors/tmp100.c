#include "sensors/tmp100.h"

void
tmp100_init(struct tmp100_ctx *ctx, uint8_t address, i2c_cb *cb, void *cbdata)
{
        ctx->buffer[0] = TMP100_REG_CONFIG;
        ctx->buffer[1] = ((struct tmp100_config) {
                        .sd  = 0,
                        .tm  = 0,
                        .pol = 0,
                        .f   = 0,
                        .r   = TMP100_RES_12BITS,
                        .os_alert = 0
                        }).raw;
        ctx->trans = (struct i2c_transaction) {
                .address = address,
                .direction = I2C_WRITE,
                .stop = I2C_STOP,
                .buffer = ctx->buffer,
                .length = 2,
                .cb = cb,
                .cbdata = cbdata
        };
        i2c_queue(&ctx->trans);
}

static void
tmp100_sample_done(uint8_t *buf, enum i2c_result result, void *cbdata)
{
        uint16_t word = (buf[0] << 4) | (buf[1] >> 4);
        int16_t temp = word & ~(1 << 11);
        if (word & (1<<11))
                temp = 0x1000 - temp;
        accum real_temp = 1. * temp / 16;
        struct sensor *sensor = cbdata;
        sensor_new_sample(sensor, 0, real_temp);
}

static void
tmp100_sample_start(enum i2c_result result, struct i2c_transaction *trans)
{
        struct sensor *sensor = trans->cbdata;
        struct tmp100_sensor_data *sd = sensor->sensor_data;
        i2c_reg_read(&sd->reg_read, sd->tmp100.trans.address,
                     TMP100_REG_TEMPERATURE, sd->buf, 2,
                     tmp100_sample_done, sensor);
}

static void
tmp100_sample(struct sensor *sensor)
{
        struct tmp100_sensor_data *sd = sensor->sensor_data;
        tmp100_init(&sd->tmp100, sd->address, tmp100_sample_start, sensor);
}

static struct measurable tmp100_measurables[] = {
        {.id = 0, .name = "temperature", .unit = "Celcius"},
};

struct sensor_type tmp100_sensor_type = {
        .sample_fn = tmp100_sample,
        // Needs I2C
        .no_stop = true,
        .n_measurables = 1,
        .measurables = tmp100_measurables
};

void tmp100_hack() {}
