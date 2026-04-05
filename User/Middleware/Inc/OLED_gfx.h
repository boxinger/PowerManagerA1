#ifndef __OLED_GFX_H
#define __OLED_GFX_H

#include "main.h"
#include "oled_driver.h"
#include <string.h>

/* ==============================================================================
 * GFX 层控制函数
 * ============================================================================== */
void OLED_GFX_Init(void);
void OLED_GFX_Start(void);
void OLED_GFX_Stop(void);
void OLED_GFX_Refresh(void);
void OLED_GFX_Clear(void);

/* ==============================================================================
 * 基础绘图函数 (像素级操作)
 * ============================================================================== */
/* 在指定位置画一个点 */
/* t: 1=亮, 0=灭 */
void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t t);

/* ==============================================================================
 * 高级显示函数
 * 坐标定义: 
 * x: 0 ~ 127 (列像素)
 * y: 0 ~ 63  (行像素) <-- 修改点：现在是像素坐标了
 * ============================================================================== */
void OLED_ShowChar(uint8_t x, uint8_t y, char Char);
void OLED_ShowString(uint8_t x, uint8_t y, char *String);
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t Number, uint8_t Length);
void OLED_ShowSignedNum(uint8_t x, uint8_t y, int32_t Number, uint8_t Length);
void OLED_ShowFloat(uint8_t x, uint8_t y, float Number, uint8_t Width, uint8_t Precision);

#endif /* __OLED_GFX_H */
