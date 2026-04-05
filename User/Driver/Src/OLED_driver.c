#include "oled_driver.h"

/* ==============================================================================
 * 内部辅助函数 (Private)
 * ============================================================================== */

/**
 * @brief  发送命令字节
 */
static void OLED_Write_Cmd(uint8_t cmd)
{
    /* 寄存器 0x00 表示 Command */
    HAL_I2C_Mem_Write(OLED_I2C_HANDLE, OLED_I2C_ADDR, 0x00, 1, &cmd, 1, 10);
}

/**
 * @brief  发送数据流
 * @note   利用 I2C 连续写入特性，寄存器 0x40 表示 Data
 */
static void OLED_Write_Data_Stream(uint8_t *data, uint16_t len)
{
    HAL_I2C_Mem_Write(OLED_I2C_HANDLE, OLED_I2C_ADDR, 0x40, 1, data, len, 100);
}

/* ==============================================================================
 * 外部接口函数 (Public)
 * ============================================================================== */

/**
 * @brief  1. OLED 初始化函数
 * @note   设置为水平寻址模式，以支持一键 Buffer 刷新
 */
void OLED_Init(void)
{
    /* 硬件复位延时 (如果是软件复位忽略此步，但建议上电延时) */
    HAL_Delay(100);

    /* --- SSD1306 基础初始化序列 --- */
    OLED_Write_Cmd(0xAE); // 关闭显示 (Sleep Mode)

    OLED_Write_Cmd(0x20); // 设置内存寻址模式 (Set Memory Addressing Mode)
    OLED_Write_Cmd(0x00); // -> 00: 水平寻址模式 (Horizontal Addressing Mode)
                          //    这种模式下，列地址指针自动递增，到头后页地址自动递增。
                          //    非常适合一次性灌入 1024 字节的 Buffer。

    OLED_Write_Cmd(0xB0); // 设置页地址起始 (Page Start Address) -> Page 0
    OLED_Write_Cmd(0xC8); // COM 扫描方向 (Output Scan Direction) -> 从 COM[N-1] 到 COM0
    
    OLED_Write_Cmd(0x00); // 设置低列地址 (Set Low Column Address)
    OLED_Write_Cmd(0x10); // 设置高列地址 (Set High Column Address)

    OLED_Write_Cmd(0x40); // 设置显示起始行 (Display Start Line) -> Line 0

    OLED_Write_Cmd(0x81); // 对比度设置 (Set Contrast Control)
    OLED_Write_Cmd(0xCF); // -> 值 (0x00~0xFF)

    OLED_Write_Cmd(0xA1); // 设置段重映射 (Set Segment Re-map) -> column 127 mapped to SEG0

    OLED_Write_Cmd(0xA6); // 正常显示 (Normal Display) (A7为反色)

    OLED_Write_Cmd(0xA8); // 多路复用率 (Multiplex Ratio)
    OLED_Write_Cmd(0x3F); // -> 1/64 duty (0x3F)

    OLED_Write_Cmd(0xA4); // 全局显示开启 (Entire Display On) -> Resume to RAM content

    OLED_Write_Cmd(0xD3); // 设置显示偏移 (Set Display Offset)
    OLED_Write_Cmd(0x00); // -> 0

    OLED_Write_Cmd(0xD5); // 设置显示时钟分频 (Display Clock Divide Ratio)
    OLED_Write_Cmd(0x80); // -> 默认

    OLED_Write_Cmd(0xD9); // 预充电周期 (Pre-charge Period)
    OLED_Write_Cmd(0xF1); 

    OLED_Write_Cmd(0xDA); // COM 硬件配置 (COM Pins Hardware Configuration)
    OLED_Write_Cmd(0x12);

    OLED_Write_Cmd(0xDB); // VCOMH 电压倍率 (VCOMH Deselect Level)
    OLED_Write_Cmd(0x40); 

    OLED_Write_Cmd(0x8D); // 电荷泵设置 (Charge Pump Setting)
    OLED_Write_Cmd(0x14); // -> Enable (必须开启才能亮)

    OLED_Clear_Hardware(); // 初始化后先清屏防止花屏

    OLED_Write_Cmd(0xAF); // 开启显示 (Display ON)
}

/**
 * @brief  2. OLED 停止函数
 * @note   进入低功耗睡眠模式
 */
void OLED_Stop(void)
{
    OLED_Write_Cmd(0xAE); // Display OFF
}

/**
 * @brief  3. OLED 开启函数
 * @note   从睡眠中唤醒
 */
void OLED_Start(void)
{
    OLED_Write_Cmd(0xAF); // Display ON
}

/**
 * @brief  4. OLED 硬件清屏函数
 * @note   直接向硬件发送 1024 个 0x00，不依赖 L2 层的 Buffer
 */
void OLED_Clear_Hardware(void)
{
    uint16_t i;
    /* 由于设置了水平寻址模式，只需重置指针，然后连续发 1024 个 0 即可 */
    
    /* 重置指针到 (0, 0) */
    OLED_Write_Cmd(0x21); OLED_Write_Cmd(0);   OLED_Write_Cmd(127); // 列范围
    OLED_Write_Cmd(0x22); OLED_Write_Cmd(0);   OLED_Write_Cmd(7);   // 页范围

    /* 这种方式比 for 循环调用 1024 次 Write_Data 效率高非常多 */
    uint8_t zero_buff[16] = {0}; // 小块 buffer 辅助清屏
    
    /* 循环发送小块 0，总共发送 128*8 = 1024 字节 */
    /* 1024 / 16 = 64 次 */
    for(i = 0; i < 64; i++)
    {
        OLED_Write_Data_Stream(zero_buff, 16);
    }
}

/**
 * @brief  5. OLED 显示缓冲区函数 (全屏刷新)
 * @param  pBuffer: 指向 L2 层维护的显存数组 (大小必须是 1024 字节)
 * @note   这是最常用的接口，将内存中的画面一次性刷到屏幕上
 */
void OLED_Refresh_Gram(uint8_t *pBuffer)
{
    if (pBuffer == NULL) return;

    /* 1. 设置列地址范围 (Column Address): 0 ~ 127 */
    OLED_Write_Cmd(0x21); 
    OLED_Write_Cmd(0); 
    OLED_Write_Cmd(127);

    /* 2. 设置页地址范围 (Page Address): 0 ~ 7 */
    OLED_Write_Cmd(0x22); 
    OLED_Write_Cmd(0); 
    OLED_Write_Cmd(7);

    /* 3. 利用 I2C 连续写入模式，一口气把 1024 字节发过去 */
    /* STM32 I2C DMA 或 硬件 I2C 处理这个非常快 */
    OLED_Write_Data_Stream(pBuffer, 128 * 8);
}
