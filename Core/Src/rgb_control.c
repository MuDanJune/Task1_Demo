/**
  ******************************************************************************
  * @file    rgb_control.c
  * @brief   RGB灯控制模块实现
  *
  * 硬件连接：
  *   红灯  → PA0 (GPIO推挽输出)
  *   绿灯  → PA1 (TIM2_CH2 PWM输出)
  *   共阴极接法：高电平亮，低电平灭
  *
  * 状态机：
  *   GREEN_ON → RED_ON → GREEN_BREATHE → GREEN_ON（循环）
  *
  ******************************************************************************
  */

#include "rgb_control.h"

extern TIM_HandleTypeDef htim2;

/*
 * 全局状态变量（中断与主循环共享，需加 volatile）
 * current_rgb_state : 当前RGB工作状态（状态机当前值）
 * saved_rgb_state   : KEY2按下时的状态快照，松开后恢复用
 * key2_is_pressed   : KEY2按住标志，1=按住期间KEY1无效
 * breathe_enabled   : 呼吸灯运行标志，1=正常更新，0=暂停
 */
volatile RGB_State_t current_rgb_state = RGB_STATE_GREEN_ON;
volatile RGB_State_t saved_rgb_state = RGB_STATE_GREEN_ON;
volatile uint8_t key2_is_pressed = 0;
volatile uint8_t breathe_enabled = 1;

/*
 * 呼吸灯内部变量（仅本文件使用，static 限定作用域）
 * current_duty        : 当前PWM占空比值，范围 0 ~ PWM_PERIOD
 * breathe_direction   : 呼吸方向，1=占空比递增（渐亮），0=递减（渐暗）
 * last_breathe_update : 上次更新占空比的时间戳，用于控制更新频率
 */
static uint16_t current_duty = 0;
static uint8_t breathe_direction = 1;
static uint32_t last_breathe_update = 0;

/**
  * @brief  RGB模块初始化
  * @note   启动TIM2 PWM输出，初始状态所有灯关闭
  */
void RGB_Init(void)
{
    HAL_GPIO_WritePin(RGB_RED_GPIO_Port, RGB_RED_Pin, GPIO_PIN_RESET);
    RGB_Set_Green_PWM(0);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
}

/**
  * @brief  红灯开关控制
  * @param  on  0=灭，1=亮
  */
void RGB_Set_Red(uint8_t on)
{
    GPIO_PinState state = (on) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    HAL_GPIO_WritePin(RGB_RED_GPIO_Port, RGB_RED_Pin, state);
}

/**
  * @brief  绿灯常亮/常灭控制
  * @note   通过PWM占空比 0% / 100% 实现，统一用PWM通道控制
  * @param  on  0=灭（占空比0%），1=亮（占空比100%）
  */
void RGB_Set_Green(uint8_t on)
{
    if (on) {
        RGB_Set_Green_PWM(PWM_PERIOD);
    } else {
        RGB_Set_Green_PWM(0);
    }
}

/**
  * @brief  绿灯PWM占空比设置
  * @param  duty  占空比值，范围 0 ~ PWM_PERIOD（0~1000）
  *               0=完全熄灭，PWM_PERIOD=完全点亮
  */
void RGB_Set_Green_PWM(uint16_t duty)
{
    if (duty > PWM_PERIOD) {
        duty = PWM_PERIOD;
    }
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, duty);
    current_duty = duty;
}

/**
  * @brief  关闭所有RGB灯
  */
void RGB_All_Off(void)
{
    RGB_Set_Red(0);
    RGB_Set_Green(0);
}

/**
  * @brief  设置RGB工作状态（状态机入口函数）
  * @param  state  目标状态（枚举值）
  * @note   根据状态自动配置红灯、绿灯、呼吸使能
  */
void RGB_Set_State(RGB_State_t state)
{
    current_rgb_state = state;
    
    switch (state) {
        case RGB_STATE_GREEN_ON:        /* 绿灯常亮 */
            RGB_Set_Red(0);
            RGB_Set_Green(1);
            breathe_enabled = 0;
            break;
            
        case RGB_STATE_RED_ON:          /* 红灯常亮 */
            RGB_Set_Red(1);
            RGB_Set_Green(0);
            breathe_enabled = 0;
            break;
            
        case RGB_STATE_GREEN_BREATHE:   /* 绿灯呼吸 */
            RGB_Set_Red(0);
            breathe_enabled = 1;
            break;
            
        default:
            break;
    }
}

/**
  * @brief  呼吸灯任务（主循环中周期调用）
  *
  * 工作原理：
  *   内部用 last_breathe_update 记录上次更新时间
  *   每隔 PWM_STEP_MS（20ms）更新一次占空比
  *   递增到顶后改为递减，递减到底后改为递增，循环往复
  *
  * 呼吸周期计算：
  *   半周期步数 = (BREATHE_CYCLE_MS / 2) / PWM_STEP_MS = 1000/20 = 50 步
  *   每步占空比变化 = PWM_PERIOD / 50 = 1000/50 = 20
  *   完整周期 = 20ms × 50 × 2 = 2000ms（2秒）
  *
  * @note   breathe_enabled = 0 时直接返回，不占用CPU
  */
void RGB_Breathe_Task(void)
{
    uint32_t now;
    
    if (!breathe_enabled) {
        return;
    }
    
    now = HAL_GetTick();
    
    if ((now - last_breathe_update) >= PWM_STEP_MS) {
        last_breathe_update = now;
        
        if (breathe_direction) {                    /* 渐亮：占空比递增 */
            current_duty += 20;
            if (current_duty >= PWM_PERIOD) {
                current_duty = PWM_PERIOD;
                breathe_direction = 0;              /* 到顶，改为渐暗 */
            }
        } else {                                    /* 渐暗：占空比递减 */
            if (current_duty >= 20) {
                current_duty -= 20;
            } else {
                current_duty = 0;
                breathe_direction = 1;              /* 到底，改为渐亮 */
            }
        }
        
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, current_duty);
    }
}

/**
  * @brief  恢复到保存的状态
  * @note   KEY2松开时调用，恢复KEY2按下前的RGB状态
  */
void RGB_Restore_State(void)
{
    RGB_Set_State(saved_rgb_state);
}

/**
  * @brief  强制绿灯常亮
  * @note   KEY2按下时调用：
  *         1. 保存当前状态到 saved_rgb_state
  *         2. 关闭红灯，绿灯设为100%占空比
  *         3. 暂停呼吸灯更新
  */
void RGB_Force_Green_On(void)
{
    saved_rgb_state = current_rgb_state;
    RGB_Set_Red(0);
    RGB_Set_Green(1);
    breathe_enabled = 0;
}
