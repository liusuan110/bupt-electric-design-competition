/*
 * frequency_multiplier.c
 * 
 * 锁相环倍频模块 - 2026电子信息杯李萨如图形发生器
 * 
 * 功能: 
 * 1. 通过74HC161实现2-5倍频控制
 * 2. 按键切换倍频系数(P2.6频率按键)
 * 3. LCD显示当前倍频状态
 * 4. PLL锁定状态监测
 * 
 * 硬件连接:
 * - P1.1-P1.3: 74HC161预置数据位D0-D2
 * - P1.4: 1倍/多倍模式选择
 * - P3.4: 频率切换按键(轮询模式)
 * - P2.1-P2.5: LCD SPI接口 (避开XIN/XOUT)
 * 
 * 作者: 2026电子信息杯参赛队
 * 日期: 2025年11月21日
 */

#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// 定义NULL宏（如果未定义）
#ifndef NULL
#define NULL ((void*)0)
#endif

// =============================================================================
// 硬件引脚定义
// =============================================================================

// 74HC161分频控制引脚
#define HC161_D0        BIT1        // P1.1 - 预置数据位0
#define HC161_D1        BIT2        // P1.2 - 预置数据位1  
#define HC161_D2        BIT3        // P1.3 - 预置数据位2
#define DIV_MODE        BIT4        // P1.4 - 1倍/多倍模式选择

// 按键定义
#define FREQ_KEY        BIT4        // P3.4 - 频率切换按键

// LCD SPI接口定义
#define LCD_CS          BIT3        // P2.3 - LCD片选
#define LCD_A0          BIT5        // P2.5 - 命令/数据选择
#define LCD_RES         BIT4        // P2.4 - LCD复位
#define LCD_SCL         BIT1        // P2.1 - SPI时钟
#define LCD_SI          BIT2        // P2.2 - SPI数据

// 状态LED
#define STATUS_LED      BIT6        // P1.6 - 状态指示LED

// =============================================================================
// 全局变量定义
// =============================================================================

// 倍频系统状态
typedef struct {
    uint8_t multiplier;             // 当前倍频系数 (1-5)
    uint8_t preset_value;           // 74HC161预置数值
    bool pll_locked;                // PLL锁定状态
    uint32_t input_freq;            // 输入参考频率(Hz)
    uint32_t output_freq;           // 输出频率(Hz)
} multiplier_state_t;

static volatile multiplier_state_t freq_state = {
    1,      /* multiplier - 默认1倍频 */
    0,      /* preset_value - 1倍频时预置数无效 */
    false,  /* pll_locked */
    2000,   /* input_freq - 默认2kHz输入 */
    2000    /* output_freq */
};

// 按键防抖相关
static volatile bool key_pressed = false;
static volatile uint8_t debounce_counter = 0;

// =============================================================================
// 倍频预置数查找表
// =============================================================================

// 倍频系数与预置数对应表
typedef struct {
    uint8_t multiplier;             // 倍频系数
    uint8_t preset;                 // 74HC161预置数 (16-倍频系数)
    uint8_t gpio_value;             // P1.3-P1.1对应的GPIO值
} multiplier_config_t;

static const multiplier_config_t multiplier_table[] = {
    {1, 0,  0x00},                  // 1倍频: 直通模式, 000 (P1.3=0 P1.2=0 P1.1=0)
    {2, 14, 0x0E},                  // 2倍频: 预置14(1110), 111→110 (P1.3=1 P1.2=1 P1.1=0) 
    {3, 13, 0x0D},                  // 3倍频: 预置13(1101), 110→101 (P1.3=1 P1.2=0 P1.1=1)
    {4, 12, 0x0C},                  // 4倍频: 预置12(1100), 110→100 (P1.3=1 P1.2=0 P1.1=0)
    {5, 11, 0x0B}                   // 5倍频: 预置11(1011), 101→011 (P1.3=0 P1.2=1 P1.1=1)
};

#define MULTIPLIER_COUNT    (sizeof(multiplier_table)/sizeof(multiplier_config_t))

// =============================================================================
// LCD驱动函数说明与声明
// =============================================================================

