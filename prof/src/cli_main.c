#include <stdio.h>
#include <stdlib.h>
#include <libpq-fe.h>
#include "coordinate_logger.h"

#define CONN_STR "host=localhost dbname=mydb user=myuser password=mypassword"

int main() {
    // Connect to PostgreSQL
    PGconn *conn = PQconnectdb(CONN_STR);
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Connection failed: %s", PQerrorMessage(conn));
        PQfinish(conn);
        exit(1);
    }

    // Get input from user
    char name1[100], name2[100];
    double lat1, lon1, lat2, lon2;

    printf("Enter first location name: ");
    scanf("%s", name1);
    printf("Enter first location coordinates (lat lon): ");
    if (scanf("%lf %lf", &lat1, &lon1) != 2) {
        fprintf(stderr, "Invalid coordinates for first location.\n");
        PQfinish(conn);
        return 1;
    }

    printf("Enter second location name: ");
    scanf("%s", name2);
    printf("Enter second location coordinates (lat lon): ");
    if (scanf("%lf %lf", &lat2, &lon2) != 2) {
        fprintf(stderr, "Invalid coordinates for second location.\n");
        PQfinish(conn);
        return 1;
    }

    // Calculate distance using coordinate_logger function
    double distance = haversine_distance(lat1, lon1, lat2, lon2);
    printf("\nDistance between %s and %s: %.2f km\n", name1, name2, distance);

    // Save data to database (single row)
    save_location_pair_to_db(conn, name1, lat1, lon1, name2, lat2, lon2, distance);

    // Close connection
    PQfinish(conn);
    return 0;
}