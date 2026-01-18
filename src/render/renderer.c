#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

// External assembly functions
extern void draw_iso_tile_asm(uint8_t* dest, int x, int y, uint32_t color, int screen_width);
extern void fill_rect_asm(uint8_t* dest, int x, int y, int width, int height, uint32_t color, int screen_width);

// External getters from simulation
extern int get_num_guests(void);
extern void get_guest_position(int index, float* x, float* y);
extern uint32_t get_guest_color(int index);

// External ride functions
extern int get_num_rides(void);
extern void get_ride_info(int idx, int* x, int* y, int* width, int* height, int* status);

// External staff functions
extern int get_num_staff(void);
extern void get_staff_position(int index, float* x, float* y);
extern int get_staff_type(int index);

// External scenery functions
extern int get_num_scenery(void);
extern void get_scenery_info(int idx, int* x, int* y, int* type, uint32_t* color);

// External shop functions
extern int get_num_shops(void);
extern void get_shop_info(int idx, int* x, int* y, int* type);

// External litter functions
extern int get_num_litter(void);
extern void get_litter_position(int idx, float* x, float* y);

// External lighting functions
extern void update_lighting(float time_of_day);
extern uint32_t apply_lighting(uint32_t color, float additional_brightness);
extern uint32_t get_sky_color(int screen_y, int screen_height);
extern bool are_lamps_on(void);
extern uint32_t get_lamp_glow_color(void);
extern uint32_t get_ride_lights(int ride_index);

// External time function
extern float get_time_of_day(void);

// Forward declare UI framebuffer setter
extern void set_ui_framebuffer(uint8_t* fb);

#define MAP_SIZE 32
#define TILE_WIDTH 64
#define TILE_HEIGHT 32

typedef struct {
    uint8_t* framebuffer;
    int screen_width;
    int screen_height;
    int camera_x;
    int camera_y;
} Renderer;

typedef struct {
    uint8_t height;
    uint8_t type;  // 0=grass, 1=path, 2=ride
} Tile;

static Renderer g_renderer = {0};
static Tile g_map[MAP_SIZE][MAP_SIZE] = {0};

void init_renderer(uint8_t* framebuffer, int width, int height) {
    printf("init_renderer: framebuffer=%p, width=%d, height=%d\n",
           (void*)framebuffer, width, height);

    g_renderer.framebuffer = framebuffer;
    g_renderer.screen_width = width;
    g_renderer.screen_height = height;
    g_renderer.camera_x = 0;
    g_renderer.camera_y = 0;

    // Set framebuffer for UI system as well
    set_ui_framebuffer(framebuffer);

    // Initialize a more interesting test map
    for (int y = 0; y < MAP_SIZE; y++) {
        for (int x = 0; x < MAP_SIZE; x++) {
            g_map[y][x].height = 0;
            g_map[y][x].type = 0;

            // Create a winding path
            if ((x == 5 && y >= 5 && y <= 25) ||
                (y == 25 && x >= 5 && x <= 15) ||
                (x == 15 && y >= 10 && y <= 25) ||
                (y == 10 && x >= 15 && x <= 25)) {
                g_map[y][x].type = 1; // path
            }

            // Create some hills
            int dx = x - 20;
            int dy = y - 20;
            float dist = sqrtf(dx*dx + dy*dy);
            if (dist < 5) {
                g_map[y][x].height = (int)(3 - dist * 0.6f);
            }

            // Create a flat area for rides
            if (x >= 8 && x <= 12 && y >= 8 && y <= 12) {
                g_map[y][x].height = 0;
                if (x == 10 && y == 10) {
                    g_map[y][x].type = 2; // ride
                }
            }
        }
    }
}

// Convert isometric coordinates to screen coordinates
void iso_to_screen(int iso_x, int iso_y, int* screen_x, int* screen_y) {
    *screen_x = (iso_x - iso_y) * (TILE_WIDTH / 2) + g_renderer.screen_width / 2 - g_renderer.camera_x;
    *screen_y = (iso_x + iso_y) * (TILE_HEIGHT / 2) + 100 - g_renderer.camera_y;
}

