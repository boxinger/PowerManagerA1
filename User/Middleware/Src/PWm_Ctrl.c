#include "pwm_ctrl.h"
#include <math.h> 
#include <stddef.h> 
#include <stdbool.h>

/* ==============================================================================
 * 私有变量与内部函数
 * ============================================================================== */
static PWM_CTRL_Status_TypeDef PWM_CTRL_StatusStructure = {0};
static PID_Handle_t hPID_Voltage;
static PID_Handle_t hPID_Current;

/* 复位 PID */
static void PID_Reset(PID_Handle_t *pid)
{
    pid->Integral = 0.0f;
    pid->PrevError = 0.0f;
}

/* 原始 PID 计算 (不含积分更新) */
static float PID_Compute_Raw(PID_Handle_t *pid, float target, float measured)
{
    float error = target - measured;
    float p_out = pid->Kp * error;
    float d_out = pid->Kd * (error - pid->PrevError);
    pid->PrevError = error;
    return (p_out + pid->Integral + d_out);
}

/* 浮点限幅辅助函数 */
static float Float_Clamp(float val, float min, float max)
{
    if (val > max) return max;
    if (val < min) return min;
    return val;
}

/* * 应用占空比限制 
 * 优先级: 硬件绝对限制 > 逻辑限制
 */
static float Apply_Duty_Limit(float raw_duty)
{
    // 1. 逻辑限制: 33.33% ~ 66.67% (0.5Vin ~ 2.0Vin)
    float limited = Float_Clamp(raw_duty, PWM_LOGIC_MIN_DUTY, PWM_LOGIC_MAX_DUTY);
    
    // 2. 硬件绝对限制: 0% ~ 95%
    limited = Float_Clamp(limited, PWM_ABS_MIN_DUTY, PWM_ABS_MAX_DUTY);
    
    return limited;
}

/* 方向判断逻辑 */
static PWM_CTRL_Direction_t PWM_Ctrl_Alg_Check_Direction(void)
{
    float v_left = PWM_CTRL_StatusStructure.Port_V_Left_V;
    float v_right = PWM_CTRL_StatusStructure.Port_V_Right_V;

    if (v_left > PWM_CTRL_VOLTAGE_THRESHOLD_V && v_right < PWM_CTRL_VOLTAGE_THRESHOLD_V)
        return PWM_CTRL_DIR_L_TO_R;
    else if (v_right > PWM_CTRL_VOLTAGE_THRESHOLD_V && v_left < PWM_CTRL_VOLTAGE_THRESHOLD_V)
        return PWM_CTRL_DIR_R_TO_L;
    
	PWM_Ctrl_Stop();
	PWM_CTRL_StatusStructure.RunState = PWM_CTRL_STATE_ERROR;
    return PWM_CTRL_DIR_UNKNOWN;
}

/* ==============================================================================
 * API 接口实现
 * ============================================================================== */

const PWM_CTRL_Status_TypeDef* PWM_Ctrl_Get_Status(void)
{
    return &PWM_CTRL_StatusStructure;
}

void PWM_Ctrl_Update_Feedback(void)
{
    /* 底层单位为微(u)，转换为标准单位(V/A)需除以 1,000,000 */
    PWM_CTRL_StatusStructure.Port_V_Left_V   = (float)Sample_Get_VL_Actual() / 1000000.0f;
    PWM_CTRL_StatusStructure.Port_V_Right_V  = (float)Sample_Get_VR_Actual() / 1000000.0f;
    
    PWM_CTRL_StatusStructure.Port_I_Left_A   = (float)Sample_Get_IL_Actual() / 1000000.0f;
    PWM_CTRL_StatusStructure.Port_I_Right_A  = (float)Sample_Get_IR_Actual() / 1000000.0f;
    PWM_CTRL_StatusStructure.Inductor_I_A    = (float)Sample_Get_IM_Actual() / 1000000.0f;
    
    PWM_CTRL_StatusStructure.MCU_Supply_V    = (float)Sample_Get_VDDA_Realtime() / 1000000.0f;
}

