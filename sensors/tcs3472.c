#include "sensors/tcs3472.h"

#define CMD_BIT 0x80
#define INCR_MODE (1<<5)

static void
tcs_write_reg_done(enum i2c_result result, struct i2c_transaction *transaction)
{
        struct tcs_ctx *ctx = transaction->cbdata;
        ctx->write_reg_cb(result, ctx->cbdata);
}

void
tcs_write_reg(struct tcs_ctx *ctx, uint8_t reg, uint8_t value, tcs_write_reg_cb *cb, void *cbdata)
{
        ctx->buffer[0] = CMD_BIT | reg;
        ctx->buffer[1] = value;
        ctx->write_reg_cb = cb;
        ctx->cbdata = cbdata;
        // re-use reg_read_ctx's i2c_transaction
        ctx->reg_read.trans = (struct i2c_transaction) {
                .address = ctx->address,
                .direction = I2C_WRITE,
                .stop = I2C_STOP,
                .buffer = ctx->buffer,
                .length = 2,
                .cb = tcs_write_reg_done,
                .cbdata = ctx
        };
        i2c_queue(&ctx->reg_read.trans);
}

#define FLIP(a) a = (a & 0xff) << 8 | (a >> 8)

static void
tcs_flip_data(uint8_t *buf, enum i2c_result result, void *cbdata)
{
        if (result == I2C_RESULT_SUCCESS) {
                struct tcs_sample *s = (struct tcs_sample *) buf;
                FLIP(s->cdata);
                FLIP(s->rdata);
                FLIP(s->gdata);
                FLIP(s->bdata);
        }

        struct tcs_ctx *ctx = cbdata;
        ctx->read_reg_cb(buf, result, ctx->cbdata);
}

void
tcs_read_data(struct tcs_ctx *ctx, struct tcs_sample *buf,
              i2c_reg_read_cb cb, void *cbdata)
{
        ctx->read_reg_cb = cb;
        ctx->cbdata = cbdata;
        i2c_reg_read(&ctx->reg_read, ctx->address, CMD_BIT | INCR_MODE | TCS_REG_CDATA,
                     (uint8_t *) buf, sizeof(struct tcs_sample), tcs_flip_data, ctx);
}

void
tcs_read_reg(struct tcs_ctx *ctx, enum tcs_reg reg,
             i2c_reg_read_cb cb, void *cbdata)
{
        i2c_reg_read(&ctx->reg_read, ctx->address, CMD_BIT | reg,
                     ctx->buffer, 1, cb, cbdata);
} 
