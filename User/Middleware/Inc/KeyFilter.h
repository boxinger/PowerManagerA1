#ifndef __KEYFILTER_H
#define __KEYFILTER_H

#include "main.h"
#include "tim.h"
#include "Key.h"

#define KEYFILTER_TIM	TIM4
#define KEYFILTER_TIM_HANDLE	(&htim4)

void KeyFilter_Init(void);

void Key1_Pressed_Handler(void);
void Key2_Pressed_Handler(void);

#endif
