/******************************************************************************
 * MSP430G2553 - 整形、分频与波形变换控制系统
 * 
 * 功能说明:
 * 1. LM339比较器整形电路 - 将峰峰值2V正弦波整形为方波
 *    - 静态电平设为VDD/2 (通过4个10k电阻分压)
 *    - 输入电容隔离直流分量,构成过零比较器
 *    - 10k+220k正反馈构成滞回比较器,避免噪声毛刺
 * 
 * 2. 74HC161分频电路控制
 *    - D0~D2通过P1.1/P1.2/P1.3控制预置数
 *    - D3固定为1
 *    - 实现2/3/4/5分频功能
 * 
 * 3. 74HC00二选一电路
 *    - 控制信号"多倍/1倍"通过P1.4输出
 *    - 逻辑1: 多倍分频模式 (2-5分频)
 *    - 逻辑0: 1倍模式 (VCO直接反馈)
 * 
 * 4. 波形变换模块 (CD4046方波 -> 正弦波/三角波)
 *    - 5个低通滤波器对应5个波段 (2k/4k/6k/8k/10kHz)
 *    - CD4052: 四选一,选择2k/4k/6k/8kHz
 *    - CD4053: 第一级二选一,从4选1结果和10kHz中选择
 *    - CD4053: 第二级二选一,选择正弦波或三角波输出
 * 
 * 接口定义:
 *   P1.1 -> 74HC161 D0
 *   P1.2 -> 74HC161 D1
 *   P1.3 -> 74HC161 D2
 *   P1.4 -> 多倍/1倍控制信号
 *   P1.5 -> 按键输入(切换分频比)
 *   P1.6 -> 状态LED
 *   P3.0 -> CD4052 A (波段选择)
 *   P3.1 -> CD4052 B (波段选择)
 *   P3.2 -> CD4053 A (10kHz二次选择)
 *   P3.3 -> CD4053 B (波形选择: 正弦波/三角波)
 * 
 ******************************************************************************/

#include <msp430.h>
#include <stdint.h>

/******************************************************************************
 * 宏定义
 ******************************************************************************/
// GPIO引脚定义 - 分频控制
#define HC161_D0        BIT1    // P1.1 -> 74HC161 D0
#define HC161_D1        BIT2    // P1.2 -> 74HC161 D1
#define HC161_D2        BIT3    // P1.3 -> 74HC161 D2
#define DIV_MODE        BIT4    // P1.4 -> 多倍/1倍模式控制
#define KEY_INPUT       BIT5    // P1.5 -> 按键输入
#define STATUS_LED      BIT6    // P1.6 -> 状态指示LED
#define I2C_SDA         BIT7    // P1.7 -> I2C数据线(AD5242)

// GPIO引脚定义 - 相位测量 (捕获输入)
#define X_PHASE_INPUT   BIT0    // P2.0 -> TA1.0捕获 (X轴过零信号)
#define Y_PHASE_INPUT   BIT1    // P2.1 -> TA1.1捕获 (Y轴过零信号)

// GPIO引脚定义 - LCD控制 (SPI接口)
#define LCD_CS          BIT2    // P2.2 -> LCD片选
#define LCD_RS          BIT3    // P2.3 -> LCD数据/命令选择 (DC/A0)
#define LCD_RES         BIT4    // P2.4 -> LCD复位
#define LCD_SCL         BIT5    // P2.5 -> LCD时钟 (SCK)
#define LCD_SDA         BIT7    // P2.7 -> LCD数据 (MOSI)

// GPIO引脚定义 - 波形变换控制
#define CD4052_A        BIT0    // P3.0 -> CD4052 A (波段选择)
#define CD4052_B        BIT1    // P3.1 -> CD4052 B (波段选择)
#define CD4053_A        BIT2    // P3.2 -> CD4053 A (10kHz选择)
#define CD4053_B        BIT3    // P3.3 -> CD4053 B (波形选择)
#define I2C_SCL         BIT4    // P3.4 -> I2C时钟线(AD5242)

// 74HC161预置数定义 (D3D2D1D0)
// 预置数 = 16 - n (n为分频数)
#define DIV_1_MODE      0       // 1分频: 直通模式
#define DIV_2_PRESET    6       // 2分频: 预置数 = 14 = 1110b (D2D1D0=110)
#define DIV_3_PRESET    5       // 3分频: 预置数 = 13 = 1101b (D2D1D0=101)
#define DIV_4_PRESET    4       // 4分频: 预置数 = 12 = 1100b (D2D1D0=100)
#define DIV_5_PRESET    3       // 5分频: 预置数 = 11 = 1011b (D2D1D0=011)

// 波形类型定义
typedef enum {
    WAVEFORM_SINE     = 0,      // 正弦波
    WAVEFORM_TRIANGLE = 1       // 三角波
} waveform_type_t;

// 频率波段定义
typedef enum {
    FREQ_2KHZ  = 0,             // 2kHz
    FREQ_4KHZ  = 1,             // 4kHz
    FREQ_6KHZ  = 2,             // 6kHz
    FREQ_8KHZ  = 3,             // 8kHz
    FREQ_10KHZ = 4              // 10kHz
} freq_band_t;

// 输出幅度选择
typedef enum {
    AMPLITUDE_1V = 0,           // 1Vpp
    AMPLITUDE_2V = 1,           // 2Vpp
    AMPLITUDE_3V = 2            // 3Vpp
} amplitude_t;

