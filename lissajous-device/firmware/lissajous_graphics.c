//******************************************************************************
// 李萨如图形演示装置 - 李萨如图形绘制与显示算法
// 功能：在128×64 LCD上实时绘制李萨如图形
//******************************************************************************

#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

//==============================================================================
// 图形绘制相关常量
//==============================================================================

#define GRAPH_WIDTH         96      // 图形显示区域宽度
#define GRAPH_HEIGHT        48      // 图形显示区域高度
#define GRAPH_OFFSET_X      8       // 图形X偏移
#define GRAPH_OFFSET_Y      8       // 图形Y偏移

#define INFO_AREA_X         104     // 信息显示区域X起始
#define INFO_AREA_Y         0       // 信息显示区域Y起始
#define INFO_AREA_WIDTH     24      // 信息显示区域宽度
#define INFO_AREA_HEIGHT    64      // 信息显示区域高度

#define SAMPLE_POINTS       256     // 李萨如图形采样点数
#define REFRESH_RATE        15      // 刷新率 (Hz)
#define TRACE_FADE_STEPS    4       // 轨迹淡化级数

#define PI                  3.14159265359f
#define TWO_PI              6.28318530718f

//==============================================================================
// 数据结构定义
//==============================================================================

typedef struct {
    int16_t x;
    int16_t y;
    uint8_t intensity;              // 点的亮度 (0-3)
    bool valid;
} lissajous_point_t;

typedef struct {
    lissajous_point_t points[SAMPLE_POINTS];
    uint16_t point_count;           // 有效点数
    uint16_t current_index;         // 当前绘制索引
    float t_step;                   // 时间步长
    float current_t;                // 当前时间参数
} lissajous_curve_t;

typedef struct {
    float amplitude_x;              // X轴幅度
    float amplitude_y;              // Y轴幅度
    float frequency_x;              // X轴频率
    float frequency_y;              // Y轴频率
    float phase_diff;               // 相位差 (弧度)
    uint8_t freq_ratio_num;         // 频率比分子
    uint8_t freq_ratio_den;         // 频率比分母
} lissajous_params_t;

typedef struct {
    uint8_t grid_spacing;           // 网格间距
    bool show_grid;                 // 显示网格
    bool show_axes;                 // 显示坐标轴
    bool show_trace;                // 显示轨迹
    uint8_t trace_length;           // 轨迹长度
} display_options_t;

//==============================================================================
// 全局变量
//==============================================================================

static lissajous_curve_t g_curve = {0};
static lissajous_params_t g_liss_params = {
    .amplitude_x = 40.0f,
    .amplitude_y = 40.0f,
    .frequency_x = 1.0f,
    .frequency_y = 1.0f,
    .phase_diff = 0.0f,
    .freq_ratio_num = 1,
    .freq_ratio_den = 1
};

static display_options_t g_display_opts = {
    .grid_spacing = 8,
    .show_grid = true,
    .show_axes = true,
    .show_trace = true,
    .trace_length = 64
};

