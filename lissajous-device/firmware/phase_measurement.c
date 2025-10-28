//******************************************************************************
// 李萨如图形演示装置 - 高精度相位测量算法
// 功能：实现±5°精度的相位差测量
//******************************************************************************

#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

//==============================================================================
// 相位测量相关常量
//==============================================================================

#define PHASE_BUFFER_SIZE       64      // 相位测量缓冲区大小
#define PHASE_FILTER_ORDER      8       // 数字滤波器阶数
#define ZERO_CROSS_THRESHOLD    50      // 过零点检测阈值 (ADC值)
#define PHASE_CALIBRATION_CNT   16      // 校准采样次数

#define PI                      3.14159265359f
#define DEG_PER_RAD             57.2957795131f

//==============================================================================
// 相位测量数据结构
//==============================================================================

typedef struct {
    uint32_t timestamp;                 // 时间戳
    uint8_t channel;                    // 通道 (0:X轴, 1:Y轴)
    bool valid;                         // 数据有效标志
} zero_cross_event_t;

typedef struct {
    zero_cross_event_t events[PHASE_BUFFER_SIZE];
    uint16_t write_index;               // 写指针
    uint16_t read_index;                // 读指针
    uint16_t count;                     // 有效事件数
} zero_cross_buffer_t;

typedef struct {
    float phase_history[PHASE_FILTER_ORDER];   // 相位历史值
    uint8_t history_index;                      // 历史值索引
    float filtered_phase;                       // 滤波后相位
    float phase_variance;                       // 相位方差
} phase_filter_t;

typedef struct {
    uint32_t x_period_sum;              // X轴周期累加
    uint32_t y_period_sum;              // Y轴周期累加
    uint16_t period_count;              // 周期计数
    float avg_x_period;                 // 平均X轴周期
    float avg_y_period;                 // 平均Y轴周期
    bool calibration_done;              // 校准完成标志
} frequency_calibration_t;

//==============================================================================
// 全局变量
//==============================================================================

static zero_cross_buffer_t g_zero_cross_buffer = {0};
static phase_filter_t g_phase_filter = {0};
static frequency_calibration_t g_freq_cal = {0};

// ADC采样缓冲区
static volatile uint16_t g_x_adc_buffer[32];
static volatile uint16_t g_y_adc_buffer[32];
static volatile uint8_t g_adc_index = 0;

//==============================================================================
// 函数声明
//==============================================================================

// 相位测量初始化
void phase_measurement_init(void);

// 过零点检测
bool detect_zero_crossing(uint16_t* buffer, uint8_t size, uint8_t channel);
void add_zero_cross_event(uint8_t channel, uint32_t timestamp);

// 相位计算
float calculate_precise_phase_difference(void);
float calculate_phase_from_events(void);

// 数字滤波
void phase_filter_init(void);
float apply_phase_filter(float new_phase);
float calculate_phase_variance(void);

// 频率校准
void frequency_calibration_init(void);
void update_frequency_calibration(uint32_t x_period, uint32_t y_period);
bool is_frequency_stable(void);

// 算法优化
float compensate_timing_error(float raw_phase, float frequency);
float normalize_phase_difference(float phase_diff);

//==============================================================================
// 相位测量初始化
//==============================================================================

void phase_measurement_init(void)
{
    // 初始化缓冲区
    g_zero_cross_buffer.write_index = 0;
    g_zero_cross_buffer.read_index = 0;
    g_zero_cross_buffer.count = 0;
    
    // 初始化过零点事件缓冲区
    for (uint16_t i = 0; i < PHASE_BUFFER_SIZE; i++) {
        g_zero_cross_buffer.events[i].valid = false;
    }
    
    // 初始化滤波器
    phase_filter_init();
    
    // 初始化频率校准
    frequency_calibration_init();
    
    // 配置Timer_A用于高精度时间测量
    TA1CTL = TASSEL_2 | MC_2 | TACLR;           // SMCLK, 连续模式, 清除计数器
    TA1CCTL0 = CM_3 | CCIS_0 | SCS | CAP | CCIE; // 双边沿捕获, CCI0A, 同步, 中断使能
    TA1CCTL1 = CM_3 | CCIS_1 | SCS | CAP | CCIE; // 双边沿捕获, CCI1A, 同步, 中断使能
}

//==============================================================================
// 过零点检测算法
//==============================================================================