// 按键防抖延时
#define KEY_DEBOUNCE_TIME   50  // ms

// AD5242数字电位器定义
#define AD5242_ADDR     0x2C    // AD5242 I2C地址 (7位地址)
#define AD5242_RDAC1    0x00    // RDAC1寄存器地址
#define AD5242_RDAC2    0x80    // RDAC2寄存器地址
#define AD5242_MAX      255     // 最大抽头位置

// AGC参数定义
#define TARGET_VPP_1V   205     // 目标峰峰值1V对应ADC值 (1V/5V*1023)
#define TARGET_VPP_2V   410     // 目标峰峰值2V对应ADC值
#define TARGET_VPP_3V   614     // 目标峰峰值3V对应ADC值
#define VPP_TOLERANCE   20      // 幅度容差 (0.1V/5V*1023)
#define AGC_STEP        5       // AGC调节步长

// LCD屏幕参数定义
#define LCD_WIDTH       128     // 屏幕宽度
#define LCD_HEIGHT      64      // 屏幕高度
#define LCD_PAGES       8       // LCD页数 (64/8=8)
#define LISSAJOUS_SIZE  60      // 李萨如图形显示大小
#define SAMPLE_POINTS   32      // 采样点数 (优化:从64减少到32)

/******************************************************************************
 * 全局变量
 ******************************************************************************/
// 分频控制
volatile uint8_t current_div_ratio = 1;     // 当前分频比 (1-5)
volatile uint8_t key_pressed = 0;           // 按键标志

// 波形变换控制
volatile waveform_type_t current_waveform = WAVEFORM_SINE;  // 当前波形类型
volatile freq_band_t current_freq_band = FREQ_2KHZ;         // 当前频率波段
volatile amplitude_t current_amplitude = AMPLITUDE_2V;      // 当前输出幅度

// AGC控制
volatile uint16_t measured_vpp = 0;         // 测量的峰峰值ADC值
volatile uint8_t rdac_value = 128;          // 当前数字电位器值
volatile uint8_t agc_enabled = 1;           // AGC使能标志

// 相位测量
volatile uint16_t x_capture_time = 0;       // X轴捕获时间
volatile uint16_t y_capture_time = 0;       // Y轴捕获时间
volatile int16_t phase_diff_deg = 0;        // 相位差(度)
volatile uint8_t phase_valid = 0;           // 相位数据有效标志

// 李萨如图形数据 (优化:不使用全局缓冲区,改用分页渲染)
int8_t sample_x[SAMPLE_POINTS];             // X轴采样数据 (优化:int16_t->int8_t)
int8_t sample_y[SAMPLE_POINTS];             // Y轴采样数据 (优化:int16_t->int8_t)
volatile uint8_t sample_ready = 0;          // 采样完成标志

/******************************************************************************
 * 函数声明
 ******************************************************************************/
// 系统初始化
void system_init(void);
void gpio_init(void);
void timer_init(void);

// 分频控制
void set_division_ratio(uint8_t ratio);
void update_hc161_preset(uint8_t preset);

// 波形变换控制
void set_frequency_band(freq_band_t band);
void set_waveform_type(waveform_type_t waveform);
void set_amplitude(amplitude_t amp);
void update_cd4052_control(uint8_t channel);
void update_cd4053_freq_select(uint8_t select_10k);
void update_cd4053_wave_select(waveform_type_t waveform);

// I2C通信 (软件模拟)
void i2c_init(void);
void i2c_start(void);
void i2c_stop(void);
void i2c_write_byte(uint8_t data);
uint8_t i2c_read_ack(void);

// AD5242数字电位器控制
void ad5242_write(uint8_t reg, uint8_t value);
uint8_t ad5242_read(uint8_t reg);
void ad5242_set_rdac1(uint8_t value);
void ad5242_set_rdac2(uint8_t value);

// ADC采样
void adc_init(void);
uint16_t adc_read_channel(uint8_t channel);
uint16_t measure_peak_to_peak(void);

// AGC自动增益控制
void agc_adjust(void);
void agc_calibrate(void);

// 相位测量
void phase_capture_init(void);
int16_t calculate_phase_difference(void);

// LCD显示
void lcd_init(void);
void lcd_clear(void);
void lcd_set_pixel_immediate(uint8_t x, uint8_t y, uint8_t color);
void lcd_draw_line_to_buffer(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t* buffer);
void lcd_draw_lissajous(void);
void lcd_show_info(void);
void lcd_write_cmd(uint8_t cmd);
void lcd_write_data(uint8_t data);
void lcd_set_pos(uint8_t x, uint8_t page);
void lcd_draw_page(uint8_t page);

// 数据采集
void adc_sample_xy(void);

// 工具函数
void key_scan(void);
void delay_ms(uint16_t ms);
void delay_us(uint16_t us);

/******************************************************************************
 * 主函数
 ******************************************************************************/
