/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "rgb_control.h"
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
TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */
/*
 * 按键标志变量（中断与主循环共享，需加 volatile 防止编译器优化）
 * key1_press_flag : KEY1按下标志，EXTI0下降沿中断中置1，主循环处理后清0
 * key1_press_tick : KEY1按下时的时间戳（单位：ms），用于记录按下时刻
 * key2_event_flag : KEY2事件标志，1=按下（下降沿），2=松开（上升沿），主循环处理后清0
 */
volatile uint32_t key1_press_tick = 0;
volatile uint8_t  key1_press_flag = 0;
volatile uint8_t  key2_event_flag = 0;

/* KEY2按住状态标志（在 rgb_control.c 中定义）*/
extern volatile uint8_t key2_is_pressed;

/* 调试用指针：方便在线调试器中观察其他文件的全局变量 */
volatile RGB_State_t* debug_current_state = &current_rgb_state;
volatile uint8_t* debug_breathe_enabled = &breathe_enabled;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */
static void Process_Keys(void);      
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  MX_GPIO_Init();
  MX_TIM2_Init();
  
  /* USER CODE BEGIN 2 */
  RGB_Init();
  
  /* 设置初始状态：绿灯常亮 */
  RGB_Set_State(RGB_STATE_GREEN_ON);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
    
    /*
     * 主循环任务调度
     * 1. Process_Keys()   : 按键事件处理（含软件消抖 + 状态切换逻辑）
     * 2. RGB_Breathe_Task(): 呼吸灯PWM占空比更新（仅呼吸模式下生效）
     * 3. HAL_Delay(5)     : 5ms 延时，控制主循环频率约200Hz
     */
    Process_Keys();
    RGB_Breathe_Task();
    HAL_Delay(5);
    
    /* USER CODE BEGIN 3 */
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

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 71;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
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
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_TIM_MspPostInit(&htim2);
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(RGB_RED_GPIO_Port, RGB_RED_Pin, GPIO_PIN_RESET);

  /* Configure GPIO pin : RGB_RED_Pin */
  GPIO_InitStruct.Pin = RGB_RED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(RGB_RED_GPIO_Port, &GPIO_InitStruct);

  /* Configure GPIO pin : KEY1_Pin */
  GPIO_InitStruct.Pin = KEY1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(KEY1_GPIO_Port, &GPIO_InitStruct);

  /* Configure GPIO pin : KEY2_Pin */
  GPIO_InitStruct.Pin = KEY2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(KEY2_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI1_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);
}

/* USER CODE BEGIN 4 */
/**
  * @brief  按键事件处理函数（主循环中周期调用）
  *
  * 软件消抖机制（双层防护）：
  *   第一层：EXTI中断中 50ms 窗口，过滤机械抖动产生的多次中断
  *   第二层：本函数中 100~200ms 窗口，防止快速连按或回弹导致状态跳变
  *
  * 按键功能：
  *   KEY1 : 短按循环切换 RGB 状态（绿常亮 → 红常亮 → 绿呼吸 → 绿常亮...）
  *          KEY2按住时 KEY1 无效
  *   KEY2 : 按住期间强制绿灯常亮，松开后恢复之前的状态
  *
  * 关键变量：
  *   key1_press_flag : KEY1按下标志（EXTI0中断置1，此处处理后清0）
  *   key2_event_flag : KEY2事件标志（1=按下，2=松开；EXTI1中断置位，此处处理后清0）
  *   last_key1_time  : KEY1上次有效触发时间戳（第二层消抖基准）
  *   last_key2_time  : KEY2上次有效触发时间戳（第二层消抖基准）
  */
static void Process_Keys(void)
{
    /* 上次有效触发时间戳（static 保留上次调用的值）*/
    static uint32_t last_key1_time = 0;
    static uint32_t last_key2_time = 0;
    uint32_t now = HAL_GetTick();
    
    /* ---------- KEY1 处理：短按切换状态 ---------- */
    if (key1_press_flag && !key2_is_pressed)
    {
        key1_press_flag = 0;
        
        /* 第二层消抖：距上次有效触发不足 200ms 则忽略 */
        if ((now - last_key1_time) > 200)
        {
            last_key1_time = now;
            
            /* 状态循环切换：绿常亮 → 红常亮 → 绿呼吸 → 绿常亮 */
            RGB_State_t new_state;
            switch (current_rgb_state) {
                case RGB_STATE_GREEN_ON:
                    new_state = RGB_STATE_RED_ON;
                    break;
                case RGB_STATE_RED_ON:
                    new_state = RGB_STATE_GREEN_BREATHE;
                    break;
                case RGB_STATE_GREEN_BREATHE:
                    new_state = RGB_STATE_GREEN_ON;
                    break;
                default:
                    new_state = RGB_STATE_GREEN_ON;
                    break;
            }
            RGB_Set_State(new_state);
        }
    }
    
    /* ---------- KEY2 处理：长按强制绿灯 ---------- */
    if (key2_event_flag)
    {
        /* 第二层消抖：距上次状态变化不足 100ms 则忽略 */
        if ((now - last_key2_time) > 100)
        {
            last_key2_time = now;
            
            if (key2_event_flag == 1)          /* 按下：保存当前状态，强制绿灯 */
            {
                key2_event_flag = 0;
                key2_is_pressed = 1;
                RGB_Force_Green_On();
            }
            else if (key2_event_flag == 2)     /* 松开：恢复保存的状态 */
            {
                key2_event_flag = 0;
                key2_is_pressed = 0;
                RGB_Restore_State();
            }
        }
        else
        {
            key2_event_flag = 0;               /* 抖动事件，直接丢弃 */
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
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */