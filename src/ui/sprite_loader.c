#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// We'll use stb_image for PNG loading (single-header library)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define MAX_SPRITES 256
#define SPRITE_CACHE_SIZE 1024 * 1024 * 16  // 16MB sprite cache

typedef struct {
    char name[64];
    uint8_t* data;      // RGBA pixel data
    int width;
    int height;
    int channels;
    bool loaded;
    int frame_count;    // For animations
    int frame_width;    // Width of single frame
} Sprite;

typedef struct {
    int sprite_id;
    int frame;          // Current animation frame
    float frame_timer;  // Timer for animation
    float frame_rate;   // Frames per second
} AnimatedSprite;

static Sprite g_sprites[MAX_SPRITES] = {0};
static int g_sprite_count = 0;
static uint8_t* g_sprite_cache = NULL;
static size_t g_cache_used = 0;

// Assembly blit function (already exists)
extern void blit_sprite_asm(uint8_t* dest, const uint8_t* sprite, int width, int height, int dest_pitch);

// Initialize sprite system
bool init_sprite_system(void) {
    printf("Initializing sprite system...\n");
    
    g_sprite_cache = (uint8_t*)malloc(SPRITE_CACHE_SIZE);
    if (!g_sprite_cache) {
        printf("Failed to allocate sprite cache!\n");
        return false;
    }
    
    g_cache_used = 0;
    g_sprite_count = 0;
    
    printf("Sprite cache allocated: %d MB\n", SPRITE_CACHE_SIZE / (1024 * 1024));
    return true;
}

// Load a sprite from PNG file
int load_sprite(const char* filename) {
    if (g_sprite_count >= MAX_SPRITES) {
        printf("Sprite limit reached!\n");
        return -1;
    }
    
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "assets/sprites/%s", filename);
    
    printf("Loading sprite: %s\n", filepath);
    
    int width, height, channels;
    uint8_t* data = stbi_load(filepath, &width, &height, &channels, 4); // Force RGBA
    
    if (!data) {
        printf("Failed to load sprite: %s\n", filepath);
        return -1;
    }
    
    // Calculate size needed
    size_t sprite_size = width * height * 4;
    
    if (g_cache_used + sprite_size > SPRITE_CACHE_SIZE) {
        printf("Sprite cache full!\n");
        stbi_image_free(data);
        return -1;
    }
    
    // Copy to cache
    int sprite_id = g_sprite_count++;
    Sprite* sprite = &g_sprites[sprite_id];
    
    sprite->data = g_sprite_cache + g_cache_used;
    memcpy(sprite->data, data, sprite_size);
    g_cache_used += sprite_size;
    
    sprite->width = width;
    sprite->height = height;
    sprite->channels = 4; // Always RGBA
    sprite->loaded = true;
    sprite->frame_count = 1;
    sprite->frame_width = width;
    strncpy(sprite->name, filename, sizeof(sprite->name) - 1);
    
    stbi_image_free(data);
    
    printf("  Loaded: %dx%d, %d bytes\n", width, height, (int)sprite_size);
    printf("  Sprite ID: %d\n", sprite_id);
    printf("  Cache used: %.2f MB / %.2f MB\n", 
           g_cache_used / (1024.0f * 1024.0f),
           SPRITE_CACHE_SIZE / (1024.0f * 1024.0f));
    
    return sprite_id;
}

// Load a sprite sheet (horizontal frames)
int load_sprite_sheet(const char* filename, int frame_width) {
    int sprite_id = load_sprite(filename);
    if (sprite_id < 0) return -1;
    
    Sprite* sprite = &g_sprites[sprite_id];
    
    // Calculate frame count
    sprite->frame_count = sprite->width / frame_width;
    sprite->frame_width = frame_width;
    
    printf("  Sprite sheet: %d frames of %dx%d\n", 
           sprite->frame_count, frame_width, sprite->height);
    
    return sprite_id;
}

// Get sprite info
bool get_sprite_info(int sprite_id, int* width, int* height, int* frame_count) {
    if (sprite_id < 0 || sprite_id >= g_sprite_count || !g_sprites[sprite_id].loaded) {
        return false;
    }
    
    Sprite* sprite = &g_sprites[sprite_id];
    *width = sprite->frame_width;
    *height = sprite->height;
    *frame_count = sprite->frame_count;
    return true;
}

