#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define MAX_RIDES 50

typedef enum {
    RIDE_TYPE_ROLLERCOASTER,
    RIDE_TYPE_CAROUSEL,
    RIDE_TYPE_FERRIS_WHEEL,
    RIDE_TYPE_BUMPER_CARS,
    RIDE_TYPE_LOG_FLUME
} RideType;

typedef enum {
    RIDE_STATUS_CLOSED,
    RIDE_STATUS_OPEN,
    RIDE_STATUS_TESTING,
    RIDE_STATUS_BROKEN
} RideStatus;

typedef struct {
    bool active;
    RideType type;
    char name[32];
    int x, y;
    int width, height;
    RideStatus status;
    int excitement;
    int intensity;
    int nausea;
    int price;
    int total_riders;
    int queue_length;
    float breakdown_progress;
} Ride;

static Ride g_rides[MAX_RIDES] = {0};
static int g_num_rides = 0;

void init_rides(void) {
    printf("Initializing rides system...\n");

    // Create a test rollercoaster
    g_rides[0].active = true;
    g_rides[0].type = RIDE_TYPE_ROLLERCOASTER;
    strcpy(g_rides[0].name, "Steel Thunder");
    g_rides[0].x = 10;
    g_rides[0].y = 10;
    g_rides[0].width = 4;
    g_rides[0].height = 4;
    g_rides[0].status = RIDE_STATUS_OPEN;
    g_rides[0].excitement = 75;
    g_rides[0].intensity = 60;
    g_rides[0].nausea = 40;
    g_rides[0].price = 5;
    g_rides[0].total_riders = 0;
    g_rides[0].queue_length = 0;
    g_rides[0].breakdown_progress = 0.0f;
    g_num_rides = 1;

    // Create a carousel
    g_rides[1].active = true;
    g_rides[1].type = RIDE_TYPE_CAROUSEL;
    strcpy(g_rides[1].name, "Merry-Go-Round");
    g_rides[1].x = 20;
    g_rides[1].y = 15;
    g_rides[1].width = 2;
    g_rides[1].height = 2;
    g_rides[1].status = RIDE_STATUS_OPEN;
    g_rides[1].excitement = 35;
    g_rides[1].intensity = 15;
    g_rides[1].nausea = 10;
    g_rides[1].price = 2;
    g_rides[1].total_riders = 0;
    g_rides[1].queue_length = 0;
    g_rides[1].breakdown_progress = 0.0f;
    g_num_rides = 2;
}

void update_rides(float dt) {
    for (int i = 0; i < g_num_rides; i++) {
        if (!g_rides[i].active) continue;

        // Random breakdowns
        if (g_rides[i].status == RIDE_STATUS_OPEN) {
            g_rides[i].breakdown_progress += dt * 0.01f;

            if (g_rides[i].breakdown_progress > 100.0f && (rand() % 100) < 2) {
                g_rides[i].status = RIDE_STATUS_BROKEN;
                printf("Ride '%s' has broken down!\n", g_rides[i].name);
                g_rides[i].breakdown_progress = 0.0f;
            }
        }

        // Process queue
        if (g_rides[i].status == RIDE_STATUS_OPEN && g_rides[i].queue_length > 0) {
            if ((rand() % 100) < 30) {
                g_rides[i].queue_length--;
                g_rides[i].total_riders++;
            }
        }
    }
}

// Check if a guest can ride
bool can_guest_ride(int ride_idx, int guest_money) {
    if (ride_idx < 0 || ride_idx >= g_num_rides) return false;
    if (!g_rides[ride_idx].active) return false;
    if (g_rides[ride_idx].status != RIDE_STATUS_OPEN) return false;
    if (guest_money < g_rides[ride_idx].price) return false;
    return true;
}

// Add guest to queue
void add_to_queue(int ride_idx) {
    if (ride_idx >= 0 && ride_idx < g_num_rides) {
        g_rides[ride_idx].queue_length++;
    }
}

// Get ride at position
int get_ride_at(int x, int y) {
    for (int i = 0; i < g_num_rides; i++) {
        if (!g_rides[i].active) continue;

        if (x >= g_rides[i].x && x < g_rides[i].x + g_rides[i].width &&
            y >= g_rides[i].y && y < g_rides[i].y + g_rides[i].height) {
            return i;
        }
    }
    return -1;
}

// Getters
int get_num_rides(void) {
    return g_num_rides;
}

void get_ride_info(int idx, int* x, int* y, int* width, int* height, RideStatus* status) {
    if (idx >= 0 && idx < g_num_rides && g_rides[idx].active) {
        *x = g_rides[idx].x;
        *y = g_rides[idx].y;
        *width = g_rides[idx].width;
        *height = g_rides[idx].height;
        *status = g_rides[idx].status;
    }
}

const char* get_ride_name(int idx) {
    if (idx >= 0 && idx < g_num_rides && g_rides[idx].active) {
        return g_rides[idx].name;
    }
    return "Unknown";
}

int get_ride_price(int idx) {
    if (idx >= 0 && idx < g_num_rides && g_rides[idx].active) {
        return g_rides[idx].price;
    }
    return 0;
}

int get_ride_queue(int idx) {
    if (idx >= 0 && idx < g_num_rides && g_rides[idx].active) {
        return g_rides[idx].queue_length;
    }
    return 0;
}

// Repair ride
void repair_ride(int idx) {
    if (idx >= 0 && idx < g_num_rides && g_rides[idx].active) {
        if (g_rides[idx].status == RIDE_STATUS_BROKEN) {
            g_rides[idx].status = RIDE_STATUS_OPEN;
            printf("Ride '%s' has been repaired!\n", g_rides[idx].name);
        }
    }
}

// Save/Load support
void save_ride_data(FILE* f) {
    fwrite(&g_num_rides, sizeof(int), 1, f);
    fwrite(g_rides, sizeof(Ride), MAX_RIDES, f);
}

void load_ride_data(FILE* f) {
    fread(&g_num_rides, sizeof(int), 1, f);
    fread(g_rides, sizeof(Ride), MAX_RIDES, f);
}
