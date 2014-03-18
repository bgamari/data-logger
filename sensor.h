#pragma once
#include <mchck.h>

struct sensor;

typedef void (*sample_func)(struct sensor *sensor);

struct measurable {
        uint8_t id;
        const char *name;
        const char *unit;
};

struct sensor_type {
        sample_func sample_fn;
        // MCU can't enter STOP modes while sensor is busy
        bool no_stop;
        uint32_t n_measurables;
        struct measurable *measurables;
};

struct sensor {
        struct sensor_type *type;
        const char *name;
        uint16_t sensor_id;
        void *sensor_data;
        accum *values;
        uint32_t last_sample_time;
        bool busy;
};


int sensor_start_sample(struct sensor *sensor);

void sensor_sample_failed(struct sensor *sensor);

void sensor_new_sample(struct sensor *sensor, accum *values);

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

/* get last measured value */
accum sensor_get_value(struct sensor *sensor, unsigned int measurable);
