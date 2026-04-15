#include "Encoder.h"

static uint32_t _encoder_enter_critical(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static void _encoder_exit_critical(uint32_t primask)
{
    if (primask == 0U)
    {
        __enable_irq();
    }
}

void Encoder_Init(void)
{
    /* 编码器初始化代码 */
    HAL_TIM_Encoder_Start(&ENCODER_TIM_HANDLE, TIM_CHANNEL_ALL);
    Encoder_clear();
}

void Encoder_DeInit(void)
{
    /* 编码器反初始化代码 */
    HAL_TIM_Encoder_Stop(&ENCODER_TIM_HANDLE, TIM_CHANNEL_ALL);
}

int16_t Encoder_GetCCR(void)
{
    /* 获取编码器计数值 */
    return (int16_t)__HAL_TIM_GET_COUNTER(&ENCODER_TIM_HANDLE);
}

void Encoder_SetCCR(int16_t value)
{
    /* 设置编码器计数值 */
    __HAL_TIM_SET_COUNTER(&ENCODER_TIM_HANDLE, value);
}

void Encoder_clear(void)
{
    /* 清零编码器计数值 */
    __HAL_TIM_SET_COUNTER(&ENCODER_TIM_HANDLE, 0);
}

int16_t Encoder_PopCount(void)
{
    /* 在同一临界区内完成读+写，避免在两步之间发生竞争 */
    uint32_t primask;
    int16_t rawCount;
    int16_t count;
    int16_t remainder;

    primask = _encoder_enter_critical();
    rawCount = (int16_t)__HAL_TIM_GET_COUNTER(&ENCODER_TIM_HANDLE);
//    count = rawCount / 4;
//    remainder = rawCount % 4;
	count = rawCount / 2;
    remainder = rawCount % 2;
    __HAL_TIM_SET_COUNTER(&ENCODER_TIM_HANDLE, (uint16_t)remainder);
    _encoder_exit_critical(primask);

    return count;
}


