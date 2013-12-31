#pragma once
#include <mchck.h>

struct sensor;

typedef void (*sample_func)(struct sensor *sensor);

struct sensor {
        struct sensor_type *type;
        const char *name;
        uint16_t sensor_id;
        void *sensor_data;
        sample_func sample;
};

void sensor_new_sample(struct sensor *sensor, accum value);

typedef void (*new_sample_cb)(struct sensor *sensor, accum value, void *cbdata);

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
