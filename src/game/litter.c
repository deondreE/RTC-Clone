#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#define MAX_LITTER 200

typedef struct {
    bool active;
    float x, y;
    float age;  // How long it's been sitting
} Litter;

static Litter g_litter[MAX_LITTER] = {0};
static int g_num_litter = 0;

void init_litter(void) {
    printf("Initializing litter system...\n");
    g_num_litter = 0;
}

void add_litter(float x, float y) {
    // Find an empty slot
    for (int i = 0; i < MAX_LITTER; i++) {
        if (!g_litter[i].active) {
            g_litter[i].active = true;
            g_litter[i].x = x;
            g_litter[i].y = y;
            g_litter[i].age = 0.0f;
            if (i >= g_num_litter) g_num_litter = i + 1;
            return;
        }
    }
}

void remove_litter_at(float x, float y, float radius) {
    for (int i = 0; i < g_num_litter; i++) {
        if (!g_litter[i].active) continue;

        float dx = g_litter[i].x - x;
        float dy = g_litter[i].y - y;
        float dist = sqrtf(dx*dx + dy*dy);

        if (dist <= radius) {
            g_litter[i].active = false;
            return;  // Clean one piece at a time
        }
    }
}

void update_litter(float dt) {
    for (int i = 0; i < g_num_litter; i++) {
        if (g_litter[i].active) {
            g_litter[i].age += dt;
        }
    }
}

// Get litter count in area (for park rating)
int get_litter_count_in_area(int x, int y, int radius) {
    int count = 0;
    for (int i = 0; i < g_num_litter; i++) {
        if (!g_litter[i].active) continue;

        int dx = (int)g_litter[i].x - x;
        int dy = (int)g_litter[i].y - y;
        if (dx*dx + dy*dy <= radius*radius) {
            count++;
        }
    }
    return count;
}

// Getters for rendering
int get_num_litter(void) {
    int count = 0;
    for (int i = 0; i < g_num_litter; i++) {
        if (g_litter[i].active) count++;
    }
    return count;
}

void get_litter_position(int idx, float* x, float* y) {
    int current = 0;
    for (int i = 0; i < g_num_litter; i++) {
        if (g_litter[i].active) {
            if (current == idx) {
                *x = g_litter[i].x;
                *y = g_litter[i].y;
                return;
            }
            current++;
        }
    }
}

// Find nearest litter for janitor
bool find_nearest_litter(float from_x, float from_y, float* target_x, float* target_y) {
    float nearest_dist = 999999.0f;
    bool found = false;

    for (int i = 0; i < g_num_litter; i++) {
        if (!g_litter[i].active) continue;

        float dx = g_litter[i].x - from_x;
        float dy = g_litter[i].y - from_y;
        float dist = sqrtf(dx*dx + dy*dy);

        if (dist < nearest_dist) {
            nearest_dist = dist;
            *target_x = g_litter[i].x;
            *target_y = g_litter[i].y;
            found = true;
        }
    }

    return found;
}

int get_total_litter_count(void) {
    return get_num_litter();
}

// Save/Load support
void save_litter_data(FILE* f) {
    fwrite(&g_num_litter, sizeof(int), 1, f);
    fwrite(g_litter, sizeof(Litter), MAX_LITTER, f);
}

void load_litter_data(FILE* f) {
    fread(&g_num_litter, sizeof(int), 1, f);
    fread(g_litter, sizeof(Litter), MAX_LITTER, f);
}
