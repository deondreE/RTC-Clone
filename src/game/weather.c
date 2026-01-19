#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#define MAX_RAINDROPS 500
#define MAX_SNOWFLAKES 300

typedef enum{
    WEATHER_SUNNY,
    WEATHER_CLOUDY,
    WEATHER_RAIN,
    WEATHER_HEAVY_RAIN,
    WEATHER_SNOW,
    WEATHER_FOG
} WeatherType;

typedef struct {
    float x, y;
    float velocity;
    float size;
    float active;
} Particle;

typedef struct {
    WeatherType current;
    WeatherType target;
    float transition_progress;  // 0.0 to 1.0
    float duration;
    float next_change_timer;
    float intensity;  // 0.0 to 1.0
    uint32_t sky_tint;
    float visibility;  // 0.0 to 1.0 (for fog)
} WeatherState;

static WeatherState g_weather = {0};
static Particle g_raindrops[MAX_RAINDROPS] = {0};
static Particle g_snowflakes[MAX_SNOWFLAKES] = {0};

void init_weather(void) {
    printf("Initializing weather system...");

    g_weather.current = WEATHER_SUNNY;
    g_weather.target = WEATHER_SUNNY;
    g_weather.transition_progress = 1.0f;
    g_weather.duration = 0.0f;
    g_weather.next_change_timer = 60.0f * (rand() % 120);
    g_weather.intensity = 0.5f;
    g_weather.sky_tint = 0xFFFFFF;
    g_weather.visibility = 1.0f;

    for (int i = 0; i < MAX_RAINDROPS; ++i) {
        g_raindrops[i].active = false;
    }

    for (int i = 0; i < MAX_SNOWFLAKES; ++i) {
        g_snowflakes[i].active = false; 
    }
}

static WeatherType pick_random_weather(void) {
    int chance = rand() % 100;

    if (chance < 40) return WEATHER_SUNNY;
    if (chance < 60) return WEATHER_CLOUDY;
    if (chance < 75) return WEATHER_RAIN;
    if (chance < 85) return WEATHER_HEAVY_RAIN;
    if (chance < 95) return WEATHER_SNOW;
    else return WEATHER_FOG;
}

static void spawn_raindrop(void) {
    for (int i = 0; i < MAX_RAINDROPS; ++i) {
        if (!g_raindrops[i].active) {
            g_raindrops[i].active = true;
            g_raindrops[i].x = (rand() % 800);
            g_raindrops[i].y = -10.0f;
            g_raindrops[i].velocity = 300.0f + (rand() % 200);
            g_raindrops[i].size = 2.0f + (rand() % 2);
            return;
        }
    }
}

static void spawn_snowflake(void) {
    for (int i = 0; i < MAX_SNOWFLAKES; ++i) {
        if (!g_snowflakes[i].active) {
            g_snowflakes[i].active = true;
            g_snowflakes[i].x = (rand() % 800);
            g_snowflakes[i].y = -10.0f;
            g_snowflakes[i].velocity = 50.0f + (rand() % 50);
            g_snowflakes[i].size = 2.0f + (rand() % 3);
            return;
        }
    }
}

static void update_particles(float dt) {
    if (g_weather.current == WEATHER_RAIN || g_weather.current == WEATHER_HEAVY_RAIN) {
        int spawn_count = (g_weather.current == WEATHER_HEAVY_RAIN) ? 15 : 8;
        for (int i = 0; i < spawn_count; ++i) {
            spawn_raindrop();
        }
    }

    for (int i = 0; i < MAX_RAINDROPS; ++i) {
        if (g_raindrops[i].active) {
            g_raindrops[i].y += g_raindrops[i].velocity * dt;
            g_raindrops[i].x += 50.0f * dt;

            if (g_raindrops[i].y > 600) {
                g_raindrops[i].active = false;
            }
        }
    }

    if (g_weather.current == WEATHER_SNOW) {
        for (int i = 0; i < 5; ++i) {
            spawn_snowflake();
        }
    }

    for (int i = 0; i < MAX_SNOWFLAKES; ++i) {
        if (g_snowflakes[i].active) {
            g_snowflakes[i].y += g_snowflakes[i].velocity * dt;
            // Drift left and right
            g_snowflakes[i].x += sinf(g_snowflakes[i].y * 0.02f) * 30.0f * dt;

            if (g_snowflakes[i].y > 600) {
                g_snowflakes[i].active = false;
            }
        }
    }
}

