#include "flash_list.h"

struct spi_flash_params spi_flash_table[] = {
        { 0xbf, 0x8e25,  SECTOR_SIZE_64KB,   16,  {.raw = 0x00} }, // SST25VF080B
        { 0x1f, 0x0048,  SECTOR_SIZE_64KB,  128,  {.raw = 0x00} }, // Adesto AT25DF641A
        { }
};
        
