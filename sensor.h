#pragma once
#include <mchck.h>

struct sensor;

typedef void (*sample_func)(struct sensor *sensor);

struct sensor_type {
        sample_func sample_fn;
        // MCU can't enter STOP modes while sensor is busy
        bool no_stop;
};

struct sensor {
        struct sensor_type *type;
        const char *name;
        const char *unit;
        uint16_t sensor_id;
        void *sensor_data;
        uint32_t last_sample_time;
        accum last_sample;
        bool busy;
};

int sensor_start_sample(struct sensor *sensor);

void sensor_new_sample_list(struct sensor *sensor, size_t elems, ...);

__attribute__((__always_inline__))
inline void
sensor_new_sample(struct sensor *sensor, unsigned int measurable, accum value, ...)
{
        (sensor_new_sample_list(sensor, __builtin_va_arg_pack_len() / 2 + 1,
                                measurable, value,
                                __builtin_va_arg_pack()));
}

typedef void (*new_sample_cb)(struct sensor *sensor,
                              uint32_t time, uint8_t measurable,
                              accum value, void *cbdata);

/*
 * listening for samples
 */
struct sensor_listener {
        struct sensor_listener *next;
        new_sample_cb new_sample;
        void *cbdata;
};

void sensor_listen(struct sensor_listener *listener,
                   new_sample_cb new_sample, void *cbdata);