// Draw sprite at position
void draw_sprite(uint8_t* framebuffer, int screen_width, int screen_height,
                 int sprite_id, int x, int y, int frame) {
    if (sprite_id < 0 || sprite_id >= g_sprite_count || !g_sprites[sprite_id].loaded) {
        return;
    }
    
    Sprite* sprite = &g_sprites[sprite_id];
    
    // Clamp frame
    if (frame < 0) frame = 0;
    if (frame >= sprite->frame_count) frame = sprite->frame_count - 1;
    
    // Calculate frame offset
    int frame_offset = frame * sprite->frame_width * 4;
    uint8_t* frame_data = sprite->data + frame_offset;
    
    // Bounds check
    if (x + sprite->frame_width < 0 || x >= screen_width ||
        y + sprite->height < 0 || y >= screen_height) {
        return;
    }
    
    // Draw each row
    for (int row = 0; row < sprite->height; row++) {
        int screen_y = y + row;
        if (screen_y < 0 || screen_y >= screen_height) continue;
        
        for (int col = 0; col < sprite->frame_width; col++) {
            int screen_x = x + col;
            if (screen_x < 0 || screen_x >= screen_width) continue;
            
            // Get pixel from sprite
            int sprite_idx = (row * sprite->frame_width + col) * 4;
            uint8_t r = frame_data[sprite_idx + 0];
            uint8_t g = frame_data[sprite_idx + 1];
            uint8_t b = frame_data[sprite_idx + 2];
            uint8_t a = frame_data[sprite_idx + 3];
            
            // Skip transparent pixels
            if (a < 128) continue;
            
            // Write to framebuffer
            int fb_idx = (screen_y * screen_width + screen_x) * 4;
            framebuffer[fb_idx + 0] = b;
            framebuffer[fb_idx + 1] = g;
            framebuffer[fb_idx + 2] = r;
            framebuffer[fb_idx + 3] = 255;
        }
    }
}

// Draw sprite with scaling
void draw_sprite_scaled(uint8_t* framebuffer, int screen_width, int screen_height,
                        int sprite_id, int x, int y, int frame, float scale) {
    if (sprite_id < 0 || sprite_id >= g_sprite_count || !g_sprites[sprite_id].loaded) {
        return;
    }
    
    Sprite* sprite = &g_sprites[sprite_id];
    
    // Clamp frame
    if (frame < 0) frame = 0;
    if (frame >= sprite->frame_count) frame = sprite->frame_count - 1;
    
    int frame_offset = frame * sprite->frame_width * 4;
    uint8_t* frame_data = sprite->data + frame_offset;
    
    int scaled_width = (int)(sprite->frame_width * scale);
    int scaled_height = (int)(sprite->height * scale);
    
    // Draw with nearest-neighbor scaling
    for (int row = 0; row < scaled_height; row++) {
        int screen_y = y + row;
        if (screen_y < 0 || screen_y >= screen_height) continue;
        
        int src_row = (int)(row / scale);
        
        for (int col = 0; col < scaled_width; col++) {
            int screen_x = x + col;
            if (screen_x < 0 || screen_x >= screen_width) continue;
            
            int src_col = (int)(col / scale);
            
            // Get pixel from sprite
            int sprite_idx = (src_row * sprite->frame_width + src_col) * 4;
            uint8_t r = frame_data[sprite_idx + 0];
            uint8_t g = frame_data[sprite_idx + 1];
            uint8_t b = frame_data[sprite_idx + 2];
            uint8_t a = frame_data[sprite_idx + 3];
            
            // Skip transparent pixels
            if (a < 128) continue;
            
            // Write to framebuffer
            int fb_idx = (screen_y * screen_width + screen_x) * 4;
            framebuffer[fb_idx + 0] = b;
            framebuffer[fb_idx + 1] = g;
            framebuffer[fb_idx + 2] = r;
            framebuffer[fb_idx + 3] = 255;
        }
    }
}

