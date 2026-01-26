/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/*
 * ВАЖНО: Включите нужный board файл для вашей платы!
 * Раскомментируйте одну из строк ниже в зависимости от типа платы:
 */
#include "board_door_fl.h"      // Front-Left door
// #include "board_door_fr.h"   // Front-Right door
// #include "board_door_rl.h"   // Rear-Left door
// #include "board_door_rr.h"   // Rear-Right door
// #include "board_dashboard.h" // Dashboard/Instrument Panel
// #include "board_rear.h"      // Rear ambient

/* Каждый board_xxx.h автоматически определяет BOARD_TYPE */
#include "scene_player.h"
#include "presets.h"
#include "zones.h"
#include "ambient.h"
#include "features.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* DEMO_MODE теперь определен в main.h */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
FDCAN_HandleTypeDef hfdcan1;

IWDG_HandleTypeDef hiwdg;

TIM_HandleTypeDef htim2;
DMA_HandleTypeDef hdma_tim2_ch1;
DMA_HandleTypeDef hdma_tim2_ch2;
DMA_HandleTypeDef hdma_tim2_ch3;
DMA_HandleTypeDef hdma_tim2_ch4;

/* USER CODE BEGIN PV */
scene_player_t g_player;             // плеер сцен
ws_theme_id_t g_current_theme;      // текущая тема
oem_color_id_t g_oem_color;          // текущий "OEM цвет" (amber/blue/white)

uint32_t g_last_tick_ms = 0;   // для delta_ms в player_tick

#if DEMO_MODE
uint32_t g_last_theme_switch_ms = 0;  // когда последний раз меняли тему (для демо режима)
static uint8_t g_was_master = 0;  // предыдущий статус master (для отслеживания перехода)
#endif

#if !DEMO_MODE
static uint8_t g_waiting_for_can = 1;  // Ждём первый CAN пакет перед запуском ленты
#endif
static uint8_t g_can_protocol_started = 0;  // OEM пакет получен, CAN протокол запущен
static uint32_t g_can_protocol_start_ms = 0;

#if AMB_ENABLE_SLEEP_MODE
static uint8_t g_sleep_fade_active = 0;   // Флаг активного затухания перед сном
static uint32_t g_sleep_fade_start_ms = 0; // Время начала затухания
static float g_sleep_fade_start_brightness = 1.0f; // Начальная яркость при затухании
#endif

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_FDCAN1_Init(void);
static void MX_IWDG_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

#if AMB_ENABLE_SLEEP_MODE && !DEMO_MODE
/**
 * @brief Настроить PA11 (FDCAN_RX) как EXTI для пробуждения из STOP mode
 */
static void configure_can_rx_exti_wakeup(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Переконфигурируем PA11 как вход с EXTI на falling edge */
    GPIO_InitStruct.Pin = GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;  /* CAN dominant = low */
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Включаем EXTI прерывание */
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

/**
 * @brief Восстановить PA11 как FDCAN_RX после пробуждения
 */
static void restore_can_rx_af(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Отключаем EXTI */
    HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_11);

    /* Восстанавливаем AF для FDCAN */
    GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_FDCAN1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}
#endif

/**
 * @brief Получить указатель на главную strip для текущего board
 * @return Указатель на главную ws2812_t strip
 */
static ws2812_t* get_main_strip(void)
{
#if (BOARD_TYPE == BOARD_TYPE_FL)
    extern ws2812_t g_fl_strip;
    return &g_fl_strip;
#elif (BOARD_TYPE == BOARD_TYPE_FR)
    extern ws2812_t g_fr_strip;
    return &g_fr_strip;
#elif (BOARD_TYPE == BOARD_TYPE_RL)
    extern ws2812_t g_rl_strip;
    return &g_rl_strip;
#elif (BOARD_TYPE == BOARD_TYPE_RR)
    extern ws2812_t g_rr_strip;
    return &g_rr_strip;
#elif (BOARD_TYPE == BOARD_TYPE_DASHBOARD)
    extern ws2812_t g_dashboard_strip;
    return &g_dashboard_strip;
#elif (BOARD_TYPE == BOARD_TYPE_REAR)
    extern ws2812_t g_rear_strip;
    return &g_rear_strip;
#else
    return NULL;
#endif
}

