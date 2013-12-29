#include <mchck.h>

#include "temp-gather.desc.h"
#include "acquire.h"
#include "blink.h"
#include "conductivity.h"

static struct cdc_ctx cdc;
static struct sample sample_buffer[512];

static void
sample_cb(void *cbdata)
{
        for (unsigned int i=0; i<512; i++) {
                struct sample *s = &sample_buffer[i];
                printf("%lu  %.1k\n", s->timestamp, s->temperature);
        }
}

static void
spiflash_id_cb(void *cbdata, uint8_t mfg_id, uint8_t memtype, uint8_t capacity)
{
        printf("mfg: %d\n", mfg_id);
        printf("memtype: %d\n", memtype);
        printf("capacity: %d\n", capacity);
}

static void
flash_erase_cb(void *cbdata)
{
        printf("erased\n");
}

static void
new_data(uint8_t *data, size_t len)
{
        for (; len > 0; ++data, --len) {
                switch (data[0]) {
                case '\n':
                        printf("hello\n");
                        break;
                case 's':
                        if (!acquire_running) {
                                start_acquire();
                        } else {
                                stop_acquire();
                        }
                        break;
                case 'v':
                        verbose = !verbose;
                        break;
                case 'i':
                        spiflash_get_id(&spiflash, spiflash_id_cb, NULL);
                        break;
                case 'g':
                        read_samples(&spiflash, sample_buffer, 0, 512, sample_cb, NULL);
                        break;
                case 'b':
                        start_blink(10, 200, 200);
                        break;
                case 'e':
                        printf("erase\n");
                        spiflash_erase_block(&spiflash, 0, true, flash_erase_cb, NULL);
                        break;
                case 'c':
                        cond_start();
                        break;
                case 'r':
                        printf("RTC time: %d\n", RTC.tsr);
                        break;
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
//rtc_init();
        usb_init(&cdc_device);
        cond_init();
        sys_yield_for_frogs();
}
