#ifndef __HT16220_H
#define __HT16220_H

#include <stdint.h>
#include <stdbool.h>

/*
 * HT16220 底层驱动说明
 * ------------------------------------------------------------
 * 1. 本文件只负责 HT16220 的串行通信和显示 RAM 缓存管理。
 * 2. 不负责“显示什么内容”，数字、字母和界面逻辑在
 *    segment_lcd_ui.c 中完成。
 * 3. HT16220 一共有 32 个 SEG，每个 SEG 对应 COM0~COM7，
 *    因此软件中使用 32 字节缓存：每个字节的 bit0~bit7
 *    分别代表该 SEG 上的 COM0~COM7。
 */
#define HT16220_SEG_COUNT 32U

/*
 * 以下 4 个端口函数必须由用户根据实际 MCU 和 GPIO 实现。
 * 驱动层不会直接依赖 STM32 HAL，方便移植到其他单片机。
 */
void HT16220_PortCS(uint8_t level);      /* CS：0=选中芯片，1=结束本次通信 */
void HT16220_PortWR(uint8_t level);      /* WR：写时钟 */
void HT16220_PortDATA(uint8_t level);    /* DATA：串行数据 */
void HT16220_PortDelay(void);            /* 时序延时，3.3V 时建议约 4us */

/* 初始化 HT16220，并开启系统振荡器和 LCD 显示。 */
void HT16220_Init(void);

/* 只清空软件显示缓存，不会立即阻塞式发送到芯片。 */
void HT16220_Clear(void);

/*
 * 设置一个实际 LCD 点阵。
 * seg：0~31
 * com：0~7
 * on ：true 点亮，false 熄灭
 */
void HT16220_SetPoint(uint8_t seg, uint8_t com, bool on);

/* 读取软件缓存中某个 SEG/COM 点当前是否点亮。 */
bool HT16220_GetPoint(uint8_t seg, uint8_t com);

/* 一次设置某个 SEG 对应的 COM0~COM7 全部状态。 */
void HT16220_SetSegByte(uint8_t seg, uint8_t value);

/* 读取某个 SEG 的缓存字节。 */
uint8_t HT16220_GetSegByte(uint8_t seg);

/* 强制把全部 32 个 SEG 在后续任务循环中重新发送一遍。 */
void HT16220_RequestFullRefresh(void);

/*
 * 非阻塞刷新一步。
 * 每次调用最多发送一个 SEG 字节，避免长时间占用 main。
 * 返回 true：本次确实发送了一个 SEG。
 * 返回 false：当前缓存已经和芯片发送记录一致。
 */
bool HT16220_FlushStep(void);

/* 判断全部待显示内容是否已经发送完成。 */
bool HT16220_IsFlushDone(void);

#endif
