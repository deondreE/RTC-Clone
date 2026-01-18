#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#define MAX_GUESTS 100

typedef struct {
    float x, y;
    float target_x, target_y;
    float speed;
    int happiness;
    int hunger;
    int thirst;
    int money;
    bool has_target;
    uint32_t color;
} Guest;

typedef struct {
    int num_guests;
    int park_rating;
    int total_money;
    float time;
} ParkState;

static Guest g_guests[MAX_GUESTS];
static ParkState g_park = {0};

void init_simulation(void) {
    printf("Initializing simulation...\n");
    
    g_park.num_guests = 10;
    g_park.park_rating = 800;
    g_park.total_money = 10000;
    g_park.time = 0.0f;

    // Initialize some test guests
    for (int i = 0; i < g_park.num_guests; i++) {
        g_guests[i].x = 16.0f + (rand() % 10) - 5;
        g_guests[i].y = 16.0f + (rand() % 10) - 5;
        g_guests[i].target_x = 16.0f;
        g_guests[i].target_y = 16.0f;
        g_guests[i].speed = 2.0f + (rand() % 100) / 100.0f;
        g_guests[i].happiness = 80 + (rand() % 20);
        g_guests[i].hunger = rand() % 50;
        g_guests[i].thirst = rand() % 50;
        g_guests[i].money = 50 + (rand() % 100);
        g_guests[i].has_target = false;
        
        // Random color for visual variety
        uint32_t colors[] = {0xFF00FF, 0x00FFFF, 0xFFFF00, 0xFF8800, 0x00FF88};
        g_guests[i].color = colors[i % 5];
    }
}

void update_guest(Guest* guest, float dt) {
    // Update needs
    guest->hunger += dt * 0.5f;
    guest->thirst += dt * 0.7f;
    
    if (guest->hunger > 100) guest->hunger = 100;
    if (guest->thirst > 100) guest->thirst = 100;
    
    // Happiness decreases with unmet needs
    if (guest->hunger > 70 || guest->thirst > 70) {
        guest->happiness -= dt * 2;
    } else {
        guest->happiness += dt * 0.5f;
    }
    
    if (guest->happiness > 100) guest->happiness = 100;
    if (guest->happiness < 0) guest->happiness = 0;

    // Simple wandering AI
    if (!guest->has_target) {
        guest->target_x = 8.0f + (rand() % 16);
        guest->target_y = 8.0f + (rand() % 16);
        guest->has_target = true;
    }

    // Move towards target
    float dx = guest->target_x - guest->x;
    float dy = guest->target_y - guest->y;
    float dist = sqrtf(dx * dx + dy * dy);

    if (dist < 0.5f) {
        guest->has_target = false;
    } else {
        guest->x += (dx / dist) * guest->speed * dt;
        guest->y += (dy / dist) * guest->speed * dt;
    }

    // Keep in bounds
    if (guest->x < 0) guest->x = 0;
    if (guest->y < 0) guest->y = 0;
    if (guest->x > 31) guest->x = 31;
    if (guest->y > 31) guest->y = 31;
}

void update_simulation(float dt) {
    g_park.time += dt;

    // Update all guests
    for (int i = 0; i < g_park.num_guests; i++) {
        update_guest(&g_guests[i], dt);
    }

    // Update park rating based on guest happiness
    int total_happiness = 0;
    for (int i = 0; i < g_park.num_guests; i++) {
        total_happiness += g_guests[i].happiness;
    }
    
    if (g_park.num_guests > 0) {
        g_park.park_rating = (total_happiness * 10) / g_park.num_guests;
    }

    // Spawn new guests occasionally
    if ((int)g_park.time % 10 == 0 && g_park.num_guests < MAX_GUESTS) {
        static int last_spawn_time = 0;
        if ((int)g_park.time != last_spawn_time) {
            int idx = g_park.num_guests;
            g_guests[idx].x = 1.0f;
            g_guests[idx].y = 1.0f;
            g_guests[idx].speed = 2.0f;
            g_guests[idx].happiness = 90;
            g_guests[idx].hunger = 20;
            g_guests[idx].thirst = 20;
            g_guests[idx].money = 75;
            g_guests[idx].has_target = false;
            uint32_t colors[] = {0xFF00FF, 0x00FFFF, 0xFFFF00, 0xFF8800, 0x00FF88};
            g_guests[idx].color = colors[idx % 5];
            g_park.num_guests++;
            last_spawn_time = (int)g_park.time;
        }
    }
}

// Getters for rendering
int get_num_guests(void) {
    return g_park.num_guests;
}

void get_guest_position(int index, float* x, float* y) {
    if (index >= 0 && index < g_park.num_guests) {
        *x = g_guests[index].x;
        *y = g_guests[index].y;
    }
}

int get_park_rating(void) {
    return g_park.park_rating;
}

int get_park_money(void) {
    return g_park.total_money;
}

uint32_t get_guest_color(int index) {
    if (index >= 0 && index < g_park.num_guests) {
        return g_guests[index].color;
    }
    return 0xFFFFFF;
}