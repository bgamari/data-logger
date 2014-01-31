#include <mchck.h>

enum spiflash_sector_size {
        SECTOR_SIZE_64KB = 16,
};

static inline uint32_t
spiflash_sector_size_to_bytes(enum spiflash_sector_size size)
{
        return 1 << size;
}

struct spiflash_params {
        uint8_t mfg_id;
        uint8_t device_id1;
        uint8_t device_id2;
        enum spiflash_sector_size sector_size : 8; // log 2
        uint16_t n_sectors;
        struct {
                UNION_STRUCT_START(8);
                uint8_t byte_program : 1;
                UNION_STRUCT_END;
        } flags;
};

extern struct spiflash_params spiflash_device_params[];