void PWM_Ctrl_Init_And_Detect(void)
{
    /* 1. 复位状态 */
    PWM_CTRL_StatusStructure.RunState = PWM_CTRL_STATE_STOP;
	
    /* 2. 初始化底层模块 */
    // 初始化 ADC 和 DMA
    Sample_Init();
    
    // 初始化 PWM 硬件 (MOE关闭, Duty=0)
    PWM_Init_Outputs(); 
    
    /* [重要] 给 ADC/DMA 一点时间完成首轮采样 */
    /* 防止立即读取导致全0数据，造成方向判断错误 */
    HAL_Delay(10); 

    /* 3. 首次刷新数据 (此时 ADC 数据已准备好) */
    PWM_Ctrl_Update_Feedback();
    
    /* 4. 判断方向 (无需传参，函数内部自取) */
    PWM_CTRL_StatusStructure.Direction = PWM_Ctrl_Alg_Check_Direction();

    /* 5. 指针绑定 ... (以下代码保持不变) */
    if (PWM_CTRL_StatusStructure.Direction == PWM_CTRL_DIR_L_TO_R)
    {
        PWM_CTRL_StatusStructure.Ptr_Feedback_Voltage = &PWM_CTRL_StatusStructure.Port_V_Right_V;
        PWM_CTRL_StatusStructure.Ptr_Feedback_Current = &PWM_CTRL_StatusStructure.Port_I_Right_A; 
        PWM_CTRL_StatusStructure.Ptr_Feedback_SourceV = &PWM_CTRL_StatusStructure.Port_V_Left_V;
    }
    else if (PWM_CTRL_StatusStructure.Direction == PWM_CTRL_DIR_R_TO_L)
    {
        PWM_CTRL_StatusStructure.Ptr_Feedback_Voltage = &PWM_CTRL_StatusStructure.Port_V_Left_V;
        PWM_CTRL_StatusStructure.Ptr_Feedback_Current = &PWM_CTRL_StatusStructure.Port_I_Left_A;
        PWM_CTRL_StatusStructure.Ptr_Feedback_SourceV = &PWM_CTRL_StatusStructure.Port_V_Right_V;
    }
    else
    {
        PWM_CTRL_StatusStructure.Ptr_Feedback_Voltage = NULL;
        PWM_CTRL_StatusStructure.RunState = PWM_CTRL_STATE_ERROR;
    }
    
    /* 6. PID 参数初始化 ... (以下代码保持不变) */
    hPID_Voltage.Kp = 0.1f; hPID_Voltage.Ki = 0.05f; hPID_Voltage.Kd = 0.0f;
    hPID_Voltage.MaxOutput = PWM_LOGIC_MAX_DUTY; 
    hPID_Voltage.MinOutput = PWM_LOGIC_MIN_DUTY; 
    hPID_Voltage.MaxIntegral = 100.0f; 
    PID_Reset(&hPID_Voltage);

    hPID_Current.Kp = 0.15f; hPID_Current.Ki = 0.1f; hPID_Current.Kd = 0.0f;
    hPID_Current.MaxOutput = PWM_LOGIC_MAX_DUTY; 
    hPID_Current.MinOutput = PWM_LOGIC_MIN_DUTY;
    hPID_Current.MaxIntegral = 100.0f;
    PID_Reset(&hPID_Current);
}

void PWM_Ctrl_Set_Targets(float target_vol_v, float max_curr_a)
{
    PWM_CTRL_StatusStructure.Target_Voltage_V = target_vol_v;
    PWM_CTRL_StatusStructure.Max_Current_Limit_A = max_curr_a;
}

void PWM_Ctrl_Start(void)
{
    if (PWM_CTRL_StatusStructure.RunState == PWM_CTRL_STATE_ERROR){
		PWM_Ctrl_Stop();
		return;
	}

    PWM_Ctrl_Update_Feedback();
    // 启动前再次校验方向
    if (PWM_Ctrl_Alg_Check_Direction() != PWM_CTRL_StatusStructure.Direction) {
        PWM_CTRL_StatusStructure.RunState = PWM_CTRL_STATE_ERROR;
        PWM_Ctrl_Stop();
        return;
    }

    if (PWM_CTRL_StatusStructure.Ptr_Feedback_Voltage != NULL)
    {
        PID_Reset(&hPID_Voltage);
        PID_Reset(&hPID_Current);
        
        float current_out_v = *(PWM_CTRL_StatusStructure.Ptr_Feedback_Voltage);
        float input_v = *(PWM_CTRL_StatusStructure.Ptr_Feedback_SourceV);

        // 软启动预偏置处理
        PWM_CTRL_StatusStructure.Ramp_Ref_Voltage_V = current_out_v; 

        // 前馈计算: D = Vout / (Vin + Vout)
        if(input_v > 1.0f) 
        {
            float initial_duty = (current_out_v / (input_v + current_out_v)) * 100.0f;
            initial_duty = Apply_Duty_Limit(initial_duty);
            
            hPID_Voltage.Integral = initial_duty;
            hPID_Current.Integral = PWM_LOGIC_MAX_DUTY; // 让电流环让路
        }
        
        PWM_CTRL_StatusStructure.RunState = PWM_CTRL_STATE_SOFT_START;
    }
}

