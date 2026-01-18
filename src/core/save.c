#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#define SAVE_VERSION 1
#define SAVE_MAGIC 0x52435453  // "RCTS" in hex
#define MAX_SAVE_SLOTS 10
#define SAVE_DIR "saves"

typedef struct {
    uint32_t magic;
    uint32_t version;
    time_t timestamp;
    char park_name[64];
    int park_rating;
    int total_money;
    int num_guests;
    float game_time;
    float time_of_day;
} SaveHeader;

// External getter/setter functions for all game state
extern void get_park_state(int* rating, int* money, int* guests, float* time, float* tod, int* total_entered, int* entrance_fee);
extern void set_park_state(int rating, int money, int guests, float time, float tod, int total_entered, int entrance_fee);

extern int get_num_rides(void);
extern void save_ride_data(FILE* f);
extern void load_ride_data(FILE* f);

extern int get_num_staff(void);
extern void save_staff_data(FILE* f);
extern void load_staff_data(FILE* f);

extern int get_num_scenery(void);
extern void save_scenery_data(FILE* f);
extern void load_scenery_data(FILE* f);

extern int get_num_shops(void);
extern void save_shop_data(FILE* f);
extern void load_shop_data(FILE* f);

extern void save_guest_data(FILE* f);
extern void load_guest_data(FILE* f);

extern void save_map_data(FILE* f);
extern void load_map_data(FILE* f);

extern int get_num_litter(void);
extern void save_litter_data(FILE* f);
extern void load_litter_data(FILE* f);

static char g_park_name[64] = "My Amazing Park";

// Ensure save directory exists
static bool ensure_save_dir(void) {
    #ifdef _WIN32
    mkdir(SAVE_DIR);
    #else
    mkdir(SAVE_DIR, 0755);
    #endif
    return true;
}

// Get save file path for a slot
static void get_save_path(int slot, char* buffer, size_t size) {
    snprintf(buffer, size, "%s/park_%d.sav", SAVE_DIR, slot);
}

// Save the entire game state
bool save_game(int slot) {
    if (slot < 0 || slot >= MAX_SAVE_SLOTS) {
        printf("Invalid save slot: %d\n", slot);
        return false;
    }

    ensure_save_dir();

    char filepath[256];
    get_save_path(slot, filepath, sizeof(filepath));

    FILE* f = fopen(filepath, "wb");
    if (!f) {
        printf("Failed to open save file: %s\n", filepath);
        return false;
    }

    // Write header
    SaveHeader header = {0};
    header.magic = SAVE_MAGIC;
    header.version = SAVE_VERSION;
    header.timestamp = time(NULL);
    strncpy(header.park_name, g_park_name, sizeof(header.park_name) - 1);

    int rating, money, guests, total_entered, entrance_fee;
    float game_time, tod;
    get_park_state(&rating, &money, &guests, &game_time, &tod, &total_entered, &entrance_fee);

    header.park_rating = rating;
    header.total_money = money;
    header.num_guests = guests;
    header.game_time = game_time;
    header.time_of_day = tod;

    fwrite(&header, sizeof(SaveHeader), 1, f);

    // Write park state details
    fwrite(&total_entered, sizeof(int), 1, f);
    fwrite(&entrance_fee, sizeof(int), 1, f);

    // Write map data
    save_map_data(f);

    // Write rides
    int num_rides = get_num_rides();
    fwrite(&num_rides, sizeof(int), 1, f);
    save_ride_data(f);

    // Write staff
    int num_staff = get_num_staff();
    fwrite(&num_staff, sizeof(int), 1, f);
    save_staff_data(f);

    // Write scenery
    int num_scenery = get_num_scenery();
    fwrite(&num_scenery, sizeof(int), 1, f);
    save_scenery_data(f);

    // Write shops
    int num_shops = get_num_shops();
    fwrite(&num_shops, sizeof(int), 1, f);
    save_shop_data(f);

    // Write guests
    fwrite(&guests, sizeof(int), 1, f);
    save_guest_data(f);

    // Write litter
    int num_litter = get_num_litter();
    fwrite(&num_litter, sizeof(int), 1, f);
    save_litter_data(f);

    fclose(f);

    printf("Game saved to slot %d: %s\n", slot, filepath);
    return true;
}

