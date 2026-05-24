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
#include "board_selected.h" /* defines BOARD_TYPE for the whole firmware build. */
#include "board_dispatch.h"
#include "base_scene.h"
#include "director.h"
#include "event_layer.h"
#include "zones.h"
#include "ambient.h"
#include "ambient_config.h"
#include "handle_pwm.h"
#include "runtime_event_queue.h"
#include "runtime_debug_hooks.h"
#include "runtime_flow.h"
#include "app_runtime.h"
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
FDCAN_HandleTypeDef hfdcan1;

IWDG_HandleTypeDef hiwdg;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
DMA_HandleTypeDef hdma_tim2_ch1;
DMA_HandleTypeDef hdma_tim2_ch2;
DMA_HandleTypeDef hdma_tim2_ch3;
DMA_HandleTypeDef hdma_tim2_ch4;

/* USER CODE BEGIN PV */
base_scene_t g_base_scene;             // base scene
director_t g_director;                 // orchestration/director layer
oem_color_id_t g_oem_color;            // current OEM color selector

uint32_t g_last_tick_ms = 0;          // delta_ms source for runtime tick

static uint8_t g_can_protocol_started = 0;  // first OEM packet received
static uint32_t g_can_protocol_start_ms = 0;
static uint32_t g_wait_oem_enter_ms = 0u;
static uint8_t g_wait_oem_after_wake = 0u;

#if AMB_ENABLE_SLEEP_MODE
static uint8_t g_sleep_fade_active = 0;      // sleep fade-out is active
static uint32_t g_sleep_fade_start_ms = 0;   // sleep fade start timestamp
static float g_sleep_fade_start_brightness = 1.0f; // brightness at fade start
#endif
static runtime_mode_t g_runtime_state = RUNTIME_BOOT;
static runtime_flow_ctx_t g_runtime_flow;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_FDCAN1_Init(void);
static void MX_IWDG_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
#if AMB_ENABLE_SLEEP_MODE && AMB_ENABLE_WATCHDOG
static void rtc_wakeup_init_1hz(void);
static void rtc_wakeup_start_1s(void);
static void rtc_wakeup_stop(void);
#endif
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

#if AMB_ENABLE_SLEEP_MODE
/**
 * @brief Configure PA11 (FDCAN_RX) as EXTI wake source for STOP mode.
 */
static void configure_can_rx_exti_wakeup(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Reconfigure PA11 as EXTI input (falling edge). */
    GPIO_InitStruct.Pin = GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;  /* CAN dominant = low */
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_11);

    /* Enable EXTI IRQs. */
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

    /* Additional wake source from CAN transceiver WAKE/INT (PB7). */
    GPIO_InitStruct.Pin = FDCAN1_WAKEUP_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(FDCAN1_WAKEUP_GPIO_Port, &GPIO_InitStruct);
    __HAL_GPIO_EXTI_CLEAR_IT(FDCAN1_WAKEUP_Pin);
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}

/**
 * @brief Restore PA11 as FDCAN_RX after wakeup.
 */
static void restore_can_rx_af(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Disable EXTI IRQs. */
    HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
    HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_11);
    __HAL_GPIO_EXTI_CLEAR_IT(FDCAN1_WAKEUP_Pin);

    /* Restore alternate-function mapping for FDCAN. */
    GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_FDCAN1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Restore PB7 to normal GPIO input (without EXTI). */
    GPIO_InitStruct.Pin = FDCAN1_WAKEUP_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(FDCAN1_WAKEUP_GPIO_Port, &GPIO_InitStruct);
}
#endif

#if AMB_ENABLE_SLEEP_MODE && AMB_ENABLE_WATCHDOG
/**
 * @brief Initialize RTC wakeup time base (LSI -> RTC, 1Hz ck_spre).
 */