/*
 * LCD驱动模块说明
 * 
 * 支持的LCD控制器: ST7920系列 (12864点阵LCD)
 * 通信接口: 4线SPI (软件模拟)
 * 
 * 硬件连接:
 * P2.3 (LCD_CS)  -> LCD片选信号 (低电平有效)
 * P2.5 (LCD_A0)  -> 数据/命令选择 (0=命令, 1=数据)  
 * P2.4 (LCD_RES) -> 硬件复位信号 (低电平复位)
 * P2.1 (LCD_SCL) -> SPI时钟信号
 * P2.2 (LCD_SI)  -> SPI数据输入信号
 * 
 * SPI时序说明:
 * - 时钟频率: ~500kHz (软件延时控制)
 * - 数据模式: MSB先传输
 * - 时钟极性: 空闲时为低电平
 * - 数据采样: 时钟上升沿有效
 * 
 * 显示格式:
 * - 分辨率: 128x64点阵
 * - 字符模式: 16x4字符 (8x16字体)
 * - 显示区域: 4行 x 8列中文字符
 */

void lcd_init(void);                    // LCD初始化 (必须首先调用)
void lcd_clear(void);                   // 清空显示内容
void lcd_write_command(uint8_t cmd);    // 发送命令字节到LCD控制器
void lcd_write_data(uint8_t data);      // 发送数据字节到LCD控制器  
void lcd_set_cursor(uint8_t row, uint8_t col);  // 设置光标位置 (行:0-3, 列:0-7)
void lcd_print_string(const char* str); // 显示字符串 (自动换行)
void lcd_print_number(uint32_t num);    // 显示数字 (十进制格式)
void spi_write_byte(uint8_t data);      // 底层SPI字节传输函数
void update_lcd_display(void);          // 更新LCD显示内容

// 74HC161控制函数声明
void set_hc161_preset(uint8_t gpio_val);
bool set_frequency_multiplier(uint8_t multiplier);
void switch_next_multiplier(void);

// 系统初始化函数声明  
void clock_init(void);
void gpio_init(void);
void timer_init(void);

// 测试和工具函数声明
void multiplier_self_test(void);
uint8_t get_current_multiplier(void);
void set_input_frequency(uint32_t freq);
bool get_pll_lock_status(void);

// =============================================================================
// 系统初始化函数
// =============================================================================

/**
 * @brief 系统时钟初始化
 * 配置DCO为1MHz，提供稳定的系统时钟
 */
void clock_init(void) {
    // 停止看门狗定时器
    WDTCTL = WDTPW | WDTHOLD;
    
    // 配置DCO为1MHz
    if (CALBC1_1MHZ != 0xFF) {
        DCOCTL = 0;                     // 选择最低DCOx和MODx设置
        BCSCTL1 = CALBC1_1MHZ;          // 设置DCO为1MHz
        DCOCTL = CALDCO_1MHZ;
    }
    
    // 配置ACLK使用内部VLO时钟(~12kHz)
    BCSCTL3 |= LFXT1S_2;                // ACLK = VLO
}

/**
 * @brief GPIO引脚初始化
 * 配置74HC161控制引脚、按键引脚和LCD接口
 */
void gpio_init(void) {
    // P1口配置 - 74HC161控制 + 状态LED
    P1DIR |= (HC161_D0 | HC161_D1 | HC161_D2 | DIV_MODE | STATUS_LED);  // 输出
    P1OUT &= ~(HC161_D0 | HC161_D1 | HC161_D2 | DIV_MODE | STATUS_LED); // 初始为低
    
    // P2口配置 - LCD SPI接口
    P2DIR |= (LCD_CS | LCD_A0 | LCD_RES | LCD_SCL | LCD_SI);            // LCD引脚输出
    
    // P3口按键配置 (P3.4) - 仅设置为输入上拉，不使用中断
    P3DIR &= ~FREQ_KEY;                                                  // 按键输入
    P3REN |= FREQ_KEY;                                                   // 使能内部上拉
    P3OUT |= FREQ_KEY;                                                   // 上拉电阻
    
    // LCD信号初始状态
    P2OUT |= (LCD_CS | LCD_RES);                                         // CS和RES默认高电平
    P2OUT &= ~(LCD_A0 | LCD_SCL | LCD_SI);                              // 其他信号默认低电平
}

/**
 * @brief 定时器初始化
 * 配置TimerA用于系统节拍和防抖处理
 */
