#include "sample_store.h"
#include "flash_list.h"
#include "spiflash_utils.h"

#define MAX(a,b) ((a) > (b)) ? (a) : (b)
#define MIN(a,b) ((a) < (b)) ? (a) : (b)

// number of reserved sectors at beginning of addressing space
#define RESERVED_SECTORS 1

static bool sample_store_ready = false;
static struct spiflash_params *flash_params = NULL;
static uint32_t flash_size = 0;

static enum sample_store_full_behavior full_behavior = STOP_ON_FULL;

#define SECTOR_SIZE 4096
#define SAMPLES_PER_SECTOR (SECTOR_SIZE / sizeof(struct sample))

typedef unsigned int sector_n;
typedef uint32_t flash_addr;

static sector_n
sample_idx_to_sector(unsigned int sample_idx)
{
        return sample_idx / SAMPLES_PER_SECTOR + RESERVED_SECTORS;
}

static flash_addr
sample_idx_to_address(unsigned int sample_idx)
{
        sector_n sector = sample_idx_to_sector(sample_idx);
        unsigned int offset = sample_idx % SAMPLES_PER_SECTOR;
        return SECTOR_SIZE * sector + offset * sizeof(struct sample);
}

static uint32_t
address_to_sample_idx(uint32_t addr)
{
        sector_n sector = addr / SECTOR_SIZE;
        unsigned int offset = addr % SAMPLES_PER_SECTOR;
        return (sector - RESERVED_SECTORS) * SAMPLES_PER_SECTOR + offset;
}

/*
 * reading samples
 */
int
sample_store_read(struct spiflash_transaction *trans, struct sample *buffer,
                  unsigned int start, unsigned int nsamples,
                  spi_cb cb, void *cbdata)
{
        if (sample_idx_to_address(start+nsamples) >= flash_size)
                return 1;
        return spiflash_read_page(&onboard_flash, trans,
                                  (uint8_t*) buffer,
                                  sample_idx_to_address(start),
                                  nsamples * sizeof(struct sample),
                                  cb, cbdata);
}

void
sample_store_set_full_behavior(enum sample_store_full_behavior s)
{
        full_behavior = s;
}

enum sample_store_full_behavior
sample_store_get_full_behavior()
{
        return full_behavior;
}

/*
 * writing samples
 *
 * we need to ensure that we only issue once write at a time, hence
 * the queue.
 */
struct write_sample {
        struct sample sample;
        struct spiflash_write_bytes write_bytes;
        bool pending;
        uint32_t addr;
        struct write_sample *next;
};

static volatile unsigned int sample_idx = 0;
static volatile int last_erased_sector = RESERVED_SECTORS - 1;

/*
 * the write queue
 * a write will remain at the head until it has completed
 */
static struct write_sample *write_queue;

#define N_WRITE_SAMPLES 4
static struct write_sample write_samples[N_WRITE_SAMPLES];

static int write_sample(struct write_sample *w);

static void
ensure_erased(void *cbdata)
{
        struct write_sample *w = cbdata;
        /* We add one here to ensure we've always erased one sector ahead of where
         * we are writing. This allows us to easily reconstruct where
         * we last wrote to on power-loss
         */
        int next_sector = w->addr / SECTOR_SIZE + 1;
        if (next_sector > last_erased_sector) {
                // erase one sector and check again
                last_erased_sector++;
                // NOTE: we are reusing write_bytes' transaction
                spiflash_erase_sector(&onboard_flash, &w->write_bytes.trans,
                                      last_erased_sector * SECTOR_SIZE,
                                      ensure_erased, w);
        } else {
                write_sample(w);
        }
}

/*
 * dispatch head of write queue
 * call in critical section
 */
static void
_dispatch_queue()
{
        struct write_sample *w = write_queue;
        if (w == NULL)
                return;
        ensure_erased(w);
}

static void
sample_written(void *cbdata)
{
        struct write_sample *w = cbdata;
        crit_enter();
        w->pending = false;
        write_queue = w->next;
        _dispatch_queue();
        crit_exit();
}

static int
write_sample(struct write_sample *w)
{
        spiflash_write_bytes(&onboard_flash, &w->write_bytes, w->addr,
                             (const uint8_t*) &w->sample, sizeof(struct sample),
                             sample_written, w);
        return 0;
}

/* call in critical section */
static int
_enqueue_sample_write(struct write_sample *w)
{
        // append write to end of queue
        w->next = NULL;
        if (write_queue == NULL) {
                write_queue = w;
                _dispatch_queue();
                return 0;
        } else {
                struct write_sample *tail = write_queue;
                while (tail->next)
                        tail = tail->next;
                tail->next = w;
                return 0; // although we don't know whether _dispatch_queue will fail
        }
}

int
sample_store_push(const struct sample s)
{
        if (!sample_store_ready)
                return 3;

        crit_enter();

        // handle FLASH full condition
        uint32_t addr = sample_idx_to_address(sample_idx);
        if (addr >= flash_size) {
                switch (full_behavior) {
                case WRAP_ON_FULL:
                        sample_store_reset();
                        break;
                case STOP_ON_FULL:
                        crit_exit();
                        return 4;
                }
        }

        // find available write_sample
        struct write_sample *w = NULL;
        for (unsigned int i=0; i<N_WRITE_SAMPLES; i++) {
                if (!write_samples[i].pending)
                        w = &write_samples[i];
        }
        if (w == NULL) {
                crit_exit();
                return 1;
        } else {
                w->pending = true;
                w->sample = s;
                w->addr = addr;
                if (w->addr > flash_size) {
                        crit_exit();
                        return 2;
                }

                int ret = _enqueue_sample_write(w);
                if (ret == 0)
                        sample_idx++;
                crit_exit();
                return ret;
        }
}

