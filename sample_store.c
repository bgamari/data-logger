#include "sample_store.h"

#define MAX(a,b) ((a) > (b)) ? (a) : (b)
#define MIN(a,b) ((a) < (b)) ? (a) : (b)

#define PAGE_SIZE 256
#define SAMPLES_PER_PAGE (PAGE_SIZE / sizeof(struct sample))
#define SECTOR_SIZE 4096
#define SAMPLES_PER_SECTOR (SECTOR_SIZE / sizeof(struct sample))

static size_t flash_size = 0;

static uint32_t
sample_address(unsigned int sample_idx)
{
        unsigned int page = sample_idx / SAMPLES_PER_PAGE;
        unsigned int offset = sample_idx % SAMPLES_PER_PAGE;
        return PAGE_SIZE * page + offset * sizeof(struct sample);
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
        bool pending;
        uint32_t addr;
        struct write_sample *next;
};

static volatile unsigned int sample_idx = 0;
static volatile int last_erased_sector = -1;

/*
 * the write queue
 * a write will remain at the head until it has completed
 */
static struct write_sample *write_queue;

#define N_WRITE_SAMPLES 4
static struct write_sample write_samples[N_WRITE_SAMPLES];

static int write_sample(struct write_sample *w);

static void
sector_erased(void *cbdata)
{
        write_sample(cbdata);
}

/*
 * dispatch head of write queue
 * call in critical section
 */
static int
_dispatch_queue()
{
        struct write_sample *w = write_queue;
        if (w == NULL)
                return 0;
        
        int next_sector = (w->addr + sizeof(struct sample)) / SAMPLES_PER_SECTOR;
        if (next_sector > last_erased_sector) {
                // erase if starting new sector
                last_erased_sector = next_sector;
                return spiflash_erase_sector(&onboard_flash, &w->transaction,
                                             next_sector * SECTOR_SIZE, sector_erased, w);
        } else {
                return write_sample(w);
        }
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
        return spiflash_program_page(&onboard_flash, &w->transaction, w->addr,
                                     (const uint8_t*) &w->sample, sizeof(struct sample),
                                     sample_written, w);
}

/* call in critical section */
static int
_enqueue_sample_write(struct write_sample *w)
{
        // append write to end of queue
        w->next = NULL;
        if (write_queue == NULL) {
                write_queue = w;
                return _dispatch_queue();
        } else {
                struct write_sample *tail = write_queue;
                while (tail->next)
                        tail = tail->next;
                tail->next = w;
                return 0; // although we don't know whether dispatch will fail
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
 * initialization
 */
static void
identify_flash_cb(void *cbdata, uint8_t mfg_id, uint8_t memtype, uint8_t capacity)
{
        flash_size = spiflash_capacity_to_bytes(capacity);
}

static struct spiflash_transaction trans;

void
sample_store_init()
{
        spiflash_get_id(&onboard_flash, &trans, identify_flash_cb, NULL);
}
