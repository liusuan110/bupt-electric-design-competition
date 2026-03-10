/*******************************************************************************
 * 李萨如图形发生器主程序 - 2026电子信息杯
 * 
 * 功能模块:
 * 1. 锁相环倍频控制 (PLL Frequency Multiplier) - 核心技术
 * 2. AGC自动增益控制 (Automatic Gain Control)
 * 3. LCD显示界面 (User Interface) 
 * 4. 按键交互控制 (Key Input Processing)
 * 
 * 硬件连接:
 * - P1.1-P1.3: 74HC161预置数控制
 * - P1.4: 1倍/多倍模式选择  
 * - P1.5: 幅度切换按键
 * - P2.6: 频率切换按键
 * - P2.2-P2.5,P2.7: LCD SPI接口
 * - P1.0: Y轴ADC输入(AGC专用)
 * - P1.6-P1.7: I2C数字电位器
 * 
 * 作者: 2026电子信息杯参赛队
 * 日期: 2025年11月21日
 *******************************************************************************/

#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// 包含倍频模块
#include "frequency_multiplier.h"

//==============================================================================
// 硬件引脚定义 (基于完整引脚对照表v2.3)
//==============================================================================

// 74HC161倍频控制引脚 (P1口)
#define HC161_D0        BIT1        // P1.1 - 预置数据位0
#define HC161_D1        BIT2        // P1.2 - 预置数据位1  
#define HC161_D2        BIT3        // P1.3 - 预置数据位2
#define DIV_MODE        BIT4        // P1.4 - 1倍/多倍模式选择

// 按键定义 (更新后的分配)
#define AMP_KEY         BIT5        // P1.5 - 幅度切换按键
#define FREQ_KEY        BIT6        // P2.6 - 频率切换按键

// ADC和I2C引脚
#define Y_AGC_INPUT     BIT0        // P1.0 - Y轴AGC专用ADC输入
#define I2C_SCL         BIT6        // P1.6 - 数字电位器SCL
#define I2C_SDA         BIT7        // P1.7 - 数字电位器SDA

// 相位检测输入
#define X_PHASE_INPUT   BIT0        // P2.0 - X轴过零检测
#define Y_PHASE_INPUT   BIT1        // P2.1 - Y轴过零检测

// LCD SPI接口 (P2口)
#define LCD_CS          BIT2        // P2.2 - LCD片选
#define LCD_A0          BIT3        // P2.3 - 命令/数据选择
#define LCD_RES         BIT4        // P2.4 - LCD复位
#define LCD_SCL         BIT5        // P2.5 - SPI时钟
#define LCD_SI          BIT7        // P2.7 - SPI数据

// 状态指示
#define STATUS_LED      BIT6        // P1.6 - 状态LED (与I2C_SCL复用)

//==============================================================================
// 系统配置常量
//==============================================================================

// 系统版本信息
#define SYSTEM_VERSION_MAJOR    2
#define SYSTEM_VERSION_MINOR    3
#define BUILD_DATE              "2025-11-21"

// LCD显示参数
#define LCD_WIDTH       128
#define LCD_HEIGHT      64  
#define LCD_PAGES       8
#define LCD_ROWS        4
#define LCD_COLS        16

// 倍频系统参数
#define FREQ_MULT_MIN   1       // 最小倍频系数
#define FREQ_MULT_MAX   5       // 最大倍频系数
#define AMP_LEVELS      3       // 幅度等级: 1V, 2V, 3V
#define DEFAULT_INPUT_FREQ  2000    // 默认输入频率 2kHz

// ADC采样参数 (AGC控制)
#define ADC_SAMPLES     32      // ADC采样点数(优化后)
#define AGC_TARGET_1V   205     // 1V对应ADC值 (1024*1V/5V)
#define AGC_TARGET_2V   410     // 2V对应ADC值
#define AGC_TARGET_3V   615     // 3V对应ADC值
#define AGC_TOLERANCE   50      // AGC容差 (±2.5%)

