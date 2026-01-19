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
    GUEST_STATE_HEADING_TO_SHOP,
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
    int bathroom;
    int money;
    bool has_target;
    uint32_t color;
    GuestState state;
    int target_ride;
    int target_shop;
    char thought[64];
    float litter_timer;
} Guest;

typedef struct {
    int num_guests;
    int park_rating;
    int total_money;
    float time;
    int entrance_fee;
    int total_guests_entered;
    float time_of_day;  // 0-24 hours
} ParkState;

static Guest g_guests[MAX_GUESTS];
static ParkState g_park = {0};

// External functions
extern void init_rides(void);
extern void update_rides(float dt);
extern int get_ride_at(int x, int y);
extern bool can_guest_ride(int ride_idx, int guest_money);
extern void add_to_queue(int ride_idx);
extern int get_ride_price(int ride_idx);

extern void init_staff(void);
extern void update_staff(float dt);

extern void init_scenery(void);
extern bool is_trash_can_nearby(int x, int y, int radius);

extern void init_shops(void);
extern int find_nearest_shop(int type, int from_x, int from_y);
extern void get_shop_info(int idx, int* x, int* y, int* type);
extern void make_purchase(int shop_idx, int* cost);

extern void init_litter(void);
extern void add_litter(float x, float y);
extern void update_litter(float dt);
extern int get_total_litter_count(void);

extern void init_weather(void);
extern void update_weather(float dt);
extern int get_weather_happiness_modifier(void);
extern bool is_raining(void);
extern bool can_ride_operate_in_weather(void);

void init_simulation(void) {
    printf("Initializing simulation...\n");
    
    g_park.num_guests = 5;
    g_park.park_rating = 800;
    g_park.total_money = 10000;
    g_park.time = 0.0f;
    g_park.entrance_fee = 10;
    g_park.total_guests_entered = 5;
    g_park.time_of_day = 10.0f;  // Start at 10 AM

    // Initialize guests
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
        g_guests[i].bathroom = rand() % 30;
        g_guests[i].money = 50 + (rand() % 100);
        g_guests[i].has_target = false;
        g_guests[i].state = GUEST_STATE_WANDERING;
        g_guests[i].target_ride = -1;
        g_guests[i].target_shop = -1;
        g_guests[i].litter_timer = 0.0f;
        strcpy(g_guests[i].thought, "I love this park!");
        
        uint32_t colors[] = {0xFF00FF, 0x00FFFF, 0xFFFF00, 0xFF8800, 0x00FF88};
        g_guests[i].color = colors[i % 5];
    }
    
    // Initialize subsystems
    init_rides();
    init_staff();
    init_scenery();
    init_shops();
    init_litter();
    init_weather();
}

