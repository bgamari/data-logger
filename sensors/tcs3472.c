#include "sensors/tcs3472.h"

static void
tcs_read_reg_done(enum i2c_result result, struct i2c_transaction *transaction)
{
        struct tcs_ctx *ctx = transaction->cbdata;
        ctx->user_read_reg_cb(ctx->buffer[1], result, ctx->user_cbdata);
}

void
tcs_read_reg(struct tcs_ctx *ctx, uint8_t reg, tcs_read_reg_cb *cb, void *cbdata)
{
        ctx->buffer[0] = reg;
        ctx->user_read_reg_cb = cb;
        ctx->user_cbdata = cbdata;
        ctx->trans[0] = (struct i2c_transaction) {
                .address = ctx->address,
                .direction = I2C_WRITE,
                .stop = I2C_NOSTOP,
                .buffer = ctx->buffer,
                .length = 1,
        };
        ctx->trans[1] = (struct i2c_transaction) {
                .address = ctx->address,
                .direction = I2C_READ,
                .stop = I2C_STOP,
                .buffer = ctx->buffer,
                .length = 1,
                .cb = tcs_read_reg_done,
                .cbdata = ctx
        };
        i2c_queue(&ctx->trans[0]);
        i2c_queue(&ctx->trans[1]);
}

static void
tcs_read_data_done(enum i2c_result result, struct i2c_transaction *transaction)
{
        struct tcs_ctx *ctx = transaction->cbdata;
        ctx->user_read_cb(result, ctx->user_cbdata);
}

void
tcs_read_data(struct tcs_ctx *ctx, struct tcs_sample *buf, tcs_read_cb *cb, void *cbdata)
{
        ctx->buffer[0] = 0x14;
        ctx->user_read_cb = cb;
        ctx->user_cbdata = cbdata;
        ctx->trans[0] = (struct i2c_transaction) {
                .address = ctx->address,
                .direction = I2C_WRITE,
                .stop = I2C_NOSTOP,
                .buffer = ctx->buffer,
                .length = 1,
        };
        ctx->trans[1] = (struct i2c_transaction) {
                .address = ctx->address,
                .direction = I2C_READ,
                .stop = I2C_STOP,
                .buffer = (uint8_t *) buf,
                .length = sizeof(struct tcs_sample),
                .cb = tcs_read_data_done,
                .cbdata = ctx
        };
        i2c_queue(&ctx->trans[0]);
        i2c_queue(&ctx->trans[1]);
}

static void
tcs_write_reg_done(enum i2c_result result, struct i2c_transaction *transaction)
{
        struct tcs_ctx *ctx = transaction->cbdata;
        ctx->user_write_reg_cb(result, ctx->user_cbdata);
}

void
tcs_write_reg(struct tcs_ctx *ctx, uint8_t reg, uint8_t value, tcs_write_reg_cb *cb, void *cbdata)
{
        ctx->buffer[0] = reg;
        ctx->buffer[1] = value;
        ctx->user_write_reg_cb = cb;
        ctx->user_cbdata = cbdata;
        ctx->trans[0] = (struct i2c_transaction) {
                .address = ctx->address,
                .direction = I2C_WRITE,
                .stop = I2C_STOP,
                .buffer = ctx->buffer,
                .length = 1,
                .cb = tcs_write_reg_done,
                .cbdata = ctx
        };
        i2c_queue(&ctx->trans[0]);
}

static void
tcs_start_sampling_done(enum i2c_result result, void *cbdata)
{
        struct tcs_ctx *ctx = cbdata;
        ctx->user_write_reg_cb(result, ctx->user_cbdata);
}

static void
tcs_start_sampling_rgbc_en(void *cbdata)
{
        struct tcs_ctx *ctx = cbdata;
        tcs_write_reg(ctx, TCS_REG_ENABLE, 0x03, tcs_start_sampling_done, ctx);
}
        
static void
tcs_start_sampling_on(enum i2c_result result, void *cbdata)
{
        struct tcs_ctx *ctx = cbdata;
        if (result != I2C_RESULT_SUCCESS) {
                // TODO
                while (1);
        }
        timeout_add(&ctx->timeout, 3, tcs_start_sampling_rgbc_en, ctx);
}

void 
tcs_start_sampling(struct tcs_ctx *ctx, tcs_write_reg_cb *cb, void *cbdata)
{
        ctx->user_write_reg_cb = cb;
        ctx->user_cbdata = cbdata;
        tcs_write_reg(ctx, 0x00, 0x1, tcs_start_sampling_on, ctx);
}