// 数学函数查找表 (优化性能)
static const int16_t sine_table[360] = {
    // 预计算的正弦值表 (0-359度, 放大1000倍)
    0, 17, 35, 52, 70, 87, 105, 122, 139, 156, 174, 191, 208, 225, 242, 259,
    276, 292, 309, 326, 342, 358, 375, 391, 407, 423, 438, 454, 469, 485, 500, 515,
    530, 545, 559, 574, 588, 602, 616, 629, 643, 656, 669, 682, 695, 707, 719, 731,
    743, 755, 766, 777, 788, 799, 809, 819, 829, 839, 848, 857, 866, 875, 883, 891,
    899, 906, 914, 921, 927, 934, 940, 946, 951, 956, 961, 966, 970, 974, 978, 982,
    985, 988, 990, 993, 995, 996, 998, 999, 999, 1000, 1000, 1000, 999, 999, 998, 996,
    995, 993, 990, 988, 985, 982, 978, 974, 970, 966, 961, 956, 951, 946, 940, 934,
    927, 921, 914, 906, 899, 891, 883, 875, 866, 857, 848, 839, 829, 819, 809, 799,
    788, 777, 766, 755, 743, 731, 719, 707, 695, 682, 669, 656, 643, 629, 616, 602,
    588, 574, 559, 545, 530, 515, 500, 485, 469, 454, 438, 423, 407, 391, 375, 358,
    342, 326, 309, 292, 276, 259, 242, 225, 208, 191, 174, 156, 139, 122, 105, 87,
    70, 52, 35, 17, 0, -17, -35, -52, -70, -87, -105, -122, -139, -156, -174, -191,
    -208, -225, -242, -259, -276, -292, -309, -326, -342, -358, -375, -391, -407, -423, -438, -454,
    -469, -485, -500, -515, -530, -545, -559, -574, -588, -602, -616, -629, -643, -656, -669, -682,
    -695, -707, -719, -731, -743, -755, -766, -777, -788, -799, -809, -819, -829, -839, -848, -857,
    -866, -875, -883, -891, -899, -906, -914, -921, -927, -934, -940, -946, -951, -956, -961, -966,
    -970, -974, -978, -982, -985, -988, -990, -993, -995, -996, -998, -999, -999, -1000, -1000, -1000,
    -999, -999, -998, -996, -995, -993, -990, -988, -985, -982, -978, -974, -970, -966, -961, -956,
    -951, -946, -940, -934, -927, -921, -914, -906, -899, -891, -883, -875, -866, -857, -848, -839,
    -829, -819, -809, -799, -788, -777, -766, -755, -743, -731, -719, -707, -695, -682, -669, -656,
    -643, -629, -616, -602, -588, -574, -559, -545, -530, -515, -500, -485, -469, -454, -438, -423,
    -407, -391, -375, -358, -342, -326, -309, -292, -276, -259, -242, -225, -208, -191, -174, -156,
    -139, -122, -105, -87, -70, -52, -35, -17
};

//==============================================================================
// 函数声明
//==============================================================================

// 图形绘制初始化
void lissajous_graphics_init(void);

// 李萨如参数设置
void set_lissajous_parameters(float freq_mult, uint8_t amplitude, float phase_deg);
void update_frequency_ratio(uint8_t numerator, uint8_t denominator);

// 李萨如曲线计算
void calculate_lissajous_curve(void);
void calculate_parametric_point(float t, int16_t* x, int16_t* y);

// 图形绘制
void draw_lissajous_curve(void);
void draw_grid_and_axes(void);
void draw_parameter_info(void);
void draw_trace_with_fade(void);

// 显示控制
void clear_graph_area(void);
void update_display(void);
void set_display_options(bool grid, bool axes, bool trace);

// 优化函数
int16_t fast_sin(uint16_t angle_deg);
int16_t fast_cos(uint16_t angle_deg);
void optimize_curve_calculation(void);

// 字符显示
void draw_char(uint8_t x, uint8_t y, char c);
void draw_string(uint8_t x, uint8_t y, const char* str);
void draw_number(uint8_t x, uint8_t y, int16_t number);

//==============================================================================
// 李萨如图形绘制初始化
//==============================================================================

void lissajous_graphics_init(void)
{
    // 初始化曲线结构
    g_curve.point_count = 0;
    g_curve.current_index = 0;
    g_curve.t_step = TWO_PI / SAMPLE_POINTS;
    g_curve.current_t = 0.0f;
    
    // 清除所有点
    for (uint16_t i = 0; i < SAMPLE_POINTS; i++) {
        g_curve.points[i].valid = false;
        g_curve.points[i].intensity = 0;
    }
    
    // 设置默认显示选项
    set_display_options(true, true, true);
    
    // 计算初始曲线
    calculate_lissajous_curve();
}

//==============================================================================
// 李萨如参数设置
//==============================================================================

void set_lissajous_parameters(float freq_mult, uint8_t amplitude, float phase_deg)
{
    // 更新频率参数
    g_liss_params.frequency_y = g_liss_params.frequency_x * freq_mult;
    
    // 更新幅度参数 (根据amplitude等级设置)
    switch (amplitude) {
        case 0: // 1V
            g_liss_params.amplitude_y = 20.0f;
            break;
        case 1: // 2V
            g_liss_params.amplitude_y = 35.0f;
            break;
        case 2: // 3V
            g_liss_params.amplitude_y = 45.0f;
            break;
        default:
            g_liss_params.amplitude_y = 35.0f;
            break;
    }
    
    // 更新相位差 (转换为弧度)
    g_liss_params.phase_diff = phase_deg * PI / 180.0f;
    
    // 更新频率比
    update_frequency_ratio((uint8_t)freq_mult, 1);
    
    // 重新计算曲线
    calculate_lissajous_curve();
}

