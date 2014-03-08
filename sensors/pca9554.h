#include <mchck.h>

struct pca9554_ctx {
        struct i2c_transaction trans;
        uint8_t buffer[2];
        i2c_cb *cb;
        void *cbdata;
};

enum pca9554_reg {
        PCA_REG_INPUT     = 0x0,
        PCA_REG_OUTPUT    = 0x1,
        PCA_REG_POLARITY  = 0x2,
        PCA_REG_CONFIG    = 0x3
};

void pca9554_init(struct pca9554_ctx *ctx, uint8_t address);

void pca9554_read_reg(struct pca9554_ctx *ctx, enum pca9554_reg reg,
                      i2c_cb *cb, void *cbdata);

void pca9554_write_reg(struct pca9554_ctx *ctx, enum pca9554_reg reg, uint8_t value,
                       i2c_cb *cb, void *cbdata);
