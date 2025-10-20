#pragma once
#ifdef __MSP430__
#include <msp430.h>
#else
// 非 MSP430 编译环境的占位，便于 IntelliSense
#include <stdint.h>
extern uint8_t P1DIR, P1OUT, P1SEL;
#ifndef BIT0
#define BIT0 0x01
#endif
#ifndef BIT2
#define BIT2 0x04
#endif
#ifndef BIT5
#define BIT5 0x20
#endif
#ifndef BIT7
#define BIT7 0x80
#endif
#ifndef __no_operation
#define __no_operation() ((void)0)
#endif
#ifndef __delay_cycles
#define __delay_cycles(x) ((void)0)
#endif
#endif

// 引脚映射（可按硬件实际修改）：
// ST7920 SPI 三线：SCLK, SID(数据), CS(片选)
#ifndef ST7920_SCLK_PORT
#define ST7920_SCLK_DIR  P1DIR
#define ST7920_SCLK_OUT  P1OUT
#define ST7920_SCLK_BIT  BIT5   // P1.5
#endif

#ifndef ST7920_SID_PORT
#define ST7920_SID_DIR   P1DIR
#define ST7920_SID_OUT   P1OUT
#define ST7920_SID_BIT   BIT7   // P1.7
#endif

#ifndef ST7920_CS_PORT
#define ST7920_CS_DIR    P1DIR
#define ST7920_CS_OUT    P1OUT
#define ST7920_CS_BIT    BIT0   // P1.0
#endif

// ST7920 CS 有的模块为高有效，有的为低有效；如需反相，调整下方两个宏。
#ifndef ST7920_CS_ACTIVE
#define ST7920_CS_ACTIVE()   (ST7920_CS_OUT |= ST7920_CS_BIT)   // 片选有效
#define ST7920_CS_IDLE()     (ST7920_CS_OUT &= ~ST7920_CS_BIT)  // 片选无效
#endif

// Timer_A 捕获输入：默认使用 TA0.1 CCI1A（典型为 P1.2），可改为 CCI1B（如 P2.0）。
#ifndef TA0_CCI1A_SEL
#define TA0_CCI1A_SEL() do { P1DIR &= ~BIT2; P1SEL |= BIT2; } while(0) // P1.2 作为 TA0.1 CCI1A
#endif
