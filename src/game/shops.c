#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define MAX_SHOPS 50

typedef enum {
    SHOP_TYPE_FOOD,
    SHOP_TYPE_DRINK,
    SHOP_TYPE_BATHROOM,
    SHOP_TYPE_GIFT
} ShopType;

typedef struct {
    bool active;
    ShopType type;
    char name[32];
    int x, y;
    int price;
    int total_sales;
    int total_revenue;
} Shop;

static Shop g_shops[MAX_SHOPS] = {0};
static int g_num_shops = 0;

void init_shops(void) {
    printf("Initializing shops system...\n");
    
    // Add a hot dog stand
    g_shops[0].active = true;
    g_shops[0].type = SHOP_TYPE_FOOD;
    strcpy(g_shops[0].name, "Hot Dogs");
    g_shops[0].x = 12;
    g_shops[0].y = 18;
    g_shops[0].price = 4;
    g_shops[0].total_sales = 0;
    g_shops[0].total_revenue = 0;
    
    // Add a drink stand
    g_shops[1].active = true;
    g_shops[1].type = SHOP_TYPE_DRINK;
    strcpy(g_shops[1].name, "Drinks");
    g_shops[1].x = 18;
    g_shops[1].y = 18;
    g_shops[1].price = 3;
    g_shops[1].total_sales = 0;
    g_shops[1].total_revenue = 0;
    
    // Add a bathroom
    g_shops[2].active = true;
    g_shops[2].type = SHOP_TYPE_BATHROOM;
    strcpy(g_shops[2].name, "Restroom");
    g_shops[2].x = 22;
    g_shops[2].y = 12;
    g_shops[2].price = 1;
    g_shops[2].total_sales = 0;
    g_shops[2].total_revenue = 0;
    
    g_num_shops = 3;
}

bool add_shop(ShopType type, int x, int y) {
    if (g_num_shops >= MAX_SHOPS) return false;
    
    g_shops[g_num_shops].active = true;
    g_shops[g_num_shops].type = type;
    g_shops[g_num_shops].x = x;
    g_shops[g_num_shops].y = y;
    g_shops[g_num_shops].total_sales = 0;
    g_shops[g_num_shops].total_revenue = 0;
    
    switch (type) {
        case SHOP_TYPE_FOOD:
            strcpy(g_shops[g_num_shops].name, "Food Stall");
            g_shops[g_num_shops].price = 4;
            break;
        case SHOP_TYPE_DRINK:
            strcpy(g_shops[g_num_shops].name, "Drink Stall");
            g_shops[g_num_shops].price = 3;
            break;
        case SHOP_TYPE_BATHROOM:
            strcpy(g_shops[g_num_shops].name, "Restroom");
            g_shops[g_num_shops].price = 1;
            break;
        case SHOP_TYPE_GIFT:
            strcpy(g_shops[g_num_shops].name, "Gift Shop");
            g_shops[g_num_shops].price = 10;
            break;
    }
    
    g_num_shops++;
    return true;
}

// Guest makes a purchase
void make_purchase(int shop_idx, int* cost) {
    if (shop_idx >= 0 && shop_idx < g_num_shops && g_shops[shop_idx].active) {
        *cost = g_shops[shop_idx].price;
        g_shops[shop_idx].total_sales++;
        g_shops[shop_idx].total_revenue += g_shops[shop_idx].price;
    }
}

// Find nearest shop of a type
int find_nearest_shop(ShopType type, int from_x, int from_y) {
    int nearest_idx = -1;
    int nearest_dist = 999999;
    
    for (int i = 0; i < g_num_shops; i++) {
        if (!g_shops[i].active) continue;
        if (g_shops[i].type != type) continue;
        
        int dx = g_shops[i].x - from_x;
        int dy = g_shops[i].y - from_y;
        int dist = dx*dx + dy*dy;
        
        if (dist < nearest_dist) {
            nearest_dist = dist;
            nearest_idx = i;
        }
    }
    
    return nearest_idx;
}

// Getters
int get_num_shops(void) {
    return g_num_shops;
}

void get_shop_info(int idx, int* x, int* y, ShopType* type) {
    if (idx >= 0 && idx < g_num_shops && g_shops[idx].active) {
        *x = g_shops[idx].x;
        *y = g_shops[idx].y;
        *type = g_shops[idx].type;
    }
}

const char* get_shop_name(int idx) {
    if (idx >= 0 && idx < g_num_shops && g_shops[idx].active) {
        return g_shops[idx].name;
    }
    return "Unknown";
}

int get_shop_at(int x, int y) {
    for (int i = 0; i < g_num_shops; i++) {
        if (g_shops[i].active && g_shops[i].x == x && g_shops[i].y == y) {
            return i;
        }
    }
    return -1;
}

int get_total_shop_revenue(void) {
    int total = 0;
    for (int i = 0; i < g_num_shops; i++) {
        if (g_shops[i].active) {
            total += g_shops[i].total_revenue;
        }
    }
    return total;
}