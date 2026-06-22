/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f1xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "stm32f1xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE BEGIN EV */
#include "rgb_control.h"

/*
 * 中断与主循环通信的共享变量（在 main.c 中定义）
 * 必须加 volatile：防止编译器优化导致中断修改的值主循环看不到
 *
 * key1_press_tick : KEY1按下时的时间戳（ms），记录按下时刻
 * key1_press_flag : KEY1按下标志，1=有按键事件待处理，0=已处理
 * key2_event_flag : KEY2事件标志，1=按下（下降沿），2=松开（上升沿），0=已处理
 */
extern volatile uint32_t key1_press_tick;
extern volatile uint8_t  key1_press_flag;
extern volatile uint8_t  key2_event_flag;
/* USER CODE END EV */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern TIM_HandleTypeDef htim2;
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M3 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
   while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Prefetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVCall_IRQn 0 */

  /* USER CODE END SVCall_IRQn 0 */
  /* USER CODE BEGIN SVCall_IRQn 1 */

  /* USER CODE END SVCall_IRQn 1 */
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
  /* USER CODE BEGIN PendSV_IRQn 0 */

  /* USER CODE END PendSV_IRQn 0 */
  /* USER CODE BEGIN PendSV_IRQn 1 */

  /* USER CODE END PendSV_IRQn 1 */
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32F1xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f1xx.s).                    */
/******************************************************************************/

/**
  * @brief  KEY1 外部中断服务函数（EXTI line0）
  *
  * 触发方式：下降沿触发（引脚配置为上拉输入，按键按下时电平变低）
  *
  * 第一层软件消抖原理：
  *   机械按键按下时会产生 10~20ms 的抖动，导致多次下降沿中断
  *   用 static 变量记录上次有效中断时间，50ms 内的重复中断直接忽略
  *   只保留第一次有效触发，确保一个按键动作只产生一次事件
  *
  * 处理动作：置位 key1_press_flag，通知主循环有按键事件待处理
  *           （中断中只做标志置位，不做耗时操作）
  */
void EXTI0_IRQHandler(void)
{
    static uint32_t last_interrupt_time = 0;  /* 上次有效中断时间（static 保留值）*/
    uint32_t now = HAL_GetTick();
    
    if (__HAL_GPIO_EXTI_GET_IT(KEY1_Pin) != RESET)
    {
        /* 50ms 消抖窗口：时间窗口内只响应第一次中断 */
        if ((now - last_interrupt_time) > 50)
        {
            last_interrupt_time = now;
            key1_press_tick = now;    /* 记录按下时刻 */
            key1_press_flag = 1;      /* 置位标志，通知主循环 */
        }
        
        __HAL_GPIO_EXTI_CLEAR_IT(KEY1_Pin);  /* 清除中断标志，无论是否有效都要清 */
    }
}

/**
  * @brief  KEY2 外部中断服务函数（EXTI line1）
  *
  * 触发方式：双边沿触发（按下时下降沿，松开时上升沿）
  *           引脚配置为上拉输入，按下=低电平，松开=高电平
  *
  * 第一层软件消抖原理：
  *   机械按键按下和松开时都会产生抖动，导致多次边沿中断
  *   用 static 变量记录上次有效中断时间，50ms 内的重复中断直接忽略
  *
  * 事件判断方法：
  *   进入中断后读取当前引脚电平：
  *     低电平 → 下降沿事件 → 按键按下 → key2_event_flag = 1
  *     高电平 → 上升沿事件 → 按键松开 → key2_event_flag = 2
  *
  * 处理动作：置位 key2_event_flag，通知主循环处理
  *           （中断中只做标志置位，不做耗时操作）
  */
void EXTI1_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI1_IRQn 0 */
    static uint32_t last_interrupt_time = 0;  /* 上次有效中断时间（static 保留值）*/
    uint32_t now = HAL_GetTick();
    
    if (__HAL_GPIO_EXTI_GET_IT(KEY2_Pin) != RESET)
    {
        /* 50ms 消抖窗口：时间窗口内只响应第一次中断 */
        if ((now - last_interrupt_time) > 50)
        {
            last_interrupt_time = now;
            
            /* 读引脚电平判断事件类型：低=按下，高=松开 */
            if (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET) {
                key2_event_flag = 1;   /* 按下事件 */
            } else {
                key2_event_flag = 2;   /* 松开事件 */
            }
        }
        
        __HAL_GPIO_EXTI_CLEAR_IT(KEY2_Pin);  /* 清除中断标志，无论是否有效都要清 */
    }
  /* USER CODE END EXTI1_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(KEY2_Pin);
  /* USER CODE BEGIN EXTI1_IRQn 1 */

  /* USER CODE END EXTI1_IRQn 1 */
}

/**
  * @brief This function handles TIM2 global interrupt.
  */
void TIM2_IRQHandler(void)
{
  /* USER CODE BEGIN TIM2_IRQn 0 */

  /* USER CODE END TIM2_IRQn 0 */
  HAL_TIM_IRQHandler(&htim2);
  /* USER CODE BEGIN TIM2_IRQn 1 */

  /* USER CODE END TIM2_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