// I2C数字电位器参数
#define MCP4018T_ADDR   0x2E    // MCP4018T I2C地址 (7位)
#define DAC_MAX_VALUE   127     // 7位DAC最大值

// 频率选择表
#define FREQ_COUNT      5
static const uint16_t frequency_table[FREQ_COUNT] = {2000, 4000, 6000, 8000, 10000};

//==============================================================================
// 数据结构定义
//==============================================================================

// 系统运行状态
typedef enum {
    SYS_STATE_INIT = 0,         // 系统初始化
    SYS_STATE_READY,            // 就绪状态  
    SYS_STATE_RUNNING,          // 正常运行
    SYS_STATE_AGC_ADJUSTING,    // AGC调节中
    SYS_STATE_ERROR             // 错误状态
} system_state_t;

typedef struct {
    uint8_t freq_index;         // 频率索引 (0-4对应2k/4k/6k/8k/10kHz)
    uint8_t multiplier;         // 倍频系数 (1-5)
    uint8_t amplitude;          // 幅度选择 (1:1V, 2:2V, 3:3V)
    uint16_t input_freq;        // 当前输入频率 (Hz)
    uint16_t output_freq;       // 输出频率 (Hz)
    float phase_diff;           // 相位差 (度)
    bool pll_locked;            // PLL锁定状态
    bool display_update;        // 显示更新标志
    system_state_t state;       // 系统状态
} system_params_t;

typedef struct {
    uint16_t buffer[ADC_SAMPLES];
    uint16_t index;
    bool ready;
} adc_data_t;

typedef struct {
    uint32_t x_period;          // X轴周期 (Timer计数值)
    uint32_t y_period;          // Y轴周期 (Timer计数值)
    float phase_diff;           // 计算得到的相位差
    uint16_t sample_count;      // 采样计数
} phase_measure_t;

//==============================================================================
// 全局变量
//==============================================================================

// 系统参数初始化
static system_params_t g_params = {
    .freq_index = 0,            // 默认2kHz
    .multiplier = 1,            // 默认1倍频
    .amplitude = 2,             // 默认2V
    .input_freq = 2000,         // 默认2kHz输入
    .output_freq = 2000,        // 默认2kHz输出
    .phase_diff = 0.0,
    .pll_locked = false,
    .display_update = true,
    .state = SYS_STATE_INIT
};

// AGC控制变量
static volatile uint16_t adc_buffer[ADC_SAMPLES];
static volatile uint8_t adc_index = 0;
static volatile bool adc_ready = false;
static volatile uint8_t dac_value = 64;             // 数字电位器中间值

// 按键状态变量
static volatile bool freq_key_pressed = false;
static volatile bool amp_key_pressed = false;
static volatile uint8_t debounce_counter = 0;

// 系统运行统计
static volatile uint32_t system_uptime = 0;        // 系统运行时间(秒)
static volatile uint16_t key_press_count = 0;      // 按键计数

// LCD显示缓存
static char lcd_buffer[32];
static uint8_t g_lcd_framebuffer[LCD_WIDTH * LCD_PAGES];

//==============================================================================
// 函数声明
//==============================================================================

// 系统初始化
void system_init(void);
void gpio_init(void);
void timer_init(void);
void adc_init(void);
void i2c_init(void);

// 按键处理
void process_frequency_key(void);
void process_amplitude_key(void);

// AGC控制
void agc_process(void);
void set_target_amplitude(uint8_t amplitude);
uint16_t calculate_peak_to_peak(void);
uint16_t get_agc_target(void);

// 相位测量
float measure_phase_difference(void);

// 显示控制
void show_startup_screen(void);
void update_system_display(void);
void show_error_screen(const char* error_msg);

// 系统监控
void system_monitor(void);

// LCD驱动
void lcd_init(void);
void lcd_clear(void);
void lcd_write_command(uint8_t cmd);
void lcd_write_data(uint8_t data);
void lcd_set_page(uint8_t page);
void lcd_set_column(uint8_t col);
void lcd_draw_pixel(uint8_t x, uint8_t y, uint8_t color);
void lcd_draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
void lcd_update_display(void);

