/*
 * config.h
 * 
 * 系统配置头文件 - 李萨如图形发生器
 * 包含系统级配置参数、编译选项、调试开关等
 * 
 * 作者: 2026电子信息杯参赛队
 * 日期: 2025年11月21日
 */

#ifndef CONFIG_H_
#define CONFIG_H_

// =============================================================================
// 系统版本信息
// =============================================================================

#define SYSTEM_NAME             "Lissajous Generator"
#define SYSTEM_VERSION_MAJOR    2
#define SYSTEM_VERSION_MINOR    3
#define SYSTEM_PATCH            0
#define BUILD_DATE              __DATE__
#define BUILD_TIME              __TIME__

// 版本字符串宏
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define VERSION_STRING TOSTRING(SYSTEM_VERSION_MAJOR) "." TOSTRING(SYSTEM_VERSION_MINOR) "." TOSTRING(SYSTEM_PATCH)

// =============================================================================
// 硬件平台配置
// =============================================================================

// 目标MCU
#define MCU_MSP430G2553         1
#define MCU_CLOCK_FREQ          1000000UL       // 1MHz DCO
#define MCU_FLASH_SIZE          16384           // 16KB Flash
#define MCU_RAM_SIZE            512             // 512B RAM

// 外设配置
#define USE_LCD_DISPLAY         1               // 启用LCD显示
#define USE_I2C_DAC             1               // 启用I2C数字电位器
#define USE_ADC_AGC             1               // 启用ADC AGC控制
#define USE_FREQUENCY_MUL       1               // 启用倍频模块

// =============================================================================
// 倍频模块配置
// =============================================================================

// 倍频参数
#define FREQ_MUL_MIN            1               // 最小倍频系数
#define FREQ_MUL_MAX            5               // 最大倍频系数
#define FREQ_MUL_DEFAULT        1               // 默认倍频系数

// 74HC161配置
#define HC161_PRESET_BASE       16              // 预置数基数 (16-n)
#define HC161_MAX_COUNT         15              // 最大计数值

// PLL参数
#define PLL_LOCK_TIMEOUT_MS     100             // PLL锁定超时时间
#define PLL_LOCK_CHECK_PERIOD   10              // PLL锁定检查周期(ms)

// 频率表配置
#define FREQUENCY_COUNT         5
#define DEFAULT_FREQ_INDEX      0               // 默认频率索引(2kHz)

// =============================================================================
// AGC控制配置
// =============================================================================

// ADC参数
#define ADC_SAMPLES             32              // ADC采样点数
#define ADC_RESOLUTION          1024            // 10位ADC分辨率
#define ADC_REF_VOLTAGE         5000            // 参考电压(mV)
#define ADC_SAMPLE_RATE         10000           // 采样频率(Hz)

// AGC目标值 (基于5V参考)
#define AGC_TARGET_1V           205             // 1V对应ADC值
#define AGC_TARGET_2V           410             // 2V对应ADC值  
#define AGC_TARGET_3V           615             // 3V对应ADC值
#define AGC_TOLERANCE           50              // AGC容差
#define AGC_TOLERANCE_WIDE      150             // 宽容差 (3倍)

// PID控制器参数
#define AGC_KP                  16              // 比例系数分母 (1/16)
#define AGC_KI                  64              // 积分系数分母 (1/64)
#define AGC_RESPONSE_TIME_MS    100             // AGC响应时间目标

// 数字电位器配置
#define MCP4018T_ADDR           0x2E            // I2C地址
#define DAC_RESOLUTION          128             // 7位分辨率
#define DAC_DEFAULT_VALUE       64              // 默认中间值
#define DAC_MIN_VALUE           0               // 最小值
#define DAC_MAX_VALUE           127             // 最大值

// =============================================================================
// I2C配置
// =============================================================================

#define I2C_CLOCK_FREQ          100000          // 100kHz标准速度
#define I2C_TIMEOUT_MS          10              // I2C操作超时
#define I2C_RETRY_COUNT         3               // 重试次数
#define I2C_SCL_PIN             BIT6            // P1.6
#define I2C_SDA_PIN             BIT7            // P1.7

// I2C时序参数 (基于1MHz时钟)
#define I2C_DELAY_US            5               // 基本延时
#define I2C_SETUP_TIME_US       2               // 建立时间
#define I2C_HOLD_TIME_US        2               // 保持时间

// =============================================================================
// LCD显示配置  
// =============================================================================

// LCD规格参数
#define LCD_WIDTH               128             // 像素宽度
#define LCD_HEIGHT              64              // 像素高度
#define LCD_PAGES               8               // 页数 (64/8)
#define LCD_CHAR_ROWS           4               // 字符行数
#define LCD_CHAR_COLS           16              // 字符列数

// LCD SPI配置
#define LCD_SPI_FREQ            1000000         // 1MHz SPI时钟
#define LCD_REFRESH_RATE        2               // 刷新频率(Hz)
#define LCD_INIT_DELAY_MS       100             // 初始化延时

