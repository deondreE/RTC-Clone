#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_SCENERY 500

typedef enum {
    SCENERY_TREE_OAK,
    SCENERY_TREE_PINE,
    SCENERY_BENCH,
    SCENERY_LAMP,
    SCENERY_FLOWER_BED,
    SCENERY_FOUNTAIN,
    SCENERY_STATUE,
    SCENERY_TRASH_CAN
} SceneryType;

typedef struct {
    bool active;
    SceneryType type;
    int x, y;
    uint32_t color;
    int cost;
} Scenery;

static Scenery g_scenery[MAX_SCENERY] = {0};
static int g_num_scenery = 0;

void init_scenery(void) {
    printf("Initializing scenery system...\n");
    
    // Place some initial trees
    for (int i = 0; i < 15; i++) {
        g_scenery[i].active = true;
        g_scenery[i].type = (i % 2 == 0) ? SCENERY_TREE_OAK : SCENERY_TREE_PINE;
        g_scenery[i].x = 2 + (rand() % 28);
        g_scenery[i].y = 2 + (rand() % 28);
        g_scenery[i].color = (g_scenery[i].type == SCENERY_TREE_OAK) ? 0x228B22 : 0x0F4D0F;
        g_scenery[i].cost = 50;
    }
    
    // Place some benches along paths
    for (int i = 15; i < 20; i++) {
        g_scenery[i].active = true;
        g_scenery[i].type = SCENERY_BENCH;
        g_scenery[i].x = 6 + (i - 15) * 3;
        g_scenery[i].y = 25;
        g_scenery[i].color = 0x8B4513;
        g_scenery[i].cost = 30;
    }
    
    // Place some lamps
    for (int i = 20; i < 25; i++) {
        g_scenery[i].active = true;
        g_scenery[i].type = SCENERY_LAMP;
        g_scenery[i].x = 7 + (i - 20) * 4;
        g_scenery[i].y = 10;
        g_scenery[i].color = 0xFFFF00;
        g_scenery[i].cost = 75;
    }
    
    // Place trash cans
    for (int i = 25; i < 30; i++) {
        g_scenery[i].active = true;
        g_scenery[i].type = SCENERY_TRASH_CAN;
        g_scenery[i].x = 5 + (i - 25) * 5;
        g_scenery[i].y = 24;
        g_scenery[i].color = 0x404040;
        g_scenery[i].cost = 25;
    }
    
    g_num_scenery = 30;
}

bool add_scenery(SceneryType type, int x, int y) {
    if (g_num_scenery >= MAX_SCENERY) return false;
    
    g_scenery[g_num_scenery].active = true;
    g_scenery[g_num_scenery].type = type;
    g_scenery[g_num_scenery].x = x;
    g_scenery[g_num_scenery].y = y;
    
    switch (type) {
        case SCENERY_TREE_OAK:
            g_scenery[g_num_scenery].color = 0x228B22;
            g_scenery[g_num_scenery].cost = 50;
            break;
        case SCENERY_TREE_PINE:
            g_scenery[g_num_scenery].color = 0x0F4D0F;
            g_scenery[g_num_scenery].cost = 50;
            break;
        case SCENERY_BENCH:
            g_scenery[g_num_scenery].color = 0x8B4513;
            g_scenery[g_num_scenery].cost = 30;
            break;
        case SCENERY_LAMP:
            g_scenery[g_num_scenery].color = 0xFFFF00;
            g_scenery[g_num_scenery].cost = 75;
            break;
        case SCENERY_FLOWER_BED:
            g_scenery[g_num_scenery].color = 0xFF69B4;
            g_scenery[g_num_scenery].cost = 40;
            break;
        case SCENERY_FOUNTAIN:
            g_scenery[g_num_scenery].color = 0x4169E1;
            g_scenery[g_num_scenery].cost = 150;
            break;
        case SCENERY_STATUE:
            g_scenery[g_num_scenery].color = 0xD3D3D3;
            g_scenery[g_num_scenery].cost = 200;
            break;
        case SCENERY_TRASH_CAN:
            g_scenery[g_num_scenery].color = 0x404040;
            g_scenery[g_num_scenery].cost = 25;
            break;
    }
    
    g_num_scenery++;
    return true;
}

void remove_scenery_at(int x, int y) {
    for (int i = 0; i < g_num_scenery; i++) {
        if (g_scenery[i].active && g_scenery[i].x == x && g_scenery[i].y == y) {
            g_scenery[i].active = false;
            printf("Removed scenery at (%d, %d)\n", x, y);
            return;
        }
    }
}
int get_num_scenery(void) {
    int count = 0;
    for (int i = 0; i < g_num_scenery; i++) {
        if (g_scenery[i].active) count++;
    }
    return count;
}

void get_scenery_info(int idx, int* x, int* y, SceneryType* type, uint32_t* color) {
    if (idx >= 0 && idx < g_num_scenery && g_scenery[idx].active) {
        *x = g_scenery[idx].x;
        *y = g_scenery[idx].y;
        *type = g_scenery[idx].type;
        *color = g_scenery[idx].color;
    }
}

SceneryType get_scenery_at(int x, int y) {
    for (int i = 0; i < g_num_scenery; i++) {
        if (g_scenery[i].active && g_scenery[i].x == x && g_scenery[i].y == y) {
            return g_scenery[i].type;
        }
    }
    return -1;
}

// Check if there's a trash can nearby
bool is_trash_can_nearby(int x, int y, int radius) {
    for (int i = 0; i < g_num_scenery; i++) {
        if (!g_scenery[i].active) continue;
        if (g_scenery[i].type != SCENERY_TRASH_CAN) continue;
        
        int dx = g_scenery[i].x - x;
        int dy = g_scenery[i].y - y;
        if (dx*dx + dy*dy <= radius*radius) {
            return true;
        }
    }
    return false;
}