// I2C通信 (数字电位器控制)
void i2c_start(void);
void i2c_stop(void);
void i2c_write_byte(uint8_t data);
void mcp4018t_set_value(uint8_t value);

// 信号处理
void adc_start_conversion(void);
uint16_t get_frequency(void);
void update_pll_divider(uint8_t mult);
void set_amplitude_gain(uint8_t amp_level);
void select_waveform(uint8_t wave_type);

// 相位测量
void phase_measure_init(void);
float calculate_phase_difference(void);

// 图形显示
void draw_lissajous_curve(void);
void draw_parameters_info(void);
void draw_grid(void);

// 用户界面
void process_key_input(void);
void update_system_parameters(void);

// 工具函数
void delay_ms(uint16_t ms);
void delay_us(uint16_t us);

//==============================================================================
// 主函数
//==============================================================================

/**
 * @brief 主函数 - 李萨如图形发生器
 * 集成倍频控制、AGC、LCD显示等功能模块
 */
int main(void) 
{
    // 关闭看门狗定时器
    WDTCTL = WDTPW | WDTHOLD;
    
    // 系统初始化
    system_init();
    
    // 显示启动画面  
    show_startup_screen();
    
    // 初始化倍频模块
    frequency_multiplier_init();
    
    // 设置初始参数
    set_frequency_multiplier(g_params.multiplier);
    set_input_frequency(g_params.input_freq);
    set_target_amplitude(g_params.amplitude);
    
    // 更新系统状态
    g_params.state = SYS_STATE_READY;
    
    // 全局中断使能
    __enable_interrupt();
    
    // 主循环
    while(1) 
    {
        // 处理频率按键
        if (freq_key_pressed) {
            __delay_cycles(50000);                  // 防抖延时 50ms
            
            if (!(P2IN & FREQ_KEY)) {               // 确认按键仍按下
                process_frequency_key();
                key_press_count++;
            }
            
            freq_key_pressed = false;
        }
        
        // 处理幅度按键
        if (amp_key_pressed) {
            __delay_cycles(50000);                  // 防抖延时 50ms
            
            if (!(P1IN & AMP_KEY)) {                // 确认按键仍按下  
                process_amplitude_key();
                key_press_count++;
            }
            
            amp_key_pressed = false;
        }
        
        // AGC处理
        agc_process();
        
        // 更新PLL锁定状态
        g_params.pll_locked = get_pll_lock_status();
        
        // 测量相位差 (简化实现)
        g_params.phase_diff = measure_phase_difference();
        
        // 系统监控
        system_monitor();
        
        // 定期更新显示 (每500ms更新一次)
        static uint16_t display_counter = 0;
        if (++display_counter >= 50) {             // 约500ms
            display_counter = 0;
            update_system_display();
        }
        
        // 更新系统状态
        if (g_params.pll_locked && (abs((int16_t)calculate_peak_to_peak() - get_agc_target()) < AGC_TOLERANCE)) {
            g_params.state = SYS_STATE_RUNNING;
        } else {
            g_params.state = SYS_STATE_AGC_ADJUSTING;
        }
        
        // 进入低功耗模式，等待中断
        __bis_SR_register(LPM0_bits | GIE);
    }
    
    return 0;
}

//==============================================================================
// 系统初始化函数
//==============================================================================

void system_init(void)
{
    // 设置时钟为16MHz
    DCOCTL = 0;
    BCSCTL1 = CALBC1_16MHZ;
    DCOCTL = CALDCO_16MHZ;
    
    // 初始化各个模块
    gpio_init();
    timer_init();
    adc_init();
    i2c_init();
    lcd_init();
    phase_measure_init();
    
    // 设置初始参数
    update_pll_divider(g_params.freq_mult);
    set_amplitude_gain(g_params.amplitude);
    select_waveform(g_params.wave_type);
    
    // 使能全局中断
    __bis_SR_register(GIE);
}

