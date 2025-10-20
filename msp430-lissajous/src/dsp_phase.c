#include "dsp_phase.h"
#include <math.h>

static void remove_dc(float* a, size_t N)
{
    float m=0; for(size_t i=0;i<N;i++) m+=a[i]; m/= (float)N;
    for(size_t i=0;i<N;i++) a[i]-=m;
}

static void apply_hann(float* a, size_t N)
{
    if (N<=1) return;
    for(size_t n=0;n<N;n++){
        float w = 0.5f*(1.0f - cosf(2.0f*3.14159265358979f*n/(float)(N-1)));
        a[n] *= w;
    }
}

// 轻量频率估计：零交叉法（适合近似正弦，频率 1.5~2kHz）
static float estimate_freq_zero_cross(const float* a, size_t N, float fs)
{
    int prev = (a[0]>=0)?1:0; int crossings=0;
    for(size_t i=1;i<N;i++){
        int s = (a[i]>=0)?1:0;
        if (s!=prev) { crossings++; prev=s; }
    }
    // 两次过零约半个周期，crossings/2 为周期数
    if (crossings<2) return -1.0f;
    float periods = (float)crossings/2.0f;
    float f = (periods*fs) / (float)N;
    return f;
}

// Goertzel 单频相位提取
static float goertzel_phase(const float* a, size_t N, float fs, float f0)
{
    float omega = 2.0f*3.14159265358979f * (f0/fs);
    float coeff = 2.0f*cosf(omega);
    float q0, q1=0.0f, q2=0.0f;
    for(size_t n=0;n<N;n++){
        q0 = coeff*q1 - q2 + a[n];
        q2 = q1; q1 = q0;
    }
    // 复数形式: S = q1 - q2*e^{-j omega}
    float re = q1 - q2 * cosf(omega);
    float im = - q2 * (-sinf(omega)); // q1 - q2*cos - j(-q2*sin) => imag = q2*sin
    im = q2*sinf(omega);
    float ph = atan2f(im, re); // [-pi,pi]
    return ph;
}

static float wrap_deg(float d)
{
    while(d>180.0f) d-=360.0f; while(d<=-180.0f) d+=360.0f; return d;
}

// 基础版：符号互相关
float phase_diff_deg(const float* x, const float* y, size_t N)
{
    int K = (int)(N/4);
    int k_best = 0; float c_best = -1e9f;
    for (int k=-K; k<=K; ++k) {
        float c = 0.0f;
        for (size_t n=0; n<N; ++n) {
            int j = (int)n + k;
            if (j < 0 || j >= (int)N) continue;
            float sx = (x[n] >= 0.0f) ? 1.0f : -1.0f;
            float sy = (y[j] >= 0.0f) ? 1.0f : -1.0f;
            c += sx * sy;
        }
        if (c > c_best) { c_best = c; k_best = k; }
    }
    float phi = 360.0f * ((float)k_best / (float)N);
    return wrap_deg(phi);
}

// 精度版：去直流+窗+估频+Goertzel 相位
float phase_diff_deg_fs(const float* x_in, const float* y_in, size_t N, float fs_hz)
{
    // 拷贝到本地缓冲以便去直流/加窗（在 MCU 上可复用 DMA ping-pong 缓冲）
    // 为避免大栈，请在集成时改为静态缓冲或就地操作
    static float xb[512], yb[512];
    if (N>512) N=512;
    for(size_t i=0;i<N;i++){ xb[i]=x_in[i]; yb[i]=y_in[i]; }

    remove_dc(xb,N); remove_dc(yb,N);
    apply_hann(xb,N); apply_hann(yb,N);

    float f_est = estimate_freq_zero_cross(xb,N,fs_hz);
    if (f_est<=0){
        // 退化：回落到基础版
        return phase_diff_deg(x_in,y_in,N);
    }
    float ph_x = goertzel_phase(xb,N,fs_hz,f_est);
    float ph_y = goertzel_phase(yb,N,fs_hz,f_est);
    float d = (ph_y - ph_x) * (180.0f/3.14159265358979f);
    return wrap_deg(d);
}
