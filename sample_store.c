#include "sample_store.h"
#include "flash_list.h"
#include "spiflash_utils.h"

#define MAX(a,b) ((a) > (b)) ? (a) : (b)
#define MIN(a,b) ((a) < (b)) ? (a) : (b)

#define PAGE_SIZE 256
#define SAMPLES_PER_PAGE (PAGE_SIZE / sizeof(struct sample))
#define SECTOR_SIZE 4096
#define SAMPLES_PER_SECTOR (SECTOR_SIZE / sizeof(struct sample))

// number of reserved pages at beginning of addressing space
#define RESERVED_SECTORS 1

static struct spi_flash_params *flash_params = NULL;
static uint32_t flash_size = 0;

static uint32_t
sample_address(unsigned int sample_idx)
{
        unsigned int sector = sample_idx / SAMPLES_PER_SECTOR + RESERVED_SECTORS;
        unsigned int offset = sample_idx % SAMPLES_PER_SECTOR;
        return SECTOR_SIZE * sector + offset * sizeof(struct sample);
}

/*
 * reading samples
 */

static void
sample_store_read_cb(void *cbdata)
{
        struct sample_store_read_ctx *ctx = cbdata;
        if (ctx->pos >= ctx->start + ctx->len) {
                ctx->cb(ctx->cbdata);
        } else {
                unsigned int to_read = MIN(ctx->start + ctx->len - ctx->pos,
                                           SAMPLES_PER_PAGE);
                uint32_t start_addr = sample_address(ctx->pos);
                int ret = spiflash_read_page(&onboard_flash, &ctx->transaction,
                                             (uint8_t*) &ctx->buffer[ctx->pos - ctx->start],
                                             start_addr, to_read * sizeof(struct sample),
                                             sample_store_read_cb, ctx);
                ret = ret;
                ctx->pos += to_read;
        }
}

// start and len given in samples
int
sample_store_read(struct sample_store_read_ctx *ctx, struct sample *buffer,
                  unsigned int start, unsigned int len,
                  spi_cb cb, void *cbdata)
{
        ctx->buffer = buffer;
        ctx->pos    = start;
        ctx->start  = start;
        ctx->len    = len;
        ctx->cb     = cb;
        ctx->cbdata = cbdata;
        sample_store_read_cb(ctx);
        return 0;
}

/*
 * writing samples
 *
 * we need to ensure that we only issue once write at a time, hence
 * the queue.
 */
struct write_sample {
        struct sample sample;
        struct spiflash_transaction transaction;
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
                spiflash_erase_sector(&onboard_flash, &w->transaction,
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
        // find available write_sample
        struct write_sample *w = NULL;
        crit_enter();
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
                w->addr = sample_address(sample_idx);
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
                                ctx->cb(INVALID_PAGE, ctx->cbdata);
                } else {
                        ctx->cb(INVALID_PAGE, ctx->cbdata);
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

static void
sample_store_recover_cb(uint32_t addr, void *cbdata)
{
        sample_store_recover_done_cb cb = cbdata;
        if (addr != INVALID_PAGE) {
                sample_idx = (addr - RESERVED_SECTORS * SECTOR_SIZE) / sizeof(struct sample);
                last_erased_sector = addr / SECTOR_SIZE - 1;
        } else {
                sample_idx = 0;
                last_erased_sector = -1;
        }
        if (cb)
                cb();
}

struct find_empty_sector_ctx recover_ctx;

void
sample_store_recover(sample_store_recover_done_cb done_cb)
{
        sample_store_find_empty_sector(&recover_ctx, 0,
                                       sample_store_recover_cb, done_cb);
}
        
/*
 * initialization
 */
static struct spiflash_transaction trans;

static void
flash_unprotected_cb(void *cbdata) {}

static void
identify_flash_cb(void *cbdata, uint8_t mfg_id, uint8_t memtype, uint8_t capacity)
{
        for (struct spi_flash_params *i = spi_flash_table; i->mfg_id != 0x00; i++) {
                if (mfg_id == i->mfg_id
                    && memtype == i->device_id1
                    && capacity == i->device_id2) {
                        flash_params = i;
                        flash_size = (1 << i->sector_size) * i->n_sectors;
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