void gpio_init(void)
{
    // P1口配置
    P1DIR |= LCD_RS | LCD_RW | LCD_E | LCD_CS1 | LCD_CS2 | LCD_RST;  // LCD控制引脚输出
    P1DIR &= ~(KEY_FREQ | KEY_AMP | KEY_WAVE | KEY_MENU);            // 按键引脚输入
    P1DIR &= ~(X_ZERO | Y_ZERO);                                     // 过零检测引脚输入
    P1REN |= (KEY_FREQ | KEY_AMP | KEY_WAVE | KEY_MENU);             // 按键上拉电阻
    P1OUT |= (KEY_FREQ | KEY_AMP | KEY_WAVE | KEY_MENU);             // 按键上拉使能
    
    // P2口配置 (LCD数据总线)
    P2DIR = 0xFF;   // 全部配置为输出
    P2OUT = 0x00;   // 初始化为低电平
}

void timer_init(void)
{
    // Timer_A配置用于频率测量和相位检测
    TA0CTL = TASSEL_2 | MC_2 | TACLR;           // SMCLK, 连续模式, 清除计数器
    TA0CCTL0 = CM_1 | CCIS_0 | SCS | CAP | CCIE; // 上升沿捕获, CCI0A, 同步, 捕获模式, 中断使能
    TA0CCTL1 = CM_1 | CCIS_0 | SCS | CAP | CCIE; // 上升沿捕获, CCI1A, 同步, 捕获模式, 中断使能
}

void adc_init(void)
{
    // ADC10配置
    ADC10CTL0 = ADC10SHT_2 | ADC10ON | ADC10IE;    // 16 ADC10CLKs, ADC on, interrupt enable
    ADC10CTL1 = INCH_7;                             // A7 input channel
    ADC10AE0 |= BIT7;                               // PA.7 ADC option select
}

void i2c_init(void)
{
    // 软件I2C初始化
    P1DIR |= SDA_PIN | SCL_PIN;     // SDA和SCL配置为输出
    P1OUT |= SDA_PIN | SCL_PIN;     // 初始化为高电平
}

//==============================================================================
// LCD驱动函数
//==============================================================================

void lcd_init(void)
{
    // LCD复位
    P2OUT &= ~LCD_RST;
    delay_ms(10);
    P2OUT |= LCD_RST;
    delay_ms(10);
    
    // 初始化命令序列
    lcd_write_command(0x3F);    // Display ON
    lcd_write_command(0x40);    // Start line = 0
    lcd_write_command(0xB8);    // Page 0
    lcd_write_command(0xC0);    // Start column = 0
    
    lcd_clear();
}

void lcd_clear(void)
{
    for (uint8_t page = 0; page < LCD_PAGES; page++) {
        lcd_set_page(page);
        lcd_set_column(0);
        for (uint8_t col = 0; col < LCD_WIDTH; col++) {
            lcd_write_data(0x00);
        }
    }
}

void lcd_write_command(uint8_t cmd)
{
    P2OUT &= ~LCD_RS;           // RS = 0 (命令)
    P2OUT &= ~LCD_RW;           // RW = 0 (写)
    P2OUT = (P2OUT & 0x07) | ((cmd & 0x1F) << 3);  // 写入命令
    P2OUT |= LCD_E;             // E = 1
    delay_us(1);
    P2OUT &= ~LCD_E;            // E = 0
    delay_us(1);
}

void lcd_write_data(uint8_t data)
{
    P2OUT |= LCD_RS;            // RS = 1 (数据)
    P2OUT &= ~LCD_RW;           // RW = 0 (写)
    P2OUT = (P2OUT & 0x07) | ((data & 0x1F) << 3);  // 写入数据
    P2OUT |= LCD_E;             // E = 1
    delay_us(1);
    P2OUT &= ~LCD_E;            // E = 0
    delay_us(1);
}

