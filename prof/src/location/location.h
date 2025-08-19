#ifndef LOCATION_H
#define LOCATION_H

#include <json-c/json.h>
#include <h3/h3api.h>

// Location management functions
int save_user_location(const char* user_id, double latitude, double longitude, int accuracy);
json_object* get_user_locations_from_db(void);
json_object* get_friends_locations_from_db(const char* user_id);

// Distance calculation functions
double calculate_h3_distance(const char* user1_id, const char* user2_id);
double calculate_astar_distance(const char* user1_id, const char* user2_id);

// H3 utility functions
H3Index latlng_to_h3(double lat, double lng, int resolution);
int get_astar_path(H3Index start, H3Index end, H3Index** path);

#endif // LOCATION_H