void update_frequency_ratio(uint8_t numerator, uint8_t denominator)
{
    g_liss_params.freq_ratio_num = numerator;
    g_liss_params.freq_ratio_den = denominator;
    
    // 为了得到闭合曲线，需要计算最小公倍数来确定周期
    uint16_t lcm = numerator * denominator;  // 简化计算
    g_curve.point_count = (lcm > SAMPLE_POINTS) ? SAMPLE_POINTS : lcm;
    g_curve.t_step = TWO_PI * denominator / g_curve.point_count;
}

//==============================================================================
// 李萨如曲线计算
//==============================================================================

void calculate_lissajous_curve(void)
{
    float t = 0.0f;
    
    for (uint16_t i = 0; i < g_curve.point_count; i++) {
        int16_t x, y;
        calculate_parametric_point(t, &x, &y);
        
        // 检查点是否在显示区域内
        if (x >= 0 && x < GRAPH_WIDTH && y >= 0 && y < GRAPH_HEIGHT) {
            g_curve.points[i].x = x + GRAPH_OFFSET_X;
            g_curve.points[i].y = y + GRAPH_OFFSET_Y;
            g_curve.points[i].valid = true;
            g_curve.points[i].intensity = 3;  // 最高亮度
        } else {
            g_curve.points[i].valid = false;
        }
        
        t += g_curve.t_step;
    }
    
    optimize_curve_calculation();
}

void calculate_parametric_point(float t, int16_t* x, int16_t* y)
{
    // 李萨如曲线参数方程：
    // x(t) = A_x * sin(f_x * t + φ_x)
    // y(t) = A_y * sin(f_y * t + φ_y)
    
    float angle_x = g_liss_params.frequency_x * t;
    float angle_y = g_liss_params.frequency_y * t + g_liss_params.phase_diff;
    
    // 转换为度数用于查表
    uint16_t deg_x = (uint16_t)(angle_x * 180.0f / PI) % 360;
    uint16_t deg_y = (uint16_t)(angle_y * 180.0f / PI) % 360;
    
    // 使用查找表计算
    int16_t sin_x = fast_sin(deg_x);
    int16_t sin_y = fast_sin(deg_y);
    
    // 计算坐标 (中心为图形区域中心)
    *x = (int16_t)(GRAPH_WIDTH / 2 + (g_liss_params.amplitude_x * sin_x) / 1000);
    *y = (int16_t)(GRAPH_HEIGHT / 2 + (g_liss_params.amplitude_y * sin_y) / 1000);
}

//==============================================================================
// 图形绘制函数
//==============================================================================

void draw_lissajous_curve(void)
{
    // 清除图形区域
    clear_graph_area();
    
    // 绘制网格和坐标轴
    if (g_display_opts.show_grid || g_display_opts.show_axes) {
        draw_grid_and_axes();
    }
    
    // 绘制李萨如曲线
    if (g_display_opts.show_trace) {
        draw_trace_with_fade();
    } else {
        // 绘制实时点
        for (uint16_t i = 0; i < g_curve.point_count; i++) {
            if (g_curve.points[i].valid) {
                lcd_draw_pixel(g_curve.points[i].x, g_curve.points[i].y, 1);
            }
        }
    }
    
    // 绘制参数信息
    draw_parameter_info();
}

void draw_grid_and_axes(void)
{
    if (g_display_opts.show_grid) {
        // 绘制网格线
        for (uint8_t x = GRAPH_OFFSET_X; x < GRAPH_OFFSET_X + GRAPH_WIDTH; x += g_display_opts.grid_spacing) {
            for (uint8_t y = GRAPH_OFFSET_Y; y < GRAPH_OFFSET_Y + GRAPH_HEIGHT; y += 2) {
                lcd_draw_pixel(x, y, 1);
            }
        }
        
        for (uint8_t y = GRAPH_OFFSET_Y; y < GRAPH_OFFSET_Y + GRAPH_HEIGHT; y += g_display_opts.grid_spacing) {
            for (uint8_t x = GRAPH_OFFSET_X; x < GRAPH_OFFSET_X + GRAPH_WIDTH; x += 2) {
                lcd_draw_pixel(x, y, 1);
            }
        }
    }
    
    if (g_display_opts.show_axes) {
        // 绘制X轴
        uint8_t center_y = GRAPH_OFFSET_Y + GRAPH_HEIGHT / 2;
        for (uint8_t x = GRAPH_OFFSET_X; x < GRAPH_OFFSET_X + GRAPH_WIDTH; x++) {
            lcd_draw_pixel(x, center_y, 1);
        }
        
        // 绘制Y轴
        uint8_t center_x = GRAPH_OFFSET_X + GRAPH_WIDTH / 2;
        for (uint8_t y = GRAPH_OFFSET_Y; y < GRAPH_OFFSET_Y + GRAPH_HEIGHT; y++) {
            lcd_draw_pixel(center_x, y, 1);
        }
    }
}

