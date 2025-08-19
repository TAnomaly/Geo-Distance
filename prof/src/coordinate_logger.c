#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <libpq-fe.h>
#include <h3/h3api.h>
#include "coordinate_logger.h"

#define CONN_STR "host=localhost dbname=mydb user=myuser password=mypassword"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Helper function to convert degrees to radians
static double to_radians(double degrees) {
    return degrees * M_PI / 180.0;
}

// Haversine distance calculation
double haversine_distance(double lat1, double lon1, double lat2, double lon2) {
    lat1 = to_radians(lat1);
    lon1 = to_radians(lon1);
    lat2 = to_radians(lat2);
    lon2 = to_radians(lon2);

    double dlat = lat2 - lat1;
    double dlon = lon2 - lon1;
    double a = pow(sin(dlat / 2), 2) + cos(lat1) * cos(lat2) * pow(sin(dlon / 2), 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    return 6371.0 * c; // EARTH_RADIUS
}

// Note: latlng_to_h3 function moved to location.c to avoid conflicts

// Get path between two H3 indexes using A* algorithm
int get_h3_path(H3Index start, H3Index end, H3Index **path) {
    // First get the size of the path
    int64_t pathSize;
    H3Error err = gridPathCellsSize(start, end, &pathSize);
    
    if (err != E_SUCCESS) {
        return -1; // Error occurred
    }
    
    // Allocate memory for the path
    *path = (H3Index*) malloc(pathSize * sizeof(H3Index));
    if (*path == NULL) {
        return -2; // Memory allocation failed
    }
    
    // Get the actual path
    err = gridPathCells(start, end, *path);
    if (err != E_SUCCESS) {
        free(*path);
        *path = NULL;
        return -1; // Error occurred
    }
    
    return (int)pathSize; // Return the size of the path
}

// A* pathfinding using H3 grid
typedef struct {
    H3Index index;
    double g_score;  // Cost from start to current node
    double f_score;  // Estimated total cost from start to goal through current node
} AStarNode;

// Simple priority queue implementation (for demonstration purposes)
typedef struct {
    AStarNode* nodes;
    int size;
    int capacity;
} PriorityQueue;

PriorityQueue* create_priority_queue(int capacity) {
    PriorityQueue* pq = malloc(sizeof(PriorityQueue));
    pq->nodes = malloc(capacity * sizeof(AStarNode));
    pq->size = 0;
    pq->capacity = capacity;
    return pq;
}

void destroy_priority_queue(PriorityQueue* pq) {
    free(pq->nodes);
    free(pq);
}

void push_priority_queue(PriorityQueue* pq, AStarNode node) {
    if (pq->size >= pq->capacity) return;
    
    pq->nodes[pq->size++] = node;
    
    // Simple bubble up for min-heap based on f_score
    int i = pq->size - 1;
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (pq->nodes[parent].f_score <= pq->nodes[i].f_score) break;
        AStarNode temp = pq->nodes[parent];
        pq->nodes[parent] = pq->nodes[i];
        pq->nodes[i] = temp;
        i = parent;
    }
}

AStarNode pop_priority_queue(PriorityQueue* pq) {
    if (pq->size <= 0) {
        AStarNode empty = {0, 0, 0};
        return empty;
    }
    
    AStarNode result = pq->nodes[0];
    pq->nodes[0] = pq->nodes[--pq->size];
    
    // Bubble down
    int i = 0;
    while (1) {
        int left = 2 * i + 1;
        int right = 2 * i + 2;
        int smallest = i;
        
        if (left < pq->size && pq->nodes[left].f_score < pq->nodes[smallest].f_score)
            smallest = left;
            
        if (right < pq->size && pq->nodes[right].f_score < pq->nodes[smallest].f_score)
            smallest = right;
            
        if (smallest == i) break;
        
        AStarNode temp = pq->nodes[i];
        pq->nodes[i] = pq->nodes[smallest];
        pq->nodes[smallest] = temp;
        i = smallest;
    }
    
    return result;
}

// Calculate distance between two H3 indexes using Haversine formula
double h3_distance(H3Index h1, H3Index h2) {
    LatLng coord1, coord2;
    cellToLatLng(h1, &coord1);
    cellToLatLng(h2, &coord2);
    
    // Convert to degrees for haversine calculation
    double lat1 = radsToDegs(coord1.lat);
    double lon1 = radsToDegs(coord1.lng);
    double lat2 = radsToDegs(coord2.lat);
    double lon2 = radsToDegs(coord2.lng);
    
    return haversine_distance(lat1, lon1, lat2, lon2);
}

// Note: get_astar_path function moved to location.c to avoid conflicts

// Save both locations and distance in a single row, including H3 indices
void save_location_pair_to_db(PGconn *conn, const char *name1, double lat1, double lon1, 
                              const char *name2, double lat2, double lon2, double distance) {
    // Get H3 indices for both locations (using resolution 9)
    LatLng location1, location2;
    location1.lat = degsToRads(lat1);
    location1.lng = degsToRads(lon1);
    location2.lat = degsToRads(lat2);
    location2.lng = degsToRads(lon2);
    
    H3Index h3_1, h3_2;
    latLngToCell(&location1, 9, &h3_1);
    latLngToCell(&location2, 9, &h3_2);
    
    // Convert H3 indices to strings
    char h3_1_str[17], h3_2_str[17];
    h3ToString(h3_1, h3_1_str, 17);
    h3ToString(h3_2, h3_2_str, 17);
    
    char query[1200];
    
    snprintf(query, sizeof(query),
             "INSERT INTO coordinates (first_name, first_lat, first_lon, first_h3, second_name, second_lat, second_lon, second_h3, distance) "
             "VALUES ('%s', %f, %f, '%s', '%s', %f, %f, '%s', %f);",
             name1, lat1, lon1, h3_1_str, name2, lat2, lon2, h3_2_str, distance);

    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Insert failed: %s", PQerrorMessage(conn));
    }
    PQclear(res);
}