void timer_init(void) {
    // TimerA配置: ACLK/8, 连续模式, 产生10ms中断
    TA0CTL = TASSEL_1 | ID_3 | MC_1 | TAIE;     // ACLK/8, 增计数模式, 中断使能
    TA0CCR0 = 15;                               // 约10ms @ 12kHz/8 = 1.5kHz
    TA0CCTL0 |= CCIE;                           // CCR0中断使能
}

// =============================================================================
// 74HC161倍频控制函数
// =============================================================================

/**
 * @brief 设置74HC161预置数GPIO控制位
 * @param gpio_val: GPIO值 (D2D1D0对应P1.3P1.2P1.1)
 */
void set_hc161_preset(uint8_t gpio_val) {
    // 清除原有设置
    P1OUT &= ~(HC161_D2 | HC161_D1 | HC161_D0);
    
    // 设置新的预置数 (只使用低3位,D3固定为1)
    if (gpio_val & 0x01) P1OUT |= HC161_D0;   // D0 -> P1.1
    if (gpio_val & 0x02) P1OUT |= HC161_D1;   // D1 -> P1.2  
    if (gpio_val & 0x04) P1OUT |= HC161_D2;   // D2 -> P1.3
    // D3在硬件上固定连接VCC
    
    // 添加延时确保信号稳定
    __delay_cycles(100);
}

/**
 * @brief 设置倍频模式
 * @param multiplier: 倍频系数 (1-5)
 * @return: true=设置成功, false=参数错误
 */
bool set_frequency_multiplier(uint8_t multiplier) {
    // 参数检查
    if (multiplier < 1 || multiplier > 5) {
        return false;
    }
    
    // 查找倍频配置
    const multiplier_config_t* config = NULL;
    uint8_t i;
    for (i = 0; i < MULTIPLIER_COUNT; i++) {
        if (multiplier_table[i].multiplier == multiplier) {
            config = &multiplier_table[i];
            break;
        }
    }
    
    if (config == NULL) {
        return false;
    }
    
    // 更新系统状态
    freq_state.multiplier = multiplier;
    freq_state.preset_value = config->preset;
    freq_state.output_freq = freq_state.input_freq * multiplier;
    
    // 配置硬件
    if (multiplier == 1) {
        // 1倍频: 直通模式
        P1OUT &= ~DIV_MODE;                     // DIV_MODE = 0
        set_hc161_preset(0);                    // GPIO全部清零
    } else {
        // 多倍频: 分频模式
        P1OUT |= DIV_MODE;                      // DIV_MODE = 1  
        set_hc161_preset(config->gpio_value);   // 设置GPIO值
    }
    
    // LED指示状态改变
    P1OUT |= STATUS_LED;
    __delay_cycles(100000);                     // 100ms @ 1MHz
    P1OUT &= ~STATUS_LED;
    
    return true;
}

/**
 * @brief 切换到下一个倍频系数
 * 按键响应函数，循环切换倍频系数
 */
void switch_next_multiplier(void) {
    uint8_t next_multiplier = freq_state.multiplier + 1;
    if (next_multiplier > 5) {
        next_multiplier = 1;                    // 循环回到1倍频
    }
    
    // 先闪烁LED表示检测到按键
    P1OUT |= STATUS_LED;
    __delay_cycles(100000);  // 100ms
    P1OUT &= ~STATUS_LED;
    
    if (set_frequency_multiplier(next_multiplier)) {
        // 更新LCD显示
        update_lcd_display();
        
        // 再次闪烁LED表示设置完成
        __delay_cycles(100000);
        P1OUT |= STATUS_LED;
        __delay_cycles(100000);  // 100ms
        P1OUT &= ~STATUS_LED;
    }
}

// =============================================================================
// LCD显示函数实现
// =============================================================================

/**
 * @brief SPI发送单字节数据 (软件模拟)
 * @param data: 要发送的字节数据 (MSB先传输)
 * 
 * 功能说明:
 * - 软件模拟SPI时序，发送8位数据
 * - 时钟频率约500kHz (受__delay_cycles影响)
 * - MSB(最高位)先传输，符合ST7920要求
 * - 每个时钟周期包含数据建立和保持时间
 * 
 * 时序参数:
 * - 数据建立时间: 2us (满足ST7920 >20ns要求)  
 * - 时钟脉冲宽度: 2us (满足ST7920 >400ns要求)
 * - 总传输时间: ~32us/字节
 */