/**
 * @brief Инициализация board LED системы
 */
static void board_led_init(void)
{
#if (BOARD_TYPE == BOARD_TYPE_FL)
    board_fl_led_init();
#elif (BOARD_TYPE == BOARD_TYPE_FR)
    board_fr_led_init();
#elif (BOARD_TYPE == BOARD_TYPE_RL)
    board_rl_led_init();
#elif (BOARD_TYPE == BOARD_TYPE_RR)
    board_rr_led_init();
#elif (BOARD_TYPE == BOARD_TYPE_DASHBOARD)
    board_dashboard_led_init();
#elif (BOARD_TYPE == BOARD_TYPE_REAR)
    board_rear_led_init();
#endif
}

/**
 * @brief Рендеринг всех LED зон текущего board
 */
static void board_led_render_all(void)
{
#if (BOARD_TYPE == BOARD_TYPE_FL)
    board_fl_led_render_all();
#elif (BOARD_TYPE == BOARD_TYPE_FR)
    board_fr_led_render_all();
#elif (BOARD_TYPE == BOARD_TYPE_RL)
    board_rl_led_render_all();
#elif (BOARD_TYPE == BOARD_TYPE_RR)
    board_rr_led_render_all();
#elif (BOARD_TYPE == BOARD_TYPE_DASHBOARD)
    board_dashboard_led_render_all();
#elif (BOARD_TYPE == BOARD_TYPE_REAR)
    board_rear_led_render_all();
#endif
}

/**
 * @brief Демо режим: автоматическое переключение тем (только для master)
 * @param now_ms Текущее время в миллисекундах
 * @note Вызывается только если DEMO_MODE == 1 и плата является master
 */
