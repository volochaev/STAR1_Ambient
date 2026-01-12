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

int flash_storage_load(amb_can_state_t *state)
{
    if (!state) return -1;
    
    /* Читаем данные из Flash */
    const flash_storage_data_t *flash_data = (const flash_storage_data_t *)FLASH_STORAGE_BASE_ADDR;
    
    /* Проверяем magic number */
    if (flash_data->magic != FLASH_STORAGE_MAGIC) {
        /* Данные отсутствуют или невалидны */
        return -1;
    }
    
    /* Проверяем CRC */
    uint32_t calculated_crc = crc32_calculate((const uint8_t *)flash_data, 
                                               sizeof(flash_storage_data_t) - sizeof(uint32_t));
    
    if (calculated_crc != flash_data->crc) {
        /* CRC не совпадает - данные повреждены */
        return -1;
    }
    
    /* Копируем валидные данные */
    state->extended_mode = flash_data->extended_mode;
    state->bank_id = flash_data->bank_id;
    state->theme_index = flash_data->theme_index;
    
    return 0;
}

int flash_storage_save(const amb_can_state_t *state)
{
    if (!state) return -1;
    
    HAL_StatusTypeDef status;
    FLASH_EraseInitTypeDef erase_init;
    uint32_t page_error = 0;
    
    /* Подготавливаем данные для записи */
    flash_storage_data_t data;
    memset(&data, 0xFF, sizeof(data));  /* Заполняем 0xFF (пустое состояние Flash) */
    
    data.magic = FLASH_STORAGE_MAGIC;
    data.extended_mode = state->extended_mode;
    data.bank_id = state->bank_id;
    data.theme_index = state->theme_index;
    
    /* Вычисляем CRC (без поля CRC) */
    data.crc = crc32_calculate((const uint8_t *)&data, sizeof(flash_storage_data_t) - sizeof(uint32_t));
    
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
    const flash_storage_data_t *written = (const flash_storage_data_t *)FLASH_STORAGE_BASE_ADDR;
    if (written->magic != FLASH_STORAGE_MAGIC || written->crc != data.crc) {
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
