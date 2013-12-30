#pragma once
#include <stdbool.h>

void cond_init();

/* Conductivity in units of FIXME?
 * Return true to continue sampling, false to stop.
 */
typedef bool (cond_sample_cb)(unsigned accum conductivity, void *cbdata);

struct cond_sample_ctx {
        cond_sample_cb *cb;
        void *cbdata;
        struct cond_sample_ctx *next;
};

void cond_sample(struct cond_sample_ctx *ctx, cond_sample_cb cb, void *cbdata);

struct cond_average_ctx {
        struct cond_sample_ctx ctx;
        unsigned int nsamples;
        unsigned int remaining;
        unsigned accum accumulator;
};

void cond_average(struct cond_average_ctx *ctx, unsigned int n, void *cbdata);
unsigned accum cond_get_average(struct cond_average_ctx *ctx);

struct cond_sensor_data {
        struct cond_average_ctx ctx;
};