// Load the entire game state
bool load_game(int slot) {
    if (slot < 0 || slot >= MAX_SAVE_SLOTS) {
        printf("Invalid save slot: %d\n", slot);
        return false;
    }

    char filepath[256];
    get_save_path(slot, filepath, sizeof(filepath));

    FILE* f = fopen(filepath, "rb");
    if (!f) {
        printf("Save file not found: %s\n", filepath);
        return false;
    }

    // Read and validate header
    SaveHeader header;
    if (fread(&header, sizeof(SaveHeader), 1, f) != 1) {
        printf("Failed to read save header\n");
        fclose(f);
        return false;
    }

    if (header.magic != SAVE_MAGIC) {
        printf("Invalid save file magic\n");
        fclose(f);
        return false;
    }

    if (header.version != SAVE_VERSION) {
        printf("Save file version mismatch\n");
        fclose(f);
        return false;
    }

    // Read park state details
    int total_entered, entrance_fee;
    fread(&total_entered, sizeof(int), 1, f);
    fread(&entrance_fee, sizeof(int), 1, f);

    // Restore park state
    strncpy(g_park_name, header.park_name, sizeof(g_park_name) - 1);
    set_park_state(header.park_rating, header.total_money, header.num_guests,
                   header.game_time, header.time_of_day, total_entered, entrance_fee);

    // Load map data
    load_map_data(f);

    // Load rides
    int num_rides;
    fread(&num_rides, sizeof(int), 1, f);
    load_ride_data(f);

    // Load staff
    int num_staff;
    fread(&num_staff, sizeof(int), 1, f);
    load_staff_data(f);

    // Load scenery
    int num_scenery;
    fread(&num_scenery, sizeof(int), 1, f);
    load_scenery_data(f);

    // Load shops
    int num_shops;
    fread(&num_shops, sizeof(int), 1, f);
    load_shop_data(f);

    // Load guests
    int num_guests;
    fread(&num_guests, sizeof(int), 1, f);
    load_guest_data(f);

    // Load litter
    int num_litter;
    fread(&num_litter, sizeof(int), 1, f);
    load_litter_data(f);

    fclose(f);

    printf("Game loaded from slot %d: %s\n", slot, filepath);
    return true;
}

// Check if a save exists
bool save_exists(int slot) {
    if (slot < 0 || slot >= MAX_SAVE_SLOTS) {
        return false;
    }

    char filepath[256];
    get_save_path(slot, filepath, sizeof(filepath));

    FILE* f = fopen(filepath, "rb");
    if (!f) {
        return false;
    }

    fclose(f);
    return true;
}

// Get save info for display
bool get_save_info(int slot, char* name, int* rating, int* money, int* guests, time_t* timestamp) {
    if (slot < 0 || slot >= MAX_SAVE_SLOTS) {
        return false;
    }

    char filepath[256];
    get_save_path(slot, filepath, sizeof(filepath));

    FILE* f = fopen(filepath, "rb");
    if (!f) {
        return false;
    }

    SaveHeader header;
    if (fread(&header, sizeof(SaveHeader), 1, f) != 1) {
        fclose(f);
        return false;
    }

    if (header.magic != SAVE_MAGIC) {
        fclose(f);
        return false;
    }

    strncpy(name, header.park_name, 63);
    name[63] = '\0';
    *rating = header.park_rating;
    *money = header.total_money;
    *guests = header.num_guests;
    *timestamp = header.timestamp;

    fclose(f);
    return true;
}

// Delete a save
bool delete_save(int slot) {
    if (slot < 0 || slot >= MAX_SAVE_SLOTS) {
        return false;
    }

    char filepath[256];
    get_save_path(slot, filepath, sizeof(filepath));

    if (remove(filepath) == 0) {
        printf("Deleted save slot %d\n", slot);
        return true;
    }

    return false;
}

// Auto-save
bool auto_save(void) {
    return save_game(0);  // Slot 0 is auto-save
}

// Set park name
void set_park_name(const char* name) {
    strncpy(g_park_name, name, sizeof(g_park_name) - 1);
    g_park_name[sizeof(g_park_name) - 1] = '\0';
}

// Get park name
const char* get_park_name(void) {
    return g_park_name;
}
