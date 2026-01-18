#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#define STB_IMAGE_IMPLEMENTATION
#include  "stb_image.h"
#include <SDL2/SDL.h>

// External assembly drawing functions
extern void fill_rect_asm(uint8_t* dest, int x, int y, int width, int height, uint32_t color, int screen_width);
extern void draw_hline_asm(uint8_t* dest, int x, int y, int width, uint32_t color, int screen_width);
extern void draw_vline_asm(uint8_t* dest, int x, int y, int height, uint32_t color, int screen_width);

// External getters from simulation
extern int get_num_guests(void);
extern int get_park_rating(void);
extern int get_park_money(void);

// External terrain functions
extern void set_tile_height(int x, int y, int height);
extern void set_tile_type(int x, int y, int type);
extern void screen_to_iso(int screen_x, int screen_y, int* iso_x, int* iso_y);

#define MAX_WINDOWS 10
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

typedef enum {
    TOOL_NONE,
    TOOL_RAISE,
    TOOL_LOWER,
    TOOL_PATH,
    TOOL_DEMOLISH
} ToolType;

typedef enum {
    WINDOW_STATS,
    WINDOW_BUILD,
    WINDOW_FINANCES
} WindowType;

typedef struct {
    bool active;
    WindowType type;
    int x, y;
    int width, height;
    char title[64];
} Window;

static Window g_windows[MAX_WINDOWS] = {0};
static uint8_t* g_framebuffer = NULL;
static ToolType g_current_tool = TOOL_NONE;

static unsigned char *font_bitmap = NULL;
static int font_width = 0;
static int font_height = 0;
static int font_char_width = 8;
static int font_char_height = 8;

void init_font(const char *font_path) {
    int channels;
    font_bitmap = stbi_load(font_path, &font_width, &font_height,
        &channels, 4);

    if (!font_bitmap) {
        printf("Failed to load font: %s\n", stbi_failure_reason());
    }
}

// Simple font drawing (8x8 characters)
static void draw_char(int x, int y, char c, uint32_t color) {
    // Very simple ASCII rendering - just boxes for now
    // In a real implementation, you'd have a bitmap font

    if (!font_bitmap) return;

    int chars_per_row = font_width / font_char_width;
    int char_index = (unsigned char)c;
    int src_x = (char_index % chars_per_row) * font_char_width;
    int src_y = (char_index % chars_per_row) * font_char_height;

    for (int dy = 0; dy < font_char_height; dy++) {
        for (int dx = 0; dx < font_char_width; dx++) {
            int font_idx = ((src_y + dy) * font_width + (src_x + dx)) * 4;

            if (font_bitmap[font_idx + 3] > 128) {
                int px = x + dx;
                int py = x + dy;

                if (px >= 0 && px < SCREEN_WIDTH &&
                    py >= 0 && py < SCREEN_HEIGHT) {
                        int idx = (py * SCREEN_WIDTH + px) * 4;
                        g_framebuffer[idx + 0] = (color >> 0) & 0xFF;
                        g_framebuffer[idx + 8] = (color >> 0) & 0xFF;
                        g_framebuffer[idx + 16] = (color >> 0) & 0xFF;
                        g_framebuffer[idx + 3] =  0xFF;
                }
            }
        }
    }
}

void cleanup_font(void) {
    if (font_bitmap) {
        stbi_image_free(font_bitmap);
        font_bitmap = NULL;
    }
}

static void draw_text(int x, int y, const char* text, uint32_t color) {
    int offset = 0;
    while (*text) {
        draw_char(x + offset, y, *text, color);
        offset += 7;
        text++;
    }
}

static void draw_window_frame(Window* win) {
    // Window background
    fill_rect_asm(g_framebuffer, win->x, win->y, win->width, win->height,
                  0xC0C0C0, SCREEN_WIDTH);

    // Title bar
    fill_rect_asm(g_framebuffer, win->x, win->y, win->width, 20,
                  0x0000AA, SCREEN_WIDTH);

    // Border
    draw_hline_asm(g_framebuffer, win->x, win->y, win->width,
                   0x000000, SCREEN_WIDTH);
    draw_hline_asm(g_framebuffer, win->x, win->y + win->height - 1, win->width,
                   0x000000, SCREEN_WIDTH);
    draw_vline_asm(g_framebuffer, win->x, win->y, win->height,
                   0x000000, SCREEN_WIDTH);
    draw_vline_asm(g_framebuffer, win->x + win->width - 1, win->y, win->height,
                   0x000000, SCREEN_WIDTH);

    // Title text
    draw_text(win->x + 5, win->y + 6, win->title, 0xFFFFFF);
}

