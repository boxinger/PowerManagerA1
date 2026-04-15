#include "ui.h"
#include "sample.h" // [新增] 需要引用底层采样头文件以获取 RAW 数据

/* ==============================================================================
 * 布局配置 (像素坐标 Y: 0-63)
 * ============================================================================== */
#define ROW1_Y   0
#define ROW2_Y   16
#define ROW3_Y   32
#define ROW4_Y   47

/* 列坐标 X */
#define COL_L_LABEL   0    // 左侧标签起始
#define COL_L_VAL     24   // 左侧数值起始 (标签 "VL:" 占24px)
                           // 数值 4位数 占32px (24+32=56px)，不会越界

#define COL_R_LABEL   64   // 右侧标签起始 (从屏幕中间开始)
#define COL_R_VAL     88   // 右侧数值起始 (标签 "IL:" 占24px, 64+24=88)
                           // 数值 4位数 占32px (88+32=120px)，刚好在 128px 范围内

/* ==============================================================================
 * 内部函数
 * ============================================================================== */

/* 加载静态文字 (DIR:, VL:, VR: 等) */
static void UI_Load_Main_Frame(void)
{
    /* 注意：这里不要再调用 Clear，因为 Clear 由调用者控制 */

    /* Line 1 */
    OLED_ShowString(0, ROW1_Y, "DIR:");

    /* Line 2 */
    OLED_ShowString(COL_L_LABEL, ROW2_Y, "VL:");
    OLED_ShowString(COL_R_LABEL, ROW2_Y, "IL:");

    /* Line 3 */
    OLED_ShowString(COL_L_LABEL, ROW3_Y, "VR:");
    OLED_ShowString(COL_R_LABEL, ROW3_Y, "IR:");

    /* Line 4 */
//    OLED_ShowString(COL_L_LABEL, ROW4_Y, "IM:");
	OLED_ShowString(4*8, ROW4_Y, "V");
	OLED_ShowString(9*8, ROW4_Y, "A");
//    OLED_ShowString(COL_R_LABEL, ROW4_Y, "Dt:");
    // OLED_ShowString(COL_R_LABEL+8, ROW4_Y, "D:");

}

/* [修改] 辅助显示: 显示 4位 RAW 值 (不带单位，节省空间) */
static void UI_Show_Raw(uint8_t x, uint8_t y, uint16_t val)
{
    /* 显示4位整数 (0-4095)，不足补0 */
    OLED_ShowNum(x, y, (uint32_t)val, 4);
}

/* 辅助显示: 浮点数 + 单位 (用于非RAW数据显示) */
static void UI_Show_Param_Float(uint8_t x, uint8_t y, float val, char unit)
{
    OLED_ShowFloat(x, y, val, 4, 1);
    OLED_ShowChar(x + 32, y, unit);
}

/* ==============================================================================
 * API 实现
 * ============================================================================== */

void UI_Init(void)
{
    OLED_GFX_Init();    // 初始化底层
    // UI_Load_Main_Frame(); // 初始化时不需要加载，因为 Update_Values 会清屏并重绘
}

void UI_Show_Stop_Page(void)
{
    OLED_GFX_Clear();
    OLED_ShowString(32, 20, "SYSTEM");
    OLED_ShowString(28, 36, "STOPPED");
    OLED_GFX_Refresh();
}

void UI_Update_Values(void)
{
    const PWM_CTRL_Status_TypeDef* status = PWM_Ctrl_Get_Status();
    
    /* 1. 清空缓冲区 */
    OLED_GFX_Clear();
    
    /* 2. [重要修复] 清屏后必须重新画出静态框架，否则标签会消失 */
    UI_Load_Main_Frame();

    /* 3. 刷新状态栏 (Row 1) */
    if (status->RunState == PWM_CTRL_STATE_ERROR) {
        OLED_ShowString(32, ROW1_Y, "ERROR!  ");
    } else if (status->RunState == PWM_CTRL_STATE_PAUSE) {
        OLED_ShowString(32, ROW1_Y, "STOPPED ");
    } else if (status->RunState == PWM_CTRL_STATE_SOFT_START){
		OLED_ShowString(32, ROW1_Y, "SOFT ");
	} else {
        if (status->Direction == PWM_CTRL_DIR_L_TO_R)
            OLED_ShowString(32, ROW1_Y, "L -> R  ");
        else if (status->Direction == PWM_CTRL_DIR_R_TO_L)
            OLED_ShowString(32, ROW1_Y, "R -> L  ");
        else
            OLED_ShowString(32, ROW1_Y, "UNKNOWN ");
    }

    /* 显示 MCU 电压 (保持浮点显示，右上角空间足够) */
    OLED_ShowFloat(96, ROW1_Y, status->MCU_Supply_V, 3, 1);
    OLED_ShowChar(120, ROW1_Y, 'V');

    /* =========================================================
     * 4. 刷新左/右端口 (Row 2 & 3) - 改为显示 RAW ADC 值
     * ========================================================= */
    
    /* Row 2: VL, IL (Raw) */
//    UI_Show_Raw(COL_L_VAL, ROW2_Y, Sample_Get_VL_Raw());
//    UI_Show_Raw(COL_R_VAL, ROW2_Y, Sample_Get_IL_Raw());
	OLED_ShowFloat(COL_L_VAL, ROW2_Y, status->Port_V_Left_V, 4, 1);
	OLED_ShowFloat(COL_R_VAL, ROW2_Y, status->Port_I_Left_A, 4, 1);

    /* Row 3: VR, IR (Raw) */
//    UI_Show_Raw(COL_L_VAL, ROW3_Y, Sample_Get_VR_Raw());
//    UI_Show_Raw(COL_R_VAL, ROW3_Y, Sample_Get_IR_Raw());
	OLED_ShowFloat(COL_L_VAL, ROW3_Y, status->Port_V_Right_V, 4, 1);
	OLED_ShowFloat(COL_R_VAL, ROW3_Y, status->Port_I_Right_A, 4, 1);

    /* ========================================================= */

    /* 5. 刷新内部数据 (Row 4) */
    /* 电感电流 (建议也看Raw以便调试，或者保持浮点) - 这里暂保持浮点 */
    // 如果你想看电感电流 RAW，请用: UI_Show_Raw(COL_L_VAL, ROW4_Y, Sample_Get_IM_Raw());
//	UI_Show_Raw(COL_L_VAL, ROW4_Y, Sample_Get_IM_Raw());
//    UI_Show_Param_Float(COL_L_VAL, ROW4_Y, status->Inductor_I_A, ' ');
//	OLED_ShowFloat(COL_L_VAL, ROW4_Y, status->Target_Voltage_V, 4, 1);
	OLED_ShowFloat(0, ROW4_Y,status->Target_Voltage_V, 2, 1);
	OLED_ShowFloat(6*8, ROW4_Y,status->Max_Current_Limit_A, 2, 1);

    /* 占空比 (整数%) */
    OLED_ShowNum(COL_R_VAL, ROW4_Y, (uint32_t)status->Final_Duty_Percent, 3);
    OLED_ShowChar(COL_R_VAL + 24, ROW4_Y, '%');

    /* 6. 提交显存 */
    OLED_GFX_Refresh();
}
