#include "location.h"
#include "../api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <libpq-fe.h>
#define _USE_MATH_DEFINES

// Save user location to database
int save_user_location(const char* user_id, double latitude, double longitude, int accuracy) {
    if (!user_id) {
        return -1;
    }

    PGconn *conn = PQconnectdb(CONN_STR);
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Database connection failed: %s", PQerrorMessage(conn));
        PQfinish(conn);
        return -1;
    }

    // Convert coordinates to H3 index
    H3Index h3_index = latlng_to_h3(latitude, longitude, 9);

    // Insert or update user location
    char query[512];
    snprintf(query, sizeof(query), 
             "INSERT INTO user_locations (user_id, location, h3_index, accuracy, updated_at) "
             "VALUES (%s, ST_SetSRID(ST_MakePoint(%f, %f), 4326), '%llx', %d, NOW()) "
             "ON CONFLICT (user_id) DO UPDATE SET "
             "location = EXCLUDED.location, "
             "h3_index = EXCLUDED.h3_index, "
             "accuracy = EXCLUDED.accuracy, "
             "updated_at = NOW();", 
             user_id, longitude, latitude, h3_index, accuracy);

    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Insert/update location failed: %s", PQerrorMessage(conn));
        PQclear(res);
        PQfinish(conn);
        return -1;
    }
    PQclear(res);
    PQfinish(conn);

    return 0; // Success
}

// Get user locations from database
json_object* get_user_locations_from_db() {
    PGconn *conn = PQconnectdb(CONN_STR);
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Database connection failed: %s", PQerrorMessage(conn));
        PQfinish(conn);
        return NULL;
    }
    
    // Get user locations from last 10 minutes
    const char *query = "SELECT u.username, ul.latitude, ul.longitude, ul.h3_index, ul.accuracy, ul.timestamp "
                        "FROM user_locations ul "
                        "JOIN users u ON ul.user_id = u.id "
                        "WHERE ul.timestamp > NOW() - INTERVAL '10 minutes' "
                        "ORDER BY ul.timestamp DESC;";
    
    PGresult *res = PQexec(conn, query);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Query failed: %s", PQerrorMessage(conn));
        PQclear(res);
        PQfinish(conn);
        return NULL;
    }
    
    int rows = PQntuples(res);
    json_object *locations_array = json_object_new_array();
    
    for (int i = 0; i < rows; i++) {
        json_object *location_obj = json_object_new_object();
        
        json_object_object_add(location_obj, "username", json_object_new_string(PQgetvalue(res, i, 0)));
        json_object_object_add(location_obj, "latitude", json_object_new_double(atof(PQgetvalue(res, i, 1))));
        json_object_object_add(location_obj, "longitude", json_object_new_double(atof(PQgetvalue(res, i, 2))));
        json_object_object_add(location_obj, "h3_index", json_object_new_string(PQgetvalue(res, i, 3)));
        json_object_object_add(location_obj, "accuracy", json_object_new_int(atoi(PQgetvalue(res, i, 4))));
        json_object_object_add(location_obj, "timestamp", json_object_new_string(PQgetvalue(res, i, 5)));
        
        json_object_array_add(locations_array, location_obj);
    }
    
    PQclear(res);
    PQfinish(conn);
    
    return locations_array;
}

