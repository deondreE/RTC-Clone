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
} Staff;

static Staff g_staff[MAX_STAFF] = {0};
static int g_num_staff = 0;

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
    g_num_staff = 1;
    
    // Hire a mechanic
    g_staff[1].active = true;
    g_staff[1].type = STAFF_MECHANIC;
    g_staff[1].x = 10.0f;
    g_staff[1].y = 10.0f;
    g_staff[1].patrol_area_x = 10;
    g_staff[1].patrol_area_y = 10;
    g_staff[1].patrol_radius = 15;
    g_staff[1].energy = 100.0f;
    g_staff[1].wage = 80;
    g_staff[1].has_target = false;
    g_num_staff = 2;
}

void update_staff_member(Staff* staff, float dt) {
    // Decrease energy
    staff->energy -= dt * 0.5f;
    if (staff->energy < 0) staff->energy = 0;

    // Find new patrol target if needed
    if (!staff->has_target || staff->energy < 30) {
         // Go to patrol area
        staff->target_x = staff->patrol_area_x + (rand() % (staff->patrol_radius * 2)) - staff->patrol_radius;
        staff->target_y = staff->patrol_area_y + (rand() % (staff->patrol_radius * 2)) - staff->patrol_radius;
        staff->has_target = true;

         // Restore energy when reaching target
        if (staff->energy < 30) {
            staff->energy = 100.0f;
        }
    }

    float dx = staff->target_x - staff->x;
    float dy = staff->target_y - staff->y;
    float dist = sqrtf(dx * dx  + dy * dy);

    if (dist < 0.5f) {
        staff->has_target = false;
    } else {
        float speed = 1.5f;
        staff->x += (dx / dist) * speed * dt;
        staff->y += (dx / dist) * speed * dt;
    }

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

void dispatch_mechanic(int ride_x, int ride_y) {
    // Find nearest available mechanic
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