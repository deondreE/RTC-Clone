#include <stdint.h>
#include <math.h>
#include <stdbool.h>

typedef struct {
    uint8_t r, g, b;
} Color;

typedef struct {
    Color sky_top;
    Color sky_bottom;
    float ambient_brightness;
    bool lamps_on;
} LightingState;

static LightingState g_lighting = {0};

// Get sky gradient colors based on time of day
void calculate_sky_color(float time_of_day, Color* top, Color* bottom) {
    // time_of_day is 0.0-24.0 (hours)

    // Sunrise: 5-7
    // Day: 7-17
    // Sunset: 17-19
    // Night: 19-5

    if (time_of_day >= 5.0f && time_of_day < 7.0f) {
        // Sunrise - orange/pink to blue
        float t = (time_of_day - 5.0f) / 2.0f;

        top->r = (uint8_t)(255 * (1.0f - t) + 135 * t);
        top->g = (uint8_t)(140 * (1.0f - t) + 206 * t);
        top->b = (uint8_t)(100 * (1.0f - t) + 235 * t);

        bottom->r = (uint8_t)(255 * (1.0f - t) + 135 * t);
        bottom->g = (uint8_t)(200 * (1.0f - t) + 215 * t);
        bottom->b = (uint8_t)(150 * (1.0f - t) + 255 * t);
    } else if (time_of_day >= 7.0f && time_of_day < 17.0f) {
        // Daytime - bright blue sky
        top->r = 135;
        top->g = 206;
        top->b = 235;

        bottom->r = 135;
        bottom->g = 215;
        bottom->b = 255;
    } else if (time_of_day >= 17.0f && time_of_day < 19.0f) {
        // Sunset - blue to orange/red
        float t = (time_of_day - 17.0f) / 2.0f;

        top->r = (uint8_t)(135 * (1.0f - t) + 25 * t);
        top->g = (uint8_t)(206 * (1.0f - t) + 25 * t);
        top->b = (uint8_t)(235 * (1.0f - t) + 112 * t);

        bottom->r = (uint8_t)(135 * (1.0f - t) + 255 * t);
        bottom->g = (uint8_t)(215 * (1.0f - t) + 140 * t);
        bottom->b = (uint8_t)(255 * (1.0f - t) + 50 * t);
    } else {
        // Night - dark blue/purple
        top->r = 25;
        top->g = 25;
        top->b = 112;

        bottom->r = 15;
        bottom->g = 15;
        bottom->b = 60;
    }
}

float calculate_ambient_brightness(float time_of_day) {
    if (time_of_day >= 6.0f && time_of_day < 18.0f) {
        if (time_of_day >= 7.0f && time_of_day < 17.0f) {
            return 1.0f;
        }

        if (time_of_day < 7.0f) {
            return 0.3f + 0.7f * (time_of_day - 6.0f);
        } else {
            return 1.0f - 0.6f * (time_of_day - 17.0f);
        }
    } else {
        return 0.4f;
    }
}

void update_lighting(float time_of_day) {
    calculate_sky_color(time_of_day, &g_lighting.sky_top, &g_lighting.sky_bottom);
    g_lighting.ambient_brightness  = calculate_ambient_brightness(time_of_day);

    g_lighting.lamps_on = (time_of_day < 6.0f || time_of_day >= 18.0f);
}

uint32_t apply_lighting(uint32_t color, float additional_brightness) {
    float brightness = g_lighting.ambient_brightness + additional_brightness;
    if (brightness > 1.0f) brightness = 1.0f;

    uint8_t r = ((color >> 16) & 0xFF) * brightness;
    uint8_t g = ((color >> 8) & 0xFF) * brightness;
    uint8_t b = ((color >> 0) & 0xFF) * brightness;

    return (r << 16) | (g << 8) | b;
}

uint32_t get_sky_color(int screen_y, int screen_height) {
    float t = (float)screen_y / (float)screen_height;

    uint8_t r = g_lighting.sky_top.r * (1.0f - t) + g_lighting.sky_bottom.r * t;
    uint8_t g = g_lighting.sky_top.g * (1.0f - t) + g_lighting.sky_bottom.g * t;
    uint8_t b = g_lighting.sky_top.b * (1.0f - t) + g_lighting.sky_bottom.b * t;

    return (r << 16) | (g << 8) | b;
}

bool are_lamps_on(void) {
    return g_lighting.lamps_on;
}

float get_ambient_brightness(void) {
    return g_lighting.ambient_brightness;
}

uint32_t get_lamp_glow_color(void) {
    if (g_lighting.lamps_on) {
        return 0xFFFF99;  // Warm yellow glow
    }
    return 0xFFFF00;  // Normal yellow
}

uint32_t apply_night_tint(uint32_t color) {
    if (!g_lighting.lamps_on) {
        return apply_lighting(color, 0.0f);
    }

    // At night, add slight blue tint
    uint8_t r = ((color >> 16) & 0xFF);
    uint8_t g = ((color >> 8) & 0xFF);
    uint8_t b = ((color >> 0) & 0xFF);

    // Reduce red, slightly boost blue
    r = r * 0.7f;
    b = b * 1.1f;
    if (b > 255) b = 255;

    color = (r << 16) | (g << 8) | b;

    return apply_lighting(color, 0.0f);
}

uint32_t get_ride_lights(int ride_index) {
    if (!g_lighting.lamps_on) {
        return 0;  // No lights during day
    }

    // Different colored lights for different rides
    uint32_t colors[] = {
        0xFF0000,  // Red
        0x00FF00,  // Green
        0x0000FF,  // Blue
        0xFF00FF,  // Magenta
        0xFFFF00,  // Yellow
        0x00FFFF   // Cyan
    };

    return colors[ride_index % 6];
}

float get_shadow_intensity(void) {
    if (g_lighting.lamps_on) {
        return 0.3f;  // Softer shadows at night
    }
    return 0.6f;  // Stronger shadows during day
}
