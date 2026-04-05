#include "sample.h"

volatile uint32_t adc_dma_buffer[SAMPLE_DMA_BUFFER_SIZE];
volatile uint32_t adc2_it_buffer[SAMPLE_DMA_BUFFER_SIZE];
volatile uint8_t  adc2_conv_index = 0;
static int32_t g_VDDA_Filtered = VDDA_DEFAULT_UV;

/* ==============================================================================
 * 弱函数默认实现
 * ============================================================================== */
__weak int32_t Sample_Correct_VL(int32_t theory_val) { return theory_val; }
__weak int32_t Sample_Correct_VR(int32_t theory_val) { return theory_val; }
__weak int32_t Sample_Correct_IL(int32_t theory_val) { return theory_val; }
__weak int32_t Sample_Correct_IR(int32_t theory_val) { return theory_val; }
__weak int32_t Sample_Correct_IM(int32_t theory_val) { return theory_val; }

/* ==============================================================================
 * 初始化与 Raw Data
 * ============================================================================== */
void Sample_Init(void)
{
    /* 1. ADC 校准 */
    /* 使用宏定义的句柄，不依赖具体的 hadc1/hadc2 变量名 */
    if (HAL_ADCEx_Calibration_Start(SAMPLE_ADC_MASTER_HANDLE) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_ADCEx_Calibration_Start(SAMPLE_ADC_SLAVE_HANDLE) != HAL_OK)
    {
        Error_Handler();
    }

    /* 2. 启动从机 (Slave) */
    if (HAL_ADC_Start(SAMPLE_ADC_SLAVE_HANDLE) != HAL_OK)
    {
        Error_Handler();
    }
//	HAL_ADC_Start_IT(SAMPLE_ADC_SLAVE_HANDLE);

    /* 3. 启动主机 (Master) 并开启 DMA */
    /* DMA 句柄通常绑定在 ADC 句柄结构体中，因此只需传 Master 句柄即可 */
//    if (HAL_ADCEx_MultiModeStart_DMA(SAMPLE_ADC_MASTER_HANDLE, (uint32_t*)adc_dma_buffer, SAMPLE_DMA_BUFFER_SIZE) != HAL_OK)
	if (HAL_ADC_Start_DMA(SAMPLE_ADC_MASTER_HANDLE, (uint32_t*)adc_dma_buffer, SAMPLE_DMA_BUFFER_SIZE) != HAL_OK)
    {
        Error_Handler();
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
//    /* 判断是哪个 ADC 触发的中断 */
//    if (hadc->Instance == ADC2)
//    {
//        /* 读取当前转换值 */
//        uint16_t val = HAL_ADC_GetValue(hadc);
//        
//        /* 存入缓冲区 */
//        if (adc2_conv_index < 3) {
//            adc2_it_buffer[adc2_conv_index++] = val;
//        }
//        
//        /* 如果是单次扫描结束 (EOS)，重置索引 */
//        /* 注意：如果配置了 Continuous 模式，逻辑需要调整 */
//        if (adc2_conv_index >= 3) {
//            adc2_conv_index = 0;
//        }
//    }
	
	static uint8_t fast_counter = 0;
    if (hadc->Instance == ADC1)
    {
        fast_counter++;
        
        /* 1. 计数达到 10 次 */
        if (fast_counter >= 20)
        {
            fast_counter = 0;
			
            // 【核心代码】手动挂起 PendSV 中断
            // 这行代码告诉 CPU："稍后有空时，请执行 PendSV_Handler"
            SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
        }
    }
}



/* 原始数据获取保持不变，因为它们只涉及内存 buffer */
uint16_t Sample_Get_VL_Raw(void) { return (uint16_t)(adc_dma_buffer[0] & 0xFFFF); }
//uint16_t Sample_Get_IL_Raw(void) { return (uint16_t)((adc_dma_buffer[0] & 0xFFFF0000) >> 16); }
//uint16_t Sample_Get_IL_Raw(void) { return (uint16_t)adc2_it_buffer[0]; }
uint16_t Sample_Get_IL_Raw(void) { return (uint16_t)(adc_dma_buffer[3] & 0xFFFF); }

uint16_t Sample_Get_VR_Raw(void) { return (uint16_t)(adc_dma_buffer[1] & 0xFFFF); }
//uint16_t Sample_Get_IR_Raw(void) { return (uint16_t)((adc_dma_buffer[1] & 0xFFFF0000) >> 16); }
//uint16_t Sample_Get_IR_Raw(void) { return (uint16_t)adc2_it_buffer[1]; }
uint16_t Sample_Get_IR_Raw(void) { return (uint16_t)(adc_dma_buffer[4] & 0xFFFF); }

uint16_t Sample_Get_VREF_Raw(void) { return (uint16_t)(adc_dma_buffer[2] & 0xFFFF); }
//uint16_t Sample_Get_IM_Raw(void)   { return (uint16_t)((adc_dma_buffer[2] & 0xFFFF0000) >> 16); }
//uint16_t Sample_Get_IM_Raw(void) { return (uint16_t)adc2_it_buffer[2]; }
uint16_t Sample_Get_IM_Raw(void) { return (uint16_t)(adc_dma_buffer[5] & 0xFFFF); }

/* ==============================================================================
 * 核心物理计算
 * ============================================================================== */

/* VDDA 计算 */
int32_t Sample_Get_VDDA_Realtime(void)
{
    uint16_t vref_raw = Sample_Get_VREF_Raw();
    if (vref_raw == 0) return g_VDDA_Filtered;

    int64_t numerator = (int64_t)STM32_INTERNAL_VREF_UV * ADC_FULL_SCALE;
    int32_t vdda_inst = (int32_t)(numerator / vref_raw);

    g_VDDA_Filtered = g_VDDA_Filtered + (vdda_inst - g_VDDA_Filtered) / VDDA_FILTER_DIV;
    return g_VDDA_Filtered;
}

/* 引脚电压计算 */
int32_t Sample_Calculate_Pin_Voltage(uint16_t raw_val)
{
    int64_t temp = (int64_t)raw_val * g_VDDA_Filtered;
    return (int32_t)(temp / ADC_FULL_SCALE);
}

/* 理论电压计算 (使用 NUM/DEN) */
static int32_t Sample_Calc_Voltage_Theoretical(uint16_t raw_val)
{
    int32_t pin_uV = Sample_Calculate_Pin_Voltage(raw_val);
    int64_t temp = (int64_t)pin_uV * VOLTAGE_RATIO_NUM;
    return (int32_t)(temp / VOLTAGE_RATIO_DEN);
}

/* 理论电流计算 (使用 NUM/DEN) */
static int32_t Sample_Calc_Current_Theoretical(uint16_t raw_val)
{
    int32_t pin_uV = Sample_Calculate_Pin_Voltage(raw_val);
    int32_t zero_point = g_VDDA_Filtered / 2;
    int32_t diff_uV = pin_uV - zero_point;
    
    int64_t temp = (int64_t)diff_uV * CURRENT_CALC_NUM;
    return (int32_t)(temp / CURRENT_CALC_DEN); 
}

/* ==============================================================================
 * 外部接口层
 * ============================================================================== */
int32_t Sample_Get_VL_Theoretical(void) { return Sample_Calc_Voltage_Theoretical(Sample_Get_VL_Raw()); }
int32_t Sample_Get_VR_Theoretical(void) { return Sample_Calc_Voltage_Theoretical(Sample_Get_VR_Raw()); }
int32_t Sample_Get_IL_Theoretical(void) { return -Sample_Calc_Current_Theoretical(Sample_Get_IL_Raw()); }
int32_t Sample_Get_IR_Theoretical(void) { return -Sample_Calc_Current_Theoretical(Sample_Get_IR_Raw()); }
int32_t Sample_Get_IM_Theoretical(void) { return Sample_Calc_Current_Theoretical(Sample_Get_IM_Raw()); }

int32_t Sample_Get_VL_Actual(void) { return Sample_Correct_VL(Sample_Get_VL_Theoretical()); }
int32_t Sample_Get_VR_Actual(void) { return Sample_Correct_VR(Sample_Get_VR_Theoretical()); }
int32_t Sample_Get_IL_Actual(void) { return Sample_Correct_IL(Sample_Get_IL_Theoretical()); }
int32_t Sample_Get_IR_Actual(void) { return Sample_Correct_IR(Sample_Get_IR_Theoretical()); }
int32_t Sample_Get_IM_Actual(void) { return Sample_Correct_IM(Sample_Get_IM_Theoretical()); }