// Draw sprite with tint/color multiplication
void draw_sprite_tinted(uint8_t* framebuffer, int screen_width, int screen_height,
                        int sprite_id, int x, int y, int frame, uint32_t tint) {
    if (sprite_id < 0 || sprite_id >= g_sprite_count || !g_sprites[sprite_id].loaded) {
        return;
    }
    
    Sprite* sprite = &g_sprites[sprite_id];
    
    if (frame < 0) frame = 0;
    if (frame >= sprite->frame_count) frame = sprite->frame_count - 1;
    
    int frame_offset = frame * sprite->frame_width * 4;
    uint8_t* frame_data = sprite->data + frame_offset;
    
    uint8_t tint_r = (tint >> 16) & 0xFF;
    uint8_t tint_g = (tint >> 8) & 0xFF;
    uint8_t tint_b = (tint >> 0) & 0xFF;
    
    for (int row = 0; row < sprite->height; row++) {
        int screen_y = y + row;
        if (screen_y < 0 || screen_y >= screen_height) continue;
        
        for (int col = 0; col < sprite->frame_width; col++) {
            int screen_x = x + col;
            if (screen_x < 0 || screen_x >= screen_width) continue;
            
            int sprite_idx = (row * sprite->frame_width + col) * 4;
            uint8_t r = frame_data[sprite_idx + 0];
            uint8_t g = frame_data[sprite_idx + 1];
            uint8_t b = frame_data[sprite_idx + 2];
            uint8_t a = frame_data[sprite_idx + 3];
            
            if (a < 128) continue;
            
            // Apply tint
            r = (r * tint_r) / 255;
            g = (g * tint_g) / 255;
            b = (b * tint_b) / 255;
            
            int fb_idx = (screen_y * screen_width + screen_x) * 4;
            framebuffer[fb_idx + 0] = b;
            framebuffer[fb_idx + 1] = g;
            framebuffer[fb_idx + 2] = r;
            framebuffer[fb_idx + 3] = 255;
        }
    }
}

// Update animated sprite
void update_animated_sprite(AnimatedSprite* anim, float dt) {
    anim->frame_timer += dt;
    
    float frame_duration = 1.0f / anim->frame_rate;
    
    while (anim->frame_timer >= frame_duration) {
        anim->frame_timer -= frame_duration;
        anim->frame++;
        
        // Get frame count
        if (anim->sprite_id >= 0 && anim->sprite_id < g_sprite_count) {
            Sprite* sprite = &g_sprites[anim->sprite_id];
            if (anim->frame >= sprite->frame_count) {
                anim->frame = 0; // Loop
            }
        }
    }
}

// Create animated sprite
AnimatedSprite create_animated_sprite(int sprite_id, float frame_rate) {
    AnimatedSprite anim = {0};
    anim.sprite_id = sprite_id;
    anim.frame = 0;
    anim.frame_timer = 0.0f;
    anim.frame_rate = frame_rate;
    return anim;
}

// List all loaded sprites (debug)
void list_sprites(void) {
    printf("\n=== Loaded Sprites ===\n");
    for (int i = 0; i < g_sprite_count; i++) {
        Sprite* sprite = &g_sprites[i];
        if (sprite->loaded) {
            printf("  [%d] %s - %dx%d", i, sprite->name, sprite->width, sprite->height);
            if (sprite->frame_count > 1) {
                printf(" (%d frames)", sprite->frame_count);
            }
            printf("\n");
        }
    }
    printf("Total: %d sprites\n", g_sprite_count);
    printf("Cache: %.2f MB / %.2f MB\n", 
           g_cache_used / (1024.0f * 1024.0f),
           SPRITE_CACHE_SIZE / (1024.0f * 1024.0f));
    printf("======================\n\n");
}

// Cleanup
void cleanup_sprite_system(void) {
    if (g_sprite_cache) {
        free(g_sprite_cache);
        g_sprite_cache = NULL;
    }
    g_sprite_count = 0;
    g_cache_used = 0;
    printf("Sprite system cleaned up\n");
}

// Get sprite by name (for convenience)
int find_sprite(const char* name) {
    for (int i = 0; i < g_sprite_count; i++) {
        if (g_sprites[i].loaded && strcmp(g_sprites[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}