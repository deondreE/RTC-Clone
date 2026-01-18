#ifndef FONT_H
#define FONT_H

#include <stdint.h>

// Draw a single character using bitmap font
void draw_char_bitmap(uint8_t* framebuffer, int x, int y, char c, 
                      uint32_t color, int screen_width);

// Draw a text string using bitmap font
void draw_text_bitmap(uint8_t* framebuffer, int x, int y, const char* text, 
                      uint32_t color, int screen_width);

#endif // FONT_H