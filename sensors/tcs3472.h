#include <mchck.h>

typedef void (tcs_read_reg_cb)(uint8_t value, enum i2c_result result, void *cbdata);
typedef void (tcs_read_cb)(enum i2c_result result, void *cbdata);
typedef void (tcs_write_reg_cb)(enum i2c_result result, void *cbdata);

struct tcs_ctx {
        uint8_t address;
        struct i2c_transaction trans[2];
        struct timeout_ctx timeout;
        uint8_t buffer[2];
        tcs_write_reg_cb *write_reg_cb;
        union {
                tcs_write_reg_cb *user_write_reg_cb;
                tcs_read_reg_cb *user_read_reg_cb;
                tcs_read_cb *user_read_cb;
        };
        void *user_cbdata;
};

struct tcs_sample {
        uint16_t cdata, rdata, gdata, bdata;
};

enum tcs_reg {
        TCS_REG_ENABLE = 0x00,
        TCS_REG_STATUS = 0x13,
};

void tcs_read_reg(struct tcs_ctx *ctx, enum tcs_reg reg, tcs_read_reg_cb *cb, void *cbdata);
void tcs_read_data(struct tcs_ctx *ctx, struct tcs_sample *buf, tcs_read_cb *cb, void *cbdata);
void tcs_write_reg(struct tcs_ctx *ctx, enum tcs_reg reg, uint8_t value, tcs_write_reg_cb *cb, void *cbdata);

void tcs_start_sampling(struct tcs_ctx *ctx, tcs_write_reg_cb *cb, void *cbdata);