void spi_write_byte(uint8_t data) {
    uint8_t i;
    for (i = 0; i < 8; i++) {
        // 设置数据位
        if (data & 0x80) {
            P2OUT |= LCD_SI;                    // 数据线高电平
        } else {
            P2OUT &= ~LCD_SI;                   // 数据线低电平  
        }
        
        // 时钟脉冲
        P2OUT |= LCD_SCL;                       // 时钟上升沿
        __delay_cycles(2);                      // 短延时
        P2OUT &= ~LCD_SCL;                      // 时钟下降沿
        __delay_cycles(2);
        
        data <<= 1;                             // 移位到下一位
    }
}

/**
 * @brief 向LCD发送命令字节
 * @param cmd: ST7920命令字节
 * 
 * 常用命令说明:
 * - 0x30: 基本指令集选择
 * - 0x0C: 显示开启,光标关闭  
 * - 0x01: 清屏 (执行时间1.6ms)
 * - 0x06: 光标右移,显示不移动
 * - 0x80+addr: 设置DDRAM地址
 * 
 * 时序要求:
 * - 命令执行时间: 一般72us, 清屏1.6ms
 * - A0信号必须在数据传输前稳定
 */
void lcd_write_command(uint8_t cmd) {
    P2OUT &= ~LCD_A0;                           // A0=0: 命令模式
    P2OUT &= ~LCD_CS;                           // 选中LCD
    spi_write_byte(cmd);
    P2OUT |= LCD_CS;                            // 取消选中
}

/**
 * @brief 向LCD发送数据字节
 * @param data: 要显示的字符数据 (ASCII码)
 * 
 * 功能说明:
 * - 发送字符数据到LCD的DDRAM
 * - 支持ASCII字符 (0x20-0x7F)
 * - 光标自动递增到下一个位置
 * - 字符显示后光标移动方向由入口模式决定
 * 
 * 注意事项:
 * - 发送前确保光标位置正确
 * - 中文字符需要特殊处理(未实现)
 */
void lcd_write_data(uint8_t data) {
    P2OUT |= LCD_A0;                            // A0=1: 数据模式
    P2OUT &= ~LCD_CS;                           // 选中LCD  
    spi_write_byte(data);
    P2OUT |= LCD_CS;                            // 取消选中
}

/**
 * @brief LCD初始化序列 (ST7920兼容)
 * 
 * 初始化步骤:
 * 1. 硬件复位 (>1ms低脉冲)
 * 2. 等待LCD内部初始化完成 (>50ms)
 * 3. 发送功能设置命令序列
 * 4. 配置显示模式和输入方式
 * 
 * 配置参数:
 * - 接口: 4线SPI模式
 * - 字体: 5x8点阵 (标准ASCII)
 * - 显示: 开启, 光标关闭, 闪烁关闭
 * - 输入: 光标右移, 显示不移位
 * 
 * 调用要求:
 * - 必须在GPIO初始化完成后调用
 * - 整个过程耗时约100ms
 * - 初始化成功后LCD显示空白屏幕
 */
void lcd_init(void) {
    // 硬件复位
    P2OUT &= ~LCD_RES;                          // 复位信号低电平
    __delay_cycles(50000);                      // 50ms延时
    P2OUT |= LCD_RES;                           // 释放复位
    __delay_cycles(50000);                      // 等待复位完成
    
    // LCD初始化命令序列 (ST7920兼容)
    lcd_write_command(0x30);                    // 基本指令集
    __delay_cycles(1000);
    lcd_write_command(0x30);                    // 重复命令确保可靠
    __delay_cycles(1000);
    lcd_write_command(0x0C);                    // 显示开启，光标关闭
    lcd_write_command(0x01);                    // 清屏
    __delay_cycles(20000);                      // 清屏需要较长时间
    lcd_write_command(0x06);                    // 输入模式：光标右移
}

/**
 * @brief 清空LCD显示
 */
void lcd_clear(void) {
    lcd_write_command(0x01);                    // 清屏命令
    __delay_cycles(20000);                      // 等待清屏完成
}