void draw_trace_with_fade(void)
{
    // 动态轨迹绘制 - 显示曲线的形成过程
    static uint16_t trace_index = 0;
    static uint8_t fade_counter = 0;
    
    fade_counter++;
    if (fade_counter >= TRACE_FADE_STEPS) {
        fade_counter = 0;
        
        // 淡化旧的轨迹点
        for (uint16_t i = 0; i < g_curve.point_count; i++) {
            if (g_curve.points[i].intensity > 0) {
                g_curve.points[i].intensity--;
            }
        }
        
        // 添加新的轨迹点
        if (trace_index < g_curve.point_count) {
            g_curve.points[trace_index].intensity = 3;
            trace_index++;
        } else {
            trace_index = 0;  // 重新开始
        }
    }
    
    // 绘制所有有效的轨迹点
    for (uint16_t i = 0; i < g_curve.point_count; i++) {
        if (g_curve.points[i].valid && g_curve.points[i].intensity > 0) {
            // 根据亮度绘制不同强度的点
            if (g_curve.points[i].intensity >= 2) {
                lcd_draw_pixel(g_curve.points[i].x, g_curve.points[i].y, 1);
            } else {
                // 较暗的点 - 间隔绘制
                if ((i % 2) == 0) {
                    lcd_draw_pixel(g_curve.points[i].x, g_curve.points[i].y, 1);
                }
            }
        }
    }
}

void draw_parameter_info(void)
{
    // 在右侧信息区域显示参数
    uint8_t info_x = INFO_AREA_X;
    uint8_t info_y = INFO_AREA_Y + 8;
    
    // 显示频率比
    draw_string(info_x, info_y, "Fx:Fy");
    info_y += 8;
    draw_number(info_x, info_y, g_liss_params.freq_ratio_num);
    draw_char(info_x + 8, info_y, ':');
    draw_number(info_x + 12, info_y, g_liss_params.freq_ratio_den);
    
    // 显示相位差
    info_y += 12;
    draw_string(info_x, info_y, "Phase");
    info_y += 8;
    int16_t phase_deg = (int16_t)(g_liss_params.phase_diff * 180.0f / PI);
    draw_number(info_x, info_y, phase_deg);
    draw_char(info_x + 12, info_y, 'D');
    
    // 显示幅度信息
    info_y += 12;
    draw_string(info_x, info_y, "Amp");
    info_y += 8;
    uint8_t amp_level = 0;
    if (g_liss_params.amplitude_y > 30) amp_level = (g_liss_params.amplitude_y > 40) ? 3 : 2;
    else amp_level = 1;
    draw_number(info_x, info_y, amp_level);
    draw_char(info_x + 8, info_y, 'V');
}

//==============================================================================
// 显示控制函数
//==============================================================================

void clear_graph_area(void)
{
    // 清除图形显示区域
    for (uint8_t x = GRAPH_OFFSET_X; x < GRAPH_OFFSET_X + GRAPH_WIDTH; x++) {
        for (uint8_t y = GRAPH_OFFSET_Y; y < GRAPH_OFFSET_Y + GRAPH_HEIGHT; y++) {
            lcd_draw_pixel(x, y, 0);
        }
    }
}

void update_display(void)
{
    // 重新计算和绘制李萨如曲线
    calculate_lissajous_curve();
    draw_lissajous_curve();
    lcd_update_display();
}

void set_display_options(bool grid, bool axes, bool trace)
{
    g_display_opts.show_grid = grid;
    g_display_opts.show_axes = axes;
    g_display_opts.show_trace = trace;
}

