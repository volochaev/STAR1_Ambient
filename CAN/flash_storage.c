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

/* Простая CRC32 реализация (полином 0xEDB88320) */
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

/* Вычисляем CRC с исключением поля crc (обнуляем его перед подсчетом) */
static uint32_t flash_storage_calc_crc(const flash_storage_data_t *in)
{
    flash_storage_data_t tmp;
    memcpy(&tmp, in, sizeof(tmp));
    tmp.crc = 0;
    return crc32_calculate((const uint8_t *)&tmp, sizeof(tmp));
}

int flash_storage_load(can_state_t *state)
{
    if (!state) return -1;
    
    /* Читаем данные из Flash */
    const flash_storage_data_t *flash_data = (const flash_storage_data_t *)FLASH_STORAGE_BASE_ADDR;
    flash_storage_data_t data;
    memcpy(&data, flash_data, sizeof(data));
    
    /* Проверяем magic number */
    if (data.magic != FLASH_STORAGE_MAGIC) {
        /* Данные отсутствуют или невалидны */
        return -1;
    }
    
    /* Проверяем CRC */
    uint32_t calculated_crc = flash_storage_calc_crc(&data);
    
    if (calculated_crc != data.crc) {
        /* CRC не совпадает - данные повреждены */
        return -1;
    }
    
    /* Копируем валидные данные */
    state->bank_id = data.bank_id;
    state->theme_index = data.theme_index;
    state->last_oem_color = data.last_oem_color;
    for (int i = 0; i < FLASH_OEM_BANK_COUNT; i++) {
        state->oem_theme_indices[i] = data.oem_theme_indices[i];
    }
    
    return 0;
}

int flash_storage_save(const can_state_t *state)
{
    if (!state) return -1;
    
    HAL_StatusTypeDef status;
    FLASH_EraseInitTypeDef erase_init;
    uint32_t page_error = 0;
    
    /* Подготавливаем данные для записи */
    flash_storage_data_t data;
    memset(&data, 0xFF, sizeof(data));  /* Заполняем 0xFF (пустое состояние Flash) */
    
    data.magic = FLASH_STORAGE_MAGIC;
    data.bank_id = state->bank_id;
    data.theme_index = state->theme_index;
    data.last_oem_color = state->last_oem_color;
    for (int i = 0; i < FLASH_OEM_BANK_COUNT; i++) {
        data.oem_theme_indices[i] = state->oem_theme_indices[i];
    }
    
    /* Вычисляем CRC (без поля CRC) */
    data.crc = flash_storage_calc_crc(&data);
    
    /* Защита от прерываний во время операций с Flash */
    __disable_irq();
    
    /* Разблокируем Flash */
    HAL_FLASH_Unlock();
    
    /* Стираем страницу */
    erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
    erase_init.Banks = FLASH_BANK_1;
    erase_init.Page = FLASH_PAGE_NB - 1;  /* Последняя страница */
    erase_init.NbPages = 1;
    
    status = HAL_FLASHEx_Erase(&erase_init, &page_error);
    if (status != HAL_OK) {
        HAL_FLASH_Lock();
        __enable_irq();
        return -1;
    }
    
    /* Записываем данные по 8 байт (double word) */
    uint32_t addr = FLASH_STORAGE_BASE_ADDR;
    uint32_t words = (sizeof(flash_storage_data_t) + 7) / 8;  /* Округляем вверх до 8 байт */
    
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
    
    /* Блокируем Flash */
    HAL_FLASH_Lock();
    
    /* Восстанавливаем прерывания */
    __enable_irq();
    
    /* Проверяем записанные данные */
    flash_storage_data_t written;
    memcpy(&written, (const void *)FLASH_STORAGE_BASE_ADDR, sizeof(written));
    if (written.magic != FLASH_STORAGE_MAGIC || written.crc != flash_storage_calc_crc(&written)) {
        return -1;
    }
    
    return 0;
}

int flash_storage_erase(void)
{
    HAL_StatusTypeDef status;
    FLASH_EraseInitTypeDef erase_init;
    uint32_t page_error = 0;
    
    /* Защита от прерываний во время операций с Flash */
    __disable_irq();
    
    HAL_FLASH_Unlock();
    
    erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
    erase_init.Banks = FLASH_BANK_1;
    erase_init.Page = FLASH_PAGE_NB - 1;  /* Последняя страница */
    erase_init.NbPages = 1;
    
    status = HAL_FLASHEx_Erase(&erase_init, &page_error);
    
    HAL_FLASH_Lock();
    
    /* Восстанавливаем прерывания */
    __enable_irq();
    
    return (status == HAL_OK) ? 0 : -1;
}
