#pragma once
#include <mchck.h>

struct spiflash_write_bytes {
        struct spiflash_device *dev;
        struct spiflash_transaction trans;
        spi_cb *cb;
        void *cbdata;
        uint32_t addr;
        const uint8_t *src;
        uint8_t remaining;
};

void spiflash_write_bytes(struct spiflash_device *dev,
                          struct spiflash_write_bytes *wb,
                          uint32_t addr, const uint8_t *src, uint8_t len,
                          spi_cb cb, void *cbdata);
