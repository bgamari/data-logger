#include <mchck.h>
#include "blink.h"
#include "acquire.h"
#include "config.h"

bool acquire_running = false;

struct spiflash_ctx spiflash;
static struct timeout_ctx timeout;

static unsigned int sample_idx = 0;

#define PAGE_SIZE 256
#define SAMPLES_PER_PAGE (PAGE_SIZE / sizeof(struct sample))
#define SECTOR_SIZE 4096
#define SAMPLES_PER_SECTOR (SECTOR_SIZE / sizeof(struct sample))

// start and len given in samples
int
read_samples(struct spiflash_ctx *spiflash,
             struct sample *buffer,
             unsigned int start, unsigned int len,
             spi_cb cb, void *cbdata)
{
        return spiflash_read_page(spiflash, (uint8_t*) buffer,
                                  start * sizeof(struct sample),
                                  len * sizeof(struct sample),
                                  cb, cbdata);
}

struct write_sample {
        struct spiflash_ctx spiflash;
        struct sample sample;
        bool pending;
        uint32_t addr;
        struct write_sample *next;
};

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
        if (write_queue == NULL)
                return 0;
        
        struct write_sample *w = write_queue;
        if (sample_idx % SAMPLES_PER_SECTOR == 0) {
                // erase if starting new sector
                return spiflash_erase_sector(&w->spiflash, w->addr, sector_erased, w);
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
        return spiflash_program_page(&w->spiflash, w->addr,
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
        } else {
                struct write_sample *tail = write_queue;
                while (tail->next)
                        tail = tail->next;
                tail->next = w;
        }

        return _dispatch_queue();
}

static int
push_sample(const struct sample s)
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
                w->addr = sample_idx * sizeof(struct sample);

                int ret = _enqueue_sample_write(w);
                if (ret == 0)
                        sample_idx++;
                crit_exit();
                return ret;
        }
}

static void
on_sample_cb(struct sensor *sensor, accum value, void *cbdata)
{
        push_sample((struct sample) {
                        .time = rtc_get_time(),
                        .sensor_id = sensor->sensor_id,
                        .value = value});
}

void
take_sample()
{
        start_blink(1, 50, 50);
        for (struct sensor **s = &sensors[0]; *s != NULL; s++) {
                (*s)->sample(*s);
        }
}

static void
timeout_cb(void *data)
{
        take_sample();
        timeout_add(&timeout, 500, timeout_cb, NULL);
}

void start_acquire()
{
        start_blink(2, 50, 50);
        timeout_add(&timeout, 1000, timeout_cb, NULL);
        acquire_running = true;
}

void stop_acquire()
{
        start_blink(3, 50, 50);
        timeout_cancel(&timeout);
        acquire_running = false;
}

static struct sensor_listener listener;

void acquire_init()
{
        sensor_listen(&listener, on_sample_cb, NULL);
}
