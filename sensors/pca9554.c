#include "pca9554.h"

void
pca9554_init(struct pca9554_ctx *ctx, uint8_t address)
{
        ctx->trans.address = address;
        ctx->trans.buffer = ctx->buffer;
}

static void
pca9554_read_reg_2(enum i2c_result result, struct i2c_transaction *trans)
{
        struct pca9554_ctx *ctx = trans->cbdata;
        ctx->trans.direction = I2C_READ;
        ctx->trans.stop = I2C_STOP;
        ctx->trans.length = 1;
        ctx->trans.cb = ctx->cb;
        ctx->trans.cbdata = ctx->cbdata;
        i2c_queue(&ctx->trans);
}

void
pca9554_read_reg(struct pca9554_ctx *ctx, enum pca9554_reg reg,
                 i2c_cb *cb, void *cbdata)
{
        ctx->buffer[0] = reg;
        ctx->trans.direction = I2C_WRITE;
        ctx->trans.stop = I2C_NOSTOP;
        ctx->trans.length = 1;
        ctx->trans.cb = pca9554_read_reg_2;
        ctx->trans.cbdata = ctx;
        ctx->cb = cb;
        ctx->cbdata = cbdata;
        i2c_queue(&ctx->trans);
}

void
pca9554_write_reg(struct pca9554_ctx *ctx, enum pca9554_reg reg, uint8_t value,
                  i2c_cb *cb, void *cbdata)
{
        ctx->buffer[0] = reg;
        ctx->buffer[1] = value;
        ctx->trans.direction = I2C_WRITE;
        ctx->trans.stop = I2C_STOP;
        ctx->trans.length = 2;
        ctx->trans.cb = cb;
        ctx->trans.cbdata = cbdata;
        i2c_queue(&ctx->trans);
}
