#pragma once
#include <stdbool.h>

void cond_init();

/* Conductivity in ???
 * Return true to continue sampling, false to stop.
 */
typedef bool (cond_sample_cb)(signed accum conductivity, void *cbdata);

struct cond_sample_ctx {
        cond_sample_cb *cb;
        void *cbdata;
        struct cond_sample_ctx *next;
};

void cond_sample(struct cond_sample_ctx *ctx, cond_sample_cb cb, void *cbdata);

