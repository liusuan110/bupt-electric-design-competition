//******************************************************************************
// 李萨如图形演示装置 - MSP430G2553主程序
// 功能：X/Y轴信号处理、相位测量、LCD显示、用户交互
//******************************************************************************

#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

//==============================================================================
// 硬件引脚定义
//==============================================================================

// LCD控制引脚 (P2口)
#define LCD_RS      BIT0    // P2.0
#define LCD_RW      BIT1    // P2.1  
#define LCD_E       BIT2    // P2.2
#define LCD_CS1     BIT3    // P2.3 (左半屏)
#define LCD_CS2     BIT4    // P2.4 (右半屏)
#define LCD_RST     BIT5    // P2.5

// 按键引脚 (P1口)
#define KEY_FREQ    BIT3    // P1.3 (频率倍数调节)
#define KEY_AMP     BIT4    // P1.4 (幅度调节)
#define KEY_WAVE    BIT5    // P1.5 (波形选择)
#define KEY_MENU    BIT6    // P1.6 (菜单/确认)

// 信号输入引脚
#define X_SIGNAL    BIT7    // P1.7 (X轴ADC输入)
#define X_ZERO      BIT0    // P1.0 (X轴过零检测)
#define Y_ZERO      BIT1    // P1.1 (Y轴过零检测)

// I2C引脚 (数字电位器控制)
#define SDA_PIN     BIT5    // P1.5
#define SCL_PIN     BIT6    // P1.6

//==============================================================================
// 常量定义
//==============================================================================

#define LCD_WIDTH       128
#define LCD_HEIGHT      64
#define LCD_PAGES       8

#define FREQ_MULT_MIN   1
#define FREQ_MULT_MAX   5
#define AMP_LEVELS      3       // 1V, 2V, 3V
#define WAVE_TYPES      2       // 正弦波, 三角波

#define ADC_SAMPLES     256     // ADC采样缓冲区大小
#define PHASE_SAMPLES   32      // 相位测量采样点数

#define MCP4018T_ADDR   0x2F    // 数字电位器I2C地址

//==============================================================================
// 数据结构定义
//==============================================================================

typedef struct {
    uint8_t freq_mult;          // 频率倍数 (1-5)
    uint8_t amplitude;          // 幅度选择 (0:1V, 1:2V, 2:3V)
    uint8_t wave_type;          // 波形类型 (0:正弦波, 1:三角波)
    uint16_t x_freq;            // X轴频率 (Hz)
    float phase_diff;           // 相位差 (度)
    bool display_update;        // 显示更新标志
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

static system_params_t g_params = {
    .freq_mult = 1,
    .amplitude = 1,             // 默认2V
    .wave_type = 0,             // 默认正弦波
    .x_freq = 1000,             // 默认1kHz
    .phase_diff = 0.0,
    .display_update = true
};

static adc_data_t g_adc_data = {0};
static phase_measure_t g_phase_data = {0};

// LCD显示缓冲区
static uint8_t g_lcd_buffer[LCD_WIDTH * LCD_PAGES];

//==============================================================================
// 函数声明
//==============================================================================

// 系统初始化
void system_init(void);
void gpio_init(void);
void timer_init(void);
void adc_init(void);
void i2c_init(void);

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

int main(void)
{
    // 关闭看门狗
    WDTCTL = WDTPW | WDTHOLD;
    
    // 系统初始化
    system_init();
    
    // 主循环
    while(1)
    {
        // 处理按键输入
        process_key_input();
        
        // 更新系统参数
        update_system_parameters();
        
        // 测量X轴频率
        g_params.x_freq = get_frequency();
        
        // 测量相位差
        g_params.phase_diff = calculate_phase_difference();
        
        // 更新显示
        if (g_params.display_update) {
            lcd_clear();
            draw_grid();
            draw_lissajous_curve();
            draw_parameters_info();
            lcd_update_display();
            g_params.display_update = false;
        }
        
        // 延时避免过度刷新
        delay_ms(50);
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

// ADC中断
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void)
{
    // 存储ADC采样值
    g_adc_data.buffer[g_adc_data.index] = ADC10MEM;
    g_adc_data.index = (g_adc_data.index + 1) % ADC_SAMPLES;
    
    if (g_adc_data.index == 0) {
        g_adc_data.ready = true;
    }
}