static void demo_mode_update(uint32_t now_ms)
{
#if DEMO_MODE
    uint8_t is_master = can_ambient_is_master();

    // Если плата только что стала master, сбрасываем таймер переключения
    if (is_master && !g_was_master) {
        g_last_theme_switch_ms = now_ms;
        g_was_master = 1;
        return;  // Не переключаем тему сразу после становления master
    }

    g_was_master = is_master;

    if (!is_master) {
        return;  // Демо режим работает только на master
    }

    // Переключение темы раз в 20 секунд
    if ((now_ms - g_last_theme_switch_ms) >= 20000u) {
        ws2812_t *main_strip = get_main_strip();
        if (main_strip) {
            // В демо режиме используем текущий g_oem_color (начальное значение BLUE)
            // и обновляем его из CAN только если пришло реальное значение (не 0 = дефолтное AMBER)
            extern volatile amb_can_state_t g_amb_can;
            __disable_irq();
            oem_color_id_t oem_col = (oem_color_id_t)g_amb_can.oem_color;
            __enable_irq();

            // Обновляем g_oem_color только если пришло реальное значение через CAN (не 0 = дефолтное)
            // Это предотвращает переключение на AMBER при старте, если g_oem_color был инициализирован как BLUE
            if (oem_col != 0 && oem_col != g_oem_color) {
                g_oem_color = oem_col;
            }

            const ws_theme_bank_t *bank = ws_theme_get_bank(g_oem_color);
            if (bank) {
                ws_theme_id_t next = ws_theme_bank_next(bank, g_current_theme);
                g_current_theme = next;

                player_start_theme(main_strip, &g_player, g_current_theme);
                player_start_intro(main_strip, &g_player); // мягкий вход в новую тему
            }
        }
        g_last_theme_switch_ms = now_ms;
    }
#else
    (void)now_ms;  // Suppress unused parameter warning
#endif
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_FDCAN1_Init();
#if AMB_ENABLE_WATCHDOG
  MX_IWDG_Init();
#endif
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

	// 1) Инициализируем board (TIM2 + все ws2812_t, зоны, питание выкл)
	board_led_init();

	// 2) Инициализация CAN ambient системы (загружает настройки из flash)
	can_ambient_init(&hfdcan1);

	// 3) Начальные значения (будут обновлены из CAN)
	g_oem_color = OEM_COLOR_AMBER;
	g_current_theme = ws_theme_default_for_oem(g_oem_color);

	// 4) Инициализируем плеер
	player_init(&g_player, g_current_theme);

#if DEMO_MODE
	// В демо режиме запускаем intro сразу
	{
		ws2812_t *main_strip = get_main_strip();
		if (main_strip) {
			player_start_intro(main_strip, &g_player);
		}
	}
#endif
	// В обычном режиме плеер остаётся в PST_IDLE до получения первого CAN пакета

	g_last_tick_ms = HAL_GetTick();
#if DEMO_MODE
	g_last_theme_switch_ms = g_last_tick_ms;
#endif

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		uint32_t now = HAL_GetTick();
		uint32_t dt = now - g_last_tick_ms;

		// Защита от переполнения dt (если система была в sleep или произошел сбой)
		if (dt > 1000u) {
			dt = 16u;  // Максимальный разумный dt для анимаций (~60 FPS)
		}

		g_last_tick_ms = now;

		ws2812_t *main_strip = get_main_strip();

		// Запускаем CAN протокол только после первого OEM пакета
		uint8_t oem_received = can_ambient_oem_received();
		if (oem_received && !g_can_protocol_started) {
			g_can_protocol_started = 1;
			g_can_protocol_start_ms = now;
		}
		if (oem_received) {
			// Обновляем роль master/slave (discovery и failover)
			can_ambient_update_role(now);
		}

#if !DEMO_MODE
		// В обычном режиме ждём первый CAN пакет перед запуском ленты
		if (g_waiting_for_can) {
			if (oem_received) {
				// Первый OEM пакет получен - выбираем тему и запускаем intro
				extern volatile amb_can_state_t g_amb_can;
				__disable_irq();
				oem_color_id_t oem_col = (oem_color_id_t)g_amb_can.oem_color;
				uint8_t theme_idx = g_amb_can.oem_theme_indices[oem_col];
				__enable_irq();

				g_oem_color = oem_col;

				// Выбираем тему на основе OEM цвета и сохранённого индекса
				const ws_theme_bank_t *bank = ws_theme_get_bank(oem_col);
				if (bank && bank->count > 0) {
					if (theme_idx == 0xFF || theme_idx >= bank->count) {
						theme_idx = 0;  // Первая тема если индекс невалидный
					}
					g_current_theme = bank->themes[theme_idx];
				} else {
					g_current_theme = ws_theme_default_for_oem(oem_col);
				}

				// Инициализируем плеер с выбранной темой
				player_init(&g_player, g_current_theme);

				// Запускаем intro
				if (main_strip) {
					player_start_intro(main_strip, &g_player);
				}

				g_waiting_for_can = 0;
			} else {
				// Ещё не получили OEM пакет - ждём
				continue;
			}
		}
#endif

		// Обновляем ambient систему из CAN (синхронизация яркости и OEM цвета)
		// В демо режиме тема управляется через demo_mode_update(), поэтому
		// can_ambient_update() обновляет только яркость и OEM цвет
		if (main_strip) {
			can_ambient_update(main_strip, &g_player);
		}

		// Синхронизируем g_oem_color с CAN состоянием (для авто-ротации)
		{
			extern volatile amb_can_state_t g_amb_can;
			__disable_irq();
			oem_color_id_t oem_col = (oem_color_id_t)g_amb_can.oem_color;
			__enable_irq();
			g_oem_color = oem_col;
		}

		// Демо режим: автоматическое переключение тем (только если DEMO_MODE == 1 и master)
		// Вызывается после can_ambient_update(), чтобы демо режим мог переопределить тему
		demo_mode_update(now);

#if AMB_ENABLE_SLEEP_MODE && !DEMO_MODE
		// Sleep mode: проверяем таймаут и выполняем плавное затухание перед сном
		can_ambient_check_sleep_timeout();

		/* Если во время fade/outro пришла новая CAN активность — отменяем засыпание */
		if (g_sleep_fade_active && (can_ambient_get_idle_time_ms() < (AMB_SLEEP_TIMEOUT_SEC * 1000u / 2u))) {
			g_sleep_fade_active = 0;
			can_ambient_clear_sleep_request();
			if (main_strip) {
				ws_release_brightness(main_strip);
			}
		}

		if (can_ambient_should_sleep() && !g_sleep_fade_active) {
			// Начинаем плавное затухание
			g_sleep_fade_active = 1;
			g_sleep_fade_start_ms = now;
			g_sleep_fade_start_brightness = g_player.calc_brightness;
		}

		if (g_sleep_fade_active) {
			uint32_t fade_elapsed = now - g_sleep_fade_start_ms;
			if (fade_elapsed >= AMB_SLEEP_FADE_OUT_MS) {
				// Затухание завершено - переходим в сон
				g_sleep_fade_active = 0;
				can_ambient_clear_sleep_request();
				uint8_t sleep_cancelled = 0;

				// Запускаем outro перед сном
				if (main_strip && g_player.stage == PST_SCENE) {
					player_start_outro(main_strip, &g_player);
				}

				// Ждём завершения outro (или пропускаем если уже IDLE)
				while (g_player.stage == PST_OUTRO) {
					/* Прерываем outro, если CAN снова активен (возврат к работе) */
					if (can_ambient_get_idle_time_ms() < (AMB_SLEEP_TIMEOUT_SEC * 1000u / 2u)) {
						sleep_cancelled = 1;
						can_ambient_clear_sleep_request();
						if (main_strip) {
							ws_release_brightness(main_strip);
						}
						break;
					}
					uint32_t outro_now = HAL_GetTick();
					uint32_t outro_dt = outro_now - g_last_tick_ms;
					if (outro_dt > 100u) outro_dt = 16u;
					g_last_tick_ms = outro_now;
					player_tick(main_strip, &g_player, outro_dt);

					// Вычисляем прогресс outro для зон
					uint32_t outro_elapsed = outro_now - g_player.outro.start_ms;
					uint32_t outro_duration = g_player.outro.duration_ms ? g_player.outro.duration_ms : 1u;
					if (outro_elapsed > outro_duration) outro_elapsed = outro_duration;
					float outro_t = (float)outro_elapsed / (float)outro_duration;
					zones_apply_scene(&g_player);  // Сначала рисуем сцену
					zones_apply_outro(&g_player, outro_t);  // Потом затухание
					board_led_render_all();
					HAL_Delay(10);
				}

				if (!sleep_cancelled) {
					/* Отпускаем принудительную яркость перед сном */
					if (main_strip) {
						ws_release_brightness(main_strip);
					}

					/* Enter sleep mode */
					can_ambient_enter_sleep();

					/* Configure EXTI on CAN RX for wakeup */
					configure_can_rx_exti_wakeup();

					/* Enter STOP mode (wakeup via EXTI from CAN RX) */
					HAL_SuspendTick();
					HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

					/* Woke up - restore operation */
					SystemClock_Config();  /* Restore clock after STOP */
					HAL_ResumeTick();
					restore_can_rx_af();   /* Restore AF for FDCAN */

					can_ambient_exit_sleep();

					// Ждём первый CAN пакет заново
					g_waiting_for_can = 1;
					g_can_protocol_started = 0;
					g_can_protocol_start_ms = 0;
					can_ambient_reset_oem_received();
					continue;
				} else {
					/* Outro прерван CAN активностью — продолжаем обычную работу */
					if (main_strip) {
						ws_release_brightness(main_strip);
					}
				}
			} else {
				// Плавное затухание яркости
				float fade_progress = (float)fade_elapsed / (float)AMB_SLEEP_FADE_OUT_MS;
				float fade_brightness = g_sleep_fade_start_brightness * (1.0f - fade_progress);
				if (fade_brightness < 0.0f) fade_brightness = 0.0f;

				// Применяем затухание к главной ленте
				if (main_strip) {
					ws_force_brightness(main_strip, fade_brightness);
				}
			}
		}
#endif

		// Обновляем player (главная strip)
		if (main_strip) {
			player_tick(main_strip, &g_player, dt);
		}

		float t_norm = 0.0f;

		switch (g_player.stage) {
		case PST_INTRO:
		{
		    // прогресс intro: используем oneshot.intro.{start_ms,duration_ms}
		    // Используем уже вычисленное now вместо повторного вызова HAL_GetTick()
		    uint32_t elapsed  = now - g_player.intro.start_ms;
		    uint32_t duration = g_player.intro.duration_ms ? g_player.intro.duration_ms : 1u;
		    if (elapsed > duration) elapsed = duration;
		    t_norm = (float)elapsed / (float)duration;

		    zones_apply_intro(&g_player, t_norm);
		}
		break;

		case PST_SCENE:
		    zones_apply_scene(&g_player);
		    break;

		case PST_BRIDGE:
		{
		    uint32_t elapsed  = now - g_player.t0_ms;
		    uint32_t duration = AMB_BRIDGE_DURATION_MS ? AMB_BRIDGE_DURATION_MS : 1u;
		    if (elapsed > duration) elapsed = duration;
		    t_norm = (float)elapsed / (float)duration;
		    zones_apply_bridge(&g_player, t_norm);
		}
		break;

		case PST_OUTRO:
		{
		    // Используем уже вычисленное now вместо повторного вызова HAL_GetTick()
		    uint32_t elapsed  = now - g_player.outro.start_ms;
		    uint32_t duration = g_player.outro.duration_ms ? g_player.outro.duration_ms : 1u;
		    if (elapsed > duration) elapsed = duration;
		    t_norm = (float)elapsed / (float)duration;

		    zones_apply_outro(&g_player, t_norm);
		}
		break;

		default:
		    // PST_IDLE/PST_BRIDGE — можно не трогать зоны или просто не обновлять
		    break;
		}

		// Отправляем все зоны на ленты (рендеринг всех LED)
		board_led_render_all();

		// Master отправляет пакеты синхронизации
		static uint32_t last_master_send_ms = 0;
		static uint32_t last_sync_send_ms = 0;
		static uint32_t last_discovery_send_ms = 0;

		// CAN TX: после OEM пакета, discovery-only окно, затем master/sync/ext
		{
			uint8_t allow_can_tx = oem_received;
#if AMB_ENABLE_SLEEP_MODE && !DEMO_MODE
			if (g_sleep_fade_active || can_ambient_should_sleep() || can_ambient_is_sleeping()) {
				allow_can_tx = 0;
			}
			if (allow_can_tx && (can_ambient_get_idle_time_ms() >= AMB_CAN_ACTIVE_TIMEOUT_MS)) {
				allow_can_tx = 0;
			}
#endif
			uint8_t startup_discovery_only = 0;
			if (allow_can_tx && g_can_protocol_started) {
				uint32_t elapsed;
				if (now >= g_can_protocol_start_ms) {
					elapsed = now - g_can_protocol_start_ms;
				} else {
					elapsed = UINT32_MAX;
				}
				startup_discovery_only = (elapsed < AMB_CAN_STARTUP_DISCOVERY_MS);
			}

			if (allow_can_tx) {
				// Master packet каждые 100мс
				if (can_ambient_is_master() && !startup_discovery_only &&
				    (now - last_master_send_ms) >= AMB_CAN_MASTER_TX_INTERVAL_MS) {
					can_ambient_send_master_packet();
					last_master_send_ms = now;
				}

				// Sync packet каждые 250мс
				if (can_ambient_is_master() && !startup_discovery_only &&
				    (now - last_sync_send_ms) >= AMB_CAN_SYNC_INTERVAL_MS) {
					can_ambient_send_sync_packet();
					last_sync_send_ms = now;
				}

				// Discovery packet каждые 1000мс (все платы)
				if ((now - last_discovery_send_ms) >= AMB_CAN_DISCOVERY_INTERVAL_MS) {
					can_ambient_send_discovery_packet();
					last_discovery_send_ms = now;
				}
			}
		}

		/* Non-blocking delay: use WFI for power saving when loop runs faster than 1ms.
		 * WFI wakes CPU on any interrupt (CAN, SysTick, DMA, etc.) */
		if (dt == 0u) {
			__WFI();
		}

		HAL_Delay(1);

#if AMB_ENABLE_WATCHDOG
		/* Feed watchdog to prevent reset */
		HAL_IWDG_Refresh(&hiwdg);
#endif
	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV2;
  RCC_OscInitStruct.PLL.PLLN = 80;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV4;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief FDCAN1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_FDCAN1_Init(void)
{

  /* USER CODE BEGIN FDCAN1_Init 0 */

  /* USER CODE END FDCAN1_Init 0 */

  /* USER CODE BEGIN FDCAN1_Init 1 */

  /* USER CODE END FDCAN1_Init 1 */
  hfdcan1.Instance = FDCAN1;
  hfdcan1.Init.ClockDivider = FDCAN_CLOCK_DIV1;
  hfdcan1.Init.FrameFormat = FDCAN_FRAME_CLASSIC;
  hfdcan1.Init.Mode = FDCAN_MODE_NORMAL;
  hfdcan1.Init.AutoRetransmission = ENABLE;
  hfdcan1.Init.TransmitPause = DISABLE;
  hfdcan1.Init.ProtocolException = DISABLE;
  hfdcan1.Init.NominalPrescaler = 40;
  hfdcan1.Init.NominalSyncJumpWidth = 1;
  hfdcan1.Init.NominalTimeSeg1 = 13;
  hfdcan1.Init.NominalTimeSeg2 = 2;
  hfdcan1.Init.DataPrescaler = 1;
  hfdcan1.Init.DataSyncJumpWidth = 1;
  hfdcan1.Init.DataTimeSeg1 = 1;
  hfdcan1.Init.DataTimeSeg2 = 1;
  hfdcan1.Init.StdFiltersNbr = 0;
  hfdcan1.Init.ExtFiltersNbr = 0;
  hfdcan1.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
  if (HAL_FDCAN_Init(&hfdcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN FDCAN1_Init 2 */

  /* USER CODE END FDCAN1_Init 2 */

}

/**
  * @brief IWDG Initialization Function
  * @param None
  * @retval None
  */
#if AMB_ENABLE_WATCHDOG
static void MX_IWDG_Init(void)
{

  /* USER CODE BEGIN IWDG_Init 0 */

  /* USER CODE END IWDG_Init 0 */

  /* USER CODE BEGIN IWDG_Init 1 */

  /* USER CODE END IWDG_Init 1 */
  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_64;
  hiwdg.Init.Window = 4095;
  /* LSI ~32kHz, prescaler 64 -> 500 Hz -> 2 ms per tick */
  hiwdg.Init.Reload = (AMB_WATCHDOG_TIMEOUT_MS * 500u) / 1000u;
  if (hiwdg.Init.Reload > 4095u) {
      hiwdg.Init.Reload = 4095u;  /* Max value */
  }
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN IWDG_Init 2 */
  /* USER CODE END IWDG_Init 2 */

}
#else
static void MX_IWDG_Init(void)
{
  /* Watchdog disabled at compile time */
}
#endif

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 199;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMAMUX1_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  /* DMA1_Channel2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);
  /* DMA1_Channel3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);
  /* DMA1_Channel4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, CH1_EN_Pin|CH2_EN_Pin|CH3_EN_Pin|CH4_EN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_PWR_EN_GPIO_Port, LED_PWR_EN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LED_DATA_OE_Pin|FDCAN1_STBY_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : CH1_EN_Pin CH2_EN_Pin CH3_EN_Pin CH4_EN_Pin */
  GPIO_InitStruct.Pin = CH1_EN_Pin|CH2_EN_Pin|CH3_EN_Pin|CH4_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LED_PWR_EN_Pin LED_DATA_OE_Pin FDCAN1_STBY_Pin */
  GPIO_InitStruct.Pin = LED_PWR_EN_Pin|LED_DATA_OE_Pin|FDCAN1_STBY_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : FDCAN1_WAKEUP_Pin */
  GPIO_InitStruct.Pin = FDCAN1_WAKEUP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(FDCAN1_WAKEUP_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
#if defined(CH1_EN_Pin) && defined(CH1_EN_GPIO_Port)
  HAL_GPIO_WritePin(CH1_EN_GPIO_Port, CH1_EN_Pin, GPIO_PIN_RESET);
  GPIO_InitStruct.Pin = CH1_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CH1_EN_GPIO_Port, &GPIO_InitStruct);
#endif

#if defined(CH2_EN_Pin) && defined(CH2_EN_GPIO_Port)
  HAL_GPIO_WritePin(CH2_EN_GPIO_Port, CH2_EN_Pin, GPIO_PIN_RESET);
  GPIO_InitStruct.Pin = CH2_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CH2_EN_GPIO_Port, &GPIO_InitStruct);
#endif

#if defined(CH3_EN_Pin) && defined(CH3_EN_GPIO_Port)
  HAL_GPIO_WritePin(CH3_EN_GPIO_Port, CH3_EN_Pin, GPIO_PIN_RESET);
  GPIO_InitStruct.Pin = CH3_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CH3_EN_GPIO_Port, &GPIO_InitStruct);
#endif

#if defined(CH4_EN_Pin) && defined(CH4_EN_GPIO_Port)
  HAL_GPIO_WritePin(CH4_EN_GPIO_Port, CH4_EN_Pin, GPIO_PIN_RESET);
  GPIO_InitStruct.Pin = CH4_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CH4_EN_GPIO_Port, &GPIO_InitStruct);
