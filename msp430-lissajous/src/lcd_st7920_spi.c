#include "lcd.h"
// 占位实现：后续按你的引脚在此补 SPI/时序
// ST7920 SPI 典型三线：SCLK, SID, RS(E)

void lcd_init(void){ /* TODO: GPIO/SPI init, ST7920 init sequence */ }
void lcd_clear(void){ /* TODO: write GDRAM clear */ }
void lcd_draw_pixel(uint8_t x, uint8_t y, uint8_t on){ (void)x; (void)y; (void)on; }
void lcd_draw_text(uint8_t x, uint8_t y, const char* s){ (void)x; (void)y; (void)s; }