int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // 停止看门狗
    
    system_init();               // 系统初始化
    
    // 默认设置
    set_division_ratio(1);              // 1分频模式
    set_frequency_band(FREQ_2KHZ);      // 2kHz波段
    set_waveform_type(WAVEFORM_SINE);   // 正弦波输出
    set_amplitude(AMPLITUDE_2V);        // 2Vpp输出
    
    __enable_interrupt();        // 使能全局中断
    
    // AGC初始校准
    delay_ms(100);  // 等待电路稳定
    agc_calibrate();
    
    uint16_t agc_counter = 0;
    uint16_t phase_counter = 0;
    uint16_t lcd_counter = 0;
    
    while(1)
    {
        // 按键扫描
        key_scan();
        
        // 如果按键被按下,切换分频比
        if(key_pressed)
        {
            key_pressed = 0;
            
            // 循环切换分频比 1->2->3->4->5->1
            current_div_ratio++;
            if(current_div_ratio > 5)
                current_div_ratio = 1;
            
            set_division_ratio(current_div_ratio);
            
            // 更新显示
            lcd_show_info();
            
            // LED闪烁指示状态改变
            P1OUT ^= STATUS_LED;
            delay_ms(100);
            P1OUT ^= STATUS_LED;
        }
        
        // AGC调节 (每200ms执行一次)
        agc_counter++;
        if(agc_counter >= 20)  // 20 * 10ms = 200ms
        {
            agc_counter = 0;
            agc_adjust();
        }
        
        // 相位测量 (每100ms更新一次)
        phase_counter++;
        if(phase_counter >= 10)  // 10 * 10ms = 100ms
        {
            phase_counter = 0;
            phase_diff_deg = calculate_phase_difference();
        }
        
        // LCD显示更新 (每50ms更新一次)
        lcd_counter++;
        if(lcd_counter >= 5)  // 5 * 10ms = 50ms
        {
            lcd_counter = 0;
            
            // 采集XY信号
            adc_sample_xy();
            
            // 绘制李萨如图形
            lcd_clear();
            lcd_draw_lissajous();
            lcd_show_info();
        }
        
        delay_ms(10);  // 主循环延时
    }
}

/******************************************************************************
 * 系统初始化
 ******************************************************************************/
void system_init(void)
{
    // 配置DCO为1MHz
    BCSCTL1 = CALBC1_1MHZ;
    DCOCTL = CALDCO_1MHZ;
    
    gpio_init();
    timer_init();
    i2c_init();
    adc_init();
    phase_capture_init();
    lcd_init();
    
    // 初始化数字电位器为中间值
    ad5242_set_rdac1(128);
    ad5242_set_rdac2(128);
    
    // 显示启动画面
    lcd_clear();
    lcd_show_info();
}

/******************************************************************************
 * GPIO初始化
 ******************************************************************************/
void gpio_init(void)
{
    // 配置P1.1/P1.2/P1.3/P1.4/P1.6为输出
    P1DIR |= HC161_D0 | HC161_D1 | HC161_D2 | DIV_MODE | STATUS_LED;
    P1OUT &= ~(HC161_D0 | HC161_D1 | HC161_D2 | DIV_MODE | STATUS_LED);
    
    // 配置P1.5为输入,使能上拉电阻
    P1DIR &= ~KEY_INPUT;
    P1REN |= KEY_INPUT;
    P1OUT |= KEY_INPUT;
    
    // 使能P1.5中断(下降沿触发)
    P1IE |= KEY_INPUT;
    P1IES |= KEY_INPUT;
    P1IFG &= ~KEY_INPUT;
    
    // 配置P2.0/P2.1为输入 (相位捕获)
    P2DIR &= ~(X_PHASE_INPUT | Y_PHASE_INPUT);
    P2SEL |= X_PHASE_INPUT | Y_PHASE_INPUT;  // 选择TA1功能
    
    // 配置P2.2/P2.3/P2.4/P2.5/P2.7为输出 (LCD控制)
    P2DIR |= LCD_CS | LCD_RS | LCD_RES | LCD_SCL | LCD_SDA;
    P2OUT |= LCD_CS | LCD_RS | LCD_RES;  // 默认高电平
    P2OUT &= ~(LCD_SCL | LCD_SDA);       // SCL/SDA低电平
    
    // 配置P3.0/P3.1/P3.2/P3.3为输出 (波形变换控制)
    P3DIR |= CD4052_A | CD4052_B | CD4053_A | CD4053_B;
    P3OUT &= ~(CD4052_A | CD4052_B | CD4053_A | CD4053_B);
}

/******************************************************************************
 * 定时器初始化 (用于延时和防抖)
 ******************************************************************************/
void timer_init(void)
{
    // 配置Timer A0为连续模式
    TA0CTL = TASSEL_2 | MC_2 | TACLR;  // SMCLK, 连续模式, 清除计数器
}

/******************************************************************************
 * 设置分频比
 * 参数: ratio - 分频比 (1-5)
 ******************************************************************************/
void set_division_ratio(uint8_t ratio)
{
    switch(ratio)
    {
        case 1:
            // 1分频模式: 多倍/1倍控制信号为0, VCO直接反馈
            P1OUT &= ~DIV_MODE;
            // HC161的预置数无关紧要,但为了规范设为0
            update_hc161_preset(0);
            break;
            
        case 2:
            // 2分频: 多倍/1倍控制信号为1, 预置数=14 (1110b)
            P1OUT |= DIV_MODE;
            update_hc161_preset(DIV_2_PRESET);
            break;
            
        case 3:
            // 3分频: 多倍/1倍控制信号为1, 预置数=13 (1101b)
            P1OUT |= DIV_MODE;
            update_hc161_preset(DIV_3_PRESET);
            break;
            
        case 4:
            // 4分频: 多倍/1倍控制信号为1, 预置数=12 (1100b)
            P1OUT |= DIV_MODE;
            update_hc161_preset(DIV_4_PRESET);
            break;
            
        case 5:
            // 5分频: 多倍/1倍控制信号为1, 预置数=11 (1011b)
            P1OUT |= DIV_MODE;
            update_hc161_preset(DIV_5_PRESET);
            break;
            
        default:
            // 默认1分频
            P1OUT &= ~DIV_MODE;
            update_hc161_preset(0);
            break;
    }
}

