#ifndef ROUTING_H
#define ROUTING_H

#include <json-c/json.h>
#include <h3/h3api.h>

// Route calculation functions
json_object* calculate_route(double start_lat, double start_lon, const char* end_user_id);

// H3 routing functions
double calculate_h3_route_distance(H3Index start, H3Index end);
json_object* get_h3_route_path(H3Index start, H3Index end);

// A* routing functions
double calculate_astar_route_distance(H3Index start, H3Index end);
json_object* get_astar_route_path(H3Index start, H3Index end);

// Kring routing functions (for finding nearby places)
json_object* find_nearby_places(double lat, double lon, int radius_km);
json_object* get_kring_cells(H3Index center, int k);

#endif // ROUTING_H