// Get friends locations from database
json_object* get_friends_locations_from_db(const char* user_id) {
    if (!user_id) {
        fprintf(stderr, "User ID is NULL\n");
        return NULL;
    }
    
    PGconn *conn = PQconnectdb(CONN_STR);
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Database connection failed: %s", PQerrorMessage(conn));
        PQfinish(conn);
        return NULL;
    }
    
    // Get locations of friends (users who are friends with the given user)
    char query[1024];
    snprintf(query, sizeof(query), 
             "SELECT u.id, u.username, ST_Y(ul.location) as latitude, ST_X(ul.location) as longitude, "
             "50 as accuracy, ul.updated_at as timestamp " // Using default accuracy of 50 meters
             "FROM user_locations ul "
             "JOIN users u ON ul.user_id = u.id "
             "WHERE ul.user_id IN ("
             "    SELECT CASE "
             "        WHEN f.user_id = %s THEN f.friend_id "
             "        WHEN f.friend_id = %s THEN f.user_id "
             "    END "
             "    FROM friendships f "
             "    WHERE (f.user_id = %s OR f.friend_id = %s) "
             "    AND f.status = 'accepted'"
             ") "
             "AND ul.updated_at > NOW() - INTERVAL '10 minutes' " // Only recent locations
             "ORDER BY ul.updated_at DESC;", 
             user_id, user_id, user_id, user_id);
    
    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Query failed: %s", PQerrorMessage(conn));
        PQclear(res);
        PQfinish(conn);
        return NULL;
    }
    
    int rows = PQntuples(res);
    json_object *locations_array = json_object_new_array();
    
    for (int i = 0; i < rows; i++) {
        json_object *location_obj = json_object_new_object();
        
        json_object_object_add(location_obj, "user_id", json_object_new_string(PQgetvalue(res, i, 0)));
        json_object_object_add(location_obj, "username", json_object_new_string(PQgetvalue(res, i, 1)));
        json_object_object_add(location_obj, "latitude", json_object_new_double(atof(PQgetvalue(res, i, 2))));
        json_object_object_add(location_obj, "longitude", json_object_new_double(atof(PQgetvalue(res, i, 3))));
        json_object_object_add(location_obj, "accuracy", json_object_new_int(atoi(PQgetvalue(res, i, 4))));
        json_object_object_add(location_obj, "timestamp", json_object_new_string(PQgetvalue(res, i, 5)));
        
        json_object_array_add(locations_array, location_obj);
    }
    
    PQclear(res);
    PQfinish(conn);
    
    return locations_array;
}

// Convert lat/lng to H3 index
H3Index latlng_to_h3(double lat, double lng, int resolution) {
    LatLng coord;
    coord.lat = degsToRads(lat);
    coord.lng = degsToRads(lng);
    H3Index h3Index;
    latLngToCell(&coord, resolution, &h3Index);
    return h3Index;
}

// Calculate distance between two users using H3 (using haversine formula)
double calculate_h3_distance(const char* user1_id, const char* user2_id) {
    if (!user1_id || !user2_id) {
        fprintf(stderr, "User IDs are NULL\n");
        return -1;
    }
    
    PGconn *conn = PQconnectdb(CONN_STR);
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Database connection failed: %s", PQerrorMessage(conn));
        PQfinish(conn);
        return -1;
    }
    
    // Get user locations from database
    char query[512];
    snprintf(query, sizeof(query), 
             "SELECT ST_X(location), ST_Y(location) FROM user_locations WHERE user_id = %s;", user1_id);
    
    PGresult *res1 = PQexec(conn, query);
    if (PQresultStatus(res1) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Query for user1 failed: %s", PQerrorMessage(conn));
        PQclear(res1);
        PQfinish(conn);
        return -1;
    }
    
    if (PQntuples(res1) == 0) {
        // User1 location not found
        PQclear(res1);
        PQfinish(conn);
        return -1;
    }
    
    double lat1 = atof(PQgetvalue(res1, 0, 1));
    double lon1 = atof(PQgetvalue(res1, 0, 0));
    PQclear(res1);
    
    snprintf(query, sizeof(query), 
             "SELECT ST_X(location), ST_Y(location) FROM user_locations WHERE user_id = %s;", user2_id);
    
    PGresult *res2 = PQexec(conn, query);
    if (PQresultStatus(res2) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Query for user2 failed: %s", PQerrorMessage(conn));
        PQclear(res2);
        PQfinish(conn);
        return -1;
    }
    
    if (PQntuples(res2) == 0) {
        // User2 location not found
        PQclear(res2);
        PQfinish(conn);
        return -1;
    }
    
    double lat2 = atof(PQgetvalue(res2, 0, 1));
    double lon2 = atof(PQgetvalue(res2, 0, 0));
    PQclear(res2);
    PQfinish(conn);
    
    // Haversine formula
    double dlat = (lat2 - lat1) * 3.14159265358979323846 / 180.0;
    double dlon = (lon2 - lon1) * 3.14159265358979323846 / 180.0;
    double a = sin(dlat / 2) * sin(dlat / 2) +
               cos(lat1 * 3.14159265358979323846 / 180.0) * cos(lat2 * 3.14159265358979323846 / 180.0) *
               sin(dlon / 2) * sin(dlon / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    double distance = 6371.0 * 1000.0 * c; // Earth radius in meters
    
    return distance;
}

