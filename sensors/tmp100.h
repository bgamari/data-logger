#pragma once
#include "sensor.h"

struct tmp100_ctx {
        struct i2c_transaction trans;
        uint8_t buffer[2];
};

enum tmp100_reg {
        TMP100_REG_TEMPERATURE = 0x0,
        TMP100_REG_CONFIG      = 0x1,
        TMP100_REG_T_LOW       = 0x2,
        TMP100_REG_T_HIGH      = 0x3,
};

enum tmp100_resolution {
        TMP100_RES_9BITS   = 0x0,
        TMP100_RES_10BITS  = 0x1,
        TMP100_RES_11BITS  = 0x2,
        TMP100_RES_12BITS  = 0x3,
};

struct tmp100_config {
        UNION_STRUCT_START(8);
        uint8_t sd       : 1;
        uint8_t tm       : 1;
        uint8_t pol      : 1;
        uint8_t f        : 2;
        enum tmp100_resolution r : 2;
        uint8_t os_alert : 1;
        UNION_STRUCT_END;
};

void tmp100_init(struct tmp100_ctx *ctx, uint8_t address, i2c_cb *cb, void *cbdata);
void tmp100_hack();

struct tmp100_sensor_data {
        uint8_t address;

        // internal
        struct tmp100_ctx tmp100;
        uint8_t buf[2];
        struct i2c_reg_read_ctx reg_read;
        accum value;
};

struct sensor_type tmp100_sensor_type;
