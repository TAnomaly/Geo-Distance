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

// Convert latitude/longitude to H3 index
H3Index latlng_to_h3(double lat, double lon, int resolution) {
    LatLng location;
    location.lat = degsToRads(lat);
    location.lng = degsToRads(lon);
    
    H3Index index = 0;
    H3Error err = latLngToCell(&location, resolution, &index);
    
    // Return 0 (H3_NULL) if there was an error
    if (err != E_SUCCESS) {
        return H3_NULL;
    }
    
    return index;
}

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

// Save both locations and distance in a single row, including H3 indices
void save_location_pair_to_db(PGconn *conn, const char *name1, double lat1, double lon1, 
                              const char *name2, double lat2, double lon2, double distance) {
    // Get H3 indices for both locations (using resolution 9)
    H3Index h3_1 = latlng_to_h3(lat1, lon1, 9);
    H3Index h3_2 = latlng_to_h3(lat2, lon2, 9);
    
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