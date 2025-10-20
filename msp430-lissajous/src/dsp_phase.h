#pragma once
#include <stddef.h>
#include <stdint.h>

// 相位差估计（度数，范围 -180..+180）
// 1) 基础版：基于符号互相关，轻量但对噪声/失配更敏感
float phase_diff_deg(const float* x, const float* y, size_t N);

// 2) 精度版：提供采样率 fs(Hz)，内部先估计基频，再用 Goertzel/DFT 单频相位法求相位差
//    - 自动去直流 + 可选汉宁窗，稳健性更好，易达 ≤5°
float phase_diff_deg_fs(const float* x, const float* y, size_t N, float fs_hz);
