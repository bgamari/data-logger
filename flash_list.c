#include "flash_list.h"

struct spiflash_params spiflash_device_params[] = {
        { 0xbf, 0x25, 0x8e,  BLOCK_SIZE_64KB,   16,  {.raw = 0x00} }, // SST25VF080B
        { 0x1f, 0x48, 0x00,  BLOCK_SIZE_64KB,  128,  {.raw = 0x00} }, // Adesto AT25DF641A
        { 0xef, 0x40, 0x14,  BLOCK_SIZE_64KB,   16,  {.raw = 0x00} }, // Winbond W25Q80
        { 0xef, 0x40, 0x17,  BLOCK_SIZE_64KB,  128,  {.raw = 0x00} }, // Winbond W25Q64
        { }
};
        
