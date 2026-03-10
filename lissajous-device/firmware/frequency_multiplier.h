/*
 * frequency_multiplier.h
 * 
 * 锁相环倍频模块头文件 - 2026电子信息杯李萨如图形发生器
 * 
 * 提供倍频控制、按键处理、LCD显示等接口函数声明
 * 
 * 作者: 2026电子信息杯参赛队
 * 日期: 2025年11月21日
 */

#ifndef FREQUENCY_MULTIPLIER_H_
#define FREQUENCY_MULTIPLIER_H_

#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>

// =============================================================================
// 公共常量定义
// =============================================================================

// 倍频系数范围
#define MIN_MULTIPLIER      1
#define MAX_MULTIPLIER      5

// 默认频率值 (Hz)
#define DEFAULT_INPUT_FREQ  2000        // 2kHz
#define MAX_INPUT_FREQ      20000       // 20kHz
#define MIN_INPUT_FREQ      1000        // 1kHz

// LCD显示参数
#define LCD_ROWS            4
#define LCD_COLS            16

// 系统状态码
typedef enum {
    FREQ_MUL_OK = 0,                    // 操作成功
    FREQ_MUL_ERROR_PARAM,               // 参数错误  
    FREQ_MUL_ERROR_HARDWARE,            // 硬件错误
    FREQ_MUL_ERROR_PLL_UNLOCK           // PLL未锁定
} freq_mul_status_t;

// =============================================================================
// 数据结构定义
// =============================================================================

/**
 * @brief 倍频系统状态结构体
 * 包含当前倍频配置和系统状态信息
 */
typedef struct {
    uint8_t multiplier;                 // 当前倍频系数 (1-5)
    uint8_t preset_value;               // 74HC161预置数值
    bool pll_locked;                    // PLL锁定状态
    uint32_t input_freq;                // 输入参考频率(Hz)
    uint32_t output_freq;               // 输出频率(Hz)
} multiplier_state_t;

/**
 * @brief 倍频配置参数结构体  
 * 用于配置倍频系数与硬件参数的对应关系
 */
typedef struct {
    uint8_t multiplier;                 // 倍频系数
    uint8_t preset;                     // 74HC161预置数 (16-倍频系数)
    uint8_t gpio_value;                 // P1.3-P1.1对应的GPIO值
} multiplier_config_t;

// =============================================================================
// 核心功能函数声明
// =============================================================================

/**
 * @brief 倍频模块初始化
 * 初始化GPIO、定时器、LCD等硬件模块
 * @return: 初始化状态码
 */
freq_mul_status_t frequency_multiplier_init(void);

/**
 * @brief 设置倍频系数
 * @param multiplier: 目标倍频系数 (1-5)
 * @return: 设置状态码
 */
freq_mul_status_t set_frequency_multiplier(uint8_t multiplier);

/**
 * @brief 获取当前倍频系数
 * @return: 当前倍频系数 (1-5)
 */
uint8_t get_current_multiplier(void);

/**
 * @brief 切换到下一个倍频系数
 * 循环切换: 1→2→3→4→5→1...
 * @return: 切换后的倍频系数
 */
uint8_t switch_next_multiplier(void);

/**
 * @brief 设置输入参考频率
 * @param freq: 输入频率 (Hz), 范围: MIN_INPUT_FREQ ~ MAX_INPUT_FREQ
 * @return: 设置状态码
 */
freq_mul_status_t set_input_frequency(uint32_t freq);

/**
 * @brief 获取输出频率
 * @return: 当前输出频率 (Hz)
 */
uint32_t get_output_frequency(void);

// =============================================================================
// 硬件控制函数声明  
// =============================================================================

/**
 * @brief 设置74HC161预置数
 * @param preset_val: 预置数值 (11-14对应5-2倍频)
 */
void set_hc161_preset(uint8_t preset_val);

/**
 * @brief 获取PLL锁定状态
 * @return: true=已锁定, false=未锁定  
 */
bool get_pll_lock_status(void);

/**
 * @brief 等待PLL锁定
 * @param timeout_ms: 超时时间 (毫秒)
 * @return: true=锁定成功, false=超时
 */
bool wait_pll_lock(uint16_t timeout_ms);

// =============================================================================
// LCD显示函数声明
// =============================================================================

/**
 * @brief LCD模块初始化
 * 配置SPI接口和LCD显示参数
 */
void lcd_init(void);

/**
 * @brief 更新LCD显示内容
 * 显示当前倍频系数、频率信息、PLL状态等
 */
void update_lcd_display(void);

/**
 * @brief 清空LCD显示
 */
