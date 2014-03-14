#pragma once

typedef void (*bmp085_data_cb)(uint16_t value, void *cbdata);

// calibration data
struct bmp085_cal {
        int16_t ac1, ac2, ac3;
        uint16_t ac4, ac5, ac6;
        int16_t b1, b2;
        int16_t mb, mc, md;
};

typedef void (*bmp085_read_reg_cb)(uint16_t value, void *cbdata);

// oversampling modes
enum bmp085_oss {
        BMP085_OVERSAMPLE_0 = 0,
        BMP085_OVERSAMPLE_1 = 1,
        BMP085_OVERSAMPLE_2 = 2,
        BMP085_OVERSAMPLE_3 = 3,
};

struct bmp085_ctx {
        struct i2c_transaction trans;
        uint8_t buf[4];
        uint8_t read_cal_offset;
        struct timeout_ctx timeout;
        bmp085_read_reg_cb read_reg_cb;
        void *read_reg_cbdata;
        struct bmp085_cal cal;
        enum bmp085_oss oss;
        uint16_t last_pressure, last_temperature;
};

void bmp085_init(struct bmp085_ctx *ctx);

int bmp085_correct_temperature(struct bmp085_ctx *ctx, uint16_t ut);

int bmp085_correct_pressure(struct bmp085_ctx *ctx, uint16_t ut, uint16_t up);

void bmp085_sample_pressure(struct bmp085_ctx *ctx,
                            bmp085_read_reg_cb cb, void *cbdata);

void bmp085_sample_temperature(struct bmp085_ctx *ctx,
                               bmp085_read_reg_cb cb, void *cbdata);
