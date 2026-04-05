#ifndef __PWM_H
#define __PWM_H

#ifdef __cplusplus
extern "C" {
#endif

/* 包含 main.h 以获取系统级定义 */
#include "main.h"
/* 包含 tim.h 以获取定时器句柄定义 (htim1) */
#include "tim.h"

/* ==============================================================================
 * 1. 核心参数配置 (Configuration Parameters)
 * ============================================================================== */

/**
 * @brief  PWM 自动重装载值 (ARR)
 * @note   当前配置: 72MHz / (100kHz * 2) = 360
 */
#define PWM_ARR_PERIOD          720

/**
 * @brief  最大允许占空比百分比 (0.0f - 100.0f)
 * @note   预留死区和自举电容充电时间
 */
#define PWM_MAX_DUTY_PERCENT    98.0f

/**
 * @brief  最小允许占空比百分比
 */
#define PWM_MIN_DUTY_PERCENT    2.0f

/* ---------------- 预计算系数 (优化核心) ---------------- */

/**
 * @brief  百分比转CCR 预计算系数
 * @note   编译器会执行常量折叠 (Constant Folding)，运行时直接使用结果。
 * 原公式: CCR = (Percent * ARR) / 100
 * 优化后: CCR = Percent * COEFF
 * 例如 ARR=360时, COEFF = 3.6。运行时只需做一次浮点乘法，消除了除法开销。
 */
#define PWM_PERCENT_TO_CCR_COEFF  ((float)PWM_ARR_PERIOD / 100.0f)


/* ==============================================================================
 * 2. 硬件抽象层 (Hardware Abstraction Layer)
 * ============================================================================== */

/* 定义使用的定时器句柄 */
#define PWM_TIM_HANDLE          (&htim1)

/* 定义通道映射 */
#define PWM_CH_LEFT             TIM_CHANNEL_1   /* 左半桥 */
#define PWM_CH_RIGHT            TIM_CHANNEL_2   /* 右半桥 */


/* ==============================================================================
 * 3. API 接口声明 (Public Interface)
 * ============================================================================== */

void PWM_Init_Outputs(void);
void PWM_Stop_All(void);

/* 原始数值控制 (0 ~ PWM_ARR_PERIOD) */
void PWM_Set_Left_Duty_Raw(uint16_t ccr_val);
void PWM_Set_Right_Duty_Raw(uint16_t ccr_val);

/* 百分比控制 (0.0 ~ 100.0) - 使用预计算加速 */
void PWM_Set_Left_Duty_Percent(float duty_percent);
void PWM_Set_Right_Duty_Percent_Inv(float duty_percent);

#ifdef __cplusplus
}
#endif

#endif /* __PWM_H */