void lcd_set_page(uint8_t page)
{
    lcd_write_command(0xB8 | (page & 0x07));
}

void lcd_set_column(uint8_t col)
{
    lcd_write_command(0x40 | (col & 0x3F));
}

void lcd_draw_pixel(uint8_t x, uint8_t y, uint8_t color)
{
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    
    uint8_t page = y / 8;
    uint8_t bit = y % 8;
    uint16_t index = page * LCD_WIDTH + x;
    
    if (color) {
        g_lcd_buffer[index] |= (1 << bit);
    } else {
        g_lcd_buffer[index] &= ~(1 << bit);
    }
}

void lcd_update_display(void)
{
    for (uint8_t page = 0; page < LCD_PAGES; page++) {
        lcd_set_page(page);
        lcd_set_column(0);
        for (uint8_t col = 0; col < LCD_WIDTH; col++) {
            uint16_t index = page * LCD_WIDTH + col;
            lcd_write_data(g_lcd_buffer[index]);
        }
    }
}

//==============================================================================
// I2C通信函数 (数字电位器控制)
//==============================================================================

void i2c_start(void)
{
    P1OUT |= SDA_PIN | SCL_PIN;
    delay_us(5);
    P1OUT &= ~SDA_PIN;
    delay_us(5);
    P1OUT &= ~SCL_PIN;
    delay_us(5);
}

void i2c_stop(void)
{
    P1OUT &= ~SDA_PIN;
    delay_us(5);
    P1OUT |= SCL_PIN;
    delay_us(5);
    P1OUT |= SDA_PIN;
    delay_us(5);
}

void i2c_write_byte(uint8_t data)
{
    for (uint8_t i = 0; i < 8; i++) {
        if (data & 0x80) {
            P1OUT |= SDA_PIN;
        } else {
            P1OUT &= ~SDA_PIN;
        }
        delay_us(2);
        P1OUT |= SCL_PIN;
        delay_us(5);
        P1OUT &= ~SCL_PIN;
        delay_us(2);
        data <<= 1;
    }
    
    // ACK
    P1DIR &= ~SDA_PIN;      // SDA输入
    delay_us(2);
    P1OUT |= SCL_PIN;
    delay_us(5);
    P1OUT &= ~SCL_PIN;
    delay_us(2);
    P1DIR |= SDA_PIN;       // SDA输出
}

void mcp4018t_set_value(uint8_t value)
{
    i2c_start();
    i2c_write_byte(MCP4018T_ADDR << 1);  // 器件地址 + 写位
    i2c_write_byte(value & 0x7F);        // 7位数据
    i2c_stop();
}

//==============================================================================
// 信号处理函数
//==============================================================================

void adc_start_conversion(void)
{
    ADC10CTL0 |= ENC | ADC10SC;     // 使能转换并开始
}

uint16_t get_frequency(void)
{
    // 基于Timer_A捕获的频率测量
    // 这里使用简化的算法，实际应用中需要更精确的计算
    static uint32_t last_capture = 0;
    uint32_t current_capture = TA0CCR0;
    uint32_t period = current_capture - last_capture;
    last_capture = current_capture;
    
    if (period > 0) {
        return (uint16_t)(16000000UL / period);  // 16MHz时钟
    }
    return 1000;  // 默认值
}

void update_pll_divider(uint8_t mult)
{
    // 通过74HC161控制PLL分频比
    // 这里需要根据具体的硬件连接实现
    // 示例：通过GPIO控制分频器
    P1OUT = (P1OUT & 0xF0) | (mult & 0x0F);
}

void set_amplitude_gain(uint8_t amp_level)
{
    // 计算数字电位器值以实现目标幅度
    uint8_t dac_value;
    
    switch (amp_level) {
        case 0: dac_value = 25;  break;   // 1V
        case 1: dac_value = 64;  break;   // 2V  
        case 2: dac_value = 100; break;   // 3V
        default: dac_value = 64; break;
    }
    
    mcp4018t_set_value(dac_value);
}

