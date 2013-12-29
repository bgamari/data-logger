#include <mchck.h>
#include <stdlib.h>

#include "temp-gather.desc.h"
#include "acquire.h"
#include "blink.h"
#include "conductivity.h"

static struct cdc_ctx cdc;
static struct cond_sample_ctx cond_sample_ctx;

static bool cond_new_sample_cb(unsigned accum conductivity, void *cbdata)
{
        if (verbose)
                printf("cond: %3.4k\n", conductivity);
        return true;
}

static struct sample sample_buffer[4];

static void
samples_read_cb(void *cbdata)
{
        for (unsigned int i=0; i<4; i++) {
                struct sample *s = &sample_buffer[i];
                switch (s->type) {
                case TIME:
                        printf("time        %d\n", s->time);
                        break;
                case TEMPERATURE:
                        printf("temperature %.1k\n", s->temperature);
                        break;
                case CONDUCTIVITY:
                        printf("temperature %.1k\n", s->conductivity);
                        break;
                default:
                        printf("unknown     %x\n", s->time);
                }
        }
}

static void
spiflash_id_cb(void *cbdata, uint8_t mfg_id, uint8_t memtype, uint8_t capacity)
{
        printf("flash: mfg=%x memtype=%x capacity=%x\n", mfg_id, memtype, capacity);
}

unsigned int read_offset = 4;

uint8_t cmd_buffer[16];
unsigned int cmd_tail = 0;

static void
process_command(uint8_t *data, size_t len)
{
        switch (data[0]) {
        case 's':
                if (!acquire_running) {
                        start_acquire();
                } else {
                        stop_acquire();
                }
                break;
        case 'v':
                verbose = !verbose;
                printf("verbose = %d\n", verbose);
                break;
        case 'i':
                spiflash_get_id(&spiflash, spiflash_id_cb, NULL);
                break;
        case 'g':
                read_samples(&spiflash, sample_buffer, read_offset, 4,
                             samples_read_cb, NULL);
                read_offset += 4;
                break;
        case 'c':
                cond_sample(&cond_sample_ctx, cond_new_sample_cb, NULL);
                break;
        case 't':
                if (data[1] == '=') {
                        uint32_t time = strtoul((char *) &data[2], NULL, 10);
                        rtc_set_time(time);
                        rtc_start_counter();
                }
                printf("RTC time: %d\n", RTC.tsr);
                break;
        default:
                printf("Unknown command\n");
        }
}

static void
new_data(uint8_t *data, size_t len)
{
        for (unsigned int i=0; i<len; i++) {
                if (data[i] == '\n') {
                        cmd_buffer[cmd_tail] = 0;
                        process_command(cmd_buffer, cmd_tail);
                        cmd_tail = 0;
                } else {
                        cmd_buffer[cmd_tail] = data[i];
                        cmd_tail = (cmd_tail + 1) % sizeof(cmd_buffer);
                }
        }
        
        cdc_read_more(&cdc);
}

void
init_vcdc(int config)
{
        cdc_init(new_data, NULL, &cdc);
        cdc_set_stdout(&cdc);
}

void
main(void)
{
        adc_init();
        spi_init();
        spiflash_pins_init();
        timeout_init();
        rtc_init();
        usb_init(&cdc_device);
        cond_init();
        start_blink(5, 100, 100);
        sys_yield_for_frogs();
}
