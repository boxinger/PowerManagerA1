#ifndef __KEY_H
#define __KEY_H

#include "main.h"

#define KEY1_PORT	GPIOA
#define KEY1_PIN	GPIO_PIN_2
#define KEY2_PORT	GPIOA
#define KEY2_PIN	GPIO_PIN_3

uint8_t KEY_Key1IsPressed(void);
uint8_t KEY_Key2IsPressed(void);


#endif