//==============================================================================
// 优化函数
//==============================================================================

int16_t fast_sin(uint16_t angle_deg)
{
    return sine_table[angle_deg % 360];
}

int16_t fast_cos(uint16_t angle_deg)
{
    return sine_table[(angle_deg + 90) % 360];
}

void optimize_curve_calculation(void)
{
    // 动态调整采样点数以优化性能
    if (g_liss_params.freq_ratio_num == 1 && g_liss_params.freq_ratio_den == 1) {
        // 1:1频率比，可以使用较少的采样点
        g_curve.point_count = 64;
    } else if (g_liss_params.freq_ratio_num <= 3 && g_liss_params.freq_ratio_den <= 3) {
        // 简单频率比
        g_curve.point_count = 128;
    } else {
        // 复杂频率比，需要更多采样点
        g_curve.point_count = SAMPLE_POINTS;
    }
    
    g_curve.t_step = TWO_PI / g_curve.point_count;
}

//==============================================================================
// 字符显示函数 (简化版)
//==============================================================================

// 5x7点阵字体数据 (部分字符)
static const uint8_t font_5x7[][5] = {
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // '0'
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // '1'
    {0x42, 0x61, 0x51, 0x49, 0x46}, // '2'
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // '3'
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // '4'
    {0x27, 0x45, 0x45, 0x45, 0x39}, // '5'
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // '6'
    {0x01, 0x71, 0x09, 0x05, 0x03}, // '7'
    {0x36, 0x49, 0x49, 0x49, 0x36}, // '8'
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // '9'
    {0x00, 0x36, 0x36, 0x00, 0x00}, // ':'
    {0x7C, 0x12, 0x11, 0x12, 0x7C}, // 'A'
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // 'B'
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // 'C'
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // 'D'
    // 更多字符...
};

void draw_char(uint8_t x, uint8_t y, char c)
{
    uint8_t char_index;
    
    if (c >= '0' && c <= '9') {
        char_index = c - '0';
    } else if (c >= 'A' && c <= 'Z') {
        char_index = c - 'A' + 11;
    } else {
        return;  // 不支持的字符
    }
    
    // 绘制5x7字符
    for (uint8_t col = 0; col < 5; col++) {
        uint8_t column_data = font_5x7[char_index][col];
        for (uint8_t row = 0; row < 7; row++) {
            if (column_data & (1 << row)) {
                lcd_draw_pixel(x + col, y + row, 1);
            }
        }
    }
}

void draw_string(uint8_t x, uint8_t y, const char* str)
{
    uint8_t pos_x = x;
    while (*str) {
        draw_char(pos_x, y, *str);
        pos_x += 6;  // 字符间距
        str++;
    }
}

void draw_number(uint8_t x, uint8_t y, int16_t number)
{
    char str[8];
    uint8_t i = 0;
    
    if (number < 0) {
        str[i++] = '-';
        number = -number;
    }
    
    // 简单的整数转字符串
    if (number == 0) {
        str[i++] = '0';
    } else {
        uint8_t temp_i = i;
        while (number > 0) {
            str[i++] = '0' + (number % 10);
            number /= 10;
        }
        
        // 反转数字部分
        for (uint8_t j = temp_i; j < (i + temp_i) / 2; j++) {
            char temp = str[j];
            str[j] = str[i - 1 - (j - temp_i)];
            str[i - 1 - (j - temp_i)] = temp;
        }
    }
    
    str[i] = '\0';
    draw_string(x, y, str);
}

//==============================================================================
// 对外接口函数
//==============================================================================

// 更新李萨如参数并重绘
void update_lissajous_display(float freq_mult, uint8_t amplitude, float phase_deg)
{
    set_lissajous_parameters(freq_mult, amplitude, phase_deg);
    update_display();
}

// 切换显示模式
void toggle_display_mode(void)
{
    static uint8_t mode = 0;
    mode = (mode + 1) % 4;
    
    switch (mode) {
        case 0: set_display_options(true, true, true); break;   // 全显示
        case 1: set_display_options(false, true, true); break; // 无网格
        case 2: set_display_options(false, false, true); break; // 仅轨迹
        case 3: set_display_options(true, true, false); break; // 无淡化
    }
}

// 获取当前李萨如参数
lissajous_params_t* get_lissajous_params(void)
{
    return &g_liss_params;
}