/******************************************************************************
 * 更新74HC161预置数
 * 参数: preset - 预置数的低3位 (D2D1D0), D3始终为1
 * 
 * 预置数计算:
 *   2分频: 预置数=14=1110b -> D2D1D0=110b=6
 *   3分频: 预置数=13=1101b -> D2D1D0=101b=5
 *   4分频: 预置数=12=1100b -> D2D1D0=100b=4
 *   5分频: 预置数=11=1011b -> D2D1D0=011b=3
 ******************************************************************************/
void update_hc161_preset(uint8_t preset)
{
    // 清除P1.1/P1.2/P1.3
    P1OUT &= ~(HC161_D0 | HC161_D1 | HC161_D2);
    
    // 设置新的预置数
    if(preset & 0x01)  // D0
        P1OUT |= HC161_D0;
    if(preset & 0x02)  // D1
        P1OUT |= HC161_D1;
    if(preset & 0x04)  // D2
        P1OUT |= HC161_D2;
    
    // 注意: D3在硬件上连接到VDD,始终为1
}

/******************************************************************************
 * 按键扫描 (软件防抖)
 ******************************************************************************/
void key_scan(void)
{
    static uint8_t key_state = 0;
    static uint16_t debounce_count = 0;
    
    // 读取按键状态 (低电平有效)
    if((P1IN & KEY_INPUT) == 0)
    {
        debounce_count++;
        if(debounce_count >= KEY_DEBOUNCE_TIME && key_state == 0)
        {
            key_state = 1;
            key_pressed = 1;
        }
    }
    else
    {
        debounce_count = 0;
        key_state = 0;
    }
}

/******************************************************************************
 * 延时函数 (毫秒级)
 * 参数: ms - 延时时间(毫秒)
 * 基于1MHz时钟
 ******************************************************************************/
void delay_ms(uint16_t ms)
{
    uint16_t i, j;
    for(i = 0; i < ms; i++)
        for(j = 0; j < 110; j++);  // 约1ms @ 1MHz
}

/******************************************************************************
 * 延时函数 (微秒级)
 * 参数: us - 延时时间(微秒)
 ******************************************************************************/
void delay_us(uint16_t us)
{
    while(us--)
        __delay_cycles(1);  // 1us @ 1MHz
}

/******************************************************************************
 * I2C初始化
 ******************************************************************************/
void i2c_init(void)
{
    // 配置SDA和SCL为输出，初始高电平
    P1DIR |= I2C_SDA;
    P3DIR |= I2C_SCL;
    P1OUT |= I2C_SDA;
    P3OUT |= I2C_SCL;
}

/******************************************************************************
 * I2C起始信号
 ******************************************************************************/
void i2c_start(void)
{
    P1DIR |= I2C_SDA;   // SDA输出
    P1OUT |= I2C_SDA;   // SDA高
    P3OUT |= I2C_SCL;   // SCL高
    delay_us(5);
    P1OUT &= ~I2C_SDA;  // SDA下降沿
    delay_us(5);
    P3OUT &= ~I2C_SCL;  // SCL低
    delay_us(5);
}

/******************************************************************************
 * I2C停止信号
 ******************************************************************************/
void i2c_stop(void)
{
    P1DIR |= I2C_SDA;   // SDA输出
    P1OUT &= ~I2C_SDA;  // SDA低
    P3OUT |= I2C_SCL;   // SCL高
    delay_us(5);
    P1OUT |= I2C_SDA;   // SDA上升沿
    delay_us(5);
}

/******************************************************************************
 * I2C写字节
 ******************************************************************************/
void i2c_write_byte(uint8_t data)
{
    uint8_t i;
    P1DIR |= I2C_SDA;   // SDA输出
    
    for(i = 0; i < 8; i++)
    {
        if(data & 0x80)
            P1OUT |= I2C_SDA;
        else
            P1OUT &= ~I2C_SDA;
        
        delay_us(2);
        P3OUT |= I2C_SCL;   // SCL高，数据有效
        delay_us(5);
        P3OUT &= ~I2C_SCL;  // SCL低
        delay_us(2);
        
        data <<= 1;
    }
}

/******************************************************************************
 * I2C读取ACK
 * 返回: 0=ACK, 1=NACK
 ******************************************************************************/
uint8_t i2c_read_ack(void)
{
    uint8_t ack;
    
    P1DIR &= ~I2C_SDA;  // SDA输入
    P1OUT |= I2C_SDA;   // 释放SDA
    delay_us(2);
    
    P3OUT |= I2C_SCL;   // SCL高
    delay_us(5);
    
    ack = (P1IN & I2C_SDA) ? 1 : 0;
    
    P3OUT &= ~I2C_SCL;  // SCL低
    delay_us(2);
    
    return ack;
}

/******************************************************************************
 * AD5242写寄存器
 * 参数: reg - 寄存器地址 (RDAC1=0x00, RDAC2=0x80)
 *       value - 写入值 (0-255)
 ******************************************************************************/
