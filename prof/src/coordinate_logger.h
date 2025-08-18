#ifndef COORDINATE_LOGGER_H
#define COORDINATE_LOGGER_H

#include <libpq-fe.h>

// Function declarations
double haversine_distance(double lat1, double lon1, double lat2, double lon2);
void save_location_pair_to_db(PGconn *conn, const char *name1, double lat1, double lon1, 
                              const char *name2, double lat2, double lon2, double distance);

#endif // COORDINATE_LOGGER_H