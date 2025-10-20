# 电赛信号类算法代码片段（嵌入式友好）

这些片段面向 STM32/Arduino 等 MCU，注重实时性和易移植性：
- goertzel.h / goertzel.c：单频能量检测，适合 DTMF/FSK 子载波检测。
- fir_simple.h / fir_simple.c：定点/浮点 FIR 直达型实现。
- fft_wrapper.h / fft_wrapper.c：CMSIS-DSP 封装（可选），便于快速拉起频谱分析。
- fsk_demod_zero_cross.h / fsk_demod_zero_cross.c：基于过零计数的 2-FSK 解调。
- window.h / window.c：常见窗口（矩形/Hann/Hamming/Blackman）生成与应用。
- biquad.h / biquad.c：IIR 二阶节（低/高通、带通、陷波），支持级联。
- moving_avg.h / moving_avg.c：移动平均与指数平均（平滑/RMS 近似/噪声抑制）。
- cordic_atan2.h / cordic_atan2.c：CORDIC 相位求解，适合定点化。
- autocorr_freq.h / autocorr_freq.c：自相关测频（主周期滞后搜索）。
- rms_agc.h / rms_agc.c：RMS 估计与简易 AGC。
- notch.h：基于 biquad 的陷波器封装（如 50/60Hz 工频抑制）。

注意：
- 若启用 CMSIS-DSP，请在工程中添加对应库并配置浮点/定点选项。
- ADC 采样建议：定时器触发 + DMA 双缓冲（Ping-Pong）+ 后台处理。

示例用法（片段）：
- Hann 窗：
	- 先生成系数：window_fill(w, N, WINDOW_HANN); 再 window_apply(x, w, N)
- 陷波 50Hz：
	- notch_t n; notch_init(&n, fs, 50.0f, 20.0f); y = notch_process(&n, x)
- 自相关测频：
	- size_t P = autocorr_estimate_period(x, N, fs/2500, fs/100); f ≈ fs/P
- CORDIC 相位：
	- float ang = cordic_atan2f(y, x, 14); // 约 14 次迭代
