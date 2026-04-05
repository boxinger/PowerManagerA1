#ifndef __SAMPLE_H
#define __SAMPLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "adc.h" /* 必须包含此文件以识别 ADC_HandleTypeDef */

/* ==============================================================================
 * 1. 硬件句柄映射 (Hardware Handle Mapping) - 核心修改点
 * ============================================================================== */

/**
 * @brief  ADC 主机句柄 (Master)
 * @note   通常连接 DMA，负责触发转换
 */
#define SAMPLE_ADC_MASTER_HANDLE    (&hadc1)

/**
 * @brief  ADC 从机句柄 (Slave)
 * @note   在同步模式下，需先于主机启动
 */
#define SAMPLE_ADC_SLAVE_HANDLE     (&hadc2)


/* ==============================================================================
 * 2. 硬件与算法参数配置 (Configuration)
 * ============================================================================== */

/* ---------------- ADC/DMA 基础配置 ---------------- */
#define SAMPLE_DMA_BUFFER_SIZE      6
#define ADC_FULL_SCALE              4096        /* 12-bit ADC */

/* 内部参考电压 (单位: uV) -> 1.20V */
#define STM32_INTERNAL_VREF_UV      1200000

/* ---------------- VDDA 滤波配置 ---------------- */
#define VDDA_DEFAULT_UV             3300000
#define VDDA_FILTER_DIV             20

/* ---------------- 电流采样配置 ---------------- */
/* 采样电阻 (单位: uOhm) -> 5mR = 5000 uOhm */
#define CURRENT_SENSE_R_UOHM        5000
/* 运放增益 */
#define CURRENT_AMP_GAIN            20

/* 电流转换系数 (I_uA = Vdiff_uV * NUM / DEN) */
#define CURRENT_CALC_NUM            1000000
#define CURRENT_CALC_DEN            (CURRENT_SENSE_R_UOHM * CURRENT_AMP_GAIN)

/* ---------------- 电压采样配置 ---------------- */
/* 电阻比例 x10 */
#define VOLTAGE_R_HIGH_X10          1000  /* 100.0 k */
#define VOLTAGE_R_LOW_X10           51    /* 5.1 k */

/* 分压系数 (Vin = Vpin * NUM / DEN) */
 #define VOLTAGE_RATIO_NUM           (VOLTAGE_R_HIGH_X10 + VOLTAGE_R_LOW_X10)
#define VOLTAGE_RATIO_DEN           VOLTAGE_R_LOW_X10


/* ==============================================================================
 * 3. API 接口声明
 * ============================================================================== */

void Sample_Init(void);
int32_t Sample_Get_VDDA_Realtime(void);

/* 实际物理值 (含矫正) */
int32_t Sample_Get_VL_Actual(void);
int32_t Sample_Get_VR_Actual(void);
int32_t Sample_Get_IL_Actual(void);
int32_t Sample_Get_IR_Actual(void);
int32_t Sample_Get_IM_Actual(void);

/* 理论计算值 (调试用) */
int32_t Sample_Get_VL_Theoretical(void);
int32_t Sample_Get_VR_Theoretical(void);
int32_t Sample_Get_IL_Theoretical(void);
int32_t Sample_Get_IR_Theoretical(void);
int32_t Sample_Get_IM_Theoretical(void);

/* 底层调试 */
int32_t Sample_Calculate_Pin_Voltage(uint16_t raw_val);

/* 原始数据 */
uint16_t Sample_Get_VL_Raw(void);
uint16_t Sample_Get_IL_Raw(void);
uint16_t Sample_Get_VR_Raw(void);
uint16_t Sample_Get_IR_Raw(void);
uint16_t Sample_Get_IM_Raw(void);
uint16_t Sample_Get_VREF_Raw(void);

/* 矫正接口 (Weak) */
int32_t Sample_Correct_VL(int32_t theory_val);
int32_t Sample_Correct_VR(int32_t theory_val);
int32_t Sample_Correct_IL(int32_t theory_val);
int32_t Sample_Correct_IR(int32_t theory_val);
int32_t Sample_Correct_IM(int32_t theory_val);

#ifdef __cplusplus
}
#endif
#endif /* __SAMPLE_H */