// Convert screen coordinates to isometric tile coordinates
void screen_to_iso(int screen_x, int screen_y, int* iso_x, int* iso_y) {
    // Adjust for camera
    screen_x += g_renderer.camera_x - g_renderer.screen_width / 2;
    screen_y += g_renderer.camera_y - 100;

    // Convert from screen to isometric
    float fx = screen_x / (float)(TILE_WIDTH / 2);
    float fy = screen_y / (float)(TILE_HEIGHT / 2);

    *iso_x = (int)((fx + fy) / 2.0f);
    *iso_y = (int)((fy - fx) / 2.0f);
}

void render_frame(void) {
    if (!g_renderer.framebuffer) {
        printf("ERROR: framebuffer is NULL in render_frame!\n");
        return;
    }

    // Update lighting for current time
    float time_of_day = get_time_of_day();
    update_lighting(time_of_day);

    // Draw sky gradient first
    for (int y = 0; y < g_renderer.screen_height; y++) {
        uint32_t sky_color = get_sky_color(y, g_renderer.screen_height);
        for (int x = 0; x < g_renderer.screen_width; x++) {
            int idx = (y * g_renderer.screen_width + x) * 4;
            g_renderer.framebuffer[idx + 0] = (sky_color >> 0) & 0xFF;  // B
            g_renderer.framebuffer[idx + 1] = (sky_color >> 8) & 0xFF;  // G
            g_renderer.framebuffer[idx + 2] = (sky_color >> 16) & 0xFF; // R
            g_renderer.framebuffer[idx + 3] = 0xFF;                      // A
        }
    }

    // Render tiles in proper isometric order (back to front)
    for (int map_y = 0; map_y < MAP_SIZE; map_y++) {
        for (int map_x = 0; map_x < MAP_SIZE; map_x++) {
            int screen_x, screen_y;
            iso_to_screen(map_x, map_y, &screen_x, &screen_y);

            // Adjust for tile height
            screen_y -= g_map[map_y][map_x].height * 8;

            // Choose color based on tile type
            uint32_t color;
            switch (g_map[map_y][map_x].type) {
                case 0: color = 0x228B22; break; // Grass (green)
                case 1: color = 0x808080; break; // Path (gray)
                case 2: color = 0xFF6347; break; // Ride (tomato)
                default: color = 0x228B22; break;
            }

            // Darken based on height for depth effect
            int brightness = 255 - (g_map[map_y][map_x].height * 20);
            if (brightness < 100) brightness = 100;

            uint8_t r = ((color >> 16) & 0xFF) * brightness / 255;
            uint8_t g = ((color >> 8) & 0xFF) * brightness / 255;
            uint8_t b = ((color >> 0) & 0xFF) * brightness / 255;
            color = (r << 16) | (g << 8) | b;

            // Apply lighting
            color = apply_lighting(color, 0.0f);

            // Use assembly optimized tile drawing
            draw_iso_tile_asm(g_renderer.framebuffer, screen_x, screen_y,
                            color, g_renderer.screen_width);
        }
    }

    // Render rides
    int num_rides = get_num_rides();
    for (int i = 0; i < num_rides; i++) {
        int rx, ry, rw, rh, rs;
        get_ride_info(i, &rx, &ry, &rw, &rh, &rs);

        // Draw ride footprint
        for (int dy = 0; dy < rh; dy++) {
            for (int dx = 0; dx < rw; dx++) {
                int screen_x, screen_y;
                iso_to_screen(rx + dx, ry + dy, &screen_x, &screen_y);

                uint32_t ride_color = (rs == 3) ? 0x800000 : 0xFF6347;

                // Apply lighting
                ride_color = apply_lighting(ride_color, 0.0f);

                // Add ride lights at night
                if (are_lamps_on() && rs != 3) {
                    uint32_t light_color = get_ride_lights(i);
                    // Mix ride color with light color
                    uint8_t r = ((ride_color >> 16) & 0xFF) * 0.6f + ((light_color >> 16) & 0xFF) * 0.4f;
                    uint8_t g = ((ride_color >> 8) & 0xFF) * 0.6f + ((light_color >> 8) & 0xFF) * 0.4f;
                    uint8_t b = ((ride_color >> 0) & 0xFF) * 0.6f + ((light_color >> 0) & 0xFF) * 0.4f;
                    ride_color = (r << 16) | (g << 8) | b;
                }

                draw_iso_tile_asm(g_renderer.framebuffer, screen_x, screen_y,
                                ride_color, g_renderer.screen_width);
            }
        }
    }

    // Render shops
    int num_shops = get_num_shops();
    for (int i = 0; i < num_shops; i++) {
        int sx, sy, st;
        get_shop_info(i, &sx, &sy, &st);

        int screen_x, screen_y;
        iso_to_screen(sx, sy, &screen_x, &screen_y);

        // Color based on shop type
        uint32_t shop_color;
        switch (st) {
            case 0: shop_color = 0xFF8C00; break;  // Food - orange
            case 1: shop_color = 0x1E90FF; break;  // Drink - blue
            case 2: shop_color = 0xFFFFFF; break;  // Bathroom - white
            case 3: shop_color = 0xFF1493; break;  // Gift - pink
            default: shop_color = 0xFFFFFF; break;
        }

        // Apply lighting
        shop_color = apply_lighting(shop_color, 0.0f);

        draw_iso_tile_asm(g_renderer.framebuffer, screen_x, screen_y,
                        shop_color, g_renderer.screen_width);
    }

    // Render scenery
    int num_scenery = get_num_scenery();
    for (int i = 0; i < num_scenery; i++) {
        int scx, scy, sct;
        uint32_t sc_color;
        get_scenery_info(i, &scx, &scy, &sct, &sc_color);

        int screen_x, screen_y;
        iso_to_screen(scx, scy, &screen_x, &screen_y);

        // Draw scenery based on type
        if (sct == 0 || sct == 1) {  // Trees
            // Draw tree trunk
            uint32_t trunk_color = apply_lighting(0x8B4513, 0.0f);
            fill_rect_asm(g_renderer.framebuffer, screen_x - 2, screen_y - 6,
                         4, 10, trunk_color, g_renderer.screen_width);
            // Draw tree canopy
            uint32_t canopy_color = apply_lighting(sc_color, 0.0f);
            fill_rect_asm(g_renderer.framebuffer, screen_x - 6, screen_y - 16,
                         12, 12, canopy_color, g_renderer.screen_width);
        } else if (sct == 2) {  // Bench
            uint32_t bench_color = apply_lighting(sc_color, 0.0f);
            fill_rect_asm(g_renderer.framebuffer, screen_x - 4, screen_y - 4,
                         8, 4, bench_color, g_renderer.screen_width);
        } else if (sct == 3) {  // Lamp
            uint32_t pole_color = apply_lighting(0x808080, 0.0f);
            fill_rect_asm(g_renderer.framebuffer, screen_x - 1, screen_y - 12,
                         2, 12, pole_color, g_renderer.screen_width);

            // Lamp glows at night
            uint32_t lamp_color = are_lamps_on() ? get_lamp_glow_color() : apply_lighting(sc_color, 0.0f);
            fill_rect_asm(g_renderer.framebuffer, screen_x - 3, screen_y - 14,
                         6, 4, lamp_color, g_renderer.screen_width);

            // Add glow effect at night
            if (are_lamps_on()) {
                uint32_t glow = 0xFFFFAA;
                fill_rect_asm(g_renderer.framebuffer, screen_x - 5, screen_y - 16,
                             10, 8, glow, g_renderer.screen_width);
            }
        } else {  // Other scenery
            uint32_t scenery_color = apply_lighting(sc_color, 0.0f);
            fill_rect_asm(g_renderer.framebuffer, screen_x - 3, screen_y - 6,
                         6, 6, scenery_color, g_renderer.screen_width);
        }
    }

    // Render litter
    int num_litter = get_num_litter();
    for (int i = 0; i < num_litter; i++) {
        float litter_x, litter_y;
        get_litter_position(i, &litter_x, &litter_y);

        int screen_x, screen_y;
        iso_to_screen((int)litter_x, (int)litter_y, &screen_x, &screen_y);

        // Draw small red square for litter
        fill_rect_asm(g_renderer.framebuffer, screen_x - 2, screen_y - 2,
                     4, 4, 0xFF0000, g_renderer.screen_width);
    }

    // Render guests on top of tiles
    int num_guests = get_num_guests();
    for (int i = 0; i < num_guests; i++) {
        float guest_x, guest_y;
        get_guest_position(i, &guest_x, &guest_y);

        int screen_x, screen_y;
        iso_to_screen((int)guest_x, (int)guest_y, &screen_x, &screen_y);

        // Adjust for tile height at guest position
        int tile_x = (int)guest_x;
        int tile_y = (int)guest_y;
        if (tile_x >= 0 && tile_x < MAP_SIZE && tile_y >= 0 && tile_y < MAP_SIZE) {
            screen_y -= g_map[tile_y][tile_x].height * 8;
        }

        // Draw guest with their unique color
        uint32_t guest_color = get_guest_color(i);

        // Draw a simple "person" shape (head + body)
        fill_rect_asm(g_renderer.framebuffer, screen_x - 3, screen_y - 10,
                     6, 4, guest_color, g_renderer.screen_width); // Head
        fill_rect_asm(g_renderer.framebuffer, screen_x - 4, screen_y - 6,
                     8, 6, guest_color, g_renderer.screen_width); // Body
    }

    // Render staff
    int num_staff = get_num_staff();
    for (int i = 0; i < num_staff; i++) {
        float staff_x, staff_y;
        get_staff_position(i, &staff_x, &staff_y);

        int screen_x, screen_y;
        iso_to_screen((int)staff_x, (int)staff_y, &screen_x, &screen_y);

        int tile_x = (int)staff_x;
        int tile_y = (int)staff_y;
        if (tile_x >= 0 && tile_x < MAP_SIZE && tile_y >= 0 && tile_y < MAP_SIZE) {
            screen_y -= g_map[tile_y][tile_x].height * 8;
        }

        // Staff color based on type (0=janitor, 1=mechanic)
        int staff_type = get_staff_type(i);
        uint32_t staff_color = (staff_type == 0) ? 0x00FF00 : 0x0000FF;

        // Draw staff with uniform
        fill_rect_asm(g_renderer.framebuffer, screen_x - 3, screen_y - 10,
                     6, 4, 0xFFDDCC, g_renderer.screen_width); // Head
        fill_rect_asm(g_renderer.framebuffer, screen_x - 4, screen_y - 6,
                     8, 6, staff_color, g_renderer.screen_width); // Uniform
    }
}

// Camera control
void move_camera(int dx, int dy) {
    g_renderer.camera_x += dx;
    g_renderer.camera_y += dy;
}

// Modify terrain
void set_tile_height(int x, int y, int height) {
    if (x >= 0 && x < MAP_SIZE && y >= 0 && y < MAP_SIZE) {
        g_map[y][x].height = height;
    }
}

void set_tile_type(int x, int y, int type) {
    if (x >= 0 && x < MAP_SIZE && y >= 0 && y < MAP_SIZE) {
        g_map[y][x].type = type;
    }
}

// Save/Load map data
void save_map_data(FILE* f) {
    fwrite(g_map, sizeof(Tile), MAP_SIZE * MAP_SIZE, f);
}

void load_map_data(FILE* f) {
    fread(g_map, sizeof(Tile), MAP_SIZE * MAP_SIZE, f);
}
