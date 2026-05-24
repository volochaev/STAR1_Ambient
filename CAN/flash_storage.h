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
/* Store settings in the last Flash page. */
#define FLASH_STORAGE_BASE_ADDR    0x0801F800U  /* Last page address (128KB - 2KB) */
#define FLASH_STORAGE_PAGE_SIZE    0x800U       /* 2KB */
#define FLASH_STORAGE_MAGIC         0x53544152U  /* "STAR" in ASCII */

typedef struct {
    uint8_t valid;
    uint8_t color_id;
    uint8_t brightness_raw;
    uint8_t effect_id;
    uint32_t timestamp_ms;
} flash_modern_state_t;

/* Payload persisted in Flash page. */
typedef struct {
    uint32_t magic;              /* Magic number for validity check. */
    uint32_t crc;                /* CRC32 for integrity check. */
    uint8_t  bank_id;            /* Last bank_id (0..2). */
    uint8_t  oem_color;          /* Last OEM color (0..2). */
    uint8_t  oem_brightness_raw; /* 0..5 */
    uint8_t  night_mode;         /* 0/1 */
    uint8_t  reserved[2];        /* Reserved for alignment. */
    uint32_t reserved2[14];      /* Reserved for future extensions. */
} __attribute__((packed)) flash_storage_data_t;

/* Flash storage API. */
/**
 * @brief Load settings from Flash.
 * @return 0 on success, -1 when data is invalid or absent.
 */
int flash_storage_load(can_state_t *state);
int flash_storage_load_extended(can_state_t *state, flash_modern_state_t *modern_state);

/**
 * @brief Save settings to Flash.
 * @param state Pointer to settings snapshot to persist.
 * @return 0 on success, -1 on error.
 */
int flash_storage_save(const can_state_t *state);
int flash_storage_save_extended(const can_state_t *state, const flash_modern_state_t *modern_state);

/**
 * @brief Erase Flash page (debug/reset helper).
 * @return 0 on success, -1 on error.
 */
int flash_storage_erase(void);

#ifdef __cplusplus
}
#endif

#endif /* FLASH_STORAGE_H */
