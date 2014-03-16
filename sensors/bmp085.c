#include <mchck.h>
#include "sensors/bmp085.h"

/*
 * basic register I/O
 */
static void
bmp085_read_done(enum i2c_result result,
                 struct i2c_transaction *trans)
{
        struct bmp085_ctx *ctx = trans->cbdata;
        uint16_t value = (ctx->buf[0] << 8) | ctx->buf[1];
        ctx->read_reg_cb(value, ctx->read_reg_cbdata);
}

static void
bmp085_read_address_set(enum i2c_result result,
                        struct i2c_transaction *trans)
{
        struct bmp085_ctx *ctx = trans->cbdata;
        ctx->trans = (struct i2c_transaction) {
                .address = 0x77,
                .direction = I2C_READ,
                .buffer = ctx->buf,
                .length = 2,
                .stop = I2C_STOP,
                .cb = bmp085_read_done,
                .cbdata = ctx
        };
        i2c_queue(&ctx->trans);
}

static void
bmp085_read_reg(struct bmp085_ctx *ctx, uint8_t reg,
                bmp085_read_reg_cb cb, void *cbdata)
{
        ctx->read_reg_cb = cb;
        ctx->read_reg_cbdata = cbdata;
        ctx->buf[0] = reg;
        ctx->trans = (struct i2c_transaction) {
                .address = 0x77,
                .direction = I2C_WRITE,
                .buffer = ctx->buf,
                .length = 1,
                .stop = I2C_NOSTOP,
                .cb = bmp085_read_address_set,
                .cbdata = ctx
        };
        i2c_queue(&ctx->trans);
}

static void
bmp085_set_control(struct bmp085_ctx *ctx, uint8_t value)
{
        ctx->buf[0] = 0xf4;
        ctx->buf[1] = value;
        ctx->trans = (struct i2c_transaction) {
                .address = 0x77,
                .direction = I2C_WRITE,
                .buffer = ctx->buf,
                .length = 2,
                .stop = I2C_STOP,
                .cb = NULL
        };
        i2c_queue(&ctx->trans);
}


/*
 * sample correction
 */

/* Returns corrected temperature in units of 0.1 Celcius. */
int
bmp085_correct_temperature(struct bmp085_ctx *ctx, uint16_t ut)
{
        struct bmp085_cal *cal = &ctx->cal;
        int x1 = (ut - cal->ac6) * cal->ac5 >> 15;
        int x2 = (cal->mc << 11) / (x1 + cal->md);
        int b5 = x1 + x2;
        return (b5 + 8) >> 4;
}

/* Returns corrected pressure in Pascals */
int
bmp085_correct_pressure(struct bmp085_ctx *ctx, uint16_t ut, uint16_t up)
{
        struct bmp085_cal *cal = &ctx->cal;
        // correct temperature (again)
        int x1 = (ut - cal->ac6) * cal->ac5 >> 15;
        int x2 = (cal->mc << 11) / (x1 + cal->md);
        int b5 = x1 + x2;
        
        // correct pressure
        int b6 = b5 - 4000;
        x1 = (cal->b2 * (b6 * b6 >> 12)) >> 11;
        x2 = cal->ac2 * b6 >> 11;
        int x3 = x1 + x2;
        int b3 = ((((int32_t) cal->ac1 * 4 + x3) << ctx->oss) + 2) / 4;
        x1 = cal->ac3 * b6 >> 13;
        x2 = (cal->b1 * (b6 * b6 >> 12)) >> 16;
        x3 = ((x1 + x2) + 2) >> 2;
        unsigned int b4 = cal->ac4 * (uint32_t) (x3 + 0x8000) >> 15;
        unsigned int b7 = ((uint32_t) up - b3) * (50000 >> ctx->oss);
        int p = (b7 < 0x80000000) ? ((b7 * 2) / b4) : ((b7 / b4) * 2);
        x1 = (p >> 8) * (p >> 8);
        x1 = (x1 * 3038) >> 16;
        x2 = (-7357 * p) >> 16;
        p = p + ((x1 + x2 + 3791) >> 4);
        return p;
}

/*
 * Initiating conversions
 */
static void
bmp085_read_conversion(void *cbdata)
{
        struct bmp085_ctx *ctx = cbdata;
        bmp085_read_reg(ctx, 0xf6, ctx->read_reg_cb, ctx->read_reg_cbdata);
}

/* Take a pressure sample */
void
bmp085_sample_pressure(struct bmp085_ctx *ctx, bmp085_read_reg_cb cb, void *cbdata)
{
        uint8_t value = 0x34 | (ctx->oss << 6);
        bmp085_set_control(ctx, value);
        ctx->read_reg_cb = cb;
        ctx->read_reg_cbdata = cbdata;

        unsigned int delay;
        switch (ctx->oss) {
        case BMP085_OVERSAMPLE_0: delay = 5; break;
        case BMP085_OVERSAMPLE_1: delay = 8; break;
        case BMP085_OVERSAMPLE_2: delay = 14; break;
        case BMP085_OVERSAMPLE_3: delay = 26; break;
        default: while (1); // shouldn't happen
        }
        timeout_add(&ctx->timeout, delay, bmp085_read_conversion, ctx);
}

/* Take a temperature sample */
void
bmp085_sample_temperature(struct bmp085_ctx *ctx, bmp085_read_reg_cb cb, void *cbdata)
{
        bmp085_set_control(ctx, 0x2e);
        ctx->read_reg_cb = cb;
        ctx->read_reg_cbdata = cbdata;
        timeout_add(&ctx->timeout, 5, bmp085_read_conversion, ctx);
}

/*
 * reading calibration constants
 */
static void bmp085_read_cal(struct bmp085_ctx *ctx);

static void
bmp085_read_cal_cb(uint16_t value, void *cbdata)
{
        struct bmp085_ctx *ctx = cbdata;
        uint16_t *cal = (uint16_t *) &ctx->cal;
        cal[ctx->read_cal_offset] = value;
        ctx->read_cal_offset++;
        bmp085_read_cal(ctx);
}

static void
bmp085_read_cal(struct bmp085_ctx *ctx)
{
        if (ctx->read_cal_offset < 11) {
                bmp085_read_reg(ctx, 0xaa + 2*ctx->read_cal_offset,
                                bmp085_read_cal_cb, ctx);
        }
}

void
bmp085_init(struct bmp085_ctx *ctx)
{
        ctx->read_cal_offset = 0;
        bmp085_read_cal(ctx);
}
