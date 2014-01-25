#include <mchck.h>
#include "nv_config.h"

struct nv_config nv_config;
static struct spiflash_transaction trans;

#define NV_CONFIG_MAGIC 0xdeadbeef

static const struct nv_config default_nv_config = {
        .magic              = NV_CONFIG_MAGIC,
        .acquire_on_boot    = false,
        .sample_period      = 60,
        .name               = "unnamed-sensor",
};
  
int
nv_config_save(spi_cb cb, void *cbdata)
{
        return spiflash_program_page(&onboard_flash, &trans,
                                     0, (const uint8_t *) &nv_config, sizeof(struct nv_config),
                                     cb, cbdata);
}

static int
nv_config_load(struct nv_config *dest, spi_cb cb, void *cbdata)
{
        return spiflash_read_page(&onboard_flash, &trans,
                                  (uint8_t *) dest, 0, sizeof(struct nv_config),
                                  cb, cbdata);
}

static nv_config_loaded_cb loaded_cb = NULL;

static void
nv_config_load_done(void *cbdata)
{
        if (nv_config.magic != NV_CONFIG_MAGIC)
                memcpy(&nv_config, &default_nv_config, sizeof(struct nv_config));
        if (loaded_cb)
                loaded_cb();
}

void
nv_config_init(nv_config_loaded_cb cb)
{
        loaded_cb = cb;
        nv_config_load(&nv_config, nv_config_load_done, NULL);
}
