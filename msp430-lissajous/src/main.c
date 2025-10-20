#include <msp430.h>
#include <stdint.h>
#include "dds.h"
#include "dsp_phase.h"
#include "lcd.h"

// 说明：此 main.c 仅为演示骨架（伪代码风格），具体引脚/时钟/外设需按你的原理图改动。

static volatile uint16_t adc_buf_x[256];
static volatile uint16_t adc_buf_y[256];

static void clock_init(void) {
    // 配置 DCO 为 16MHz（示意），实际按 LCD/I2C/ADC 需要调整
    // 具体寄存器配置略
}

static void gpio_init(void) {
    // 配置 LCD/I2C/SPI/按键引脚（占位）
}

static void adc_init(void) {
    // 使用定时器触发序列采样 X/Y 通道到双缓冲（占位）
}

static void lcd_plot_lissajous(const float* x, const float* y, uint16_t N) {
    // 将 x,y ∈ [-1,1] 映射到 128x64 并画点（简化示例）
    for (uint16_t i=0;i<N;i++){
        int xi = (int)( (x[i]+1.0f)*0.5f * 127.0f );
        int yi = (int)( (1.0f-(y[i]+1.0f)*0.5f) * 63.0f ); // y 轴向下
        if (xi>=0 && xi<128 && yi>=0 && yi<64) lcd_draw_pixel((uint8_t)xi,(uint8_t)yi,1);
    }
}

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;
    clock_init(); gpio_init(); adc_init(); lcd_init(); lcd_clear();

    // 参数：倍频(1..5)、幅度(1/2/3Vpp)、波形（正弦/三角）从按键或菜单读取（占位）

    // 演示：用 DDS 产生 Y 波形（实际项目中 Y 也可来自外部后端）
    dds_t d; dds_init(&d, 20000, 2000); // fs=20kHz, f_out=2kHz 示例

    while (1) {
        // 1) 获取一帧 X/Y 样本，转换到 [-1,1]
        float x[128], y[128];
        for (int i=0;i<128;i++) {
            // 这里用 DDS 生成 Y，X 假设来自 ADC（占位映射）
            float xi = 0.7f * (float)sin(i*0.1f); // demo
            float yi = dds_sin_next(&d);
            x[i] = xi; y[i] = yi;
        }
        // 2) 估计相位差
        // 若已知 fs，这里优先用精度版
        float fs_hz = 20000.0f; // 示例采样率
        float phi = phase_diff_deg_fs(x, y, 128, fs_hz);
        // 3) 绘制李萨如
        lcd_plot_lissajous(x, y, 128);
        // 4) 在屏显上显示倍数、幅度、相位（占位）
    }
}
