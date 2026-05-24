/**
 ******************************************************************************
 * @file    ambient_system_timing.h
 * @brief   Timeouts, intervals and timing budgets for ambient firmware
 ******************************************************************************
 */

#pragma once

#include <stdint.h>
#include "ambient_feature_flags.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef AMB_WATCHDOG_TIMEOUT_MS
#if (AMB_PROFILE == AMB_PROFILE_BENCH)
#define AMB_WATCHDOG_TIMEOUT_MS 8000u
#else
#define AMB_WATCHDOG_TIMEOUT_MS 8000u
#endif
#endif

#ifndef AMB_HANDLE_PWM_ATTACK_MS
#if (AMB_PROFILE == AMB_PROFILE_BENCH)
#define AMB_HANDLE_PWM_ATTACK_MS  900u
#else
#define AMB_HANDLE_PWM_ATTACK_MS  1400u
#endif
#endif
#ifndef AMB_HANDLE_PWM_RELEASE_MS
#if (AMB_PROFILE == AMB_PROFILE_BENCH)
#define AMB_HANDLE_PWM_RELEASE_MS 1500u
#else
#define AMB_HANDLE_PWM_RELEASE_MS 2200u
#endif
#endif

#ifndef AMB_BSM_RX_TIMEOUT_MS
#define AMB_BSM_RX_TIMEOUT_MS       220u
#endif

#define AMB_WELCOME_EVENT_HOLD_MS      900u
#define AMB_UNLOCK_SIGNATURE_FADE_IN_MS   420u
#define AMB_UNLOCK_SIGNATURE_HOLD_MS      980u
#define AMB_UNLOCK_SIGNATURE_FADE_OUT_MS  1500u
#define AMB_UNLOCK_SIGNATURE_COOLDOWN_MS  50000u
#define AMB_UNLOCK_EIS_EVENT_DEBOUNCE_MS   450u
#define AMB_LOCK_EIS_EVENT_DEBOUNCE_MS     450u
#define AMB_LOCK_EVENT_PENDING_TTL_MS      1200u
#define AMB_LOCK_EVENT_SOURCE_MAX_AGE_MS   700u
#define AMB_EIS_TURNLMP_EVENT_DEBOUNCE_MS  450u
#define AMB_DOOR_OPEN_EVENT_HOLD_MS        360u
#define AMB_DOOR_OPEN_EVENT_COOLDOWN_MS    420u
#define AMB_DOOR_PRE_GLOW_MS               160u
#define AMB_HVAC_TEMP_EVENT_HOLD_MS      560u
#define AMB_HVAC_TEMP_EVENT_COOLDOWN_MS  900u
#define AMB_HVAC_TEMP_EVENT_MIN_STEP_RAW 2u
#define AMB_HVAC_TEMP_DIR_HOLD_MS        260u
#define AMB_HVAC_TEMP_EVENT_BURST_WINDOW_MS 1100u
#define AMB_HVAC_TEMP_EVENT_BURST_CYCLES    3u
#define AMB_HVAC_WAVE_FADE_OUT_MS         380u
#define AMB_HVAC_WAVE_TRAVEL_MS           920u
#define AMB_HVAC_WAVE_FADE_IN_MS          380u
#define AMB_HVAC_WAVE_GUARD_MS            320u
#define AMB_CLIMATE_MEMORY_HOLD_MS       1800u
#define AMB_CONTEXT_HOLD_DEFAULT_MS       1800u

#define AMB_DRIVE_MODE_BOOST_DURATION_MS 900u

#define AMB_AUTO_ROTATE_INTERVAL_SEC      10u
#define AMB_CROSSFADE_DURATION_MS       1600u
#define AMB_THEME_SWITCH_DEBOUNCE_MS     220u
#define AMB_FAST_BANK_SWITCH_DEBOUNCE_MS AMB_THEME_SWITCH_DEBOUNCE_MS
#define AMB_MAX_CROSSFADE_LEDS          256u

#ifndef AMB_SLEEP_TIMEOUT_SEC
#define AMB_SLEEP_TIMEOUT_SEC       4u
#endif
#ifndef AMB_SLEEP_FADE_OUT_MS
#define AMB_SLEEP_FADE_OUT_MS       2000u
#endif
#ifndef AMB_SLEEP_CANCEL_IDLE_DIV
#define AMB_SLEEP_CANCEL_IDLE_DIV   2u
#endif
#ifndef AMB_WAIT_OEM_RESLEEP_MS
#define AMB_WAIT_OEM_RESLEEP_MS     1200u
#endif

#define AMB_FLASH_SAVE_DELAY_MS     10000u

#define AMB_BRIDGE_DURATION_MS       400u
#define AMB_INTRO_DURATION_MS       4200u
#define AMB_OUTRO_DURATION_MS       1800u

#define AMB_CAN_MASTER_TX_INTERVAL_MS   200u
#define AMB_CAN_SYNC_INTERVAL_MS        500u
#define AMB_CAN_DISCOVERY_INTERVAL_MS   1500u
#define AMB_CAN_STARTUP_DISCOVERY_MS    1500u
#define AMB_CAN_ACTIVE_TIMEOUT_MS       2000u
#define AMB_IGNITION_ACTIVE_TIMEOUT_MS  6000u
#define AMB_HU_DAYNIGHT_STALE_MS        2500u
#define AMB_PARKING_WARN_HOLD_MS        220u
#define AMB_SURRILL_HANDLE_HOLD_MS      900u
#define AMB_SURRILL_OFF_CONFIRM_MS      1200u
#define AMB_PDDLLMP_HANDLE_HOLD_MS      1200u
#define AMB_PDDLLMP_OFF_CONFIRM_MS      500u
/* Keep handle/puddle reverse block shortly after R->non-R to absorb CAN phase jitter. */
#define AMB_REVERSE_HANDLE_BLOCK_HOLD_MS 2000u

#ifdef __cplusplus
}
#endif