void update_weather(float dt) {
    g_weather.duration += dt;
    g_weather.next_change_timer -= dt; 

    if (g_weather.next_change_timer <= 0.0f) {
        if (g_weather.transition_progress >= 1.0f) {
            g_weather.target = pick_random_weather();
            g_weather.transition_progress = 0.0f;
            g_weather.next_change_timer = 60.0f + (rand() % 180);

            const char* weather_names[] = {"Sunny", "Cloudy", "Rain", "Heavy Rain", "Snow", "Fog"};
            printf("Weather changing to: %s\n", weather_names[g_weather.target]);
        }
    }

    if (g_weather.transition_progress < 1.0f) {
        g_weather.transition_progress += dt * 0.2f;
        if (g_weather.transition_progress >= 1.0f) {
            g_weather.transition_progress = 1.0f;
            g_weather.current = g_weather.target;
        } 
    }

    // Update intensity based on weather type
    float target_intensity = 0.5f;
    switch (g_weather.current) {
        case WEATHER_SUNNY:
            target_intensity = 0.0f;
            g_weather.visibility = 1.0f;
            break;
        case WEATHER_CLOUDY:
            target_intensity = 0.3f;
            g_weather.visibility = 1.0f;
            break;
        case WEATHER_RAIN:
            target_intensity = 0.6f;
            g_weather.visibility = 0.9f;
            break;
        case WEATHER_HEAVY_RAIN:
            target_intensity = 1.0f;
            g_weather.visibility = 0.7f;
            break;
        case WEATHER_SNOW:
            target_intensity = 0.5f;
            g_weather.visibility = 0.8f;
            break;
        case WEATHER_FOG:
            target_intensity = 0.4f;
            g_weather.visibility = 0.5f;
            break;
    }

    if (g_weather.intensity < target_intensity) {
        g_weather.intensity += dt * 0.3f;
        if (g_weather.intensity > target_intensity) g_weather.intensity = target_intensity;
    } else {
        g_weather.intensity -= dt * 0.3f;
        if (g_weather.intensity < target_intensity) g_weather.intensity = target_intensity;
    }

    update_particles(dt);
}

WeatherType get_weather(void) {
    return g_weather.current;
}

float get_weather_intensity(void) {
    return g_weather.intensity;
}

float get_weather_visibility(void) {
    return g_weather.visibility;
}

uint32_t get_weather_sky_tint(void) {
    switch (g_weather.current) {
        case WEATHER_SUNNY:
            return 0xFFFFFF;  // No tint
        case WEATHER_CLOUDY:
            return 0xCCCCCC;  // Slight gray
        case WEATHER_RAIN:
        case WEATHER_HEAVY_RAIN:
            return 0x888899;  // Dark blue-gray
        case WEATHER_SNOW:
            return 0xDDDDEE;  // Slight blue-white
        case WEATHER_FOG:
            return 0xBBBBBB;  // Gray
        default:
            return 0xFFFFFF;
    }
}

uint32_t apply_weather_tint(uint32_t color) {
    uint32_t tint = get_weather_sky_tint();
    
    uint8_t r = ((color >> 16) & 0xFF);
    uint8_t g = ((color >> 8) & 0xFF);
    uint8_t b = ((color >> 0) & 0xFF);
    
    uint8_t tr = ((tint >> 16) & 0xFF);
    uint8_t tg = ((tint >> 8) & 0xFF);
    uint8_t tb = ((tint >> 0) & 0xFF);
    
    // Blend with tint
    r = (r * tr) / 255;
    g = (g * tg) / 255;
    b = (b * tb) / 255;
    
    // Darken based on intensity
    float darkness = 1.0f - (g_weather.intensity * 0.3f);
    r *= darkness;
    g *= darkness;
    b *= darkness;
    
    return (r << 16) | (g << 8) | b;
}

// Check if it's raining
bool is_raining(void) {
    return g_weather.current == WEATHER_RAIN || g_weather.current == WEATHER_HEAVY_RAIN;
}

// Check if it's snowing
bool is_snowing(void) {
    return g_weather.current == WEATHER_SNOW;
}

