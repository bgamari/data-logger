#include "spiflash_utils.h"

void 
write_bytes(void *cbdata)
{
        struct spiflash_write_bytes *wb = cbdata;
        if (wb->remaining == 0) {
                wb->cb(wb->cbdata);
        } else {
                int ret = spiflash_program_page(wb->dev, &wb->trans,
                                                wb->addr, wb->src, 1,
                                                write_bytes, wb);
                if (ret != 0)
                        return; 
                wb->src++;
                wb->addr++;
                wb->remaining--;
        }
}

void spiflash_write_bytes(struct spiflash_device *dev,
                          struct spiflash_write_bytes *wb,
                          uint32_t addr, const uint8_t *src, uint8_t len,
                          spi_cb cb, void *cbdata)
{
        wb->dev = dev;
        wb->cb = cb;
        wb->cbdata = cbdata;
        wb->addr = addr;
        wb->src = src;
        wb->remaining = len;
        write_bytes(wb);
}
