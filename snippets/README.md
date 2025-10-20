# 电赛信号类算法代码片段（嵌入式友好）

这些片段面向 STM32/Arduino 等 MCU，注重实时性和易移植性：
- goertzel.h / goertzel.c：单频能量检测，适合 DTMF/FSK 子载波检测。
- fir_simple.h / fir_simple.c：定点/浮点 FIR 直达型实现。
- fft_wrapper.h / fft_wrapper.c：CMSIS-DSP 封装（可选），便于快速拉起频谱分析。
- fsk_demod_zero_cross.h / fsk_demod_zero_cross.c：基于过零计数的 2-FSK 解调。

注意：
- 若启用 CMSIS-DSP，请在工程中添加对应库并配置浮点/定点选项。
- ADC 采样建议：定时器触发 + DMA 双缓冲（Ping-Pong）+ 后台处理。