// LCD引脚定义
#define LCD_CS_PIN              BIT2            // P2.2
#define LCD_A0_PIN              BIT3            // P2.3  
#define LCD_RES_PIN             BIT4            // P2.4
#define LCD_SCL_PIN             BIT5            // P2.5
#define LCD_SI_PIN              BIT7            // P2.7

// 显示内容配置
#define SHOW_STARTUP_SCREEN     1               // 显示启动画面
#define STARTUP_DELAY_MS        2000            // 启动画面延时
#define SHOW_VERSION_INFO       1               // 显示版本信息
#define SHOW_DEBUG_INFO         0               // 显示调试信息

// =============================================================================
// 按键配置
// =============================================================================

// 按键引脚
#define FREQ_KEY_PORT           P2IN
#define FREQ_KEY_PIN            BIT6            // P2.6
#define AMP_KEY_PORT            P1IN  
#define AMP_KEY_PIN             BIT5            // P1.5

// 按键参数
#define KEY_DEBOUNCE_MS         50              // 防抖时间
#define KEY_REPEAT_MS           500             // 按键重复间隔
#define KEY_LONG_PRESS_MS       1000            // 长按判定时间

// 按键功能使能
#define ENABLE_KEY_REPEAT       0               // 禁用按键重复
#define ENABLE_LONG_PRESS       0               // 禁用长按检测

// =============================================================================
// 系统定时器配置
// =============================================================================

// 系统节拍
#define SYSTEM_TICK_MS          10              // 系统节拍周期
#define TICKS_PER_SECOND        (1000/SYSTEM_TICK_MS)

// 定时器分配
#define TIMER_A0_FUNCTION       FREQ_MEASURE    // TA0用于频率测量
#define TIMER_A1_FUNCTION       SYSTEM_TICK     // TA1用于系统节拍
#define TIMER_B0_FUNCTION       ADC_TRIGGER     // TB0用于ADC触发

// 看门狗配置
#define ENABLE_WATCHDOG         0               // 禁用看门狗
#define WATCHDOG_TIMEOUT_MS     1000            // 看门狗超时

// =============================================================================
// 调试和诊断配置
// =============================================================================

// 调试开关
#ifndef DEBUG_MODE
#define DEBUG_MODE              0               // 默认关闭调试模式
#endif

#if DEBUG_MODE
    #define DEBUG_SERIAL        1               // 串口调试输出
    #define DEBUG_LED           1               // LED调试指示
    #define DEBUG_LCD           1               // LCD调试信息
    #define ENABLE_SELF_TEST    1               // 启用自检程序
    #define VERBOSE_LOGGING     1               // 详细日志
#else
    #define DEBUG_SERIAL        0
    #define DEBUG_LED           0  
    #define DEBUG_LCD           0
    #define ENABLE_SELF_TEST    0
    #define VERBOSE_LOGGING     0
#endif

// 性能监控
#define ENABLE_PERF_MONITOR     DEBUG_MODE      // 性能监控
#define ENABLE_STACK_CHECK      DEBUG_MODE      // 堆栈检查
#define ENABLE_HEAP_CHECK       0               // 堆检查 (无动态内存)

// 错误处理
#define ENABLE_ERROR_RECOVERY   1               // 启用错误恢复
#define MAX_ERROR_COUNT         10              // 最大错误计数
#define ERROR_RESET_TIMEOUT     5000            // 错误复位超时(ms)

// =============================================================================
// 内存管理配置
// =============================================================================

// 静态内存分配
#define ADC_BUFFER_SIZE         ADC_SAMPLES     // ADC缓冲区大小
#define LCD_BUFFER_SIZE         (LCD_WIDTH * LCD_PAGES)  // LCD帧缓冲区
#define STRING_BUFFER_SIZE      32              // 字符串缓冲区

// 堆栈配置
#define STACK_SIZE              128             // 堆栈大小(字节)
#define STACK_GUARD_PATTERN     0xDEAD          // 堆栈保护模式

// 内存检查
#define ENABLE_MEMORY_CHECK     DEBUG_MODE      // 内存检查
#define MEMORY_CHECK_PERIOD     1000            // 检查周期(ms)

// =============================================================================
// 功耗管理配置
// =============================================================================

// 低功耗模式
#define DEFAULT_LPM_MODE        LPM0_bits       // 默认低功耗模式
#define ENABLE_AUTO_LPM         1               // 自动进入低功耗
#define LPM_ENTER_DELAY_MS      100             // 进入低功耗延时

// 时钟管理
#define DYNAMIC_CLOCK_SCALING   0               // 动态时钟调节
#define IDLE_CLOCK_FREQ         250000UL        // 空闲时钟频率

// 外设功耗控制
#define AUTO_DISABLE_UNUSED     1               // 自动禁用未用外设
#define LCD_POWER_SAVE          1               // LCD节能模式

// =============================================================================
// 编译优化配置
// =============================================================================

// 编译器优化
#ifndef OPTIMIZATION_LEVEL
    #if DEBUG_MODE
        #define OPTIMIZATION_LEVEL  0           // 调试模式无优化
    #else
        #define OPTIMIZATION_LEVEL  2           // 发布模式O2优化
    #endif
