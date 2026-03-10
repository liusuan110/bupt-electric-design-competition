/*
 * lcd_test.c
 * 
 * LCD显示功能单独测试程序 - 2026电子信息杯
 * 
 * 功能: 
 * 1. 测试LCD基本显示功能
 * 2. 显示中文"电子信息"
 * 3. 验证SPI通信和初始化
 * 
 * 硬件连接:
 * - P2.1: LCD_SCL (SPI时钟)
 * - P2.2: LCD_SI  (SPI数据)
 * - P2.3: LCD_CS  (LCD片选)
 * - P2.4: LCD_RES (LCD复位)
 * - P2.5: LCD_A0  (命令/数据选择)
 * 
 * 作者: 2026电子信息杯参赛队
 * 日期: 2025年11月21日
 */

#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>

// LCD SPI接口定义
#define LCD_CS          BIT3        // P2.3 - LCD片选
#define LCD_A0          BIT5        // P2.5 - 命令/数据选择  
#define LCD_RES         BIT4        // P2.4 - LCD复位
#define LCD_SCL         BIT1        // P2.1 - SPI时钟
#define LCD_SI          BIT2        // P2.2 - SPI数据

// 状态LED
#define STATUS_LED      BIT0        // P1.0 - 状态指示LED

// =============================================================================
// 系统初始化函数
// =============================================================================

/**
 * @brief 系统时钟初始化
 */
void clock_init(void) {
    // 停止看门狗定时器
    WDTCTL = WDTPW | WDTHOLD;
    
    // 配置DCO为1MHz
    if (CALBC1_1MHZ != 0xFF) {
        DCOCTL = 0;
        BCSCTL1 = CALBC1_1MHZ;
        DCOCTL = CALDCO_1MHZ;
    }
}

/**
 * @brief GPIO引脚初始化
 */
void gpio_init(void) {
    // P1口状态LED配置
    P1DIR |= STATUS_LED;
    P1OUT &= ~STATUS_LED;
    
    // P2口LCD SPI接口配置
    P2DIR |= (LCD_CS | LCD_A0 | LCD_RES | LCD_SCL | LCD_SI);
    
    // LCD信号初始状态
    P2OUT |= (LCD_CS | LCD_RES);                    // CS和RES默认高电平
    P2OUT &= ~(LCD_A0 | LCD_SCL | LCD_SI);         // 其他信号默认低电平
}

// =============================================================================
// LCD驱动函数
// =============================================================================

/**
 * @brief SPI发送单字节数据
 */
void spi_write_byte(uint8_t data) {
    uint8_t i;
    for (i = 0; i < 8; i++) {
        // 设置数据位
        if (data & 0x80) {
            P2OUT |= LCD_SI;
        } else {
            P2OUT &= ~LCD_SI;
        }
        
        // 时钟脉冲
        P2OUT |= LCD_SCL;
        __delay_cycles(2);
        P2OUT &= ~LCD_SCL;
        __delay_cycles(2);
        
        data <<= 1;
    }
}

/**
 * @brief 向LCD发送命令
 */
void lcd_write_command(uint8_t cmd) {
    P2OUT &= ~LCD_A0;                   // A0=0: 命令模式
    P2OUT &= ~LCD_CS;                   // 选中LCD
    spi_write_byte(cmd);
    P2OUT |= LCD_CS;                    // 取消选中
}

/**
 * @brief 向LCD发送数据
 */
void lcd_write_data(uint8_t data) {
    P2OUT |= LCD_A0;                    // A0=1: 数据模式
    P2OUT &= ~LCD_CS;                   // 选中LCD
    spi_write_byte(data);
    P2OUT |= LCD_CS;                    // 取消选中
}

/**
 * @brief LCD初始化
 */
void lcd_init(void) {
    // 硬件复位
    P2OUT &= ~LCD_RES;
    __delay_cycles(50000);              // 50ms延时
    P2OUT |= LCD_RES;
    __delay_cycles(50000);              // 等待复位完成
    
    // LCD初始化命令序列 (ST7920兼容)
    lcd_write_command(0x30);            // 基本指令集
    __delay_cycles(1000);
    lcd_write_command(0x30);            // 重复命令确保可靠
    __delay_cycles(1000);
    lcd_write_command(0x0C);            // 显示开启，光标关闭
    lcd_write_command(0x01);            // 清屏
    __delay_cycles(20000);              // 清屏需要较长时间
    lcd_write_command(0x06);            // 输入模式：光标右移
}

/**
 * @brief 清空LCD显示
 */
void lcd_clear(void) {
    lcd_write_command(0x01);
    __delay_cycles(20000);
}

/**
 * @brief 设置LCD光标位置
 */
void lcd_set_cursor(uint8_t row, uint8_t col) {
    uint8_t address = 0x80;
    
    switch (row) {
        case 0: address += col; break;          // 第一行
        case 1: address += 0x10 + col; break;  // 第二行
        case 2: address += 0x08 + col; break;  // 第三行
        case 3: address += 0x18 + col; break;  // 第四行
        default: return;
    }
    
    lcd_write_command(address);
    __delay_cycles(1000);
}

