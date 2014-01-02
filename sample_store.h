#pragma once
#include <mchck.h>
#include <acquire.h>

struct sample_store_read_ctx {
        struct spiflash_transaction transaction;
        struct sample  *buffer;
        unsigned int    pos;
        unsigned int    start;
        unsigned int    len;
        spi_cb         *cb;
        void           *cbdata;
};

int sample_store_read(struct sample_store_read_ctx *ctx, struct sample *buffer,
                      unsigned int start, unsigned int len,
                      spi_cb cb, void *cbdata);

int sample_store_push(const struct sample s);

unsigned int sample_store_get_count();