static void rtc_wakeup_init_1hz(void)
{
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();

    __HAL_RCC_LSI_ENABLE();
    while (__HAL_RCC_GET_FLAG(RCC_FLAG_LSIRDY) == RESET) {
    }

    if (__HAL_RCC_GET_RTC_SOURCE() != RCC_RTCCLKSOURCE_LSI) {
        __HAL_RCC_BACKUPRESET_FORCE();
        __HAL_RCC_BACKUPRESET_RELEASE();
        __HAL_RCC_RTC_CONFIG(RCC_RTCCLKSOURCE_LSI);
    }

    __HAL_RCC_RTC_ENABLE();
    __HAL_RCC_RTCAPB_CLK_ENABLE();

    /* Disable write protection */
    RTC->WPR = 0xCA;
    RTC->WPR = 0x53;

    /* Enter init mode */
    RTC->ICSR |= RTC_ICSR_INIT;
    while ((RTC->ICSR & RTC_ICSR_INITF) == 0u) {
    }

    /* 1Hz ck_spre from 32k-ish LSI: 32768 / ((127+1)*(255+1)) = 1Hz */
    RTC->PRER = ((uint32_t)127u << 16) | 255u;

    /* Exit init mode */
    RTC->ICSR &= ~RTC_ICSR_INIT;

    /* Keep wakeup timer disabled until sleep entry */
    RTC->CR &= ~(RTC_CR_WUTE | RTC_CR_WUTIE | RTC_CR_WUCKSEL);
    RTC->SCR = RTC_SCR_CWUTF;

    /* Enable write protection */
    RTC->WPR = 0xFF;
}

/**
 * @brief Start periodic RTC wakeup IRQ at 1-second interval.
 */
static void rtc_wakeup_start_1s(void)
{
    __HAL_RCC_RTCAPB_CLK_ENABLE();

    RTC->WPR = 0xCA;
    RTC->WPR = 0x53;

    /* Disable WUT and wait for write access */
    RTC->CR &= ~RTC_CR_WUTE;
    while ((RTC->ICSR & RTC_ICSR_WUTWF) == 0u) {
    }

    /* ck_spre (1Hz), period = WUTR+1 => 1 second */
    RTC->WUTR = 0u;
    RTC->CR = (RTC->CR & ~RTC_CR_WUCKSEL) | RTC_CR_WUCKSEL_2;
    RTC->SCR = RTC_SCR_CWUTF;

    EXTI->PR1 = EXTI_PR1_PIF20;
    EXTI->IMR1 |= EXTI_IMR1_IM20;
    EXTI->RTSR1 |= EXTI_RTSR1_RT20;
    EXTI->FTSR1 &= ~EXTI_FTSR1_FT20;

    NVIC_SetPriority(RTC_WKUP_IRQn, 0);
    NVIC_EnableIRQ(RTC_WKUP_IRQn);

    RTC->CR |= RTC_CR_WUTIE;
    RTC->CR |= RTC_CR_WUTE;

    RTC->WPR = 0xFF;
}

/**
 * @brief Stop RTC wakeup IRQ.
 */
static void rtc_wakeup_stop(void)
{
    __HAL_RCC_RTCAPB_CLK_ENABLE();

    RTC->WPR = 0xCA;
    RTC->WPR = 0x53;

    RTC->CR &= ~(RTC_CR_WUTIE | RTC_CR_WUTE);
    RTC->SCR = RTC_SCR_CWUTF;

    RTC->WPR = 0xFF;

    EXTI->IMR1 &= ~EXTI_IMR1_IM20;
    EXTI->RTSR1 &= ~EXTI_RTSR1_RT20;
    EXTI->FTSR1 &= ~EXTI_FTSR1_FT20;
    EXTI->PR1 = EXTI_PR1_PIF20;

    NVIC_DisableIRQ(RTC_WKUP_IRQn);
}
#endif

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
  MX_IWDG_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */

#if AMB_ENABLE_SLEEP_MODE && AMB_ENABLE_WATCHDOG
	rtc_wakeup_init_1hz();
#endif

	// 1) Initialize board (TIM2 + all LED lines/zones, power off by default)
	board_dispatch_led_init();
	handle_pwm_init(&htim3, TIM_CHANNEL_1);
	handle_pwm_set_brightness_pct(100u);

	// 2) Initialize CAN ambient stack (loads settings from Flash)
	can_ambient_init(&hfdcan1);
	runtime_event_queue_init();
	runtime_debug_hooks_init();

	// 3) Seed defaults (will be updated from CAN)
	g_oem_color = OEM_COLOR_AMBER;

	// 4) Initialize runtime scene/director layers
	base_scene_init(&g_base_scene);
	director_init(&g_director);
	event_layer_init();
	memset(&g_runtime_flow, 0, sizeof(g_runtime_flow));
	g_runtime_flow.runtime_state = &g_runtime_state;
	g_runtime_flow.can_protocol_started = &g_can_protocol_started;
	g_runtime_flow.can_protocol_start_ms = &g_can_protocol_start_ms;
	g_runtime_flow.wait_oem_enter_ms = &g_wait_oem_enter_ms;
	g_runtime_flow.wait_oem_after_wake = &g_wait_oem_after_wake;
	g_runtime_flow.last_tick_ms = &g_last_tick_ms;