// Check if there's fog
bool is_foggy(void) {
    return g_weather.current == WEATHER_FOG;
}

// Render weather particles
void render_weather_particles(uint8_t* framebuffer, int screen_width, int screen_height) {
    // Render raindrops
    for (int i = 0; i < MAX_RAINDROPS; i++) {
        if (!g_raindrops[i].active) continue;
        
        int x = (int)g_raindrops[i].x;
        int y = (int)g_raindrops[i].y;
        int size = (int)g_raindrops[i].size;
        
        // Draw raindrop as vertical line
        for (int dy = 0; dy < size * 3; dy++) {
            int px = x;
            int py = y + dy;
            
            if (px >= 0 && px < screen_width && py >= 0 && py < screen_height) {
                int idx = (py * screen_width + px) * 4;
                framebuffer[idx + 0] = 200;  // B
                framebuffer[idx + 1] = 220;  // G
                framebuffer[idx + 2] = 255;  // R
                framebuffer[idx + 3] = 150;  // A (semi-transparent)
            }
        }
    }
    
    // Render snowflakes
    for (int i = 0; i < MAX_SNOWFLAKES; i++) {
        if (!g_snowflakes[i].active) continue;
        
        int x = (int)g_snowflakes[i].x;
        int y = (int)g_snowflakes[i].y;
        int size = (int)g_snowflakes[i].size;
        
        // Draw snowflake as small square
        for (int dy = -size; dy <= size; dy++) {
            for (int dx = -size; dx <= size; dx++) {
                int px = x + dx;
                int py = y + dy;
                
                if (px >= 0 && px < screen_width && py >= 0 && py < screen_height) {
                    int idx = (py * screen_width + px) * 4;
                    framebuffer[idx + 0] = 255;  // B
                    framebuffer[idx + 1] = 255;  // G
                    framebuffer[idx + 2] = 255;  // R
                    framebuffer[idx + 3] = 200;  // A
                }
            }
        }
    }
}

// Get weather effect on guest happiness
int get_weather_happiness_modifier(void) {
    switch (g_weather.current) {
        case WEATHER_SUNNY:
            return 5;   // Guests love sunny days
        case WEATHER_CLOUDY:
            return 0;   // Neutral
        case WEATHER_RAIN:
            return -10; // Guests dislike rain
        case WEATHER_HEAVY_RAIN:
            return -20; // Guests hate heavy rain
        case WEATHER_SNOW:
            return 3;   // Some guests enjoy snow
        case WEATHER_FOG:
            return -5;  // Slightly unpleasant
        default:
            return 0;
    }
}

// Get weather effect on ride operations
bool can_ride_operate_in_weather(void) {
    if (g_weather.current == WEATHER_HEAVY_RAIN) return false;
    if (g_weather.current == WEATHER_SNOW && g_weather.intensity > 0.7f) return false;
    return true;
}

// Get umbrella demand (for future shop feature)
float get_umbrella_demand(void) {
    if (g_weather.current == WEATHER_RAIN) return 0.7f;
    if (g_weather.current == WEATHER_HEAVY_RAIN) return 1.0f;
    return 0.0f;
}

// Force specific weather (for testing)
void set_weather(WeatherType type) {
    g_weather.current = type;
    g_weather.target = type;
    g_weather.transition_progress = 1.0f;
}

// Get weather name for display
const char* get_weather_name(void) {
    switch (g_weather.current) {
        case WEATHER_SUNNY: return "Sunny";
        case WEATHER_CLOUDY: return "Cloudy";
        case WEATHER_RAIN: return "Rain";
        case WEATHER_HEAVY_RAIN: return "Heavy Rain";
        case WEATHER_SNOW: return "Snow";
        case WEATHER_FOG: return "Fog";
        default: return "Unknown";
    }
}

// Save/Load support
void save_weather_data(FILE* f) {
    fwrite(&g_weather, sizeof(WeatherState), 1, f);
}

void load_weather_data(FILE* f) {
    fread(&g_weather, sizeof(WeatherState), 1, f);
    
    // Reset particles on load
    for (int i = 0; i < MAX_RAINDROPS; i++) {
        g_raindrops[i].active = false;
    }
    for (int i = 0; i < MAX_SNOWFLAKES; i++) {
        g_snowflakes[i].active = false;
    }
}