void ad5242_write(uint8_t reg, uint8_t value)
{
    i2c_start();
    i2c_write_byte((AD5242_ADDR << 1) | 0);  // 器件地址+写
    i2c_read_ack();
    i2c_write_byte(reg);                      // 寄存器地址
    i2c_read_ack();
    i2c_write_byte(value);                    // 数据
    i2c_read_ack();
    i2c_stop();
}

/******************************************************************************
 * 设置AD5242 RDAC1 (Y轴增益控制)
 ******************************************************************************/
void ad5242_set_rdac1(uint8_t value)
{
    ad5242_write(AD5242_RDAC1, value);
    rdac_value = value;
}

/******************************************************************************
 * 设置AD5242 RDAC2 (备用通道)
 ******************************************************************************/
void ad5242_set_rdac2(uint8_t value)
{
    ad5242_write(AD5242_RDAC2, value);
}

/******************************************************************************
 * ADC初始化
 ******************************************************************************/
void adc_init(void)
{
    ADC10CTL0 = ADC10SHT_2 + ADC10ON;        // ADC10ON, S&H=16 ADC10CLKs
    ADC10CTL1 = INCH_0;                      // 输入通道A0
    ADC10AE0 |= BIT0;                        // P1.0 ADC输入使能
}

/******************************************************************************
 * 读取ADC通道
 * 参数: channel - ADC通道号(0-7)
 * 返回: 10位ADC值(0-1023)
 ******************************************************************************/
uint16_t adc_read_channel(uint8_t channel)
{
    ADC10CTL1 = (channel << 12);             // 选择输入通道
    ADC10CTL0 |= ENC + ADC10SC;              // 开始转换
    while(ADC10CTL1 & ADC10BUSY);            // 等待转换完成
    return ADC10MEM;
}

/******************************************************************************
 * 测量峰峰值
 * 返回: 峰峰值对应的ADC值
 ******************************************************************************/
uint16_t measure_peak_to_peak(void)
{
    uint16_t max_val = 0, min_val = 1023;
    uint16_t sample;
    
    // 采样32次，找最大最小值
    for(uint8_t i = 0; i < 32; i++)
    {
        sample = adc_read_channel(0);  // 假设Y轴信号接A0
        
        if(sample > max_val)
            max_val = sample;
        if(sample < min_val)
            min_val = sample;
        
        delay_us(100);  // 采样间隔
    }
    
    return (max_val - min_val);
}

/******************************************************************************
 * 设置输出幅度
 * 参数: amp - 幅度选择 (AMPLITUDE_1V/2V/3V)
 ******************************************************************************/
void set_amplitude(amplitude_t amp)
{
    current_amplitude = amp;
    
    // 触发AGC校准
    if(agc_enabled)
    {
        agc_calibrate();
    }
}

/******************************************************************************
 * AGC自动增益控制
 * 功能: 自动调节数字电位器，使输出幅度达到目标值
 ******************************************************************************/
void agc_adjust(void)
{
    uint16_t target_vpp;
    int16_t error;
    
    if(!agc_enabled)
        return;
    
    // 确定目标峰峰值
    switch(current_amplitude)
    {
        case AMPLITUDE_1V:
            target_vpp = TARGET_VPP_1V;
            break;
        case AMPLITUDE_2V:
            target_vpp = TARGET_VPP_2V;
            break;
        case AMPLITUDE_3V:
            target_vpp = TARGET_VPP_3V;
            break;
        default:
            target_vpp = TARGET_VPP_2V;
    }
    
    // 测量当前峰峰值
    measured_vpp = measure_peak_to_peak();
    
    // 计算误差
    error = target_vpp - measured_vpp;
    
    // 在容差范围内，不调节
    if((error > -VPP_TOLERANCE) && (error < VPP_TOLERANCE))
        return;
    
    // 根据误差调节数字电位器
    if(error > 0)
    {
        // 实际值偏小，增大增益
        if(rdac_value < (AD5242_MAX - AGC_STEP))
            rdac_value += AGC_STEP;
        else
            rdac_value = AD5242_MAX;
    }
    else
    {
        // 实际值偏大，减小增益
        if(rdac_value > AGC_STEP)
            rdac_value -= AGC_STEP;
        else
            rdac_value = 0;
    }
    
    // 更新数字电位器
    ad5242_set_rdac1(rdac_value);
    
    // 等待稳定
    delay_ms(10);
}

/******************************************************************************
 * AGC校准
 * 功能: 切换频率/幅度后重新校准
 ******************************************************************************/
void agc_calibrate(void)
{
    uint8_t retry = 0;
    
    // 从中间值开始
    rdac_value = 128;
    ad5242_set_rdac1(rdac_value);
    delay_ms(50);  // 等待电路稳定
    
    // 迭代调节，最多10次
    while(retry++ < 10)
    {
        agc_adjust();
        
        // 检查是否达到目标
        uint16_t target_vpp;
        switch(current_amplitude)
        {
            case AMPLITUDE_1V: target_vpp = TARGET_VPP_1V; break;
            case AMPLITUDE_2V: target_vpp = TARGET_VPP_2V; break;
            case AMPLITUDE_3V: target_vpp = TARGET_VPP_3V; break;
            default: target_vpp = TARGET_VPP_2V;
        }
        
        int16_t error = target_vpp - measured_vpp;
        if((error > -VPP_TOLERANCE) && (error < VPP_TOLERANCE))
            break;  // 校准成功
    }
}