#if AMB_ENABLE_SLEEP_MODE
	g_runtime_flow.sleep_fade_active = &g_sleep_fade_active;
	g_runtime_flow.sleep_fade_start_ms = &g_sleep_fade_start_ms;
	g_runtime_flow.sleep_fade_start_brightness = &g_sleep_fade_start_brightness;
#endif
	g_runtime_flow.base_scene = &g_base_scene;
	g_runtime_flow.director = &g_director;
	g_runtime_flow.oem_color = &g_oem_color;
	g_runtime_flow.watchdog = &hiwdg;
	g_runtime_flow.system_clock_restore = SystemClock_Config;
#if AMB_ENABLE_SLEEP_MODE
	g_runtime_flow.prepare_wakeup_io = configure_can_rx_exti_wakeup;
	g_runtime_flow.restore_after_wake = restore_can_rx_af;
#if AMB_ENABLE_WATCHDOG
	g_runtime_flow.rtc_wakeup_start = rtc_wakeup_start_1s;
	g_runtime_flow.rtc_wakeup_stop = rtc_wakeup_stop;
#endif
#endif

	runtime_flow_set_state(&g_runtime_flow, RUNTIME_WAIT_OEM);
	// In normal mode, scene stays in BASE_SCENE_IDLE until first OEM CAN packet.

	g_last_tick_ms = HAL_GetTick();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		app_runtime_tick(&g_runtime_flow,
						 &g_last_tick_ms,
						 &g_can_protocol_started,
						 &g_can_protocol_start_ms);
		if (app_runtime_should_idle_wfi()) {
			__WFI();
		}

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
  RCC_OscInitStruct.PLL.PLLN = 40;
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
  /* Stable 125 kbit/s for current clock tree (HSE=16 MHz, FDCAN from PLLQ=80 MHz).
   * NOTE: If clock tree is changed, recalculate this value explicitly. */
  hfdcan1.Init.NominalPrescaler = 40;
  hfdcan1.Init.NominalSyncJumpWidth = 1;
  hfdcan1.Init.NominalTimeSeg1 = 13;
  hfdcan1.Init.NominalTimeSeg2 = 2;
  hfdcan1.Init.DataPrescaler = 1;
  hfdcan1.Init.DataSyncJumpWidth = 1;
  hfdcan1.Init.DataTimeSeg1 = 1;
  hfdcan1.Init.DataTimeSeg2 = 1;
  hfdcan1.Init.StdFiltersNbr = 24;
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
static void MX_IWDG_Init(void)
{

  /* USER CODE BEGIN IWDG_Init 0 */

  /* USER CODE END IWDG_Init 0 */

  /* USER CODE BEGIN IWDG_Init 1 */

  /* USER CODE END IWDG_Init 1 */
  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_64;
  hiwdg.Init.Window = 4095;
  hiwdg.Init.Reload = 4095;
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN IWDG_Init 2 */
  /* USER CODE END IWDG_Init 2 */

}

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
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 169;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 499;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

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
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, CH1_EN_Pin|CH2_EN_Pin|CH4_EN_Pin|CH3_EN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_PWR_EN_GPIO_Port, LED_PWR_EN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LED_DATA_OE_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(FDCAN1_STBY_GPIO_Port, FDCAN1_STBY_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : CH1_EN_Pin CH2_EN_Pin CH4_EN_Pin CH3_EN_Pin */
  GPIO_InitStruct.Pin = CH1_EN_Pin|CH2_EN_Pin|CH4_EN_Pin|CH3_EN_Pin;
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
    board_dispatch_dma_tc(htim);
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
    FDCAN_RxHeaderTypeDef rx_header;
    uint8_t rx_data[8];
    uint8_t max_messages = 10;  /* Processing budget per callback to avoid long ISR loops. */

    if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != 0) {
        while (max_messages-- > 0 &&
               HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &rx_header, rx_data) == HAL_OK) {
            uint8_t dlc = rx_header.DataLength;
            runtime_debug_hooks_note_can_rx(rx_header.Identifier, HAL_GetTick());
            if (dlc <= 8) {  /* Validate payload length. */
                (void)runtime_event_queue_push_isr(rx_header.Identifier, rx_data, dlc);
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
