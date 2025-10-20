# MSP430G2553 Lissajous 图形演示装置（最小骨架）

目标：满足院赛初赛任务要点（参考链接），基于 MSP430G2553 与 128x64 LCD 完成：
- X 轴：输入正弦（1.5–2 kHz, 2 Vpp）经前端调理采集
- Y 轴：对 X 轴做幅度变换（1/2/3 Vpp）与频率倍频（1–5 倍）并可切换正弦/三角波
- 测量并显示 X/Y 初始相位差 |φ| ≤ 5°
- 在 128×64 LCD 上绘制 Lissajous 图形

硬件建议（Proteus 原理图方向见下文）：
- 前端：LM324 做缓冲/缩放/偏置；CD4052 选择量程；MCP4018 数字电位器调幅
- 频率倍频：锁相环 CD4046 或 MCU 捕获周期推导倍频；三角/正弦由 LUT + DDS 产生；
- 采集：MSP430 ADC10/ADC12（G2553 为 10bit）定时触发；
- 显示：JLX12864（常见 ST7920/ST7565 控制器）SPI/并口；

本仓库仅为最小可编译逻辑骨架与接口抽象，具体定时器与引脚需按你的原理图调整。

## 目录结构
- src/main.c：主循环与任务调度、菜单参数
- src/dsp_phase.h/.c：相位测量（互相关/相位差估计）
- src/dds.h/.c：正弦/三角 DDS（LUT + 相位累加器）
- src/lcd.h/.c：128x64 基础绘制（点/线/坐标映射）
- src/adc.h/.c：ADC 采样（定时器触发 + 缓冲）

## 编译与集成
- 建议使用 CCS/IAR；将 src/* 加入工程；根据原理图修正时钟、SPI/引脚。
- Proteus 中将 hex 烧录入 MSP430，确保 LCD 初始化匹配控制器型号。

## Proteus 原理图要点（简版）
- X_in 经运放偏置到中点（~Vref/2），进入 ADC；
- LCD：若为 ST7920，连 SPI 或并口；若为 ST7565/KS0108，调整 lcd 驱动实现；
- MCP4018：I2C 接 MSP430（SCL/SDA 上拉），用于幅度设定；
- CD4052：做量程/通道选择；

更多细节见后续“Proteus连线建议”。

## MSP430G2553 专项说明
- 目标：使用 G2553 + ST7920(128×64) + Timer_A 捕获，完成 Lissajous 绘制与相位估计。
- 引脚（可在 `src/msp430g2553_pins.h` 修改）：
	- ST7920 SPI 三线：
		- SCLK: P1.5（BIT5）
		- SID:  P1.7（BIT7）
		- CS:   P1.0（BIT0，默认高有效）
	- 捕获输入：P1.2 -> TA0.1 CCI1A（上升沿捕获）。
- 初始化与使用：
	- LCD：`lcd_init(); lcd_clear();` 然后使用 `lcd_draw_pixel` 绘制（建议上层维护帧缓冲再批量下发）。
	- 捕获：`ta_capture_init(SMCLK_HZ);` 周期 ticks = `ta_capture_read_period_ticks()`，f≈SMCLK/ticks。
	- 相位：使用 `dsp_phase` 的 `phase_diff_deg_fs`，配合 `snippets/window` 的 Hann 窗；如有工频干扰，可在 `snippets/notch` 先做陷波。
