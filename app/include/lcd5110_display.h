#pragma once

int	 lcd5110_init(void);
void lcd5110_printf(const char *format, ...);
void lcd5110_putc(char c);
void lcd5110_setpos(uint32_t x, uint32_t y);
