#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <SDL2/SDL.h>

// External bitmap font functions
extern void draw_char_bitmap(uint8_t* framebuffer, int x, int y, char c, 
                             uint32_t color, int screen_width);
extern void draw_text_bitmap(uint8_t* framebuffer, int x, int y, const char* text, 
                             uint32_t color, int screen_width);

// External assembly drawing functions
extern void fill_rect_asm(uint8_t* dest, int x, int y, int width, int height, uint32_t color, int screen_width);
extern void draw_hline_asm(uint8_t* dest, int x, int y, int width, uint32_t color, int screen_width);
extern void draw_vline_asm(uint8_t* dest, int x, int y, int height, uint32_t color, int screen_width);

// External getters from simulation
extern int get_num_guests(void);
extern int get_park_rating(void);
extern int get_park_money(void);
extern int get_total_guests_entered(void);
extern float get_time_of_day(void);

// External weather functions
extern const char* get_weather_name(void);

// External ride functions
extern int get_num_rides(void);
extern const char* get_ride_name(int idx);
extern int get_ride_queue(int idx);

// External staff functions
extern int get_num_staff(void);

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
    WINDOW_RIDES
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
    
    // Title text using bitmap font
    draw_text_bitmap(g_framebuffer, win->x + 5, win->y + 6, win->title, 0xFFFFFF, SCREEN_WIDTH);
}

static void draw_stats_window(Window* win) {
    char buffer[64];
    
    int y = win->y + 30;
    
    // Time of day
    float time = get_time_of_day();
    int hour = (int)time;
    int minute = (int)((time - hour) * 60);
    const char* period = (hour >= 12) ? "PM" : "AM";
    int display_hour = hour % 12;
    if (display_hour == 0) display_hour = 12;
    
    snprintf(buffer, sizeof(buffer), "Time: %d:%02d %s", display_hour, minute, period);
    draw_text_bitmap(g_framebuffer, win->x + 10, y, buffer, 0x000080, SCREEN_WIDTH);
    y += 12;
    
    // Weather
    snprintf(buffer, sizeof(buffer), "Weather: %s", get_weather_name());
    draw_text_bitmap(g_framebuffer, win->x + 10, y, buffer, 0x006400, SCREEN_WIDTH);
    y += 12;
    
    snprintf(buffer, sizeof(buffer), "Guests: %d", get_num_guests());
    draw_text_bitmap(g_framebuffer, win->x + 10, y, buffer, 0x000000, SCREEN_WIDTH);
    y += 12;
    
    snprintf(buffer, sizeof(buffer), "Total: %d", get_total_guests_entered());
    draw_text_bitmap(g_framebuffer, win->x + 10, y, buffer, 0x000000, SCREEN_WIDTH);
    y += 12;
    
    snprintf(buffer, sizeof(buffer), "Rating: %d", get_park_rating());
    draw_text_bitmap(g_framebuffer, win->x + 10, y, buffer, 0x000000, SCREEN_WIDTH);
    y += 12;
    
    snprintf(buffer, sizeof(buffer), "Money: $%d", get_park_money());
    draw_text_bitmap(g_framebuffer, win->x + 10, y, buffer, 0x000000, SCREEN_WIDTH);
    y += 12;
    
    snprintf(buffer, sizeof(buffer), "Staff: %d", get_num_staff());
    draw_text_bitmap(g_framebuffer, win->x + 10, y, buffer, 0x000000, SCREEN_WIDTH);
    y += 12;
    
    snprintf(buffer, sizeof(buffer), "Rides: %d", get_num_rides());
    draw_text_bitmap(g_framebuffer, win->x + 10, y, buffer, 0x000000, SCREEN_WIDTH);
}

static void draw_build_window(Window* win) {
    const char* tools[] = {
        "[1] Raise Land",
        "[2] Lower Land", 
        "[3] Build Path",
        "[4] Demolish"
    };
    
    draw_text_bitmap(g_framebuffer, win->x + 10, win->y + 30, "Build Tools", 0x000000, SCREEN_WIDTH);
    
    for (int i = 0; i < 4; i++) {
        uint32_t color = (g_current_tool == i + 1) ? 0xFF0000 : 0x000000;
        draw_text_bitmap(g_framebuffer, win->x + 10, win->y + 50 + i * 15, tools[i], color, SCREEN_WIDTH);
    }
    
    // Show current tool
    const char* tool_names[] = {"None", "Raise", "Lower", "Path", "Demolish"};
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "Active: %s", tool_names[g_current_tool]);
    draw_text_bitmap(g_framebuffer, win->x + 10, win->y + 120, buffer, 0x0000AA, SCREEN_WIDTH);
}

static void draw_rides_window(Window* win) {
    draw_text_bitmap(g_framebuffer, win->x + 10, win->y + 30, "Rides", 0x000000, SCREEN_WIDTH);
    
    int num_rides = get_num_rides();
    for (int i = 0; i < num_rides && i < 5; i++) {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%s: %d", get_ride_name(i), get_ride_queue(i));
        draw_text_bitmap(g_framebuffer, win->x + 10, win->y + 50 + i * 12, buffer, 0x000000, SCREEN_WIDTH);
    }
}

void init_ui(void) {
    // Create stats window (taller for weather)
    g_windows[0].active = true;
    g_windows[0].type = WINDOW_STATS;
    g_windows[0].x = 10;
    g_windows[0].y = 10;
    g_windows[0].width = 200;
    g_windows[0].height = 150;
    strcpy(g_windows[0].title, "Park Stats");
    
    // Create build window
    g_windows[1].active = true;
    g_windows[1].type = WINDOW_BUILD;
    g_windows[1].x = 10;
    g_windows[1].y = 170;
    g_windows[1].width = 200;
    g_windows[1].height = 150;
    strcpy(g_windows[1].title, "Build");
    
    // Create rides window
    g_windows[2].active = true;
    g_windows[2].type = WINDOW_RIDES;
    g_windows[2].x = 220;
    g_windows[2].y = 10;
    g_windows[2].width = 200;
    g_windows[2].height = 120;
    strcpy(g_windows[2].title, "Rides");
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
            case WINDOW_RIDES:
                draw_rides_window(&g_windows[i]);
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