void select_waveform(uint8_t wave_type)
{
    // 通过CD4052控制波形选择
    if (wave_type == 0) {
        P1OUT &= ~BIT2;     // 选择正弦波
    } else {
        P1OUT |= BIT2;      // 选择三角波
    }
}

//==============================================================================
// 相位测量函数
//==============================================================================

void phase_measure_init(void)
{
    // 配置Timer_A用于相位测量
    g_phase_data.sample_count = 0;
    g_phase_data.phase_diff = 0.0;
}

float calculate_phase_difference(void)
{
    // 基于过零点时间差计算相位差
    static uint32_t x_zero_time = 0;
    static uint32_t y_zero_time = 0;
    static bool x_zero_detected = false;
    static bool y_zero_detected = false;
    
    // 检测X轴过零点
    if ((P1IN & X_ZERO) && !x_zero_detected) {
        x_zero_time = TA0R;
        x_zero_detected = true;
    }
    if (!(P1IN & X_ZERO)) {
        x_zero_detected = false;
    }
    
    // 检测Y轴过零点  
    if ((P1IN & Y_ZERO) && !y_zero_detected) {
        y_zero_time = TA0R;
        y_zero_detected = true;
    }
    if (!(P1IN & Y_ZERO)) {
        y_zero_detected = false;
    }
    
    // 计算相位差
    if (x_zero_time != 0 && y_zero_time != 0) {
        uint32_t time_diff = (y_zero_time > x_zero_time) ? 
                           (y_zero_time - x_zero_time) : 
                           (x_zero_time - y_zero_time);
        
        // 转换为相位差 (度)
        float period = 16000000.0 / g_params.x_freq;  // 一个周期的Timer计数值
        float phase = (time_diff / period) * 360.0;
        
        return phase;
    }
    
    return g_phase_data.phase_diff;  // 返回上次测量值
}

//==============================================================================
// 工具函数
//==============================================================================

void delay_ms(uint16_t ms)
{
    volatile uint16_t i, j;
    for (i = 0; i < ms; i++) {
        for (j = 0; j < 1000; j++) {
            __no_operation();
        }
    }
}

void delay_us(uint16_t us)
{
    volatile uint16_t i;
    for (i = 0; i < us; i++) {
        __delay_cycles(16);  // 16MHz时钟，16个周期 = 1us
    }
}

//==============================================================================
// 中断服务函数
//==============================================================================

// Timer_A0中断 - X轴信号捕获
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A0_ISR(void)
{
    // 处理频率测量
    static uint16_t capture_count = 0;
    capture_count++;
    
    if (capture_count >= 10) {  // 每10个周期更新一次频率
        capture_count = 0;
        g_params.display_update = true;
    }
}

// Timer_A1中断 - Y轴信号捕获
#pragma vector=TIMER0_A1_VECTOR
__interrupt void Timer_A1_ISR(void)
{
    // 处理相位测量
    g_phase_data.sample_count++;
    
    if (g_phase_data.sample_count >= PHASE_SAMPLES) {
        g_phase_data.sample_count = 0;
        g_params.display_update = true;
    }
    
    TA0IV = 0;  // 清除中断标志
}

// ADC中断 - AGC采样
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void)
{
    // 存储ADC采样值到AGC缓冲区
    adc_buffer[adc_index++] = ADC10MEM;
    
    // 检查缓冲区是否满
    if (adc_index >= ADC_SAMPLES) {
        adc_index = 0;
        adc_ready = true;                       // 标记数据准备好
    }
}

// Port1中断 - 幅度按键
#pragma vector=PORT1_VECTOR
__interrupt void Port1_ISR(void) 
{
    if (P1IFG & AMP_KEY) {                      // 幅度按键 (P1.5)
        P1IFG &= ~AMP_KEY;                      // 清除中断标志
        amp_key_pressed = true;                 // 设置按键标志
    }
}

