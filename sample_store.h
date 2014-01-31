#pragma once
#include <mchck.h>
#include <acquire.h>

/* reading samples */
struct sample_store_read_ctx {
        struct spiflash_transaction transaction;
};

int sample_store_read(struct sample_store_read_ctx *ctx, struct sample *buffer,
                      unsigned int start, unsigned int len,
                      spi_cb cb, void *cbdata);

unsigned int sample_store_get_count();

/* what to do when FLASH is full */
enum sample_store_full_behavior {
        WRAP_ON_FULL,
        STOP_ON_FULL,
};

void sample_store_set_full_behavior(enum sample_store_full_behavior s);
enum sample_store_full_behavior sample_store_get_full_behavior();

/* writing samples */
int sample_store_push(const struct sample s);

/* finding next empty sector after power-loss
 *
 * As we keep the sample count in RAM we generally shouldn't need to
 * do this. However, when RAM is lost (e.g. on power loss) this allows
 * us to recover and pick up where we left off.
 */
typedef void (*find_empty_sector_cb)(uint32_t addr, void *cbdata);

#define INVALID_PAGE 0xffffffff

struct find_empty_sector_ctx {
        uint32_t next_sector; // address of next page
        uint32_t buffer;
        struct spiflash_transaction trans;
        find_empty_sector_cb cb;
        void *cbdata;
};

void sample_store_find_empty_sector(struct find_empty_sector_ctx *ctx,
                                    unsigned int start_sector,
                                    find_empty_sector_cb cb, void *cbdata);

/*
 * recovery from power loss
 *
 * places the sample index at the beginning of the next empty page.
 */
typedef void (*sample_store_recover_done_cb)();

void sample_store_recover(sample_store_recover_done_cb done_cb);

/* initialization */
void sample_store_reset();

void sample_store_init();