#endif

// 代码大小优化
#define OPTIMIZE_FOR_SIZE       1               // 优化代码大小
#define INLINE_SMALL_FUNCTIONS  1               // 内联小函数
#define REMOVE_UNUSED_CODE      1               // 移除未使用代码

// 编译时检查
#define STATIC_ASSERT_SUPPORT   1               // 静态断言支持
#define COMPILE_TIME_CHECKS     1               // 编译时检查

// =============================================================================
// 特性开关配置
// =============================================================================

// 功能特性
#define ENABLE_PHASE_MEASURE    1               // 相位测量功能
#define ENABLE_FREQ_SWEEP       0               // 频率扫描功能
#define ENABLE_DATA_LOGGING     0               // 数据记录功能
#define ENABLE_REMOTE_CONTROL   0               // 远程控制功能

// 用户界面特性
#define ENABLE_MENU_SYSTEM      0               // 菜单系统
#define ENABLE_PARAMETER_SAVE   0               // 参数保存功能
#define ENABLE_FACTORY_RESET    1               // 出厂复位功能

// 高级特性
#define ENABLE_AUTO_CALIBRATION 0               // 自动校准
#define ENABLE_TEMP_COMPENSATION 0              // 温度补偿
#define ENABLE_AGING_COMPENSATION 0             // 老化补偿

// =============================================================================
// 安全和可靠性配置
// =============================================================================

// 参数范围检查
#define ENABLE_PARAM_VALIDATION 1               // 参数验证
#define STRICT_TYPE_CHECKING    1               // 严格类型检查
#define BOUNDARY_CHECKS         DEBUG_MODE      // 边界检查

// 故障检测
#define ENABLE_FAULT_DETECTION  1               // 故障检测
#define ENABLE_SELF_DIAGNOSIS   DEBUG_MODE      // 自诊断功能
#define ENABLE_FAULT_RECOVERY   1               // 故障恢复

// CRC校验
#define ENABLE_CRC_CHECK        0               // CRC校验 (节省Flash)
#define CRC_POLYNOMIAL          0x1021          // CRC-16多项式

// =============================================================================
// 编译时检查和验证
// =============================================================================

#if STATIC_ASSERT_SUPPORT
// 静态断言检查关键参数
_Static_assert(FREQ_MUL_MAX <= 5, "Maximum frequency multiplier is 5");
_Static_assert(ADC_SAMPLES <= 64, "Too many ADC samples for available RAM");
_Static_assert(MCU_CLOCK_FREQ <= 16000000UL, "Clock frequency too high for MSP430G2553");
_Static_assert(DAC_MAX_VALUE < DAC_RESOLUTION, "DAC max value exceeds resolution");

// 内存使用检查
_Static_assert((ADC_BUFFER_SIZE * 2 + LCD_BUFFER_SIZE + STRING_BUFFER_SIZE + STACK_SIZE) < MCU_RAM_SIZE, 
               "Total memory usage exceeds available RAM");
#endif

// 功能组合检查
#if USE_I2C_DAC && !USE_ADC_AGC
    #error "I2C DAC requires AGC functionality"
#endif

#if USE_FREQUENCY_MUL && (FREQ_MUL_MAX < FREQ_MUL_MIN)
    #error "Invalid frequency multiplier range"
#endif

// =============================================================================
// 辅助宏定义
// =============================================================================

// 时间转换宏
#define MS_TO_TICKS(ms)         ((ms) / SYSTEM_TICK_MS)
#define TICKS_TO_MS(ticks)      ((ticks) * SYSTEM_TICK_MS)
#define US_TO_CYCLES(us)        ((us) * (MCU_CLOCK_FREQ / 1000000UL))

// 数学工具宏
#define ABS(x)                  (((x) < 0) ? -(x) : (x))
#define MIN(a, b)               (((a) < (b)) ? (a) : (b))
#define MAX(a, b)               (((a) > (b)) ? (a) : (b))
#define CLAMP(x, min, max)      (MIN(MAX((x), (min)), (max)))

// 位操作宏
#define SET_BIT(reg, bit)       ((reg) |= (bit))
#define CLR_BIT(reg, bit)       ((reg) &= ~(bit))
#define TGL_BIT(reg, bit)       ((reg) ^= (bit))
#define TST_BIT(reg, bit)       (((reg) & (bit)) != 0)

// 数组大小宏
#define ARRAY_SIZE(arr)         (sizeof(arr) / sizeof((arr)[0]))

// 版本比较宏
#define VERSION_COMPARE(maj, min, pat) \
    ((SYSTEM_VERSION_MAJOR > (maj)) || \
     (SYSTEM_VERSION_MAJOR == (maj) && SYSTEM_VERSION_MINOR > (min)) || \
     (SYSTEM_VERSION_MAJOR == (maj) && SYSTEM_VERSION_MINOR == (min) && SYSTEM_PATCH >= (pat)))

#endif /* CONFIG_H_ */