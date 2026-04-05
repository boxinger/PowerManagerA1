#ifndef __OLED_DRIVER_H
#define __OLED_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "i2c.h"

/* ==============================================================================
 * 1. 硬件句柄配置 (Hardware Handle) - 核心修改点
 * ============================================================================== */
/**
 * @brief  OLED 绑定的 I2C 句柄
 * @note   对应 i2c.c 中的 hi2c1
 */
#define OLED_I2C_HANDLE         (&hi2c1)

/* OLED I2C 设备地址 (7-bit 0x3C << 1 = 0x78) */
#define OLED_I2C_ADDR           0x78

/* ==============================================================================
 * 2. 屏幕参数定义
 * ============================================================================== */
#define OLED_WIDTH              128
#define OLED_HEIGHT             64
/* 页数 = 64行 / 8位 = 8页 */
#define OLED_PAGE_COUNT         8   

/* ==============================================================================
 * 3. API 接口声明 (L1 Layer)
 * ============================================================================== */

/* 1. OLED 初始化 (配置寄存器) */
void OLED_Init(void);

/* 2. OLED 停止/休眠 (关闭显示电荷泵，省电) */
void OLED_Stop(void);

/* 3. OLED 开启/唤醒 */
void OLED_Start(void);

/* 4. OLED 硬件清屏 (直接向硬件写0，不依赖上层Buffer) */
void OLED_Clear_Hardware(void);

/* 5. OLED 显示缓冲区 (核心功能：将 L2 层的显存同步到屏幕) */
/* pBuffer 大小必须为 128 * 8 = 1024 字节 */
void OLED_Refresh_Gram(uint8_t *pBuffer);

#ifdef __cplusplus
}
#endif

#endif /* __OLED_DRIVER_H */