#endif

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    // Обработка DMA завершения для всех зон текущего board
#if (BOARD_TYPE == BOARD_TYPE_FL)
    extern ws2812_t g_fl_strip;
    extern ws2812_t g_fl_handle;
    extern ws2812_t g_fl_storage;
    extern ws2812_t g_fl_footwell;
    ws_dma_tc_isr(&g_fl_strip,   htim);
    ws_dma_tc_isr(&g_fl_handle,  htim);
    ws_dma_tc_isr(&g_fl_storage, htim);
    ws_dma_tc_isr(&g_fl_footwell, htim);
#elif (BOARD_TYPE == BOARD_TYPE_FR)
    extern ws2812_t g_fr_strip;
    extern ws2812_t g_fr_handle;
    extern ws2812_t g_fr_storage;
    extern ws2812_t g_fr_footwell;
    ws_dma_tc_isr(&g_fr_strip,   htim);
    ws_dma_tc_isr(&g_fr_handle,  htim);
    ws_dma_tc_isr(&g_fr_storage, htim);
    ws_dma_tc_isr(&g_fr_footwell, htim);
#elif (BOARD_TYPE == BOARD_TYPE_RL)
    extern ws2812_t g_rl_strip;
    extern ws2812_t g_rl_handle;
    extern ws2812_t g_rl_storage;
    extern ws2812_t g_rl_footwell;
    ws_dma_tc_isr(&g_rl_strip,   htim);
    ws_dma_tc_isr(&g_rl_handle,  htim);
    ws_dma_tc_isr(&g_rl_storage, htim);
    ws_dma_tc_isr(&g_rl_footwell, htim);
