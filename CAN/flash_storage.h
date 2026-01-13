/**
 ******************************************************************************
 * @file    flash_storage.h
 * @brief   Flash storage for ambient lighting settings
 * @details This module provides non-volatile storage for ambient lighting
 *          settings using the last page of internal Flash memory.
 *
 * @section Storage Layout
 * Uses last 2KB page of Flash (address 0x0801F800).
 * Data structure includes magic number and CRC32 for integrity checking.
 *
 * @section Saved Settings
 * - extended_mode: Extended mode state (0/1)
 * - bank_id: Selected theme bank (0-3)
 * - theme_index: Theme index within bank
 *
 * @section Safety
 * Settings are saved with 2 second delay after last change to minimize
 * Flash write cycles. Flash has limited write endurance (~10,000 cycles).
 *
 * @note Settings are only saved on master board.
 *
 * @version 2.2
 * @date    2025
 ******************************************************************************
 */

#ifndef FLASH_STORAGE_H
#define FLASH_STORAGE_H

#include <stdint.h>
#include "ambient.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Flash storage configuration */
/* STM32G431CBUx: 128KB Flash, page size = 2KB */
/* Используем последнюю страницу Flash для хранения настроек */
#define FLASH_STORAGE_BASE_ADDR    0x0801F800U  /* Адрес последней страницы (128KB - 2KB) */
#define FLASH_STORAGE_PAGE_SIZE    0x800U       /* 2KB */
#define FLASH_STORAGE_MAGIC         0x53544152U  /* "STAR" в ASCII */

/* Количество OEM банков (AMBER, BLUE, WHITE) */
#define FLASH_OEM_BANK_COUNT    3

/* Структура данных для сохранения в Flash */
typedef struct {
    uint32_t magic;              /* Magic number для проверки валидности */
    uint32_t crc;                /* CRC32 для проверки целостности */
    uint8_t  extended_mode;      /* Extended режим (0/1) */
    uint8_t  bank_id;            /* Bank ID (0=auto, 1=amber, 2=blue, 3=white) */
    uint8_t  theme_index;        /* Индекс темы в банке (для extended mode) */
    uint8_t  last_oem_color;     /* Последний OEM цвет (для cyclic rotation) */
    uint8_t  oem_theme_indices[FLASH_OEM_BANK_COUNT]; /* Циклические индексы тем для каждого OEM банка */
    uint8_t  reserved;           /* Резерв для выравнивания */
    uint32_t reserved2[14];      /* Резерв для будущих расширений */
} __attribute__((packed)) flash_storage_data_t;

/* Функции для работы с Flash storage */
/**
 * @brief Загрузить настройки из Flash
 * @return 0 если успешно, -1 если данные невалидны или отсутствуют
 */
int flash_storage_load(amb_can_state_t *state);

/**
 * @brief Сохранить настройки в Flash
 * @param state Указатель на структуру с настройками для сохранения
 * @return 0 если успешно, -1 при ошибке
 */
int flash_storage_save(const amb_can_state_t *state);

/**
 * @brief Стереть страницу Flash (для отладки/сброса)
 * @return 0 если успешно, -1 при ошибке
 */
int flash_storage_erase(void);

#ifdef __cplusplus
}
#endif

#endif /* FLASH_STORAGE_H */
