#ifndef __UI_PAGE_H
#define __UI_PAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "oled_gfx.h" // 图形层
#include "pwm_ctrl.h" // 业务数据层

/* ==============================================================================
 * UI 接口声明
 * ============================================================================== */

/* 初始化 UI 并显示静态框架 */
void UI_Init(void);

/* 周期性刷新数据 (建议 200ms 调用一次) */
void UI_Update_Values(void);

/* 显示停机界面 */
void UI_Show_Stop_Page(void);

#ifdef __cplusplus
}
#endif

#endif /* __UI_PAGE_H */