/******************************************************************************
 * 相位捕获初始化
 * 使用Timer1的TA1.0和TA1.1捕获X轴和Y轴的过零信号
 ******************************************************************************/
void phase_capture_init(void)
{
    // 配置Timer1为连续模式
    TA1CTL = TASSEL_2 | MC_2 | TACLR;        // SMCLK, 连续模式
    
    // 配置TA1.0捕获(X轴)
    TA1CCTL0 = CM_1 | CCIS_0 | CAP | CCIE;   // 上升沿捕获, CCI0A, 捕获模式, 中断使能
    
    // 配置TA1.1捕获(Y轴)
    TA1CCTL1 = CM_1 | CCIS_0 | CAP | CCIE;   // 上升沿捕获, CCI1A, 捕获模式, 中断使能
}

/******************************************************************************
 * 计算相位差
 * 返回: 相位差(度), 范围-180~+180
 ******************************************************************************/
int16_t calculate_phase_difference(void)
{
    if(!phase_valid)
        return 0;
    
    int32_t time_diff;
    int32_t period;
    int16_t phase;
    
    // 计算时间差
    if(y_capture_time >= x_capture_time)
        time_diff = y_capture_time - x_capture_time;
    else
        time_diff = (65536 - x_capture_time) + y_capture_time;
    
    // 计算周期 (根据当前频率)
    // SMCLK = 1MHz, 周期以时钟周期为单位
    switch(current_freq_band)
    {
        case FREQ_2KHZ:  period = 500;  break;  // 500us = 500 cycles @ 1MHz
        case FREQ_4KHZ:  period = 250;  break;
        case FREQ_6KHZ:  period = 167;  break;
        case FREQ_8KHZ:  period = 125;  break;
        case FREQ_10KHZ: period = 100;  break;
        default: period = 500;
    }
    
    // 相位差(度) = (时间差 / 周期) * 360
    phase = (int16_t)((time_diff * 360L) / period);
    
    // 归一化到-180~+180
    if(phase > 180)
        phase -= 360;
    else if(phase < -180)
        phase += 360;
    
    return phase;
}

/******************************************************************************
 * LCD SPI写命令
 ******************************************************************************/
void lcd_write_cmd(uint8_t cmd)
{
    uint8_t i;
    
    P2OUT &= ~LCD_CS;   // CS低
    P2OUT &= ~LCD_RS;   // RS低(命令)
    
    for(i = 0; i < 8; i++)
    {
        P2OUT &= ~LCD_SCL;  // SCL低
        
        if(cmd & 0x80)
            P2OUT |= LCD_SDA;
        else
            P2OUT &= ~LCD_SDA;
        
        P2OUT |= LCD_SCL;   // SCL高,数据有效
        cmd <<= 1;
    }
    
    P2OUT |= LCD_CS;    // CS高
}

/******************************************************************************
 * LCD SPI写数据
 ******************************************************************************/
void lcd_write_data(uint8_t data)
{
    uint8_t i;
    
    P2OUT &= ~LCD_CS;   // CS低
    P2OUT |= LCD_RS;    // RS高(数据)
    
    for(i = 0; i < 8; i++)
    {
        P2OUT &= ~LCD_SCL;  // SCL低
        
        if(data & 0x80)
            P2OUT |= LCD_SDA;
        else
            P2OUT &= ~LCD_SDA;
        
        P2OUT |= LCD_SCL;   // SCL高,数据有效
        data <<= 1;
    }
    
    P2OUT |= LCD_CS;    // CS高
}

/******************************************************************************
 * LCD设置显示位置
 * page: 页地址(0-7)
 * column: 列地址(0-127)
 ******************************************************************************/
void lcd_set_pos(uint8_t column, uint8_t page)
{
    lcd_write_cmd(0xB0 + page);              // 设置页地址
    lcd_write_cmd(((column >> 4) & 0x0F) | 0x10);  // 列地址高4位
    lcd_write_cmd(column & 0x0F);            // 列地址低4位
}

/******************************************************************************
 * LCD初始化 (12864 SSD1306)
 ******************************************************************************/
void lcd_init(void)
{
    // 硬件复位
    P2OUT &= ~LCD_RES;
    delay_ms(10);
    P2OUT |= LCD_RES;
    delay_ms(10);
    
    // 初始化序列
    lcd_write_cmd(0xAE);    // 关闭显示
    lcd_write_cmd(0x00);    // 设置低列地址
    lcd_write_cmd(0x10);    // 设置高列地址
    lcd_write_cmd(0x40);    // 设置起始行地址
    lcd_write_cmd(0xB0);    // 设置页地址
    lcd_write_cmd(0x81);    // 对比度设置
    lcd_write_cmd(0xFF);    // 对比度值(最大)
    lcd_write_cmd(0xA1);    // 段重定义:列地址127映射到SEG0
    lcd_write_cmd(0xA6);    // 正常显示(0:灭,1:亮)
    lcd_write_cmd(0xA8);    // 设置复用比
    lcd_write_cmd(0x3F);    // 1/64 duty
    lcd_write_cmd(0xC8);    // COM扫描方向
    lcd_write_cmd(0xD3);    // 显示偏移
    lcd_write_cmd(0x00);    // 无偏移
    lcd_write_cmd(0xD5);    // 设置时钟分频
    lcd_write_cmd(0x80);    // 分频比
    lcd_write_cmd(0xD9);    // 预充电周期
    lcd_write_cmd(0xF1);
    lcd_write_cmd(0xDA);    // COM引脚配置
    lcd_write_cmd(0x12);
    lcd_write_cmd(0xDB);    // VCOM电压
    lcd_write_cmd(0x40);
    lcd_write_cmd(0x8D);    // 电荷泵设置
    lcd_write_cmd(0x14);    // 使能电荷泵
    lcd_write_cmd(0xAF);    // 开启显示
    
    lcd_clear();
}

