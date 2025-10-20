#include "ta_capture_freq.h"
#include "msp430g2553_pins.h"

#ifdef __MSP430__
#include <msp430.h>

static volatile uint16_t last_ccr = 0;
static volatile uint16_t period_ticks = 0;
static uint32_t g_smclk_hz = 1000000;

void ta_capture_init(uint32_t smclk_hz){
    g_smclk_hz = smclk_hz;
    // 配置输入引脚 P1.2 为 TA0.1 CCI1A
    TA0_CCI1A_SEL();
    // Timer_A 设置：SMCLK，连续计数，捕获上升沿，同步，捕获/比较 1 使能中断
    TACTL = TASSEL_2 | MC_2 | TACLR;     // SMCLK, continuous mode
    TACCTL1 = CM_1 | CCIS_0 | SCS | CAP | CCIE; // 上升沿, CCI1A, 同步, 捕获, 中断
}

uint16_t ta_capture_read_period_ticks(void){
    return period_ticks;
}

#pragma vector = TIMER0_A1_VECTOR
__interrupt void TIMER0_A1_ISR(void){
    switch (TA0IV) {
    case TA0IV_TACCR1: {
        uint16_t cur = TACCR1;
        uint16_t diff = cur - last_ccr; // 自动处理 16-bit 溢出
        last_ccr = cur;
        if (diff) period_ticks = diff;
        break; }
    default: break;
    }
}

#else
// 非 MSP430 环境：空实现，供编辑器通过
void ta_capture_init(uint32_t smclk_hz){ (void)smclk_hz; }
uint16_t ta_capture_read_period_ticks(void){ return 0; }
#endif