// Port2中断 - 频率按键 
#pragma vector=PORT2_VECTOR
__interrupt void Port2_ISR(void)
{
    if (P2IFG & FREQ_KEY) {                     // 频率按键 (P2.6)
        P2IFG &= ~FREQ_KEY;                     // 清除中断标志
        freq_key_pressed = true;                // 设置按键标志
    }
}

//==============================================================================
// 新增功能函数实现
//==============================================================================

/**
 * @brief 处理频率切换按键
 * 循环切换频率: 2kHz→4kHz→6kHz→8kHz→10kHz→2kHz
 */
void process_frequency_key(void)
{
    // 切换到下一个频率
    g_params.freq_index++;
    if (g_params.freq_index >= FREQ_COUNT) {
        g_params.freq_index = 0;                // 循环回到2kHz
    }
    
    // 更新输入频率
    g_params.input_freq = frequency_table[g_params.freq_index];
    g_params.output_freq = g_params.input_freq * g_params.multiplier;
    
    // 设置倍频模块
    set_input_frequency(g_params.input_freq);
    
    // 标记显示更新
    g_params.display_update = true;
    
    // LED指示
    P1OUT |= STATUS_LED;
    __delay_cycles(100000);                     // 100ms
    P1OUT &= ~STATUS_LED;
}

/**
 * @brief 处理幅度切换按键  
 * 循环切换幅度: 1V→2V→3V→1V
 */
void process_amplitude_key(void)
{
    // 切换到下一个幅度
    g_params.amplitude++;
    if (g_params.amplitude > 3) {
        g_params.amplitude = 1;                 // 循环回到1V
    }
    
    // 设置AGC目标
    set_target_amplitude(g_params.amplitude);
    
    // 标记显示更新
    g_params.display_update = true;
    
    // LED指示
    P1OUT |= STATUS_LED;
    __delay_cycles(100000);                     // 100ms  
    P1OUT &= ~STATUS_LED;
}

/**
 * @brief AGC处理函数
 * 根据ADC采样结果调节数字电位器
 */
void agc_process(void)
{
    if (!adc_ready) return;
    
    // 计算峰峰值
    uint16_t peak_to_peak = calculate_peak_to_peak();
    uint16_t target_value = get_agc_target();
    
    // PID控制算法 (简化PI控制)
    int16_t error = (int16_t)target_value - (int16_t)peak_to_peak;
    
    if (abs(error) > AGC_TOLERANCE) {
        // 比例控制
        int16_t adjustment = error / 16;        // 比例系数 1/16
        
        // 更新DAC值
        int16_t new_dac = (int16_t)dac_value + adjustment;
        
        // 限幅处理
        if (new_dac < 0) new_dac = 0;
        if (new_dac > DAC_MAX_VALUE) new_dac = DAC_MAX_VALUE;
        
        dac_value = (uint8_t)new_dac;
        
        // 发送到数字电位器
        mcp4018t_set_value(dac_value);
    }
    
    adc_ready = false;                          // 清除处理标志
}

/**
 * @brief 计算ADC采样的峰峰值
 * @return: 峰峰值 (ADC计数)
 */
uint16_t calculate_peak_to_peak(void) 
{
    uint16_t max_val = 0, min_val = 1023;
    
    // 查找最大最小值
    for (uint8_t i = 0; i < ADC_SAMPLES; i++) {
        if (adc_buffer[i] > max_val) max_val = adc_buffer[i];
        if (adc_buffer[i] < min_val) min_val = adc_buffer[i];
    }
    
    return (max_val > min_val) ? (max_val - min_val) : 0;
}

/**
 * @brief 获取AGC目标值
 * @return: 当前幅度对应的ADC目标值
 */
uint16_t get_agc_target(void)
{
    switch (g_params.amplitude) {
        case 1: return AGC_TARGET_1V;
        case 2: return AGC_TARGET_2V;  
        case 3: return AGC_TARGET_3V;
        default: return AGC_TARGET_2V;
    }
}