void lcd_clear(void);

/**
 * @brief 在LCD指定位置显示字符串
 * @param row: 行号 (0-3)
 * @param col: 列号 (0-15)  
 * @param str: 要显示的字符串
 */
void lcd_print_at(uint8_t row, uint8_t col, const char* str);

/**
 * @brief 在LCD显示数字
 * @param row: 行号
 * @param col: 列号
 * @param num: 要显示的数字
 * @param width: 显示宽度 (右对齐)
 */
void lcd_print_number_at(uint8_t row, uint8_t col, uint32_t num, uint8_t width);

/**
 * @brief 显示PLL锁定状态指示
 * @param locked: true=锁定, false=未锁定
 */
void lcd_show_pll_status(bool locked);

// =============================================================================
// 按键处理函数声明
// =============================================================================

/**
 * @brief 按键模块初始化
 * 配置按键GPIO和中断
 */
void key_init(void);

/**
 * @brief 按键扫描处理
 * 在主循环或定时器中调用，处理按键防抖
 */
void key_scan(void);

/**
 * @brief 频率按键回调函数
 * 按键按下时的处理函数，可重定义
 */
void __attribute__((weak)) on_freq_key_pressed(void);

// =============================================================================
// 系统状态监测函数声明
// =============================================================================

/**
 * @brief 获取倍频器系统状态
 * @return: 系统状态结构体指针
 */
const multiplier_state_t* get_multiplier_state(void);

/**
 * @brief 倍频模块自检
 * 测试所有倍频系数的设置和LCD显示
 * @return: 自检结果状态码
 */
freq_mul_status_t multiplier_self_test(void);

/**
 * @brief 系统复位
 * 将倍频器复位到默认状态
 */
void multiplier_reset(void);

// =============================================================================
// 调试和诊断函数声明
// =============================================================================

#ifdef DEBUG_MODE

/**
 * @brief 打印倍频器状态信息
 * 通过串口或其他方式输出调试信息
 */
void debug_print_state(void);

/**
 * @brief 手动设置PLL锁定状态 (调试用)
 * @param locked: 锁定状态
 */
void debug_set_pll_lock(bool locked);

/**
 * @brief 获取74HC161当前计数值 (仅调试)
 * @return: 计数值 (需要额外硬件支持)
 */
uint8_t debug_get_hc161_count(void);

#endif // DEBUG_MODE

// =============================================================================
// 中断服务程序声明
// =============================================================================

/**
 * @brief Port2中断服务程序
 * 处理频率切换按键中断 (P2.6)
 */
__interrupt void Port2_ISR(void);

/**
 * @brief TimerA中断服务程序  
 * 提供系统节拍，处理按键防抖等定时任务
 */
__interrupt void TimerA0_ISR(void);

// =============================================================================
// 宏定义辅助函数
// =============================================================================

/**
 * @brief 检查倍频系数是否有效
 * @param mult: 倍频系数
 * @return: true=有效, false=无效
 */
#define IS_VALID_MULTIPLIER(mult)   ((mult) >= MIN_MULTIPLIER && (mult) <= MAX_MULTIPLIER)

/**
 * @brief 检查频率是否在有效范围
 * @param freq: 频率值 (Hz)
 * @return: true=有效, false=无效  
 */
#define IS_VALID_FREQUENCY(freq)    ((freq) >= MIN_INPUT_FREQ && (freq) <= MAX_INPUT_FREQ)

/**
 * @brief LED状态控制宏
 */
#define LED_ON()        (P1OUT |= BIT6)
#define LED_OFF()       (P1OUT &= ~BIT6)
#define LED_TOGGLE()    (P1OUT ^= BIT6)

/**
 * @brief 延时宏 (基于1MHz时钟)
 */
#define DELAY_MS(ms)    __delay_cycles((ms) * 1000)
#define DELAY_US(us)    __delay_cycles(us)

// =============================================================================
// 错误处理宏
// =============================================================================

/**
 * @brief 参数检查宏
 * @param condition: 检查条件
 * @param error_code: 错误返回码
 */
#define CHECK_PARAM(condition, error_code) \
    do { \
        if (!(condition)) { \
            return (error_code); \
        } \
    } while(0)

/**
 * @brief 硬件状态检查宏
 * @param hw_check: 硬件检查函数
 * @param error_code: 错误返回码  
 */
#define CHECK_HARDWARE(hw_check, error_code) \
    do { \
        if (!(hw_check)) { \
            return (error_code); \
        } \
    } while(0)

#endif /* FREQUENCY_MULTIPLIER_H_ */