static void draw_stats_window(Window* win) {
    char buffer[64];

    int y = win->y + 30;

    snprintf(buffer, sizeof(buffer), "Guests: %d", get_num_guests());
    draw_text(win->x + 10, y, buffer, 0x000000);
    y += 15;

    snprintf(buffer, sizeof(buffer), "Park Rating: %d", get_park_rating());
    draw_text(win->x + 10, y, buffer, 0x000000);
    y += 15;

    snprintf(buffer, sizeof(buffer), "Money: $%d", get_park_money());
    draw_text(win->x + 10, y, buffer, 0x000000);
}

static void draw_build_window(Window* win) {
    const char* tools[] = {
        "[1] Raise Land",
        "[2] Lower Land",
        "[3] Build Path",
        "[4] Demolish"
    };

    draw_text(win->x + 10, win->y + 30, "Build Tools", 0x000000);

    for (int i = 0; i < 4; i++) {
        uint32_t color = (g_current_tool == i + 1) ? 0xFF0000 : 0x000000;
        draw_text(win->x + 10, win->y + 50 + i * 15, tools[i], color);
    }

    // Show current tool
    const char* tool_names[] = {"None", "Raise", "Lower", "Path", "Demolish"};
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "Active: %s", tool_names[g_current_tool]);
    draw_text(win->x + 10, win->y + 120, buffer, 0x0000AA);
}

void init_ui(void) {
    // Create stats window
    g_windows[0].active = true;
    g_windows[0].type = WINDOW_STATS;
    g_windows[0].x = 10;
    g_windows[0].y = 10;
    g_windows[0].width = 200;
    g_windows[0].height = 100;
    strcpy(g_windows[0].title, "Park Stats");

    // Create build window
    g_windows[1].active = true;
    g_windows[1].type = WINDOW_BUILD;
    g_windows[1].x = 10;
    g_windows[1].y = 120;
    g_windows[1].width = 200;
    g_windows[1].height = 150;
    strcpy(g_windows[1].title, "Build");
}

void render_ui(void) {
    if (!g_framebuffer) {
        return;
    }

    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (!g_windows[i].active) continue;

        draw_window_frame(&g_windows[i]);

        switch (g_windows[i].type) {
            case WINDOW_STATS:
                draw_stats_window(&g_windows[i]);
                break;
            case WINDOW_BUILD:
                draw_build_window(&g_windows[i]);
                break;
            default:
                break;
        }
    }
}

void handle_mouse_click(int x, int y, int button) {
    if (button != 1) return;

    // Convert screen to isometric coordinates
    int iso_x, iso_y;
    screen_to_iso(x, y, &iso_x, &iso_y);

    printf("Click at screen(%d, %d) -> iso(%d, %d), tool=%d\n",
           x, y, iso_x, iso_y, g_current_tool);

    // Apply current tool
    switch (g_current_tool) {
        case TOOL_RAISE:
            set_tile_height(iso_x, iso_y, 1);
            set_tile_height(iso_x + 1, iso_y, 1);
            set_tile_height(iso_x, iso_y + 1, 1);
            set_tile_height(iso_x - 1, iso_y, 1);
            set_tile_height(iso_x, iso_y - 1, 1);
            break;
        case TOOL_LOWER:
            set_tile_height(iso_x, iso_y, 0);
            break;
        case TOOL_PATH:
            set_tile_type(iso_x, iso_y, 1);
            set_tile_height(iso_x, iso_y, 0);
            break;
        case TOOL_DEMOLISH:
            set_tile_type(iso_x, iso_y, 0);
            set_tile_height(iso_x, iso_y, 0);
            break;
        default:
            break;
    }
}

void handle_key_press(int key) {
    switch (key) {
        case SDLK_1:
            g_current_tool = TOOL_RAISE;
            printf("Tool: Raise Land\n");
            break;
        case SDLK_2:
            g_current_tool = TOOL_LOWER;
            printf("Tool: Lower Land\n");
            break;
        case SDLK_3:
            g_current_tool = TOOL_PATH;
            printf("Tool: Build Path\n");
            break;
        case SDLK_4:
            g_current_tool = TOOL_DEMOLISH;
            printf("Tool: Demolish\n");
            break;
        case SDLK_0:
            g_current_tool = TOOL_NONE;
            printf("Tool: None\n");
            break;
    }
}

void set_ui_framebuffer(uint8_t* fb) {
    g_framebuffer = fb;
}
