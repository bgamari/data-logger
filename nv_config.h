#include <mchck.h>

/*
 * non-volatile configuration data
 */
struct nv_config {
        uint32_t magic;
};

extern struct nv_config nv_config;

int nv_config_save(spi_cb cb, void *cbdata);
void nv_config_init();
