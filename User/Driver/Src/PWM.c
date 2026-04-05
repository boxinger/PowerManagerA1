#include "pwm.h"

/* ==============================================================================
 * 函数实现 (Function Implementation)
 * ============================================================================== */

/**
  * @brief  PWM输出初始化函数
  * @note   此函数应在 MX_TIMx_Init() 之后调用
  */
void PWM_Init_Outputs(void)
{
    /* 1. 初始化占空比为 0 (上管关，下管开) */
    __HAL_TIM_SET_COMPARE(PWM_TIM_HANDLE, PWM_CH_LEFT, 0);
    __HAL_TIM_SET_COMPARE(PWM_TIM_HANDLE, PWM_CH_RIGHT, PWM_ARR_PERIOD);

    /* 2. 开启左半桥 PWM 输出 */
    HAL_TIM_PWM_Start(PWM_TIM_HANDLE, PWM_CH_LEFT);
    HAL_TIMEx_PWMN_Start(PWM_TIM_HANDLE, PWM_CH_LEFT);

    /* 3. 开启右半桥 PWM 输出 */
    HAL_TIM_PWM_Start(PWM_TIM_HANDLE, PWM_CH_RIGHT);
    HAL_TIMEx_PWMN_Start(PWM_TIM_HANDLE, PWM_CH_RIGHT);

    /* 4. 启用主输出 (MOE) */
    __HAL_TIM_MOE_ENABLE(PWM_TIM_HANDLE);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
}

/**
  * @brief  紧急停止所有 PWM 输出
  */
void PWM_Stop_All(void)
{
    /* 1. 硬件级切断 (最快速度关断) */
    __HAL_TIM_MOE_DISABLE(PWM_TIM_HANDLE);

    /* 2. 停止 PWM 生成 */
    HAL_TIM_PWM_Stop(PWM_TIM_HANDLE, PWM_CH_LEFT);
    HAL_TIMEx_PWMN_Stop(PWM_TIM_HANDLE, PWM_CH_LEFT);
    
    HAL_TIM_PWM_Stop(PWM_TIM_HANDLE, PWM_CH_RIGHT);
    HAL_TIMEx_PWMN_Stop(PWM_TIM_HANDLE, PWM_CH_RIGHT);
    
    /* 3. 归零占空比 */
    __HAL_TIM_SET_COMPARE(PWM_TIM_HANDLE, PWM_CH_LEFT, 0);
    __HAL_TIM_SET_COMPARE(PWM_TIM_HANDLE, PWM_CH_RIGHT, PWM_ARR_PERIOD);
}

/**
  * @brief  设置左半桥占空比 - 原始数值
  */
void PWM_Set_Left_Duty_Raw(uint16_t ccr_val)
{
    /* 软限幅保护 */
    if (ccr_val > PWM_ARR_PERIOD)
    {
        ccr_val = PWM_ARR_PERIOD;
    }
    
    __HAL_TIM_SET_COMPARE(PWM_TIM_HANDLE, PWM_CH_LEFT, ccr_val);
}

/**
  * @brief  设置右半桥占空比 - 原始数值
  */
void PWM_Set_Right_Duty_Raw(uint16_t ccr_val)
{
    /* 软限幅保护 */
    if (ccr_val > PWM_ARR_PERIOD)
    {
        ccr_val = PWM_ARR_PERIOD;
    }

    __HAL_TIM_SET_COMPARE(PWM_TIM_HANDLE, PWM_CH_RIGHT, ccr_val);
}

/**
  * @brief  设置左半桥占空比 - 百分比控制 (优化版)
  * @param  duty_percent: 目标占空比 (0.0f ~ 100.0f)
  */
void PWM_Set_Left_Duty_Percent(float duty_percent)
{
    uint16_t ccr_val;

    /* 输入参数钳位 */
    if (duty_percent < PWM_MIN_DUTY_PERCENT) 
    {
        duty_percent = PWM_MIN_DUTY_PERCENT;
    }
    else if (duty_percent > PWM_MAX_DUTY_PERCENT) 
    {
        duty_percent = PWM_MAX_DUTY_PERCENT;
    }

    /* 计算 CCR 值 (使用预计算系数，无除法) */
    /* 公式: CCR = Percent * 3.6 (假设ARR=360) */
    ccr_val = (uint16_t)(duty_percent * PWM_PERCENT_TO_CCR_COEFF);

    /* 调用 Raw 函数进行设置 */
    PWM_Set_Left_Duty_Raw(ccr_val);
}

/**
  * @brief  设置右半桥占空比 - 反向百分比控制
  * @param  duty_percent: 目标占空比 (0.0f ~ 100.0f)
  */
void PWM_Set_Right_Duty_Percent_Inv(float duty_percent)
{
    uint16_t ccr_val;

    /* 输入参数钳位 */
    if (duty_percent < PWM_MIN_DUTY_PERCENT) 
    {
        duty_percent = PWM_MIN_DUTY_PERCENT;
    }
    else if (duty_percent > PWM_MAX_DUTY_PERCENT) 
    {
        duty_percent = PWM_MAX_DUTY_PERCENT;
    }

    /* 计算 CCR 值 */
    ccr_val = (uint16_t)(duty_percent * PWM_PERCENT_TO_CCR_COEFF);

    /* 调用 Raw 函数进行设置 */
    PWM_Set_Right_Duty_Raw(ccr_val);
}