bool detect_zero_crossing(uint16_t* buffer, uint8_t size, uint8_t channel)
{
    static uint16_t last_x_value = 512;  // 上次X轴ADC值 (假设中点为512)
    static uint16_t last_y_value = 512;  // 上次Y轴ADC值
    static bool last_x_state = false;    // 上次X轴状态 (高/低)
    static bool last_y_state = false;    // 上次Y轴状态
    
    uint16_t current_value = buffer[g_adc_index];
    uint16_t threshold = 512;  // 中点阈值
    
    bool current_state = (current_value > threshold);
    bool zero_cross_detected = false;
    
    if (channel == 0) {  // X轴
        // 检测状态变化 (过零点)
        if (current_state != last_x_state) {
            // 进一步验证 - 检查幅度变化是否足够大
            if (abs((int)current_value - (int)last_x_value) > ZERO_CROSS_THRESHOLD) {
                zero_cross_detected = true;
                add_zero_cross_event(0, TA1R);
            }
        }
        last_x_value = current_value;
        last_x_state = current_state;
    } else {  // Y轴
        if (current_state != last_y_state) {
            if (abs((int)current_value - (int)last_y_value) > ZERO_CROSS_THRESHOLD) {
                zero_cross_detected = true;
                add_zero_cross_event(1, TA1R);
            }
        }
        last_y_value = current_value;
        last_y_state = current_state;
    }
    
    return zero_cross_detected;
}

void add_zero_cross_event(uint8_t channel, uint32_t timestamp)
{
    // 添加过零事件到缓冲区
    if (g_zero_cross_buffer.count < PHASE_BUFFER_SIZE) {
        uint16_t index = g_zero_cross_buffer.write_index;
        
        g_zero_cross_buffer.events[index].channel = channel;
        g_zero_cross_buffer.events[index].timestamp = timestamp;
        g_zero_cross_buffer.events[index].valid = true;
        
        g_zero_cross_buffer.write_index = (index + 1) % PHASE_BUFFER_SIZE;
        g_zero_cross_buffer.count++;
    }
}

//==============================================================================
// 高精度相位计算算法
//==============================================================================

float calculate_precise_phase_difference(void)
{
    // 等待频率稳定
    if (!g_freq_cal.calibration_done) {
        return g_phase_filter.filtered_phase;  // 返回上次滤波值
    }
    
    // 计算原始相位差
    float raw_phase = calculate_phase_from_events();
    
    // 时序误差补偿
    float compensated_phase = compensate_timing_error(raw_phase, g_freq_cal.avg_x_period);
    
    // 数字滤波处理
    float filtered_phase = apply_phase_filter(compensated_phase);
    
    // 相位归一化 (-180° ~ +180°)
    float normalized_phase = normalize_phase_difference(filtered_phase);
    
    return normalized_phase;
}

float calculate_phase_from_events(void)
{
    if (g_zero_cross_buffer.count < 4) {
        return 0.0f;  // 需要至少4个事件 (X轴和Y轴各2个)
    }
    
    // 查找最近的X轴和Y轴过零事件
    uint32_t last_x_time = 0, last_y_time = 0;
    uint32_t prev_x_time = 0, prev_y_time = 0;
    bool x_found = false, y_found = false;
    
    // 从最新事件开始向前搜索
    for (int16_t i = g_zero_cross_buffer.count - 1; i >= 0; i--) {
        uint16_t index = (g_zero_cross_buffer.write_index - 1 - i + PHASE_BUFFER_SIZE) % PHASE_BUFFER_SIZE;
        zero_cross_event_t* event = &g_zero_cross_buffer.events[index];
        
        if (!event->valid) continue;
        
        if (event->channel == 0 && !x_found) {  // X轴事件
            if (last_x_time == 0) {
                last_x_time = event->timestamp;
            } else {
                prev_x_time = event->timestamp;
                x_found = true;
            }
        } else if (event->channel == 1 && !y_found) {  // Y轴事件
            if (last_y_time == 0) {
                last_y_time = event->timestamp;
            } else {
                prev_y_time = event->timestamp;
                y_found = true;
            }
        }
        
        if (x_found && y_found) break;
    }
    
    if (!x_found || !y_found) {
        return g_phase_filter.filtered_phase;  // 返回上次值
    }
    
    // 计算周期
    uint32_t x_period = last_x_time - prev_x_time;
    uint32_t y_period = last_y_time - prev_y_time;
    
    // 更新频率校准
    update_frequency_calibration(x_period, y_period);
    
    // 计算时间差
    int32_t time_diff = (int32_t)last_y_time - (int32_t)last_x_time;
    
    // 处理时间差跨越周期边界的情况
    float avg_period = (g_freq_cal.avg_x_period + g_freq_cal.avg_y_period) / 2.0f;
    
    if (time_diff > avg_period / 2) {
        time_diff -= (int32_t)avg_period;
    } else if (time_diff < -avg_period / 2) {
        time_diff += (int32_t)avg_period;
    }
    
    // 转换为相位差 (度)
    float phase_diff = (time_diff / avg_period) * 360.0f;
    
    return phase_diff;
}

//==============================================================================
// 数字滤波器实现
//==============================================================================

void phase_filter_init(void)
{
    for (uint8_t i = 0; i < PHASE_FILTER_ORDER; i++) {
        g_phase_filter.phase_history[i] = 0.0f;
    }
    g_phase_filter.history_index = 0;
    g_phase_filter.filtered_phase = 0.0f;
    g_phase_filter.phase_variance = 0.0f;
}

