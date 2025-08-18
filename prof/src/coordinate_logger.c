#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <libpq-fe.h>
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

// Save both locations and distance in a single row
void save_location_pair_to_db(PGconn *conn, const char *name1, double lat1, double lon1, 
                              const char *name2, double lat2, double lon2, double distance) {
    char query[1024];
    
    snprintf(query, sizeof(query),
             "INSERT INTO coordinates (first_name, first_lat, first_lon, second_name, second_lat, second_lon, distance) "
             "VALUES ('%s', %f, %f, '%s', %f, %f, %f);",
             name1, lat1, lon1, name2, lat2, lon2, distance);

    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Insert failed: %s", PQerrorMessage(conn));
    }
    PQclear(res);
}