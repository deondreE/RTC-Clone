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

            // Use assembly optimized tile drawing
            draw_iso_tile_asm(g_renderer.framebuffer, screen_x, screen_y, 
                            color, g_renderer.screen_width);
        }
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