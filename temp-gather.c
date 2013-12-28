#include <mchck.h>

#include "acquire.h"
#include "blink.h"
#include "temp-gather.desc.h"

static struct cdc_ctx cdc;

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
                        if (!acquire_running)
                                start_acquire();
                        else
                                stop_acquire();
                        break;
                case 'v':
                        verbose = !verbose;
                        break;
                case 'i':
                        spiflash_get_id(&spiflash, spiflash_id_cb, NULL);
                        break;
                case 'e':
                        printf("erase\n");
                        spiflash_erase_block(&spiflash, 0, true, flash_erase_cb, NULL);
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
        usb_init(&cdc_device);
        spi_init();
        spiflash_pins_init();
        timeout_init();
        sys_yield_for_frogs();
}
