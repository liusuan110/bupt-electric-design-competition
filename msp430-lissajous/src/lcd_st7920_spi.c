// 基于 MSP430G2553，使用 GPIO 位带模拟 ST7920 SPI（串口模式），仅提供基本图形操作
#include "lcd.h"
#ifdef __MSP430__
#include <msp430.h>
#else
// 非 MSP430 环境：声明外部占位变量以通过静态检查
unsigned char P1DIR = 0, P1OUT = 0, P1SEL = 0;
#endif
#include "msp430g2553_pins.h"

// ST7920 SPI 帧：在串口模式时，RS/E 合并，使用 0xF8 作为命令头，随后高/低 4bit 各作为一个字节发送。
// 时序：SCLK 空闲低，SID 在上升沿采样。常用 2MHz 以下，以保证稳定。

static inline void delay_us(volatile unsigned int us){
	// 基于 1MHz DCO 近似延时（根据 MCLK 调整），此处为简化。
	while(us--) __delay_cycles(1);
}

static inline void st_clk_high() { ST7920_SCLK_OUT |= ST7920_SCLK_BIT; }
static inline void st_clk_low()  { ST7920_SCLK_OUT &= ~ST7920_SCLK_BIT; }
static inline void st_sid_high() { ST7920_SID_OUT  |= ST7920_SID_BIT; }
static inline void st_sid_low()  { ST7920_SID_OUT  &= ~ST7920_SID_BIT; }

static void st_gpio_init(void){
	ST7920_SCLK_DIR |= ST7920_SCLK_BIT;
	ST7920_SID_DIR  |= ST7920_SID_BIT;
	ST7920_CS_DIR   |= ST7920_CS_BIT;
	st_clk_low(); st_sid_low(); ST7920_CS_IDLE();
}

static void st_send_byte(uint8_t b){
	for (uint8_t i = 0; i < 8; ++i) {
		if (b & 0x80) st_sid_high(); else st_sid_low();
		st_clk_high();
		// 小延时保持
		__no_operation();
		st_clk_low();
		b <<= 1;
	}
}

static void st_write_cmd(uint8_t cmd){
	ST7920_CS_ACTIVE();
	st_send_byte(0xF8);          // 同步头（命令）
	st_send_byte(cmd & 0xF0);    // 高四位
	st_send_byte((cmd << 4) & 0xF0); // 低四位
	ST7920_CS_IDLE();
	delay_us(10);
}

static void st_write_data(uint8_t d){
	ST7920_CS_ACTIVE();
	st_send_byte(0xFA);          // 同步头（数据）
	st_send_byte(d & 0xF0);
	st_send_byte((d << 4) & 0xF0);
	ST7920_CS_IDLE();
}

static void st_set_gdram_addr(uint8_t x, uint8_t y){
	// 图形地址：y(0..63) 行，x(0..127) 列；ST7920 分左右两半（每半64列）。
	uint8_t row = y;
	uint8_t col = x;
	uint8_t y_addr = 0x80 | (row & 0x3F);
	uint8_t x_addr = 0x80 | (col & 0x3F);
	st_write_cmd(0x34); // 进入扩展指令（图形）
	st_write_cmd(y_addr);
	st_write_cmd((col < 64) ? 0x80 : 0x88); // 切左/右半区
	st_write_cmd(x_addr);
}

void lcd_init(void){
	st_gpio_init();
	delay_us(1200);
	st_write_cmd(0x30); // 基本指令集
	st_write_cmd(0x0C); // 显示开，关光标
	st_write_cmd(0x01); // 清屏
	delay_us(1200);
	st_write_cmd(0x34); // 进入图形指令
	st_write_cmd(0x36); // 打开图形显示
}

void lcd_clear(void){
	// 清 GDRAM：逐行写 0
	for (uint8_t y = 0; y < 64; ++y) {
		st_set_gdram_addr(0, y);
		for (uint8_t half = 0; half < 2; ++half) {
			for (uint8_t x = 0; x < 16; ++x) { // 每次写入1字节=8像素，64列/8=8字节；两半区各 8 字节
				st_write_data(0x00);
			}
		}
	}
}

void lcd_draw_pixel(uint8_t x, uint8_t y, uint8_t on){
	if (x > 127 || y > 63) return;
	// 读-改-写较复杂（需先读 GDRAM）。为简化：仅支持设置像素为 1（on!=0）而不读回；
	// 原则上应维护软件帧缓冲，再整字节下发，这里建议上层采用帧缓冲策略。
	if (!on) return; // 简化，不支持清零像素
	uint8_t byte_x = x >> 3;         // 每字节 8 像素
	uint8_t bit    = 7 - (x & 0x07); // 高位在左
	st_set_gdram_addr((byte_x << 3), y);
	// 直接写一个 bit 为 1 的字节（会覆盖同一字节其他像素）
	st_write_data((uint8_t)(1U << bit));
}

void lcd_draw_text(uint8_t x, uint8_t y, const char* s){
	// 占位：文本绘制依赖字库/点阵，建议使用软件帧缓冲+字模表实现
	(void)x; (void)y; (void)s;
}