/******************************************************************************
 * LCD清屏 (直接写入硬件,不使用缓冲区)
 ******************************************************************************/
void lcd_clear(void)
{
    uint8_t page, col;
    
    for(page = 0; page < LCD_PAGES; page++)
    {
        lcd_set_pos(0, page);
        for(col = 0; col < LCD_WIDTH; col++)
        {
            lcd_write_data(0x00);
        }
    }
}

/******************************************************************************
 * LCD绘制单页 (128字节缓冲区)
 * 参数: page - 页号(0-7)
 ******************************************************************************/
void lcd_draw_page(uint8_t page)
{
    uint8_t buffer[LCD_WIDTH];  // 单页缓冲区128字节
    uint8_t i;
    int16_t x, y, x_next, y_next;
    int16_t center_x = LCD_WIDTH / 2;
    int16_t center_y = LCD_HEIGHT / 2;
    uint8_t y_start = page * 8;
    uint8_t y_end = y_start + 8;
    
    // 清空缓冲区
    for(i = 0; i < LCD_WIDTH; i++)
        buffer[i] = 0x00;
    
    // 绘制坐标轴
    if(center_y >= y_start && center_y < y_end)
    {
        // X轴穿过此页
        uint8_t bit_pos = center_y - y_start;
        for(i = center_x - 30; i <= center_x + 30 && i < LCD_WIDTH; i++)
        {
            if(i >= 0)
                buffer[i] |= (1 << bit_pos);
        }
    }
    
    // Y轴
    if(center_x >= 0 && center_x < LCD_WIDTH)
    {
        for(i = 0; i < 8; i++)
        {
            uint8_t y_coord = y_start + i;
            if(y_coord >= center_y - 30 && y_coord <= center_y + 30)
                buffer[center_x] |= (1 << i);
        }
    }
    
    // 绘制李萨如曲线 (简化版:只绘制点,不连线)
    if(sample_ready)
    {
        for(i = 0; i < SAMPLE_POINTS; i++)
        {
            x = center_x + sample_x[i];
            y = center_y - sample_y[i];  // Y轴向上为正
            
            // 检查点是否在当前页范围内
            if(x >= 0 && x < LCD_WIDTH && y >= y_start && y < y_end)
            {
                uint8_t bit_pos = y - y_start;
                buffer[x] |= (1 << bit_pos);
            }
        }
    }
    
    // 刷新到LCD
    lcd_set_pos(0, page);
    for(i = 0; i < LCD_WIDTH; i++)
    {
        lcd_write_data(buffer[i]);
    }
}

/******************************************************************************
 * ADC采样XY信号
 * 采集SAMPLE_POINTS个点,存储到sample_x和sample_y数组
 * 优化:直接转换为int8_t,范围-30~+30
 ******************************************************************************/
void adc_sample_xy(void)
{
    uint8_t i;
    uint16_t x_val, y_val;
    
    for(i = 0; i < SAMPLE_POINTS; i++)
    {
        // 采样X轴(假设接A0)
        x_val = adc_read_channel(0);
        // 转换到屏幕坐标(-30~+30),直接存储为int8_t
        sample_x[i] = (int8_t)((x_val * 60 / 1023) - 30);
        
        // 采样Y轴(假设接A1)
        y_val = adc_read_channel(1);
        // 转换到屏幕坐标(-30~+30),直接存储为int8_t
        sample_y[i] = (int8_t)((y_val * 60 / 1023) - 30);
        
        delay_us(50);  // 采样间隔
    }
    
    sample_ready = 1;
}

/******************************************************************************
 * LCD绘制李萨如图形 (分页渲染版本)
 * 不使用全局缓冲区,逐页绘制并立即刷新到硬件
 ******************************************************************************/
void lcd_draw_lissajous(void)
{
    uint8_t page;
    
    // 逐页绘制
    for(page = 0; page < LCD_PAGES; page++)
    {
        lcd_draw_page(page);
    }
}

/******************************************************************************
 * LCD显示字符到指定页 (简化版,5x7字体)
 * 字体数据存储在Flash中以节省RAM
 ******************************************************************************/
const uint8_t font_5x7[][5] = {
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
};

void lcd_draw_char_direct(uint8_t x, uint8_t page, char c)
{
    uint8_t i;
    
    if(c >= '0' && c <= '9')
    {
        lcd_set_pos(x, page);
        for(i = 0; i < 5; i++)
        {
            lcd_write_data(font_5x7[c - '0'][i]);
        }
        lcd_write_data(0x00);  // 字符间距
    }
}

/******************************************************************************
 * LCD显示信息(频率、幅度、相位)
 * 直接写入LCD,不使用缓冲区
 ******************************************************************************/