float apply_phase_filter(float new_phase)
{
    // 移动平均滤波器
    g_phase_filter.phase_history[g_phase_filter.history_index] = new_phase;
    g_phase_filter.history_index = (g_phase_filter.history_index + 1) % PHASE_FILTER_ORDER;
    
    // 计算加权平均 (近期数据权重更大)
    float sum = 0.0f;
    float weight_sum = 0.0f;
    
    for (uint8_t i = 0; i < PHASE_FILTER_ORDER; i++) {
        float weight = (float)(i + 1) / PHASE_FILTER_ORDER;  // 权重递增
        uint8_t index = (g_phase_filter.history_index - 1 - i + PHASE_FILTER_ORDER) % PHASE_FILTER_ORDER;
        sum += g_phase_filter.phase_history[index] * weight;
        weight_sum += weight;
    }
    
    g_phase_filter.filtered_phase = sum / weight_sum;
    
    // 计算方差用于稳定性评估
    g_phase_filter.phase_variance = calculate_phase_variance();
    
    return g_phase_filter.filtered_phase;
}

float calculate_phase_variance(void)
{
    float mean = g_phase_filter.filtered_phase;
    float variance = 0.0f;
    
    for (uint8_t i = 0; i < PHASE_FILTER_ORDER; i++) {
        float diff = g_phase_filter.phase_history[i] - mean;
        variance += diff * diff;
    }
    
    return variance / PHASE_FILTER_ORDER;
}

//==============================================================================
// 频率校准算法
//==============================================================================

void frequency_calibration_init(void)
{
    g_freq_cal.x_period_sum = 0;
    g_freq_cal.y_period_sum = 0;
    g_freq_cal.period_count = 0;
    g_freq_cal.avg_x_period = 16000.0f;  // 默认1kHz对应的Timer计数值
    g_freq_cal.avg_y_period = 16000.0f;
    g_freq_cal.calibration_done = false;
}

void update_frequency_calibration(uint32_t x_period, uint32_t y_period)
{
    // 丢弃异常值
    if (x_period < 8000 || x_period > 32000 || 
        y_period < 1600 || y_period > 32000) {
        return;
    }
    
    g_freq_cal.x_period_sum += x_period;
    g_freq_cal.y_period_sum += y_period;
    g_freq_cal.period_count++;
    
    if (g_freq_cal.period_count >= PHASE_CALIBRATION_CNT) {
        g_freq_cal.avg_x_period = (float)g_freq_cal.x_period_sum / g_freq_cal.period_count;
        g_freq_cal.avg_y_period = (float)g_freq_cal.y_period_sum / g_freq_cal.period_count;
        g_freq_cal.calibration_done = true;
        
        // 重置用于下一轮校准
        g_freq_cal.x_period_sum = 0;
        g_freq_cal.y_period_sum = 0;
        g_freq_cal.period_count = 0;
    }
}

bool is_frequency_stable(void)
{
    return g_freq_cal.calibration_done && 
           g_phase_filter.phase_variance < 25.0f;  // 方差阈值 (5°^2)
}

//==============================================================================
// 算法优化函数
//==============================================================================

float compensate_timing_error(float raw_phase, float frequency)
{
    // 补偿由于采样延迟引起的时序误差
    const float SAMPLING_DELAY = 2.0f;  // 采样延迟 (微秒)
    float period_us = 1000000.0f / frequency;  // 周期 (微秒)
    float delay_phase = (SAMPLING_DELAY / period_us) * 360.0f;
    
    return raw_phase - delay_phase;
}

float normalize_phase_difference(float phase_diff)
{
    // 将相位差归一化到 [-180°, +180°] 范围
    while (phase_diff > 180.0f) {
        phase_diff -= 360.0f;
    }
    while (phase_diff < -180.0f) {
        phase_diff += 360.0f;
    }
    
    return phase_diff;
}

//==============================================================================
// 相位测量精度评估
//==============================================================================

float get_phase_measurement_accuracy(void)
{
    // 基于当前方差估算测量精度
    if (!is_frequency_stable()) {
        return 10.0f;  // 未稳定时精度较低
    }
    
    float std_dev = sqrtf(g_phase_filter.phase_variance);
    return std_dev * 2.0f;  // 2σ置信区间
}

bool is_phase_measurement_valid(void)
{
    // 判断相位测量是否有效且精度满足要求
    float accuracy = get_phase_measurement_accuracy();
    return (accuracy <= 5.0f) && is_frequency_stable();
}

//==============================================================================
// 对外接口函数
//==============================================================================

// 获取高精度相位差测量结果
float get_precise_phase_difference(void)
{
    return calculate_precise_phase_difference();
}

// 获取测量精度指标
float get_phase_accuracy(void)
{
    return get_phase_measurement_accuracy();
}

// 检查测量是否稳定
bool is_phase_stable(void)
{
    return is_phase_measurement_valid();
}

// 重置相位测量系统
void reset_phase_measurement(void)
{
    phase_measurement_init();
}