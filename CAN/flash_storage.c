/**
 ******************************************************************************
 * @file    flash_storage.c
 * @brief   Implementation of flash storage for ambient lighting settings
 * @details Provides functions to save/load settings from internal Flash memory.
 *          Uses CRC32 for data integrity checking.
 *
 * @section Implementation Notes
 * - Uses last 2KB page of Flash (address 0x0801F800)
 * - Erases page before writing (Flash requires erase before write)
 * - Disables interrupts during Flash operations for safety
 * - Validates data using magic number and CRC32
 *
 * @version 2.2
 * @date    2025
 ******************************************************************************
 */

#include "flash_storage.h"
#include "main.h"
#include "stm32g4xx_hal.h"
#include <string.h>

#define FLASH_MODERN_FMT_MAGIC 0x4D4F4452U /* "MODR" */

/* Simple CRC32 implementation (polynomial 0xEDB88320). */
static uint32_t crc32_calculate(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFU;
    const uint32_t poly = 0xEDB88320U;
    
    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ poly;
            } else {
                crc >>= 1;
            }
        }
    }
    
    return ~crc;
}

/* Compute CRC while excluding the crc field itself (forced to zero). */
static uint32_t flash_storage_calc_crc(const flash_storage_data_t *in)
{
    flash_storage_data_t tmp;
    memcpy(&tmp, in, sizeof(tmp));
    tmp.crc = 0;
    return crc32_calculate((const uint8_t *)&tmp, sizeof(tmp));
}

int flash_storage_load_extended(can_state_t *state, flash_modern_state_t *modern_state)
{
    if (!state && !modern_state) return -1;
    
    /* Read persisted payload from Flash page. */
    const flash_storage_data_t *flash_data = (const flash_storage_data_t *)FLASH_STORAGE_BASE_ADDR;
    flash_storage_data_t data;
    memcpy(&data, flash_data, sizeof(data));
    
    /* Validate magic number. */
    if (data.magic != FLASH_STORAGE_MAGIC) {
        /* Data is absent or invalid. */
        return -1;
    }
    
    /* Validate CRC. */
    uint32_t calculated_crc = flash_storage_calc_crc(&data);
    
    if (calculated_crc != data.crc) {
        /* CRC mismatch means payload is corrupted. */
        return -1;
    }
    
    /* Copy validated data to output structures. */
    if (state) {
        state->bank_id = data.bank_id;
        state->oem_color = data.oem_color;
        state->oem_brightness = (float)(data.oem_brightness_raw > 5u ? 5u : data.oem_brightness_raw) / 5.0f;
        state->night_mode = (uint8_t)(data.night_mode ? 1u : 0u);
    }

    if (modern_state) {
        uint32_t ext[3] = {0u, 0u, 0u};
        memset(modern_state, 0, sizeof(*modern_state));
        memcpy(ext, data.reserved2, sizeof(ext));
        if (ext[0] == FLASH_MODERN_FMT_MAGIC) {
            modern_state->valid = (uint8_t)(ext[1] & 0x01u);
            modern_state->color_id = (uint8_t)((ext[1] >> 8) & 0xFFu);
            modern_state->brightness_raw = (uint8_t)((ext[1] >> 16) & 0xFFu);
            modern_state->effect_id = (uint8_t)((ext[1] >> 24) & 0xFFu);
            modern_state->timestamp_ms = ext[2];
        }
    }
    
    return 0;
}

int flash_storage_load(can_state_t *state)
{
    return flash_storage_load_extended(state, NULL);
}

