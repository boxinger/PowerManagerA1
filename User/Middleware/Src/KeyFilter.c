#include "KeyFilter.h"
#include  "Encoder.h"

void KeyFilter_Init(){
    Encoder_Init();
    HAL_TIM_Base_Start_IT(KEYFILTER_TIM_HANDLE);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == KEYFILTER_TIM)
    {
		static uint8_t key1_debounce_count = 0;
		static uint8_t key1IsPress = 0;
		static uint8_t key2_debounce_count = 0;
		static uint8_t key2IsPress = 0;
		static uint8_t keySW_debounce_count = 0;
		static uint8_t keySWIsPress = 0;
	
        uint8_t key1_state = KEY_Key1IsPressed();
		uint8_t key2_state = KEY_Key2IsPressed();
		uint8_t keySW_state = KEY_KeySWIsPressed();
        
        // 消抖：连续3次检测到按下
        if(key1_state == 1)  // 按下为1
        {
            if(key1IsPress == 0 && key1_debounce_count < 3)  // 之前是释放状态
            {
                key1_debounce_count++;
            }
            // 连续3次检测到按下
            else if(key1_debounce_count == 3)
            {
                key1IsPress = 1;
				key1_debounce_count++;
                
                // 触发按键处理函数
                Key1_Pressed_Handler();
            }

        }
        else  // 释放状态
        {
            key1_debounce_count = 0;
            key1IsPress = 0;
        }
		if(key2_state == 1)  // 按下为1
        {
            if(key2IsPress == 0 && key2_debounce_count < 3)  // 之前是释放状态
            {
                key2_debounce_count++;
            }
            // 连续3次检测到按下
            else if(key2_debounce_count == 3)
            {
                key2IsPress = 1;
				key2_debounce_count++;
                
                // 触发按键处理函数
                Key2_Pressed_Handler();
            }

        }
        else  // 释放状态
        {
            key2_debounce_count = 0;
            key2IsPress = 0;
        }
		if(keySW_state == 1)
        {
            if(keySWIsPress == 0 && keySW_debounce_count < 3)  // 之前是释放状态
            {
                keySW_debounce_count++;
            }
            // 连续3次检测到按下
            else if(keySW_debounce_count == 3)
            {
                keySWIsPress = 1;
				keySW_debounce_count++;
                
                // 触发按键处理函数
                KeySW_Pressed_Handler();
            }

        }
        else
        {
            keySW_debounce_count = 0;
            keySWIsPress = 0;
        }
        int16_t value = Encoder_PopCount();
        if (value != 0)
        {
            Encoder_SetValue(value);
        }
    }
}

__weak void Key1_Pressed_Handler(void){}
__weak void Key2_Pressed_Handler(void){}
__weak void KeySW_Pressed_Handler(void){}
__weak void Encoder_SetValue(int16_t value){}
