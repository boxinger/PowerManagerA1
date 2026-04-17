#include "Key.h"

uint8_t KEY_Key1IsPressed(void){
	return !HAL_GPIO_ReadPin(KEY1_PORT,KEY1_PIN);
}

uint8_t KEY_Key2IsPressed(void){
	return HAL_GPIO_ReadPin(KEY2_PORT,KEY2_PIN);
}

uint8_t KEY_KeySWIsPressed(void){
	return HAL_GPIO_ReadPin(KEYSW_PORT,KEYSW_PIN);
}