void PWM_Ctrl_Pause(void)
{
    PWM_Set_Left_Duty_Percent(0);
    PWM_Set_Right_Duty_Percent_Inv(100);
    PWM_CTRL_StatusStructure.Real_Duty_Left = 0;
    PWM_CTRL_StatusStructure.Real_Duty_Right = 0;
}

void PWM_Ctrl_Stop(void)
{
    PWM_Stop_All();
    PWM_CTRL_StatusStructure.Real_Duty_Left = 0;
    PWM_CTRL_StatusStructure.Real_Duty_Right = 100;
    PID_Reset(&hPID_Voltage);
    PID_Reset(&hPID_Current);
}

void PWM_Ctrl_Reset_Error(void)
{
    if (PWM_CTRL_StatusStructure.RunState == PWM_CTRL_STATE_ERROR)
        PWM_CTRL_StatusStructure.RunState = PWM_CTRL_STATE_STOP;
}

/* ==============================================================================
 * 核心控制循环
 * ============================================================================== */
void PWM_Ctrl_Process_Loop(void)
{
    /* 1. 采样与单位转换 */
    PWM_Ctrl_Update_Feedback();
    
    /* 2. 状态守卫 */
    if (PWM_CTRL_StatusStructure.RunState == PWM_CTRL_STATE_ERROR){
		PWM_Ctrl_Stop();
		return;
	}
	if (PWM_CTRL_StatusStructure.RunState <= PWM_CTRL_STATE_STOP || 
        PWM_CTRL_StatusStructure.Ptr_Feedback_Voltage == NULL){
		return;
	}
    
    if (PWM_CTRL_StatusStructure.RunState == PWM_CTRL_STATE_PAUSE) {
        PWM_Set_Left_Duty_Percent(0);
        PWM_Set_Right_Duty_Percent_Inv(100);
        return;
    }
    
    /* 3. 软件级硬保护 (Failsafe) */
    if (*(PWM_CTRL_StatusStructure.Ptr_Feedback_Voltage) > PWM_HARD_OVP_LIMIT_V) {
        PWM_Ctrl_Stop();
        PWM_CTRL_StatusStructure.RunState = PWM_CTRL_STATE_ERROR;
        return;
    }
    // 使用 fabsf 获取绝对值进行双向过流保护
    if (fabsf(*(PWM_CTRL_StatusStructure.Ptr_Feedback_Current)) > PWM_HARD_OCP_LIMIT_A) {
        PWM_Ctrl_Stop();
        PWM_CTRL_StatusStructure.RunState = PWM_CTRL_STATE_ERROR;
        return;
    }

//#define OVERCHARGR_PROTECTION
#ifdef OVERCHARGR_PROTECTION
	if(*(PWM_CTRL_StatusStructure.Ptr_Feedback_Voltage) > 24.5f)
	{
		PWM_Ctrl_Stop();
		PWM_CTRL_StatusStructure.RunState = PWM_CTRL_STATE_PAUSE;
	}
#endif


    /* 4. 软启动逻辑 */
    if (PWM_CTRL_StatusStructure.RunState == PWM_CTRL_STATE_SOFT_START) {
        float current_vol = *(PWM_CTRL_StatusStructure.Ptr_Feedback_Voltage);
        if(current_vol > PWM_CTRL_StatusStructure.Ramp_Ref_Voltage_V)
             PWM_CTRL_StatusStructure.Ramp_Ref_Voltage_V = current_vol;

        PWM_CTRL_StatusStructure.Ramp_Ref_Voltage_V += PWM_SOFT_START_STEP_V;
        
        if (PWM_CTRL_StatusStructure.Ramp_Ref_Voltage_V >= PWM_CTRL_StatusStructure.Target_Voltage_V) {
            PWM_CTRL_StatusStructure.Ramp_Ref_Voltage_V = PWM_CTRL_StatusStructure.Target_Voltage_V;
            PWM_CTRL_StatusStructure.RunState = PWM_CTRL_STATE_RUNNING;
        }
    } else {
        PWM_CTRL_StatusStructure.Ramp_Ref_Voltage_V = PWM_CTRL_StatusStructure.Target_Voltage_V;
    }

    /* 5. PID 计算 */
    float actual_vol = *(PWM_CTRL_StatusStructure.Ptr_Feedback_Voltage);
    float actual_curr = *(PWM_CTRL_StatusStructure.Ptr_Feedback_Current); 
    // 正常工况下 current 应该为正，钳位负数防止积分器异常
    if (actual_curr < 0.0f){
		actual_curr = 0.0f;
	}

    float duty_v_raw = PID_Compute_Raw(&hPID_Voltage, PWM_CTRL_StatusStructure.Ramp_Ref_Voltage_V, actual_vol);
    float duty_i_raw = PID_Compute_Raw(&hPID_Current, PWM_CTRL_StatusStructure.Max_Current_Limit_A, actual_curr);

    // 应用双重限幅 (Logic: 33-67%, Hardware: 0-95%)
    duty_v_raw = Apply_Duty_Limit(duty_v_raw);
    duty_i_raw = Apply_Duty_Limit(duty_i_raw);

    /* 6. 竞争机制 */
    float final_duty;
    bool v_loop_wins = false;

    if (duty_v_raw < duty_i_raw) {
        final_duty = duty_v_raw;
        v_loop_wins = true;
    } else {
        final_duty = duty_i_raw;
        v_loop_wins = false;
    }

//    /* 7. 积分跟随 (Back-Calculation) */
	float tracking_target = final_duty;
    tracking_target = Apply_Duty_Limit(tracking_target);

    if (v_loop_wins) {
        // CV 模式
        float error_v = PWM_CTRL_StatusStructure.Ramp_Ref_Voltage_V - actual_vol;
        hPID_Voltage.Integral += hPID_Voltage.Ki * error_v;
        hPID_Voltage.Integral = Float_Clamp(hPID_Voltage.Integral, -hPID_Voltage.MaxIntegral, hPID_Voltage.MaxIntegral);

		float diff_raw = duty_i_raw - tracking_target;
		if(fabsf(diff_raw) > 3) {
			//电流反算
			hPID_Current.Integral = (duty_v_raw - hPID_Current.Kp * (PWM_CTRL_StatusStructure.Max_Current_Limit_A - actual_curr));
		}
        hPID_Current.Integral = Float_Clamp(hPID_Current.Integral, -hPID_Current.MaxIntegral, hPID_Current.MaxIntegral);
    } else {
        // CC 模式
        float error_i = PWM_CTRL_StatusStructure.Max_Current_Limit_A - actual_curr;
        hPID_Current.Integral += hPID_Current.Ki * error_i;
        hPID_Current.Integral = Float_Clamp(hPID_Current.Integral, -hPID_Current.MaxIntegral, hPID_Current.MaxIntegral);

		float diff_raw = duty_v_raw - tracking_target;
		if(fabsf(diff_raw) > 3){
			//电压反算
			hPID_Voltage.Integral = (duty_i_raw - hPID_Voltage.Kp * (PWM_CTRL_StatusStructure.Ramp_Ref_Voltage_V - actual_vol));
		}
        hPID_Voltage.Integral = Float_Clamp(hPID_Voltage.Integral, -hPID_Voltage.MaxIntegral, hPID_Voltage.MaxIntegral);
    }

    /* 8. 记录状态 */
    PWM_CTRL_StatusStructure.Calc_Duty_VoltagePID = duty_v_raw;
    PWM_CTRL_StatusStructure.Calc_Duty_CurrentPID = duty_i_raw;
    PWM_CTRL_StatusStructure.Final_Duty_Percent   = final_duty;
    PWM_CTRL_StatusStructure.Is_Current_Limited   = !v_loop_wins;

    /* 9. 硬件输出映射 (区分方向) */
    float left_pwm = 0.0f;
    float right_pwm_inv = 100.0f;

    // 再次限幅确保安全
    final_duty = Apply_Duty_Limit(final_duty);

    if (PWM_CTRL_StatusStructure.Direction == PWM_CTRL_DIR_L_TO_R)
    {
        /* L -> R (左输入, 右输出): 
           左半桥(输入侧): Q1/Q4 占空比 = final_duty 
           右半桥(输出侧): Q2/Q3 占空比 = 100 - final_duty 
        */
        left_pwm  = final_duty;
//        right_pwm = 100.0f - final_duty;
		right_pwm_inv = final_duty;
    }
    else if (PWM_CTRL_StatusStructure.Direction == PWM_CTRL_DIR_R_TO_L)
    {
        /* R -> L (右输入, 左输出): 
           右半桥(输入侧): Q2/Q3 占空比 = final_duty
           左半桥(输出侧): Q1/Q4 占空比 = 100 - final_duty
        */
        left_pwm  = 100.0f - final_duty;
//        right_pwm = final_duty;
		right_pwm_inv  = 100.0f - final_duty;
    }
    else
    {
        PWM_Ctrl_Stop();
        PWM_CTRL_StatusStructure.RunState = PWM_CTRL_STATE_ERROR;
        return;
    }

    PWM_CTRL_StatusStructure.Real_Duty_Left = left_pwm;
    PWM_CTRL_StatusStructure.Real_Duty_Right = 100 - right_pwm_inv;
	PWM_CTRL_StatusStructure.Final_Duty_Percent = final_duty;

    PWM_Set_Left_Duty_Percent(left_pwm);
    PWM_Set_Right_Duty_Percent_Inv(right_pwm_inv);
}