unsigned int
sample_store_get_count()
{
        return sample_idx;
}

/*
 * Identifying next empty sector after power-loss
 *
 * By virtue of the fact that the next unused FLASH sector has always
 * been cleared, we can identify it by looking at the first byte of
 * each sector and check whether it has been cleared. The first sector
 * with a first byte of 0xff is the next unused sector.
 */
void
find_sector_cb(void *cbdata)
{
        struct find_empty_sector_ctx *ctx = cbdata;
        if (ctx->buffer == 0xffffffff) {
                // we've found our sector
                ctx->cb(ctx->next_sector, ctx->cbdata);
        } else {
                ctx->next_sector += SECTOR_SIZE;
                if (ctx->next_sector < flash_size) {
                        int res = spiflash_read_page(&onboard_flash, &ctx->trans,
                                                     (uint8_t *) &ctx->buffer,
                                                     ctx->next_sector, 4, 
                                                     find_sector_cb, ctx);
                        if (res)
                                ctx->cb(INVALID_SECTOR, ctx->cbdata);
                } else {
                        ctx->cb(INVALID_SECTOR, ctx->cbdata);
                }
        }
}

void
sample_store_find_empty_sector(struct find_empty_sector_ctx *ctx,
                               unsigned int start_sector,
                               find_empty_sector_cb cb, void *cbdata)
{
        ctx->next_sector = SECTOR_SIZE * (start_sector + RESERVED_SECTORS);
        ctx->cb = cb;
        ctx->cbdata = cbdata;
        spiflash_read_page(&onboard_flash, &ctx->trans,
                           (uint8_t *) &ctx->buffer, ctx->next_sector, 4, 
                           find_sector_cb, ctx);
}

/*
 * recovery from power loss
 */
typedef void (*sample_store_recover_done_cb)();
struct recover_ctx {
        bool running;
        sample_store_recover_done_cb cb;
        struct find_empty_sector_ctx find_empty_sector;
        uint32_t pos;
        struct sample sample;
} recover_ctx;

/* look for first invalid sample */
static void
sample_store_recover_find_sample(void *cbdata)
{
        struct recover_ctx *ctx = cbdata;
        if (ctx->sample.time == 0xffffffff) {
                // found first invalid sample
                sample_idx = ctx->pos - 1;
                last_erased_sector = sample_idx_to_sector(ctx->pos);
                sample_store_ready = true;
                if (ctx->cb)
                        ctx->cb();
        } else {
                // WARNING: we are reusing find_empty_sector's spiflash_transaction
                int res = sample_store_read(&ctx->find_empty_sector.trans,
                                            &ctx->sample,
                                            ctx->pos, 1,
                                            sample_store_recover_find_sample,
                                            ctx);
                if (res) {
                        // error (e.g. end of FLASH)
                        sample_store_reset();
                        if (ctx->cb)
                                ctx->cb();
                }
                ctx->pos += 1;
        }
}

static void
sample_store_recover_empty_sector_found(uint32_t addr, void *cbdata)
{
        struct recover_ctx *ctx = cbdata;
        if (addr != INVALID_SECTOR) {
                ctx->pos = address_to_sample_idx(addr) - SAMPLES_PER_SECTOR;
                ctx->sample.time = 0; // initialize as a valid sample
                sample_store_recover_find_sample(ctx);
        } else {
                sample_store_reset();
                if (ctx->cb)
                        ctx->cb();
        }
}

int
sample_store_recover(sample_store_recover_done_cb done_cb)
{
        if (recover_ctx.running)
                return 1;
        sample_store_ready = false;
        sample_store_find_empty_sector(&recover_ctx.find_empty_sector, 0,
                                       sample_store_recover_empty_sector_found,
                                       &recover_ctx);
        return 0;
}
        
/*
 * initialization
 */

/* reset sample_idx to beginning of store */
void
sample_store_reset()
{
        crit_enter();
        sample_idx = 0;
        last_erased_sector = RESERVED_SECTORS - 1;
        sample_store_ready = true;
        crit_exit();
}

static struct spiflash_transaction trans;

static void
flash_recovered_cb()
{}

static void
flash_unprotected_cb(void *cbdata)
{
        sample_store_recover(flash_recovered_cb);
}

static void
identify_flash_cb(void *cbdata, uint8_t mfg_id, uint8_t memtype, uint8_t capacity)
{
        if (mfg_id == 0)
                return;

        for (struct spiflash_params *i = spiflash_device_params; i->mfg_id != 0x00; i++) {
                if (mfg_id == i->mfg_id
                    && memtype == i->device_id1
                    && capacity == i->device_id2) {
                        flash_params = i;
                        flash_size = spiflash_block_size_to_bytes(i->block_size)
                                * i->n_blocks;
                        break;
                }

        }
        spiflash_set_protection(&onboard_flash, &trans, false, flash_unprotected_cb, NULL);
}

void
sample_store_init()
{
        spiflash_get_id(&onboard_flash, &trans, identify_flash_cb, NULL);
}
