#include <mchck.h>

typedef void (tcs_write_reg_cb)(enum i2c_result result, void *cbdata);

struct tcs_ctx {
        uint8_t address;
        struct i2c_reg_read_ctx reg_read;
        uint8_t buffer[2];
        union {
                tcs_write_reg_cb *write_reg_cb;
                i2c_reg_read_cb *read_reg_cb;
        };
        void *cbdata;
};

struct tcs_sample {
        uint16_t cdata, rdata, gdata, bdata;
};

enum tcs_reg {
        TCS_REG_ENABLE = 0x00,
        TCS_REG_ATIME  = 0x01,
        TCS_REG_WTIME  = 0x03,
        TCS_REG_CONFIG = 0x0d,
        TCS_REG_CONTROL = 0x0f,
        TCS_REG_STATUS = 0x13,
        TCS_REG_CDATA  = 0x14,
        TCS_REG_CDATAH = 0x15,
        TCS_REG_RDATA  = 0x16,
        TCS_REG_RDATAH = 0x17,
        TCS_REG_GDATA  = 0x18,
        TCS_REG_GDATAH = 0x19,
        TCS_REG_BDATA  = 0x1a,
        TCS_REG_BDATAH = 0x1b,
};

enum tcs_gain {
        TCS_GAIN_1  = 0x0,
        TCS_GAIN_4  = 0x1,
        TCS_GAIN_16 = 0x2,
        TCS_GAIN_60 = 0x3
};

void tcs_write_reg(struct tcs_ctx *ctx, enum tcs_reg reg, uint8_t value, tcs_write_reg_cb *cb, void *cbdata);
void tcs_read_reg(struct tcs_ctx *ctx, enum tcs_reg reg, i2c_reg_read_cb *cb, void *cbdata);
void tcs_read_data(struct tcs_ctx *ctx, struct tcs_sample *sample, i2c_reg_read_cb *cb, void *cbdata);
