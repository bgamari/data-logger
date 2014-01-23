#include <mchck.h>

#define SECTOR_SIZE_64KB 16

struct spi_flash_params {
        uint8_t mfg_id;
        uint16_t device_id;
        uint8_t sector_size; // log 2
	uint16_t n_sectors;
	struct {
                UNION_STRUCT_START(8);
                uint8_t byte_program : 1;
                UNION_STRUCT_END;
        } flags;
};

extern struct spi_flash_params spi_flash_table[];
