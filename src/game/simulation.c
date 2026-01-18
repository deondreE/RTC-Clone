#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#define MAX_GUESTS 100

typedef enum {
    GUEST_STATE_WANDERING,
    GUEST_STATE_HEADING_TO_RIDE,
    GUEST_STATE_QUEUING,
    GUEST_STATE_ON_RIDE,
    GUEST_STATE_LEAVING
} GuestState;

typedef struct {
    float x, y;
    float target_x, target_y;
    float speed;
    int happiness;
    int hunger;
    int thirst;
    int energy;
    int money;
    bool has_target;
    uint32_t color;
    GuestState state;
    int target_ride;
    char thought[64];
} Guest;

typedef struct {
    int num_guests;
    int park_rating;
    int total_money;
    float time;
    int entrance_fee;
    int total_guests_entered;
} ParkState;

static Guest g_guests[MAX_GUESTS];
static ParkState g_park = {0};

extern void init_rides(void);
extern void update_rides(float dt);
extern int get_ride_at(int x, int y);
extern bool can_guest_ride(int ride_idx, int guest_money);
extern void add_to_queue(int ride_idx);
extern int get_ride_price(int ride_idx);

extern void init_staff(void);
extern void update_staff(float dt);

void init_simulation(void) {
    printf("Initializing simulation...\n");
    
    g_park.num_guests = 5;
    g_park.park_rating = 800;
    g_park.total_money = 10000;
    g_park.time = 0.0f;
    g_park.entrance_fee = 10;
    g_park.total_guests_entered = 5;

    // Initialize some test guests
    for (int i = 0; i < g_park.num_guests; i++) {
        g_guests[i].x = 5.0f + (rand() % 3);
        g_guests[i].y = 5.0f + (rand() % 3);
        g_guests[i].target_x = 16.0f;
        g_guests[i].target_y = 16.0f;
        g_guests[i].speed = 2.0f + (rand() % 100) / 100.0f;
        g_guests[i].happiness = 80 + (rand() % 20);
        g_guests[i].hunger = rand() % 30;
        g_guests[i].thirst = rand() % 30;
        g_guests[i].energy = 80 + (rand() % 20);
        g_guests[i].money = 50 + (rand() % 100);
        g_guests[i].has_target = false;
        g_guests[i].state = GUEST_STATE_WANDERING;
        g_guests[i].target_ride = -1;
        strcpy(g_guests[i].thought, "I love this park!");
        
        // Random color for visual variety
        uint32_t colors[] = {0xFF00FF, 0x00FFFF, 0xFFFF00, 0xFF8800, 0x00FF88};
        g_guests[i].color = colors[i % 5];
    }
    
    // Initialize subsystems
    init_rides();
    init_staff();
}

void update_guest_ai(Guest* guest, float dt) {
    // Update needs over time
    guest->hunger += dt * 0.3f;
    guest->thirst += dt * 0.5f;
    guest->energy -= dt * 0.2f;
    
    if (guest->hunger > 100) guest->hunger = 100;
    if (guest->thirst > 100) guest->thirst = 100;
    if (guest->energy < 0) guest->energy = 0;
    
    // Update happiness based on needs
    if (guest->hunger > 80 || guest->thirst > 80 || guest->energy < 20) {
        guest->happiness -= dt * 3;
        if (guest->hunger > 80) strcpy(guest->thought, "I'm hungry!");
        else if (guest->thirst > 80) strcpy(guest->thought, "I'm thirsty!");
        else strcpy(guest->thought, "I'm tired...");
    } else {
        guest->happiness += dt * 0.5f;
    }
    
    if (guest->happiness > 100) guest->happiness = 100;
    if (guest->happiness < 0) guest->happiness = 0;
    
    // Decide what to do based on state
    switch (guest->state) {
        case GUEST_STATE_WANDERING:
            if (!guest->has_target) {
                // Look for rides or just wander
                if ((rand() % 100) < 20 && guest->money > 5) {
                    // Try to find a ride
                    int ride_idx = rand() % 2; // We have 2 rides
                    if (can_guest_ride(ride_idx, guest->money)) {
                        guest->state = GUEST_STATE_HEADING_TO_RIDE;
                        guest->target_ride = ride_idx;
                        strcpy(guest->thought, "Let's ride something!");
                    }
                } else {
                    // Just wander
                    guest->target_x = 5.0f + (rand() % 22);
                    guest->target_y = 5.0f + (rand() % 22);
                    guest->has_target = true;
                    strcpy(guest->thought, "What a nice day!");
                }
            }
            break;
            
        case GUEST_STATE_HEADING_TO_RIDE:
            if (!guest->has_target && guest->target_ride >= 0) {
                // Set target to ride location
                int rx, ry, rw, rh, rs;
                extern void get_ride_info(int idx, int* x, int* y, int* w, int* h, int* s);
                get_ride_info(guest->target_ride, &rx, &ry, &rw, &rh, &rs);
                guest->target_x = rx + rw / 2.0f;
                guest->target_y = ry + rh / 2.0f;
                guest->has_target = true;
            }
            break;
            
        default:
            guest->state = GUEST_STATE_WANDERING;
            break;
    }
}