int flash_storage_save_extended(const can_state_t *state, const flash_modern_state_t *modern_state)
{
    if (!state) return -1;
    
    HAL_StatusTypeDef status;
    FLASH_EraseInitTypeDef erase_init;
    uint32_t page_error = 0;
    
    /* Prepare payload for write operation. */
    flash_storage_data_t data;
    memset(&data, 0xFF, sizeof(data));  /* Flash erased state is 0xFF. */
    
    data.magic = FLASH_STORAGE_MAGIC;
    data.bank_id = state->bank_id;
    data.oem_color = state->oem_color;
    data.oem_brightness_raw = (uint8_t)(state->oem_brightness * 5.0f + 0.5f);
    if (data.oem_brightness_raw > 5u) data.oem_brightness_raw = 5u;
    data.night_mode = (uint8_t)(state->night_mode ? 1u : 0u);

    if (modern_state) {
        data.reserved2[0] = FLASH_MODERN_FMT_MAGIC;
        data.reserved2[1] = ((uint32_t)(modern_state->valid & 0x01u)) |
                            (((uint32_t)modern_state->color_id) << 8) |
                            (((uint32_t)modern_state->brightness_raw) << 16) |
                            (((uint32_t)modern_state->effect_id) << 24);
        data.reserved2[2] = modern_state->timestamp_ms;
    }
    
    /* Compute CRC (crc field excluded). */
    data.crc = flash_storage_calc_crc(&data);
    
    /* Keep Flash operation atomic with IRQs disabled. */
    __disable_irq();
    
    /* Unlock Flash. */
    HAL_FLASH_Unlock();
    
    /* Erase last Flash page. */
    erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
    erase_init.Banks = FLASH_BANK_1;
    erase_init.Page = FLASH_PAGE_NB - 1;  /* Last page. */
    erase_init.NbPages = 1;
    
    status = HAL_FLASHEx_Erase(&erase_init, &page_error);
    if (status != HAL_OK) {
        HAL_FLASH_Lock();
        __enable_irq();
        return -1;
    }
    
    /* Program payload in 8-byte double words. */
    uint32_t addr = FLASH_STORAGE_BASE_ADDR;
    uint32_t words = (sizeof(flash_storage_data_t) + 7) / 8;  /* Round up to 8-byte granularity. */
    
    for (uint32_t i = 0; i < words; i++) {
        uint64_t word_data = 0;
        uint32_t offset = i * 8;
        if (offset < sizeof(flash_storage_data_t)) {
            uint32_t bytes_to_copy = sizeof(flash_storage_data_t) - offset;
            if (bytes_to_copy > 8) bytes_to_copy = 8;
            memcpy(&word_data, ((uint8_t *)&data) + offset, bytes_to_copy);
        }
        
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, addr, word_data);
        if (status != HAL_OK) {
            HAL_FLASH_Lock();
            __enable_irq();
            return -1;
        }
        addr += 8;
    }
    
    /* Lock Flash. */
    HAL_FLASH_Lock();
    
    /* Re-enable IRQs. */
    __enable_irq();
    
    /* Verify written payload. */
    flash_storage_data_t written;
    memcpy(&written, (const void *)FLASH_STORAGE_BASE_ADDR, sizeof(written));
    if (written.magic != FLASH_STORAGE_MAGIC || written.crc != flash_storage_calc_crc(&written)) {
        return -1;
    }
    
    return 0;
}

int flash_storage_save(const can_state_t *state)
{
    return flash_storage_save_extended(state, NULL);
}

int flash_storage_erase(void)
{
    HAL_StatusTypeDef status;
    FLASH_EraseInitTypeDef erase_init;
    uint32_t page_error = 0;
    
    /* Keep erase operation atomic with IRQs disabled. */
    __disable_irq();
    
    HAL_FLASH_Unlock();
    
    erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
    erase_init.Banks = FLASH_BANK_1;
    erase_init.Page = FLASH_PAGE_NB - 1;  /* Last page. */
    erase_init.NbPages = 1;
    
    status = HAL_FLASHEx_Erase(&erase_init, &page_error);
    
    HAL_FLASH_Lock();
    
    /* Re-enable IRQs. */
    __enable_irq();
    
    return (status == HAL_OK) ? 0 : -1;
}
