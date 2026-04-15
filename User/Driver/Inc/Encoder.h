#ifndef __ENCODER_H__
#define __ENCODER_H__  

#include "main.h"
#include "tim.h"

#define ENCODER_TIM_HANDLE htim3

void Encoder_Init(void);
void Encoder_DeInit(void);
int16_t Encoder_GetCCR(void);
void Encoder_SetCCR(int16_t value);
void Encoder_clear(void);
int16_t Encoder_PopCount(void);


#endif
