#include "routing.h"
#include "../api.h"
#include "../location/location.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <libpq-fe.h>

// Calculate route between two points
json_object* calculate_route(double start_lat, double start_lon, const char* end_user_id) {
    if (!end_user_id) {
        return NULL;
    }
    
    // Get end user's location from database
    PGconn *conn = PQconnectdb(CONN_STR);
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Database connection failed: %s", PQerrorMessage(conn));
        PQfinish(conn);
        return NULL;
    }
    
    char query[512];
    snprintf(query, sizeof(query), 
             "SELECT ST_Y(location), ST_X(location) FROM user_locations WHERE user_id = '%s' ORDER BY updated_at DESC LIMIT 1;", end_user_id);
    
    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        PQclear(res);
        PQfinish(conn);
        return NULL;
    }
    
    double end_lat = atof(PQgetvalue(res, 0, 0));
    double end_lon = atof(PQgetvalue(res, 0, 1));
    PQclear(res);
    PQfinish(conn);
    
    // Convert coordinates to H3 indexes
    H3Index start_h3 = latlng_to_h3(start_lat, start_lon, 9);
    H3Index end_h3 = latlng_to_h3(end_lat, end_lon, 9);
    
    // Get A* path between the two H3 indexes
    H3Index *path = NULL;
    int pathSize = get_astar_path(start_h3, end_h3, &path);
    
    if (pathSize < 0) {
        return NULL;
    }
    
    // Calculate total distance along the path
    double totalDistance = 0.0;
    json_object *path_array = json_object_new_array();
    
    for (int i = 0; i < pathSize; i++) {
        LatLng coord;
        cellToLatLng(path[i], &coord);
        
        // Add point to path array
        json_object *point_obj = json_object_new_object();
        json_object_object_add(point_obj, "lat", json_object_new_double(radsToDegs(coord.lat)));
        json_object_object_add(point_obj, "lng", json_object_new_double(radsToDegs(coord.lng)));
        json_object_array_add(path_array, point_obj);
        
        // Calculate distance to next point
        if (i < pathSize - 1) {
            int64_t distance;
            gridDistance(path[i], path[i + 1], &distance);
            totalDistance += distance;
        }
    }
    
    // Create response with path and distance
    json_object *response_obj = json_object_new_object();
    json_object_object_add(response_obj, "path", path_array);
    json_object_object_add(response_obj, "distance", json_object_new_double(totalDistance * 1000.0)); // Convert to meters
    
    // Clean up
    free(path);
    
    return response_obj;
}

// Calculate H3 route distance
double calculate_h3_route_distance(H3Index start, H3Index end) {
    int64_t distance;
    gridDistance(start, end, &distance);
    return distance * 1000.0; // Convert to meters
}

// Get H3 route path
json_object* get_h3_route_path(H3Index start, H3Index end) {
    json_object *path_array = json_object_new_array();
    
    // For H3, we'll create a simple direct path
    LatLng start_coord, end_coord;
    cellToLatLng(start, &start_coord);
    cellToLatLng(end, &end_coord);
    
    // Add start point
    json_object *start_point = json_object_new_object();
    json_object_object_add(start_point, "lat", json_object_new_double(radsToDegs(start_coord.lat)));
    json_object_object_add(start_point, "lng", json_object_new_double(radsToDegs(start_coord.lng)));
    json_object_array_add(path_array, start_point);
    
    // Add end point
    json_object *end_point = json_object_new_object();
    json_object_object_add(end_point, "lat", json_object_new_double(radsToDegs(end_coord.lat)));
    json_object_object_add(end_point, "lng", json_object_new_double(radsToDegs(end_coord.lng)));
    json_object_array_add(path_array, end_point);
    
    return path_array;
}

// Calculate A* route distance
double calculate_astar_route_distance(H3Index start, H3Index end) {
    H3Index *path = NULL;
    int pathSize = get_astar_path(start, end, &path);
    
    if (pathSize < 0) {
        return -1;
    }
    
    double totalDistance = 0.0;
    for (int i = 0; i < pathSize - 1; i++) {
        totalDistance += h3_distance(path[i], path[i + 1]);
    }
    
    free(path);
    return totalDistance * 1000.0; // Convert to meters
}

// Get A* route path
json_object* get_astar_route_path(H3Index start, H3Index end) {
    H3Index *path = NULL;
    int pathSize = get_astar_path(start, end, &path);
    
    if (pathSize < 0) {
        return NULL;
    }
    
    json_object *path_array = json_object_new_array();
    
    for (int i = 0; i < pathSize; i++) {
        LatLng coord;
        cellToLatLng(path[i], &coord);
        
        json_object *point_obj = json_object_new_object();
        json_object_object_add(point_obj, "lat", json_object_new_double(radsToDegs(coord.lat)));
        json_object_object_add(point_obj, "lng", json_object_new_double(radsToDegs(coord.lng)));
        json_object_array_add(path_array, point_obj);
    }
    
    free(path);
    return path_array;
}

// Find nearby places using Kring algorithm
json_object* find_nearby_places(double lat, double lon, int radius_km) {
    // Convert to H3 index
    H3Index center = latlng_to_h3(lat, lon, 9);
    
    // Calculate k value based on radius
    // This is a simplified calculation - in practice you'd use more sophisticated logic
    int k = radius_km / 2; // Rough approximation
    if (k < 1) k = 1;
    if (k > 10) k = 10; // Limit to reasonable range
    
    return get_kring_cells(center, k);
}

// Get Kring cells around a center point
json_object* get_kring_cells(H3Index center, int k) {
    int64_t maxCells;
    maxGridRingSize(k, &maxCells);
    H3Index* kring = malloc(maxCells * sizeof(H3Index));
    
    if (!kring) {
        return NULL;
    }
    
    gridRing(center, k, kring);
    
    json_object *cells_array = json_object_new_array();
    
    for (int i = 0; i < maxCells; i++) {
        if (kring[i] == 0) break; // End of valid cells
        
        LatLng coord;
        cellToLatLng(kring[i], &coord);
        
        json_object *cell_obj = json_object_new_object();
        json_object_object_add(cell_obj, "h3_index", json_object_new_string("")); // You'd convert to string
        json_object_object_add(cell_obj, "lat", json_object_new_double(radsToDegs(coord.lat)));
        json_object_object_add(cell_obj, "lng", json_object_new_double(radsToDegs(coord.lng)));
        json_object_array_add(cells_array, cell_obj);
    }
    
    free(kring);
    return cells_array;
}
