/**
  ******************************************************************************
  * @file    rgb_control.h
  * @brief   RGB灯控制模块头文件
  *
  * 功能概述：
  *   - 红灯：GPIO 直接控制（亮/灭）
  *   - 绿灯：TIM2_CH2 PWM 控制（支持常亮、常灭、呼吸灯效果）
  *   - 三种状态：绿灯常亮 / 红灯常亮 / 绿灯呼吸
  *   - KEY2 长按强制绿灯功能（保存并恢复原状态）
  *
  ******************************************************************************
  */

#ifndef __RGB_CONTROL_H
#define __RGB_CONTROL_H

#include "main.h"

/*
 * 呼吸灯参数配置
 * BREATHE_CYCLE_MS : 完整呼吸周期（亮→灭→亮）总时长，单位 ms
 * PWM_PERIOD       : PWM 周期值，对应定时器 ARR=999，即 0~999 共1000级
 * PWM_STEP_MS      : 占空比更新间隔，单位 ms
 *
 * 当前参数计算：周期2000ms，每20ms更新一次 → 半周期50步 → 每步变化 1000/50 = 20
 */
#define BREATHE_CYCLE_MS    2000
#define PWM_PERIOD          1000
#define PWM_STEP_MS         20

/*
 * RGB 工作状态枚举
 * RGB_STATE_GREEN_ON    : 绿灯常亮（红灯灭）
 * RGB_STATE_RED_ON      : 红灯常亮（绿灯灭）
 * RGB_STATE_GREEN_BREATHE : 绿灯呼吸灯效果（红灯灭）
 */
typedef enum {
    RGB_STATE_GREEN_ON = 0,
    RGB_STATE_RED_ON,
    RGB_STATE_GREEN_BREATHE
} RGB_State_t;

/*
 * 外部全局变量声明（中断与主循环共享，需加 volatile）
 * current_rgb_state : 当前 RGB 工作状态
 * saved_rgb_state   : KEY2按下时保存的状态，用于松开后恢复
 * key2_is_pressed   : KEY2按住状态标志，1=按住，0=松开
 * breathe_enabled   : 呼吸灯使能标志，1=运行，0=暂停
 */
extern volatile RGB_State_t current_rgb_state;
extern volatile RGB_State_t saved_rgb_state;
extern volatile uint8_t key2_is_pressed;
extern volatile uint8_t breathe_enabled;

/* 函数声明 ------------------------------------------------------------------*/
void RGB_Init(void);                    /* RGB模块初始化：启动PWM，默认关灯 */
void RGB_Set_Red(uint8_t on);           /* 红灯开关控制：0=灭，1=亮 */
void RGB_Set_Green(uint8_t on);         /* 绿灯常亮/常灭控制（PWM占空比0%或100%）*/
void RGB_Set_Green_PWM(uint16_t duty);  /* 绿灯PWM占空比设置：0 ~ PWM_PERIOD */
void RGB_All_Off(void);                 /* 关闭所有灯（红灯+绿灯）*/
void RGB_Set_State(RGB_State_t state);  /* 设置RGB工作状态（状态机入口）*/
void RGB_Breathe_Task(void);            /* 呼吸灯任务（主循环周期调用，内部自动计时）*/
void RGB_Force_Green_On(void);          /* 强制绿灯常亮（KEY2按下时调用，自动保存原状态）*/
void RGB_Restore_State(void);           /* 恢复到保存的状态（KEY2松开时调用）*/

#endif /* __RGB_CONTROL_H */