void lcd_show_info(void)
{
    const char* freq_str[] = {"2K", "4K", "6K", "8K", "10"};
    uint8_t i;
    
    // 左上角显示频率 (page 0)
    for(i = 0; freq_str[current_freq_band][i] != '\0' && i < 2; i++)
    {
        lcd_draw_char_direct(2 + i * 6, 0, freq_str[current_freq_band][i]);
    }
    
    // 显示幅度(右上角, page 0)
    lcd_draw_char_direct(LCD_WIDTH - 18, 0, '0' + (current_amplitude + 1));
    lcd_draw_char_direct(LCD_WIDTH - 12, 0, 'V');
    
    // 显示相位(左下角, page 7)
    uint8_t phase_abs = (phase_diff_deg < 0) ? (-phase_diff_deg) : phase_diff_deg;
    if(phase_abs > 999) phase_abs = 999;  // 防止溢出
    lcd_draw_char_direct(2, 7, '0' + (phase_abs / 100) % 10);
    lcd_draw_char_direct(8, 7, '0' + (phase_abs / 10) % 10);
    lcd_draw_char_direct(14, 7, '0' + phase_abs % 10);
}

/******************************************************************************
 * 设置频率波段
 * 参数: band - 频率波段 (FREQ_2KHZ ~ FREQ_10KHZ)
 * 
 * 波段选择策略:
 *   - 2k/4k/6k/8kHz: 通过CD4052四选一实现,CD4053_A=0
 *   - 10kHz: CD4053_A=1,直接选择10kHz滤波器输出
 ******************************************************************************/
void set_frequency_band(freq_band_t band)
{
    current_freq_band = band;
    
    if(band == FREQ_10KHZ)
    {
        // 10kHz: CD4053第一级选择10kHz通道
        update_cd4053_freq_select(1);
        // CD4052设置无关紧要,但为了规范设为0
        update_cd4052_control(0);
    }
    else
    {
        // 2k/4k/6k/8kHz: CD4053第一级选择CD4052输出
        update_cd4053_freq_select(0);
        // CD4052选择对应的滤波器
        update_cd4052_control((uint8_t)band);
    }
}

/******************************************************************************
 * 设置波形类型
 * 参数: waveform - 波形类型 (WAVEFORM_SINE 或 WAVEFORM_TRIANGLE)
 ******************************************************************************/
void set_waveform_type(waveform_type_t waveform)
{
    current_waveform = waveform;
    update_cd4053_wave_select(waveform);
}

/******************************************************************************
 * 更新CD4052控制信号
 * 参数: channel - 通道选择 (0-3)
 *   0: 2kHz滤波器
 *   1: 4kHz滤波器
 *   2: 6kHz滤波器
 *   3: 8kHz滤波器
 * 
 * CD4052真值表:
 *   B A | 选择通道
 *   0 0 | 通道0 (2kHz)
 *   0 1 | 通道1 (4kHz)
 *   1 0 | 通道2 (6kHz)
 *   1 1 | 通道3 (8kHz)
 ******************************************************************************/
void update_cd4052_control(uint8_t channel)
{
    // 清除P3.0和P3.1
    P3OUT &= ~(CD4052_A | CD4052_B);
    
    // 设置通道
    if(channel & 0x01)  // A位
        P3OUT |= CD4052_A;
    if(channel & 0x02)  // B位
        P3OUT |= CD4052_B;
}

/******************************************************************************
 * 更新CD4053频率选择控制信号
 * 参数: select_10k - 是否选择10kHz
 *   0: 选择CD4052输出 (2k/4k/6k/8kHz)
 *   1: 选择10kHz滤波器输出
 ******************************************************************************/
void update_cd4053_freq_select(uint8_t select_10k)
{
    if(select_10k)
        P3OUT |= CD4053_A;
    else
        P3OUT &= ~CD4053_A;
}

/******************************************************************************
 * 更新CD4053波形选择控制信号
 * 参数: waveform - 波形类型
 *   WAVEFORM_SINE (0): 选择正弦波输出
 *   WAVEFORM_TRIANGLE (1): 选择三角波输出
 ******************************************************************************/
void update_cd4053_wave_select(waveform_type_t waveform)
{
    if(waveform == WAVEFORM_TRIANGLE)
        P3OUT |= CD4053_B;
    else
        P3OUT &= ~CD4053_B;
}

/******************************************************************************
 * Port1 中断服务程序 (按键中断)
 ******************************************************************************/
#pragma vector=PORT1_VECTOR
__interrupt void Port1_ISR(void)
{
    if(P1IFG & KEY_INPUT)
    {
        // 清除中断标志
        P1IFG &= ~KEY_INPUT;
        
        // 简单防抖: 延时后再次检测
        __delay_cycles(50000);  // 约50ms @ 1MHz
        
        if((P1IN & KEY_INPUT) == 0)
        {
            key_pressed = 1;
        }
    }
}

/******************************************************************************
 * Timer1 CCR0 中断服务程序 (X轴捕获)
 ******************************************************************************/
#pragma vector=TIMER1_A0_VECTOR
__interrupt void Timer1_A0_ISR(void)
{
    x_capture_time = TA1CCR0;  // 读取捕获值
    phase_valid = 1;           // 标记数据有效
}

/******************************************************************************
 * Timer1 CCR1 中断服务程序 (Y轴捕获)
 ******************************************************************************/
#pragma vector=TIMER1_A1_VECTOR
__interrupt void Timer1_A1_ISR(void)
{
    switch(TA1IV)
    {
        case TA1IV_TACCR1:  // CCR1捕获
            y_capture_time = TA1CCR1;  // 读取捕获值
            break;
        default:
            break;
    }
}
