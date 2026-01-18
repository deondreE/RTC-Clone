#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#define MAP_SIZE 32
#define MAX_PATH_LENGTH 256

typedef struct {
    int x, y;
} Point;

typedef struct {
    Point pos;
    int g_cost;  // Cost from start
    int h_cost;  // Heuristic cost to end
    int f_cost;  // g_cost + h_cost
    Point parent;
} Node;

typedef struct {
    Node nodes[MAP_SIZE * MAP_SIZE];
    int count;
} NodeList;

// Simple tile passability check (would be expanded)
static bool is_walkable(int x, int y) {
    if (x < 0 || x >= MAP_SIZE || y < 0 || y >= MAP_SIZE) {
        return false;
    }
    // For now, everything is walkable
    // In real implementation, check tile type
    return true;
}

// Manhattan distance heuristic
static int heuristic(int x1, int y1, int x2, int y2) {
    return abs(x1 - x2) + abs(y1 - y2);
}

// Check if node is in list
static bool is_in_list(NodeList* list, int x, int y) {
    for (int i = 0; i < list->count; i++) {
        if (list->nodes[i].pos.x == x && list->nodes[i].pos.y == y) {
            return true;
        }
    }
    return false;
}

// Get node from list
static Node* get_node(NodeList* list, int x, int y) {
    for (int i = 0; i < list->count; i++) {
        if (list->nodes[i].pos.x == x && list->nodes[i].pos.y == y) {
            return &list->nodes[i];
        }
    }
    return NULL;
}

// Find node with lowest f_cost in open list
static int find_lowest_f_cost(NodeList* open_list) {
    int lowest_idx = 0;
    int lowest_f = open_list->nodes[0].f_cost;
    
    for (int i = 1; i < open_list->count; i++) {
        if (open_list->nodes[i].f_cost < lowest_f) {
            lowest_f = open_list->nodes[i].f_cost;
            lowest_idx = i;
        }
    }
    
    return lowest_idx;
}

// Remove node from list
static void remove_node(NodeList* list, int index) {
    for (int i = index; i < list->count - 1; i++) {
        list->nodes[i] = list->nodes[i + 1];
    }
    list->count--;
}

// A* pathfinding implementation
int find_path(int start_x, int start_y, int end_x, int end_y, Point* path, int max_length) {
    NodeList open_list = {0};
    NodeList closed_list = {0};
    
    // Add start node to open list
    Node start_node = {0};
    start_node.pos.x = start_x;
    start_node.pos.y = start_y;
    start_node.g_cost = 0;
    start_node.h_cost = heuristic(start_x, start_y, end_x, end_y);
    start_node.f_cost = start_node.h_cost;
    start_node.parent.x = -1;
    start_node.parent.y = -1;
    
    open_list.nodes[0] = start_node;
    open_list.count = 1;
    
    // Neighbor offsets (4-directional for now)
    int dx[] = {0, 1, 0, -1};
    int dy[] = {-1, 0, 1, 0};
    
    while (open_list.count > 0) {
        // Get node with lowest f_cost
        int current_idx = find_lowest_f_cost(&open_list);
        Node current = open_list.nodes[current_idx];
        
        // Move to closed list
        remove_node(&open_list, current_idx);
        closed_list.nodes[closed_list.count++] = current;
        
        // Check if we reached the end
        if (current.pos.x == end_x && current.pos.y == end_y) {
            // Reconstruct path
            int path_length = 0;
            Point pos = current.pos;
            
            while (pos.x != -1 && pos.y != -1 && path_length < max_length) {
                path[path_length++] = pos;
                Node* parent_node = get_node(&closed_list, pos.x, pos.y);
                if (parent_node) {
                    pos = parent_node->parent;
                } else {
                    break;
                }
            }
            
            // Reverse path
            for (int i = 0; i < path_length / 2; i++) {
                Point temp = path[i];
                path[i] = path[path_length - 1 - i];
                path[path_length - 1 - i] = temp;
            }
            
            return path_length;
        }
        
        // Check neighbors
        for (int i = 0; i < 4; i++) {
            int nx = current.pos.x + dx[i];
            int ny = current.pos.y + dy[i];
            
            if (!is_walkable(nx, ny) || is_in_list(&closed_list, nx, ny)) {
                continue;
            }
            
            int new_g_cost = current.g_cost + 1;
            Node* neighbor = get_node(&open_list, nx, ny);
            
            if (neighbor == NULL) {
                // Add new node to open list
                if (open_list.count >= MAP_SIZE * MAP_SIZE) {
                    return 0; // List full
                }
                
                Node new_node = {0};
                new_node.pos.x = nx;
                new_node.pos.y = ny;
                new_node.g_cost = new_g_cost;
                new_node.h_cost = heuristic(nx, ny, end_x, end_y);
                new_node.f_cost = new_node.g_cost + new_node.h_cost;
                new_node.parent = current.pos;
                
                open_list.nodes[open_list.count++] = new_node;
            } else if (new_g_cost < neighbor->g_cost) {
                // Update existing node
                neighbor->g_cost = new_g_cost;
                neighbor->f_cost = neighbor->g_cost + neighbor->h_cost;
                neighbor->parent = current.pos;
            }
        }
    }
    
    // No path found
    return 0;
}

// Simple test function
void test_pathfinding(void) {
    Point path[MAX_PATH_LENGTH];
    int length = find_path(0, 0, 10, 10, path, MAX_PATH_LENGTH);
    
    printf("Pathfinding test: Found path of length %d\n", length);
    for (int i = 0; i < length && i < 20; i++) {
        printf("  (%d, %d)\n", path[i].x, path[i].y);
    }
}