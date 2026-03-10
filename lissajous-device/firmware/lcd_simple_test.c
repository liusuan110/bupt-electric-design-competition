/*
 * lcd_simple_test.c
 * 
 * 简化版LCD测试程序 - 专门显示"电子信息"
 * 
 * 硬件连接:
 * P2.1: LCD_SCL, P2.2: LCD_SI, P2.3: LCD_CS
 * P2.4: LCD_RES, P2.5: LCD_A0
 * P1.0: LED状态指示
 */

#include <msp430.h>

// LCD引脚定义
#define LCD_CS    BIT3    // P2.3
#define LCD_A0    BIT5    // P2.5  
#define LCD_RES   BIT4    // P2.4
#define LCD_SCL   BIT1    // P2.1
#define LCD_SI    BIT2    // P2.2
#define LED       BIT0    // P1.0

// SPI发送函数
void spi_send(unsigned char data) {
    unsigned char i;
    for(i = 0; i < 8; i++) {
        if(data & 0x80) P2OUT |= LCD_SI;
        else P2OUT &= ~LCD_SI;
        
        P2OUT |= LCD_SCL;
        __delay_cycles(2);
        P2OUT &= ~LCD_SCL;
        __delay_cycles(2);
        data <<= 1;
    }
}

// 发送命令
void lcd_cmd(unsigned char cmd) {
    P2OUT &= ~LCD_A0;    // 命令模式
    P2OUT &= ~LCD_CS;    // 选中LCD
    spi_send(cmd);
    P2OUT |= LCD_CS;     // 取消选中
}

// 发送数据
void lcd_data(unsigned char data) {
    P2OUT |= LCD_A0;     // 数据模式
    P2OUT &= ~LCD_CS;    // 选中LCD
    spi_send(data);
    P2OUT |= LCD_CS;     // 取消选中
}

// LCD初始化
void lcd_init(void) {
    // 复位LCD
    P2OUT &= ~LCD_RES;
    __delay_cycles(50000);
    P2OUT |= LCD_RES;
    __delay_cycles(50000);
    
    // 初始化命令
    lcd_cmd(0x30);       // 基本指令
    __delay_cycles(1000);
    lcd_cmd(0x0C);       // 显示开启
    lcd_cmd(0x01);       // 清屏
    __delay_cycles(20000);
    lcd_cmd(0x06);       // 输入模式
}

// 清屏
void lcd_clear(void) {
    lcd_cmd(0x01);
    __delay_cycles(20000);
}

// 设置光标位置
void lcd_goto(unsigned char row, unsigned char col) {
    unsigned char addr = 0x80;
    switch(row) {
        case 0: addr += col; break;
        case 1: addr += 0x10 + col; break;
        case 2: addr += 0x08 + col; break;
        case 3: addr += 0x18 + col; break;
    }
    lcd_cmd(addr);
    __delay_cycles(1000);
}

// 显示字符串
void lcd_puts(const char* str) {
    while(*str) {
        lcd_data(*str++);
        __delay_cycles(1000);
    }
}

int main(void) {
    // 关闭看门狗
    WDTCTL = WDTPW | WDTHOLD;
    
    // 配置时钟1MHz
    if(CALBC1_1MHZ != 0xFF) {
        DCOCTL = 0;
        BCSCTL1 = CALBC1_1MHZ;
        DCOCTL = CALDCO_1MHZ;
    }
    
    // GPIO配置
    P1DIR |= LED;                                          // P1.0输出
    P1OUT &= ~LED;                                         // LED关闭
    P2DIR |= (LCD_CS|LCD_A0|LCD_RES|LCD_SCL|LCD_SI);      // P2口LCD引脚输出
    P2OUT |= (LCD_CS|LCD_RES);                            // CS和RES默认高
    P2OUT &= ~(LCD_A0|LCD_SCL|LCD_SI);                    // 其他低
    
    // 等待稳定
    __delay_cycles(200000);
    
    // 初始化LCD
    lcd_init();
    
    // LED闪烁3次表示初始化完成
    unsigned char i;
    for(i = 0; i < 6; i++) {
        P1OUT ^= LED;
        __delay_cycles(200000);
    }
    
    // 显示内容
    lcd_clear();
    
    // 第1行：2026电子信息杯
    lcd_goto(0, 0);
    lcd_puts("2026 Contest");
    
    // 第2行：电子 (Electronics)
    lcd_goto(1, 0);
    lcd_puts("Electronics");
    
    // 第3行：信息 (Information) 
    lcd_goto(2, 0);
    lcd_puts("Information");
    
    // 第4行：测试成功
    lcd_goto(3, 0);
    lcd_puts("Test OK!");
    
    // 主循环 - LED缓慢闪烁表示程序运行
    while(1) {
        P1OUT |= LED;
        __delay_cycles(500000);   // 0.5秒亮
        P1OUT &= ~LED;
        __delay_cycles(500000);   // 0.5秒灭
    }
    
    return 0;
}