void update_guest_ai(Guest* guest, float dt) {
    // Update needs over time
    guest->hunger += dt * 0.3f;
    guest->thirst += dt * 0.5f;
    guest->bathroom += dt * 0.4f;
    guest->energy -= dt * 0.2f;
    
    if (guest->hunger > 100) guest->hunger = 100;
    if (guest->thirst > 100) guest->thirst = 100;
    if (guest->bathroom > 100) guest->bathroom = 100;
    if (guest->energy < 0) guest->energy = 0;
    
    // Litter generation
    guest->litter_timer += dt;
    if (guest->litter_timer > 30.0f && (rand() % 100) < 10) {
        // Drop litter if no trash can nearby
        if (!is_trash_can_nearby((int)guest->x, (int)guest->y, 3)) {
            add_litter(guest->x, guest->y);
            guest->happiness -= 5;  // Feel slightly bad
        }
        guest->litter_timer = 0.0f;
    }
    
    // Update happiness based on needs
    if (guest->hunger > 80) {
        guest->happiness -= dt * 3;
        strcpy(guest->thought, "I'm so hungry!");
    } else if (guest->thirst > 80) {
        guest->happiness -= dt * 3;
        strcpy(guest->thought, "I need a drink!");
    } else if (guest->bathroom > 90) {
        guest->happiness -= dt * 5;
        strcpy(guest->thought, "Where's the bathroom?!");
    } else if (guest->energy < 20) {
        guest->happiness -= dt * 2;
        strcpy(guest->thought, "I'm exhausted...");
    } else {
        guest->happiness += dt * 0.5f;
    }
    
    if (guest->happiness > 100) guest->happiness = 100;
    if (guest->happiness < 0) guest->happiness = 0;
    
    // Weather affects happiness
    guest->happiness += get_weather_happiness_modifier() * dt * 0.1f;
    
    // Seek shelter in rain
    if (is_raining() && guest->state == GUEST_STATE_WANDERING) {
        if ((rand() % 100) < 30) {  // 30% chance to seek shop
            int shop_idx = find_nearest_shop(rand() % 3, (int)guest->x, (int)guest->y);
            if (shop_idx >= 0) {
                guest->state = GUEST_STATE_HEADING_TO_SHOP;
                guest->target_shop = shop_idx;
                guest->has_target = false;
                strcpy(guest->thought, "Getting out of the rain!");
            }
        }
    }

    // Priority AI - handle urgent needs first
    if (guest->bathroom > 90 && guest->state != GUEST_STATE_HEADING_TO_SHOP) {
        int shop_idx = find_nearest_shop(2, (int)guest->x, (int)guest->y);  // 2 = bathroom
        if (shop_idx >= 0) {
            guest->state = GUEST_STATE_HEADING_TO_SHOP;
            guest->target_shop = shop_idx;
            guest->has_target = false;
            strcpy(guest->thought, "Heading to restroom!");
        }
    } else if (guest->hunger > 70 && guest->state == GUEST_STATE_WANDERING) {
        int shop_idx = find_nearest_shop(0, (int)guest->x, (int)guest->y);  // 0 = food
        if (shop_idx >= 0 && guest->money >= 4) {
            guest->state = GUEST_STATE_HEADING_TO_SHOP;
            guest->target_shop = shop_idx;
            guest->has_target = false;
        }
    } else if (guest->thirst > 70 && guest->state == GUEST_STATE_WANDERING) {
        int shop_idx = find_nearest_shop(1, (int)guest->x, (int)guest->y);  // 1 = drink
        if (shop_idx >= 0 && guest->money >= 3) {
            guest->state = GUEST_STATE_HEADING_TO_SHOP;
            guest->target_shop = shop_idx;
            guest->has_target = false;
        }
    }
    
    // State machine
    switch (guest->state) {
        case GUEST_STATE_WANDERING:
            if (!guest->has_target) {
                if ((rand() % 100) < 15 && guest->money > 5 && guest->energy > 40) {
                    int ride_idx = rand() % 2;
                    if (can_guest_ride(ride_idx, guest->money)) {
                        guest->state = GUEST_STATE_HEADING_TO_RIDE;
                        guest->target_ride = ride_idx;
                        strcpy(guest->thought, "Let's ride something!");
                    }
                } else {
                    guest->target_x = 5.0f + (rand() % 22);
                    guest->target_y = 5.0f + (rand() % 22);
                    guest->has_target = true;
                    strcpy(guest->thought, "What a nice park!");
                }
            }
            break;
            
        case GUEST_STATE_HEADING_TO_RIDE:
            if (!guest->has_target && guest->target_ride >= 0) {
                int rx, ry, rw, rh, rs;
                extern void get_ride_info(int idx, int* x, int* y, int* w, int* h, int* s);
                get_ride_info(guest->target_ride, &rx, &ry, &rw, &rh, &rs);
                guest->target_x = rx + rw / 2.0f;
                guest->target_y = ry + rh / 2.0f;
                guest->has_target = true;
            }
            break;
            
        case GUEST_STATE_HEADING_TO_SHOP:
            if (!guest->has_target && guest->target_shop >= 0) {
                int sx, sy, st;
                get_shop_info(guest->target_shop, &sx, &sy, &st);
                guest->target_x = sx;
                guest->target_y = sy;
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
            
            // Reached ride
            if (guest->state == GUEST_STATE_HEADING_TO_RIDE) {
                add_to_queue(guest->target_ride);
                guest->money -= get_ride_price(guest->target_ride);
                guest->happiness += 20;
                guest->energy -= 10;
                guest->state = GUEST_STATE_WANDERING;
                guest->target_ride = -1;
                strcpy(guest->thought, "That was amazing!");
            }
            
            // Reached shop
            if (guest->state == GUEST_STATE_HEADING_TO_SHOP && guest->target_shop >= 0) {
                int cost;
                make_purchase(guest->target_shop, &cost);
                guest->money -= cost;
                
                int sx, sy, st;
                get_shop_info(guest->target_shop, &sx, &sy, &st);
                
                if (st == 0) {  // Food
                    guest->hunger = 0;
                    strcpy(guest->thought, "Mmm, delicious!");
                } else if (st == 1) {  // Drink
                    guest->thirst = 0;
                    strcpy(guest->thought, "So refreshing!");
                } else if (st == 2) {  // Bathroom
                    guest->bathroom = 0;
                    strcpy(guest->thought, "Much better!");
                }
                
                guest->happiness += 15;
                guest->state = GUEST_STATE_WANDERING;
                guest->target_shop = -1;
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
    g_park.time_of_day += dt / 60.0f;  // 1 minute real time = 1 hour game time
    if (g_park.time_of_day >= 24.0f) g_park.time_of_day -= 24.0f;

    // Update all guests
    for (int i = 0; i < g_park.num_guests; i++) {
        update_guest(&g_guests[i], dt);
    }
    
    // Update subsystems
    update_rides(dt);
    update_staff(dt);
    update_litter(dt);
    update_weather(dt);

    // Update park rating based on happiness and litter
    int total_happiness = 0;
    for (int i = 0; i < g_park.num_guests; i++) {
        total_happiness += g_guests[i].happiness;
    }
    
    int litter_penalty = get_total_litter_count() * 5;
    
    if (g_park.num_guests > 0) {
        g_park.park_rating = (total_happiness * 10) / g_park.num_guests - litter_penalty;
        if (g_park.park_rating < 0) g_park.park_rating = 0;
        if (g_park.park_rating > 999) g_park.park_rating = 999;
    }

    // Spawn new guests
    if ((int)g_park.time % 15 == 0 && g_park.num_guests < MAX_GUESTS) {
        static int last_spawn = 0;
        if ((int)g_park.time != last_spawn) {
            int idx = g_park.num_guests;
            g_guests[idx].x = 5.0f;
            g_guests[idx].y = 5.0f;
            g_guests[idx].speed = 2.0f;
            g_guests[idx].happiness = 90;
            g_guests[idx].hunger = 20;
            g_guests[idx].thirst = 20;
            g_guests[idx].energy = 90;
            g_guests[idx].bathroom = 10;
            g_guests[idx].money = 75;
            g_guests[idx].has_target = false;
            g_guests[idx].state = GUEST_STATE_WANDERING;
            g_guests[idx].target_ride = -1;
            g_guests[idx].target_shop = -1;
            g_guests[idx].litter_timer = 0.0f;
            strcpy(g_guests[idx].thought, "Wow! This park looks great!");
            uint32_t colors[] = {0xFF00FF, 0x00FFFF, 0xFFFF00, 0xFF8800, 0x00FF88};
            g_guests[idx].color = colors[idx % 5];
            g_park.num_guests++;
            g_park.total_guests_entered++;
            g_park.total_money += g_park.entrance_fee;
            last_spawn = (int)g_park.time;
        }
    }
    
    // Monthly expenses
    static float wage_timer = 0.0f;
    wage_timer += dt;
    if (wage_timer >= 30.0f) {
        extern int get_total_wages(void);
        int wages = get_total_wages();
        g_park.total_money -= wages;
        wage_timer = 0.0f;
    }
}

// Getters
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

float get_time_of_day(void) {
    return g_park.time_of_day;
}

// Save/Load support
void get_park_state(int* rating, int* money, int* guests, float* time, float* tod, int* total_entered, int* entrance_fee) {
    *rating = g_park.park_rating;
    *money = g_park.total_money;
    *guests = g_park.num_guests;
    *time = g_park.time;
    *tod = g_park.time_of_day;
    *total_entered = g_park.total_guests_entered;
    *entrance_fee = g_park.entrance_fee;
}

void set_park_state(int rating, int money, int guests, float time, float tod, int total_entered, int entrance_fee) {
    g_park.park_rating = rating;
    g_park.total_money = money;
    g_park.num_guests = guests;
    g_park.time = time;
    g_park.time_of_day = tod;
    g_park.total_guests_entered = total_entered;
    g_park.entrance_fee = entrance_fee;
}

void save_guest_data(FILE* f) {
    for (int i = 0; i < g_park.num_guests; i++) {
        fwrite(&g_guests[i], sizeof(Guest), 1, f);
    }
}

void load_guest_data(FILE* f) {
    for (int i = 0; i < g_park.num_guests; i++) {
        fread(&g_guests[i], sizeof(Guest), 1, f);
    }
}