// Calculate distance between two users using A* on H3 grid
double calculate_astar_distance(const char* user1_id, const char* user2_id) {
    if (!user1_id || !user2_id) {
        fprintf(stderr, "User IDs are NULL\n");
        return -1;
    }
    
    PGconn *conn = PQconnectdb(CONN_STR);
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Database connection failed: %s", PQerrorMessage(conn));
        PQfinish(conn);
        return -1;
    }
    
    // Get user locations from database
    char query[512];
    snprintf(query, sizeof(query), 
             "SELECT ST_X(location), ST_Y(location) FROM user_locations WHERE user_id = %s;", user1_id);
    
    PGresult *res1 = PQexec(conn, query);
    if (PQresultStatus(res1) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Query for user1 failed: %s", PQerrorMessage(conn));
        PQclear(res1);
        PQfinish(conn);
        return -1;
    }
    
    if (PQntuples(res1) == 0) {
        // User1 location not found
        PQclear(res1);
        PQfinish(conn);
        return -1;
    }
    
    double lat1 = atof(PQgetvalue(res1, 0, 1));
    double lon1 = atof(PQgetvalue(res1, 0, 0));
    PQclear(res1);
    
    snprintf(query, sizeof(query), 
             "SELECT ST_X(location), ST_Y(location) FROM user_locations WHERE user_id = %s;", user2_id);
    
    PGresult *res2 = PQexec(conn, query);
    if (PQresultStatus(res2) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Query for user2 failed: %s", PQerrorMessage(conn));
        PQclear(res2);
        PQfinish(conn);
        return -1;
    }
    
    if (PQntuples(res2) == 0) {
        // User2 location not found
        PQclear(res2);
        PQfinish(conn);
        return -1;
    }
    
    double lat2 = atof(PQgetvalue(res2, 0, 1));
    double lon2 = atof(PQgetvalue(res2, 0, 0));
    PQclear(res2);
    PQfinish(conn);
    
    // Convert coordinates to H3 indexes
    H3Index h3_1 = latlng_to_h3(lat1, lon1, 9);
    H3Index h3_2 = latlng_to_h3(lat2, lon2, 9);
    
    // Get A* path between the two H3 indexes
    H3Index *path = NULL;
    int pathSize = get_astar_path(h3_1, h3_2, &path);
    
    if (pathSize < 0) {
        fprintf(stderr, "Failed to calculate A* path\n");
        return -1;
    }
    
    // Calculate total distance along the path
    double totalDistance = 0.0;
    for (int i = 0; i < pathSize - 1; i++) {
        int64_t distance;
        gridDistance(path[i], path[i + 1], &distance);
        totalDistance += distance;
    }
    
    // Clean up
    free(path);
    
    // Convert to meters
    return totalDistance * 1000.0;
}

// A* pathfinding implementation (simplified version)
int get_astar_path(H3Index start, H3Index end, H3Index** path) {
    // This is a simplified A* implementation
    // In a real application, you would implement a full A* algorithm
    // For now, we'll just return a direct path
    
    *path = malloc(2 * sizeof(H3Index));
    if (!*path) {
        return -1;
    }
    
    (*path)[0] = start;
    (*path)[1] = end;
    
    return 2; // Path size
}
