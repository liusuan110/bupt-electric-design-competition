#pragma once
#include <stdint.h>

// Timer_A 捕获测频（MSP430G2553): 使用 TA0.1 CCI1A 输入（默认 P1.2）
// 合同：
// - 调用 ta_capture_init(smclk_hz) 初始化，配置 SMCLK 分频与捕获边沿；
// - 在中断中记录连续上升沿的 CCR1 差值，得到周期（单位：计数）；
// - 通过 ta_capture_read_period_ticks() 读取最近周期计数，freq ≈ smclk / ticks。

void ta_capture_init(uint32_t smclk_hz);
// 读取最新周期（计数值）；若为 0 表示尚无有效数据。
uint16_t ta_capture_read_period_ticks(void);
