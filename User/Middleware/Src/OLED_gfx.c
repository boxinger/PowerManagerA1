#include "oled_gfx.h"
#include "OLED_Font.h"
#include <stdio.h> 

/* * 屏幕缓存 (GRAM)
 * 格式: 8页(行) x 128列. 
 * 每一页代表垂直的8个像素. Page0: y0~y7, Page1: y8~y15...
 */
static uint8_t OLED_GRAM[8][128];

/* ==============================================================================
 * 基础控制函数
 * ============================================================================== */

void OLED_GFX_Init(void)
{
    OLED_Init();       /* L1 硬件初始化 */
    OLED_GFX_Clear();  /* 清空缓存 */
    OLED_GFX_Refresh();/* 刷黑屏幕 */
}

void OLED_GFX_Start(void)
{
    OLED_Start();
}

void OLED_GFX_Stop(void)
{
    OLED_Stop();
}

void OLED_GFX_Refresh(void)
{
    /* 将缓存一次性刷入屏幕 */
    OLED_Refresh_Gram((uint8_t *)OLED_GRAM);
}

void OLED_GFX_Clear(void)
{
    /* 极速清屏：直接将内存置0 */
    memset(OLED_GRAM, 0, sizeof(OLED_GRAM));
}

/* ==============================================================================
 * 核心绘图逻辑 (像素级)
 * ============================================================================== */

/**
 * @brief 在 (x,y) 画一个点
 * @param x: 0-127
 * @param y: 0-63
 * @param t: 1=点亮, 0=熄灭
 */
void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t t)
{
    uint8_t page, bit_offset;

    /* 边界检查 */
    if (x > 127 || y > 63) return;

    /* 计算由于硬件结构导致的内存位置 */
    page = y / 8;          // 确定在第几页 (0-7)
    bit_offset = y % 8;    // 确定在该字节的第几位 (0-7)

    if (t)
    {
        /* 置位: 使用或运算 (|) 将对应位置1 */
        OLED_GRAM[page][x] |= (1 << bit_offset);
    }
    else
    {
        /* 复位: 使用与非运算 (&~) 将对应位置0 */
        OLED_GRAM[page][x] &= ~(1 << bit_offset);
    }
}

/* ==============================================================================
 * 字符与数字显示 (基于 DrawPoint 实现任意位置显示)
 * ============================================================================== */

/**
 * @brief 显示字符 (支持任意 y 坐标)
 * @note  由于需要逐像素解析字库，速度比按页显示稍慢，但灵活性极高
 */
void OLED_ShowChar(uint8_t x, uint8_t y, char Char)
{
    uint8_t i, j;
    uint8_t font_data;
    uint8_t offset = Char - ' '; // 计算字库偏移

    /* 边界检查 (字宽8，字高16) */
    if (x > (127 - 8) || y > (63 - 16)) return;

    /* 遍历字库的16个字节 (假设是 F8x16 格式)
     * F8x16 格式通常是: 前8个字节是上半部分(8x8)，后8个字节是下半部分(8x8)
     * 每个字节代表一列垂直的8个点
     */
    for (i = 0; i < 8; i++) 
    {
        /* --- 画上半部分 (Row 0~7) --- */
        font_data = OLED_F8x16[offset][i]; // 获取第i列的上半截数据
        for (j = 0; j < 8; j++)
        {
            // 如果该位为1，则画点
            if (font_data & (1 << j)) 
                OLED_DrawPoint(x + i, y + j, 1);
            else 
				OLED_DrawPoint(x + i, y + j, 0); // 如果需要背景遮盖，取消注释这行
        }

        /* --- 画下半部分 (Row 8~15) --- */
        font_data = OLED_F8x16[offset][i + 8]; // 获取第i列的下半截数据
        for (j = 0; j < 8; j++)
        {
            if (font_data & (1 << j)) 
                OLED_DrawPoint(x + i, y + j + 8, 1); // y坐标偏移8像素
        }
    }
}

void OLED_ShowString(uint8_t x, uint8_t y, char *String)
{
    uint8_t i = 0;
    while (String[i] != '\0')
    {
        /* 自动换行处理 */
        if (x > (127 - 8)) {
            x = 0;
            y += 16; // 换行高度为16像素
        }
        if (y > (63 - 16)) break;

        OLED_ShowChar(x, y, String[i]);
        x += 8; // 字宽8像素
        i++;
    }
}

/* 辅助函数：幂运算 */
static uint32_t _oled_pow(uint32_t m, uint32_t n)
{
    uint32_t result = 1;
    while (n--) result *= m;
    return result;
}

void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t Number, uint8_t Length)
{
    uint8_t t, temp;
    for (t = 0; t < Length; t++)
    {
        temp = (Number / _oled_pow(10, Length - t - 1)) % 10;
        OLED_ShowChar(x + (t * 8), y, temp + '0');
    }
}

void OLED_ShowSignedNum(uint8_t x, uint8_t y, int32_t Number, uint8_t Length)
{
    uint32_t absNum;
    if (Number < 0) {
        OLED_ShowChar(x, y, '-');
        absNum = -Number;
    } else {
        // OLED_ShowChar(x, y, '+'); // 可选显示加号
        OLED_ShowChar(x, y, ' ');    // 用空格占位对齐
        absNum = Number;
    }
    OLED_ShowNum(x + 8, y, absNum, Length);
}

void OLED_ShowFloat(uint8_t x, uint8_t y, float Number, uint8_t Width, uint8_t Precision)
{
    char tempBuf[16];
    /* 使用 snprintf 格式化为字符串 */
    snprintf(tempBuf, sizeof(tempBuf), "%*.*f", Width, Precision, Number);
    OLED_ShowString(x, y, tempBuf);
}