void update_guest(Guest* guest, float dt) {
    update_guest_ai(guest, dt);
    
    // Move towards target
    if (guest->has_target) {
        float dx = guest->target_x - guest->x;
        float dy = guest->target_y - guest->y;
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist < 0.5f) {
            guest->has_target = false;
            
            // Check if we reached a ride
            if (guest->state == GUEST_STATE_HEADING_TO_RIDE) {
                add_to_queue(guest->target_ride);
                guest->money -= get_ride_price(guest->target_ride);
                guest->happiness += 20;
                guest->state = GUEST_STATE_WANDERING;
                guest->target_ride = -1;
                strcpy(guest->thought, "That was fun!");
            }
        } else {
            guest->x += (dx / dist) * guest->speed * dt;
            guest->y += (dy / dist) * guest->speed * dt;
        }
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
    
    // Update rides
    update_rides(dt);
    
    // Update staff
    update_staff(dt);

    // Update park rating based on guest happiness
    int total_happiness = 0;
    for (int i = 0; i < g_park.num_guests; i++) {
        total_happiness += g_guests[i].happiness;
    }
    
    if (g_park.num_guests > 0) {
        g_park.park_rating = (total_happiness * 10) / g_park.num_guests;
    }

    // Spawn new guests occasionally
    if ((int)g_park.time % 15 == 0 && g_park.num_guests < MAX_GUESTS) {
        static int last_spawn_time = 0;
        if ((int)g_park.time != last_spawn_time) {
            int idx = g_park.num_guests;
            g_guests[idx].x = 5.0f;
            g_guests[idx].y = 5.0f;
            g_guests[idx].speed = 2.0f;
            g_guests[idx].happiness = 90;
            g_guests[idx].hunger = 20;
            g_guests[idx].thirst = 20;
            g_guests[idx].energy = 90;
            g_guests[idx].money = 75;
            g_guests[idx].has_target = false;
            g_guests[idx].state = GUEST_STATE_WANDERING;
            g_guests[idx].target_ride = -1;
            strcpy(g_guests[idx].thought, "Wow! This park looks great!");
            uint32_t colors[] = {0xFF00FF, 0x00FFFF, 0xFFFF00, 0xFF8800, 0x00FF88};
            g_guests[idx].color = colors[idx % 5];
            g_park.num_guests++;
            g_park.total_guests_entered++;
            g_park.total_money += g_park.entrance_fee;
            last_spawn_time = (int)g_park.time;
            printf("New guest entered! Total: %d\n", g_park.num_guests);
        }
    }
    
    // Monthly expenses (staff wages)
    static float wage_timer = 0.0f;
    wage_timer += dt;
    if (wage_timer >= 30.0f) {
        extern int get_total_wages(void);
        int wages = get_total_wages();
        g_park.total_money -= wages;
        printf("Paid $%d in wages\n", wages);
        wage_timer = 0.0f;
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

const char* get_guest_thought(int index) {
    if (index >= 0 && index < g_park.num_guests) {
        return g_guests[index].thought;
    }
    return "";
}

int get_total_guests_entered(void) {
    return g_park.total_guests_entered;
}