#include "flash_list.h"

struct spi_flash_params spi_flash_table[] = {
        { 0xbf, 0x25, 0x8e,  SECTOR_SIZE_64KB,   16,  {.raw = 0x00} }, // SST25VF080B
        { 0x1f, 0x48, 0x00,  SECTOR_SIZE_64KB,  128,  {.raw = 0x00} }, // Adesto AT25DF641A
        { 0xef, 0x40, 0x14,  SECTOR_SIZE_64KB,   16,  {.raw = 0x00} }, // Winbond W25Q80BV
        { }
};
        
