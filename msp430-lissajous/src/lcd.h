#pragma once
#include <stdint.h>

// 通用 128x64 LCD 抽象：初始化/清屏/点/文本（占位）
void lcd_init(void);
void lcd_clear(void);
void lcd_draw_pixel(uint8_t x, uint8_t y, uint8_t on);
void lcd_draw_text(uint8_t x, uint8_t y, const char* s);
