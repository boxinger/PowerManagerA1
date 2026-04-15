#ifndef __PWM_CTRL_H
#define __PWM_CTRL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "pwm.h"    // 底层 PWM 驱动
#include "sample.h" // 底层 ADC 采样

/* ==============================================================================
 * 1. 宏定义与参数配置
 * ============================================================================== */

/* 阈值配置 */
#define PWM_CTRL_VOLTAGE_THRESHOLD_V    5.0f   // 方向自动识别电压阈值 (V)
#define PWM_SOFT_START_STEP_V           0.05f  // 软启动斜坡步进 (V/Loop, 假设10kHz, 0.05V*10000=500V/s)
#define PWM_HARD_OVP_LIMIT_V            60.0f  // 软件级过压急停阈值 (V)
#define PWM_HARD_OCP_LIMIT_A            6.0f   // 软件级过流急停阈值 (A)

/* * 占空比限制策略:
 * 1. 硬件绝对限制: 防止占空比 100% 导致自举电容没电
 * 2. 逻辑限制: 满足 Vout 在 0.5*Vin ~ 2.0*Vin 之间
 * 公式: Vout/Vin = D/(1-D)
 * D_min = 1/3 (33.33%), D_max = 2/3 (66.67%)
 */
#define PWM_ABS_MAX_DUTY                95.0f  // 硬件绝对上限
#define PWM_ABS_MIN_DUTY                0.0f   // 硬件绝对下限

#define PWM_LOGIC_MAX_DUTY              80.0f // 逻辑上限 (2倍升压)
//#define PWM_LOGIC_MIN_DUTY              20.0f // 逻辑下限 (0.5倍降压)
#define PWM_LOGIC_MIN_DUTY              0.0f // 逻辑下限 (0.5倍降压)


/* 积分跟随策略配置 */
#define PWM_TRACKING_MARGIN             2.0f   // 非主导环路的跟随安全余量 (%)

/* ==============================================================================
 * 2. 类型定义
 * ============================================================================== */

/* 功率流向枚举 */
typedef enum {
    PWM_CTRL_DIR_UNKNOWN = 0,
    PWM_CTRL_DIR_L_TO_R,     // 左侧输入 -> 右侧输出
    PWM_CTRL_DIR_R_TO_L      // 右侧输入 -> 左侧输出
} PWM_CTRL_Direction_t;

/* 系统运行状态机 */
typedef enum {
    PWM_CTRL_STATE_STOP = 0,       // 停止：硬件MOE关闭
    PWM_CTRL_STATE_PAUSE,          // 暂停：MOE开启，占空比为0
    PWM_CTRL_STATE_SOFT_START,     // 软启：参考电压斜坡上升
    PWM_CTRL_STATE_RUNNING,        // 运行：双闭环 PID
    PWM_CTRL_STATE_ERROR           // 故障：系统锁定
} PWM_CTRL_State_t;

/* PID 对象句柄 */
typedef struct {
    float Kp, Ki, Kd;
    float MaxOutput;       // 输出上限
    float MinOutput;       // 输出下限
    float MaxIntegral;     // 积分限幅
    
    /* 运行时变量 */
    float Integral;        // 积分累加值
    float PrevError;       // 上次误差
} PID_Handle_t;

/* 全局状态结构体 */
typedef struct {
    /* --- 状态标志 --- */
    PWM_CTRL_Direction_t Direction;
    PWM_CTRL_State_t     RunState;
    
    /* --- 核心解耦指针 (指向实际物理变量) --- */
    float* Ptr_Feedback_Voltage;  // 输出电压
    float* Ptr_Feedback_Current;  // 输出电流 (正方向定义为流出)
    float* Ptr_Feedback_SourceV;  // 输入电压 (用于前馈/方向)

    /* --- 控制目标 --- */
    float Target_Voltage_V;       // CV 目标电压
    float Max_Current_Limit_A;    // CC 限流值
    
    /* --- 内部计算变量 --- */
    float Ramp_Ref_Voltage_V;     // 软启参考电压
    float Calc_Duty_VoltagePID;   // 电压环计算出的Duty
    float Calc_Duty_CurrentPID;   // 电流环计算出的Duty
    float Final_Duty_Percent;     // 最终执行Duty
    bool  Is_Current_Limited;     // 是否处于恒流模式

    /* --- 实际硬件输出 --- */
    float Real_Duty_Left;        
    float Real_Duty_Right;       

    /* --- 遥测数据 (单位: V, A) --- */
    float Port_V_Left_V;         
    float Port_V_Right_V;
    float Port_I_Left_A;          // 定义: R->L 为正
    float Port_I_Right_A;         // 定义: L->R 为正
    float Inductor_I_A;          
    float MCU_Supply_V;
    
} PWM_CTRL_Status_TypeDef;

/* ==============================================================================
 * 3. API 接口声明
 * ============================================================================== */

const PWM_CTRL_Status_TypeDef* PWM_Ctrl_Get_Status(void);

void PWM_Ctrl_Init_And_Detect(void);
void PWM_Ctrl_Set_Targets(float target_vol_v, float max_curr_a);
void PWM_Ctrl_Start(void);
void PWM_Ctrl_Pause(void);
void PWM_Ctrl_Stop(void);
void PWM_Ctrl_Reset_Error(void);

/* 核心控制循环 (推荐在 10kHz~20kHz 定时器中断中调用) */
void PWM_Ctrl_Process_Loop(void);

/* 反馈数据刷新 */
void PWM_Ctrl_Update_Feedback(void);

#ifdef __cplusplus
}
#endif

#endif /* __PWM_CTRL_H */