#elif (BOARD_TYPE == BOARD_TYPE_RR)
    extern ws2812_t g_rr_strip;
    extern ws2812_t g_rr_handle;
    extern ws2812_t g_rr_storage;
    extern ws2812_t g_rr_footwell;
    ws_dma_tc_isr(&g_rr_strip,   htim);
    ws_dma_tc_isr(&g_rr_handle,  htim);
    ws_dma_tc_isr(&g_rr_storage, htim);
    ws_dma_tc_isr(&g_rr_footwell, htim);
#elif (BOARD_TYPE == BOARD_TYPE_DASHBOARD)
    extern ws2812_t g_dashboard_strip;
    extern ws2812_t g_dashboard_center;
    extern ws2812_t g_dashboard_ac_vents;
    extern ws2812_t g_dashboard_footwell;
    ws_dma_tc_isr(&g_dashboard_strip,   htim);
    ws_dma_tc_isr(&g_dashboard_center,  htim);
    ws_dma_tc_isr(&g_dashboard_ac_vents, htim);
    ws_dma_tc_isr(&g_dashboard_footwell, htim);
#elif (BOARD_TYPE == BOARD_TYPE_REAR)
    extern ws2812_t g_rear_strip;
    extern ws2812_t g_rear_handle;
    extern ws2812_t g_rear_storage;
    extern ws2812_t g_rear_footwell;
    ws_dma_tc_isr(&g_rear_strip,   htim);
    ws_dma_tc_isr(&g_rear_handle,  htim);
    ws_dma_tc_isr(&g_rear_storage, htim);
    ws_dma_tc_isr(&g_rear_footwell, htim);
#endif
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
    FDCAN_RxHeaderTypeDef rx_header;
    uint8_t rx_data[8];
    uint8_t max_messages = 10;  /* Лимит обработки за один вызов для защиты от бесконечного цикла */

    if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != 0) {
        while (max_messages-- > 0 &&
               HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &rx_header, rx_data) == HAL_OK) {
            uint8_t dlc = rx_header.DataLength;
            if (dlc <= 8) {  /* Валидация длины данных */
                can_ambient_process_rx(rx_header.Identifier, rx_data, dlc);
            }
        }
    }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