/**
 * @brief 设置AGC目标幅度
 * @param amplitude: 目标幅度 (1/2/3对应1V/2V/3V)
 */
void set_target_amplitude(uint8_t amplitude)
{
    if (amplitude >= 1 && amplitude <= 3) {
        g_params.amplitude = amplitude;
        g_params.display_update = true;
    }
}

/**
 * @brief 测量相位差 (简化实现)
 * @return: 相位差 (度)
 */
float measure_phase_difference(void)
{
    // 简化的相位测量算法
    // 实际应用中需要基于过零检测或相关算法
    static float phase = 0.0;
    
    // 模拟相位变化 (用于演示)
    phase += 0.1;
    if (phase > 360.0) phase = 0.0;
    
    return phase;
}

/**
 * @brief 显示启动画面
 */
void show_startup_screen(void)
{
    lcd_clear();
    
    // 使用简化的LCD字符显示
    // 实际需要根据具体LCD驱动实现
    lcd_set_cursor(0, 0);
    lcd_print_string("Lissajous Gen");
    lcd_set_cursor(1, 0);  
    lcd_print_string("PLL Multiplier");
    lcd_set_cursor(2, 0);
    lcd_print_string("Ver 2.3");
    lcd_set_cursor(3, 0);
    lcd_print_string("Initializing...");
    
    __delay_cycles(2000000);                    // 2秒显示时间
}

/**
 * @brief 更新系统显示
 * 显示完整的系统状态信息  
 */
void update_system_display(void)
{
    if (!g_params.display_update) return;
    
    lcd_clear();
    
    // 第一行: 倍频和幅度信息
    lcd_set_cursor(0, 0);
    sprintf(lcd_buffer, "MUL:%dX AMP:%dV", g_params.multiplier, g_params.amplitude);
    lcd_print_string(lcd_buffer);
    
    // 第二行: 输入频率
    lcd_set_cursor(1, 0);
    sprintf(lcd_buffer, "IN: %d Hz", g_params.input_freq);
    lcd_print_string(lcd_buffer);
    
    // 第三行: 输出频率  
    lcd_set_cursor(2, 0);
    sprintf(lcd_buffer, "OUT:%d Hz", g_params.output_freq);
    lcd_print_string(lcd_buffer);
    
    // 第四行: 状态信息
    lcd_set_cursor(3, 0);
    if (g_params.pll_locked) {
        sprintf(lcd_buffer, "PLL:LOCK AGC:OK");
    } else {
        sprintf(lcd_buffer, "PLL:---- AGC:ADJ");
    }
    lcd_print_string(lcd_buffer);
    
    g_params.display_update = false;
}

/**
 * @brief 显示错误画面
 * @param error_msg: 错误消息
 */
void show_error_screen(const char* error_msg)
{
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print_string("SYSTEM ERROR");
    lcd_set_cursor(2, 0); 
    lcd_print_string(error_msg);
    lcd_set_cursor(3, 0);
    lcd_print_string("Please Reset");
}

/**
 * @brief 系统监控
 * 监控系统状态，处理异常情况
 */
void system_monitor(void)
{
    static uint16_t monitor_counter = 0;
    
    if (++monitor_counter >= 1000) {           // 约10秒检查一次
        monitor_counter = 0;
        system_uptime++;
        
        // 检查PLL锁定状态
        if (!g_params.pll_locked) {
            // PLL未锁定处理
            g_params.state = SYS_STATE_AGC_ADJUSTING;
        }
        
        // 检查AGC收敛状态
        if (adc_ready) {
            uint16_t pp_value = calculate_peak_to_peak();
            uint16_t target = get_agc_target();
            
            // 如果AGC偏差过大，进行复位
            if (abs((int16_t)pp_value - (int16_t)target) > (AGC_TOLERANCE * 3)) {
                dac_value = 64;             // 恢复中间值
                mcp4018t_set_value(dac_value);
            }
        }
        
        // 标记显示更新
        g_params.display_update = true;
    }
}