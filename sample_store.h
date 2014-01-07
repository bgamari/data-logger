#pragma once
#include <mchck.h>
#include <acquire.h>

/* reading samples */
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

unsigned int sample_store_get_count();

/* writing samples */
int sample_store_push(const struct sample s);

/* finding next empty page after power-loss
 *
 * As we keep the sample count in RAM we generally shouldn't need to
 * do this. However, when RAM is lost (e.g. on power loss) this allows
 * us to recover and pick up where we left off.
 */
typedef void (*find_empty_page_cb)(uint32_t addr, void *cbdata);

#define INVALID_PAGE 0xffffffff

struct find_empty_page_ctx {
        uint32_t next_page; // address of next page
        uint32_t buffer;
        struct spiflash_transaction trans;
        find_empty_page_cb cb;
        void *cbdata;
};

void sample_store_find_empty_page(struct find_empty_page_ctx *ctx,
                                  unsigned int start_page,
                                  find_empty_page_cb cb, void *cbdata);

/*
 * recovery from power loss
 *
 * places the sample index at the beginning of the next empty page.
 */
typedef void (*sample_store_recover_done_cb)();

void sample_store_recover(sample_store_recover_done_cb done_cb);

/* initialization */
void sample_store_init();