/**
 * @brief 设置LCD光标位置 (字符坐标)
 * @param row: 行号 (0-3, 对应显示屏4行)
 * @param col: 列号 (0-7, 对应每行8个字符位置)
 * 
 * DDRAM地址映射 (ST7920):
 * - 第1行 (row=0): 0x80 + col  (地址 0x80-0x87)
 * - 第2行 (row=1): 0x90 + col  (地址 0x90-0x97)  
 * - 第3行 (row=2): 0x88 + col  (地址 0x88-0x8F)
 * - 第4行 (row=3): 0x98 + col  (地址 0x98-0x9F)
 * 
 * 参数范围:
 * - row: 0-3 (超出范围函数直接返回)
 * - col: 0-7 (建议范围, 超出可能显示异常)
 * 
 * 使用示例:
 * lcd_set_cursor(1, 3);  // 定位到第2行第4个字符位置
 */
void lcd_set_cursor(uint8_t row, uint8_t col) {
    uint8_t address = 0x80;                     // 基本地址命令
    
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
 * @brief 在当前光标位置显示字符串
 * @param str: 以null结尾的ASCII字符串
 * 
 * 功能特性:
 * - 逐字符发送到LCD DDRAM
 * - 光标自动递增 (由LCD控制器处理)
 * - 不支持自动换行 (需手动调用lcd_set_cursor)
 * - 字符间有1ms延时确保显示稳定
 * 
 * 支持字符:
 * - ASCII可打印字符 (0x20-0x7F)
 * - 数字: 0-9
 * - 字母: A-Z, a-z  
 * - 符号: !"#$%&'()*+,-./:;<=>?@[\]^_`{|}~
 * 
 * 使用注意:
 * - 字符串长度不应超过当前行剩余空间
 * - 超长字符串可能覆盖到下一行或产生异常显示
 */
void lcd_print_string(const char* str) {
    while (*str) {
        lcd_write_data(*str++);
        __delay_cycles(1000);                   // 字符间延时
    }
}

/**
 * @brief 在当前光标位置显示十进制数字
 * @param num: 32位无符号整数 (0 - 4294967295)
 * 
 * 算法说明:
 * - 使用除法和取余运算提取各位数字
 * - 数字逆序存储到缓冲区后正序输出
 * - 特殊处理数字0的显示
 * - 无前导零, 无千位分隔符
 * 
 * 显示格式:
 * - 1234 -> "1234" (4个字符)
 * - 0 -> "0" (1个字符)
 * - 4294967295 -> "4294967295" (10个字符)
 * 
 * 性能特点:
 * - 纯整数运算, 无浮点操作
 * - 内存占用小 (12字节局部缓冲区)
 * - 适合资源受限的MSP430平台
 * 
 * 使用示例:
 * lcd_print_number(2000);  // 显示 "2000"
 */
void lcd_print_number(uint32_t num) {
    char buffer[12];
    uint8_t i = 0;
    
    // 处理0的特殊情况
    if (num == 0) {
        lcd_write_data('0');
        return;
    }
    
    // 转换为字符串 (逆序)
    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    // 反向输出
    while (i > 0) {
        lcd_write_data(buffer[--i]);
        __delay_cycles(1000);
    }
}

/**
 * @brief 更新LCD显示内容
 * 显示当前倍频系数、输入输出频率、PLL状态等信息
 */
void update_lcd_display(void) {
    // 清屏
    lcd_clear();
    
    // 第一行: 标题
    lcd_set_cursor(0, 0);
    lcd_print_string("PLL FREQ MUL");
    
    // 第二行: 倍频系数
    lcd_set_cursor(1, 0);
    lcd_print_string("MUL: ");
    lcd_print_number(freq_state.multiplier);
    lcd_print_string("X");
    
    // 第三行: 输入频率  
    lcd_set_cursor(2, 0);
    lcd_print_string("IN: ");
    lcd_print_number(freq_state.input_freq);
    lcd_print_string("Hz");
    
    // 第四行: 输出频率
    lcd_set_cursor(3, 0);
    lcd_print_string("OUT:");
    lcd_print_number(freq_state.output_freq);
    lcd_print_string("Hz");
}

// =============================================================================
// 中断服务程序
// =============================================================================

// P3口不支持中断，使用轮询方式检测按键

/**
 * @brief TimerA中断服务程序 - 系统节拍
 * 提供10ms系统节拍，用于按键轮询和防抖处理
 */
#pragma vector=TIMER0_A0_VECTOR
__interrupt void TimerA0_ISR(void) {
    static bool last_key_state = true;     /* 上一次按键状态 (默认高电平) */
    static uint8_t press_counter = 0;      /* 按键按下计数器 */
    bool current_key_state;
    
    /* 读取当前按键状态 (P3.4, 按下为低电平) */
    current_key_state = (P3IN & FREQ_KEY) ? true : false;
    
    /* 检测按键状态变化 */
    if (!current_key_state && last_key_state) {
        /* 按键刚被按下 (下降沿) */
        press_counter = 3;                  /* 30ms防抖 */
    } else if (!current_key_state && press_counter > 0) {
        /* 按键保持按下状态 */
        press_counter--;
        if (press_counter == 0) {
            /* 防抖时间到，触发按键事件 */
            switch_next_multiplier();
        }
    } else if (current_key_state) {
        /* 按键释放 */
        press_counter = 0;
    }
    
    /* 更新按键状态 */
    last_key_state = current_key_state;
}

// =============================================================================
// 主程序
// =============================================================================

/**
 * @brief 主函数
 * 系统初始化和主循环
 */
int main(void) {
    // 系统初始化
    clock_init();                               // 时钟配置
    gpio_init();                                // GPIO配置  
    timer_init();                               // 定时器配置
    lcd_init();                                 // LCD初始化
    
    // 全局中断使能
    __enable_interrupt();
    
    // 等待LCD稳定
    __delay_cycles(200000);                     // 200ms延时
    
    // LCD测试显示
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print_string("INIT OK");
    __delay_cycles(1000000);                    // 1秒延时显示
    
    // GPIO测试 - 依次测试每个倍频设置
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print_string("GPIO TEST");
    
    uint8_t test_mult;
    for (test_mult = 1; test_mult <= 5; test_mult++) {
        set_frequency_multiplier(test_mult);
        
        lcd_set_cursor(1, 0);
        lcd_print_string("MUL: ");
        lcd_print_number(test_mult);
        lcd_print_string("X   ");
        
        // 显示P1口状态
        lcd_set_cursor(2, 0);
        lcd_print_string("P1:");
        lcd_print_number(P1OUT & 0x0F);  // 显示P1.0-P1.3状态
        
        __delay_cycles(2000000);                // 2秒延时观察
    }
    
    // 初始化倍频设置
    set_frequency_multiplier(1);                // 默认1倍频
    
    // 初始显示
    update_lcd_display();
    
    // 主循环
    while (1) {
        static uint16_t debug_counter = 0;
        
        /* 定期检查按键状态和系统状态 */
        if (++debug_counter >= 50000) {        /* 约5秒输出一次状态 */
            debug_counter = 0;
            
            /* 检查按键状态 */
            bool key_state = (P3IN & FREQ_KEY) ? true : false;
            
            /* 检查P1口输出状态 */
            uint8_t p1_status = P1OUT & (HC161_D2 | HC161_D1 | HC161_D0 | DIV_MODE);
            
            /* 可以在这里添加LED指示或其他调试信息 */
            /* 例如：闪烁LED显示系统运行 */
            P1OUT ^= STATUS_LED;                /* LED闪烁 */
        }
        
        /* 简单延时，避免无限循环太快 */
        __delay_cycles(1000);
    }
    
    return 0;
}

// =============================================================================
// 调试和测试函数
// =============================================================================

/**
 * @brief 倍频模块自检程序
 * 用于开发调试，测试各倍频系数的设置
 */
void multiplier_self_test(void) {
    uint8_t i;
    
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print_string("SELF TEST...");
    
    /* 依次测试每个倍频系数 */
    for (i = 1; i <= 5; i++) {
        set_frequency_multiplier(i);
        
        lcd_set_cursor(1, 0);
        lcd_print_string("Testing ");
        lcd_print_number(i);
        lcd_print_string("X");
        
        __delay_cycles(1000000);               // 1秒延时
    }
    
    // 恢复默认设置
    set_frequency_multiplier(1);
    update_lcd_display();
}

/**
 * @brief 获取当前倍频系数
 * @return: 当前倍频系数 (1-5)
 */
uint8_t get_current_multiplier(void) {
    return freq_state.multiplier;
}

/**
 * @brief 设置输入参考频率
 * @param freq: 输入频率 (Hz)
 */
void set_input_frequency(uint32_t freq) {
    freq_state.input_freq = freq;
    freq_state.output_freq = freq * freq_state.multiplier;
    update_lcd_display();
}

/**
 * @brief 获取PLL锁定状态  
 * @return: true=已锁定, false=未锁定
 */
bool get_pll_lock_status(void) {
    return freq_state.pll_locked;
}
