#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#define MAX_STAFF 20

typedef enum {
    STAFF_JANITOR,
    STAFF_MECHANIC,
    STAFF_SECURITY,
    STAFF_ENTERTAINER
} StaffType;

typedef struct {
    bool active;
    StaffType type;
    float x, y;
    float target_x, target_y;
    bool has_target;
    int patrol_area_x, patrol_area_y;
    int patrol_radius;
    float energy;
    int wage;
    bool is_working;
} Staff;

static Staff g_staff[MAX_STAFF] = {0};
static int g_num_staff = 0;

// External litter functions
extern bool find_nearest_litter(float from_x, float from_y, float* target_x, float* target_y);
extern void remove_litter_at(float x, float y, float radius);

void init_staff(void) {
    printf("Initializing staff system...\n");

    // Hire a janitor
    g_staff[0].active = true;
    g_staff[0].type = STAFF_JANITOR;
    g_staff[0].x = 16.0f;
    g_staff[0].y = 16.0f;
    g_staff[0].patrol_area_x = 16;
    g_staff[0].patrol_area_y = 16;
    g_staff[0].patrol_radius = 10;
    g_staff[0].energy = 100.0f;
    g_staff[0].wage = 50;
    g_staff[0].has_target = false;
    g_staff[0].is_working = false;

    // Hire a second janitor
    g_staff[1].active = true;
    g_staff[1].type = STAFF_JANITOR;
    g_staff[1].x = 10.0f;
    g_staff[1].y = 20.0f;
    g_staff[1].patrol_area_x = 10;
    g_staff[1].patrol_area_y = 20;
    g_staff[1].patrol_radius = 12;
    g_staff[1].energy = 100.0f;
    g_staff[1].wage = 50;
    g_staff[1].has_target = false;
    g_staff[1].is_working = false;

    // Hire a mechanic
    g_staff[2].active = true;
    g_staff[2].type = STAFF_MECHANIC;
    g_staff[2].x = 10.0f;
    g_staff[2].y = 10.0f;
    g_staff[2].patrol_area_x = 10;
    g_staff[2].patrol_area_y = 10;
    g_staff[2].patrol_radius = 15;
    g_staff[2].energy = 100.0f;
    g_staff[2].wage = 80;
    g_staff[2].has_target = false;
    g_staff[2].is_working = false;

    g_num_staff = 3;
}

void update_staff_member(Staff* staff, float dt) {
    // Decrease energy
    staff->energy -= dt * 0.5f;
    if (staff->energy < 0) staff->energy = 0;

    // Janitor AI - look for litter
    if (staff->type == STAFF_JANITOR) {
        if (!staff->has_target || staff->energy < 30) {
            // Look for nearby litter
            float litter_x, litter_y;
            if (find_nearest_litter(staff->x, staff->y, &litter_x, &litter_y)) {
                staff->target_x = litter_x;
                staff->target_y = litter_y;
                staff->has_target = true;
                staff->is_working = true;
            } else {
                // No litter, patrol
                staff->target_x = staff->patrol_area_x + (rand() % (staff->patrol_radius * 2)) - staff->patrol_radius;
                staff->target_y = staff->patrol_area_y + (rand() % (staff->patrol_radius * 2)) - staff->patrol_radius;
                staff->has_target = true;
                staff->is_working = false;
            }

            // Restore energy when needed
            if (staff->energy < 30) {
                staff->energy = 100.0f;
            }
        }

        // Check if we reached litter
        float dx = staff->target_x - staff->x;
        float dy = staff->target_y - staff->y;
        float dist = sqrtf(dx*dx + dy*dy);

        if (dist < 0.5f && staff->is_working) {
            // Clean the litter
            remove_litter_at(staff->x, staff->y, 0.5f);
            staff->has_target = false;
            staff->is_working = false;
        }
    } else {
        // Other staff types patrol
        if (!staff->has_target || staff->energy < 30) {
            staff->target_x = staff->patrol_area_x + (rand() % (staff->patrol_radius * 2)) - staff->patrol_radius;
            staff->target_y = staff->patrol_area_y + (rand() % (staff->patrol_radius * 2)) - staff->patrol_radius;
            staff->has_target = true;

            if (staff->energy < 30) {
                staff->energy = 100.0f;
            }
        }
    }

    // Move towards target
    if (staff->has_target) {
        float dx = staff->target_x - staff->x;
        float dy = staff->target_y - staff->y;
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist < 0.5f) {
            staff->has_target = false;
        } else {
            float speed = 1.5f;
            staff->x += (dx / dist) * speed * dt;
            staff->y += (dy / dist) * speed * dt;
        }
    }

    // Keep in bounds
    if (staff->x < 0) staff->x = 0;
    if (staff->y < 0) staff->y = 0;
    if (staff->x > 31) staff->x = 31;
    if (staff->y > 31) staff->y = 31;
}

void update_staff(float dt) {
    for (int i = 0; i < g_num_staff; i++) {
        if (!g_staff[i].active) continue;
        update_staff_member(&g_staff[i], dt);
    }
}

// Getters
int get_num_staff(void) {
    return g_num_staff;
}

void get_staff_position(int index, float* x, float* y) {
    if (index >= 0 && index < g_num_staff && g_staff[index].active) {
        *x = g_staff[index].x;
        *y = g_staff[index].y;
    }
}

StaffType get_staff_type(int index) {
    if (index >= 0 && index < g_num_staff && g_staff[index].active) {
        return g_staff[index].type;
    }
    return STAFF_JANITOR;
}

// Send mechanic to broken ride
void dispatch_mechanic(int ride_x, int ride_y) {
    for (int i = 0; i < g_num_staff; i++) {
        if (!g_staff[i].active) continue;
        if (g_staff[i].type != STAFF_MECHANIC) continue;

        g_staff[i].target_x = ride_x;
        g_staff[i].target_y = ride_y;
        g_staff[i].has_target = true;
        printf("Dispatching mechanic to (%d, %d)\n", ride_x, ride_y);
        break;
    }
}

int get_total_wages(void) {
    int total = 0;
    for (int i = 0; i < g_num_staff; i++) {
        if (g_staff[i].active) {
            total += g_staff[i].wage;
        }
    }
    return total;
}

// Save/Load support
void save_staff_data(FILE* f) {
    fwrite(&g_num_staff, sizeof(int), 1, f);
    fwrite(g_staff, sizeof(Staff), MAX_STAFF, f);
}

void load_staff_data(FILE* f) {
    fread(&g_num_staff, sizeof(int), 1, f);
    fread(g_staff, sizeof(Staff), MAX_STAFF, f);
}