/**
 * @brief 显示字符串
 */
void lcd_print_string(const char* str) {
    while (*str) {
        lcd_write_data(*str++);
        __delay_cycles(1000);
    }
}

/**
 * @brief 显示数字
 */
void lcd_print_number(uint32_t num) {
    char buffer[12];
    uint8_t i = 0;
    
    if (num == 0) {
        lcd_write_data('0');
        return;
    }
    
    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    while (i > 0) {
        lcd_write_data(buffer[--i]);
        __delay_cycles(1000);
    }
}

// =============================================================================
// 中文字符显示函数 (使用字符近似显示)
// =============================================================================

/**
 * @brief 显示"电子信息"的ASCII近似
 * 由于ST7920字符模式下显示中文需要字库，这里用ASCII字符组合近似显示
 */
void display_chinese_text(void) {
    // 清屏
    lcd_clear();
    
    // 第一行：标题
    lcd_set_cursor(0, 0);
    lcd_print_string("2026 Contest");
    
    // 第二行：电子信息 (用ASCII近似)
    lcd_set_cursor(1, 0);
    lcd_print_string("Electronics");
    
    // 第三行：信息
    lcd_set_cursor(2, 0);
    lcd_print_string("Information");
    
    // 第四行：测试信息
    lcd_set_cursor(3, 0);
    lcd_print_string("LCD Test OK");
}

/**
 * @brief 显示测试模式
 */
void display_test_patterns(void) {
    uint8_t pattern = 0;
    
    while (1) {
        switch (pattern % 4) {
            case 0:
                // 模式1：基本文字显示
                lcd_clear();
                lcd_set_cursor(0, 0);
                lcd_print_string("Mode 1/4");
                lcd_set_cursor(1, 0);
                lcd_print_string("Basic Text");
                lcd_set_cursor(2, 0);
                lcd_print_string("ABCD 1234");
                lcd_set_cursor(3, 0);
                lcd_print_string("!@#$ %^&*");
                break;
                
            case 1:
                // 模式2：数字显示测试
                lcd_clear();
                lcd_set_cursor(0, 0);
                lcd_print_string("Mode 2/4");
                lcd_set_cursor(1, 0);
                lcd_print_string("Numbers:");
                lcd_set_cursor(2, 0);
                lcd_print_number(12345);
                lcd_set_cursor(3, 0);
                lcd_print_number(67890);
                break;
                
            case 2:
                // 模式3：电子信息显示
                display_chinese_text();
                break;
                
            case 3:
                // 模式4：全屏测试
                lcd_clear();
                lcd_set_cursor(0, 0);
                lcd_print_string("Mode 4/4");
                lcd_set_cursor(1, 0);
                lcd_print_string("Full Test");
                lcd_set_cursor(2, 0);
                lcd_print_string("ABCDEFGH");
                lcd_set_cursor(3, 0);
                lcd_print_string("12345678");
                break;
        }
        
        // LED指示当前模式
        P1OUT |= STATUS_LED;
        __delay_cycles(500000);     // 0.5秒
        P1OUT &= ~STATUS_LED;
        __delay_cycles(500000);     // 0.5秒
        
        // 每个模式显示3秒
        __delay_cycles(2000000);    // 2秒
        
        pattern++;
    }
}

// =============================================================================
// 主程序
// =============================================================================

/**
 * @brief 主函数
 */
int main(void) {
    // 系统初始化
    clock_init();
    gpio_init();
    
    // 等待系统稳定
    __delay_cycles(100000);     // 100ms
    
    // LCD初始化
    lcd_init();
    
    // 启动时LED闪烁表示初始化完成
    uint8_t i;
    for (i = 0; i < 6; i++) {
        P1OUT ^= STATUS_LED;
        __delay_cycles(200000);  // 200ms
    }
    
    // 显示启动信息
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print_string("LCD INIT OK");
    lcd_set_cursor(1, 0);
    lcd_print_string("Starting...");
    
    __delay_cycles(2000000);    // 2秒延时
    
    // 进入测试模式循环显示
    display_test_patterns();
    
    return 0;
}

// =============================================================================
// 简化版本 - 仅显示"电子信息"
// =============================================================================

/*
 * 如果只想测试基本显示功能，可以将main函数替换为：
 
int main(void) {
    // 基本初始化
    WDTCTL = WDTPW | WDTHOLD;   // 停止看门狗
    
    // 配置DCO为1MHz
    if (CALBC1_1MHZ != 0xFF) {
        DCOCTL = 0;
        BCSCTL1 = CALBC1_1MHZ;
        DCOCTL = CALDCO_1MHZ;
    }
    
    // GPIO初始化
    gpio_init();
    
    // LCD初始化
    lcd_init();
    
    // 显示"电子信息"
    display_chinese_text();
    
    // 主循环
    while (1) {
        P1OUT ^= STATUS_LED;        // LED闪烁表示运行
        __delay_cycles(1000000);    // 1秒
    }
    
    return 0;
}
 
 */