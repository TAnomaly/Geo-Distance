#ifndef API_H
#define API_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <libpq-fe.h>
#include <microhttpd.h>
#include <json-c/json.h>
#include <h3/h3api.h>
#include <uuid/uuid.h>

#define CONN_STR "host=localhost dbname=location_sharing user=tugmirk password=tugmirk123 sslmode=disable"
#define PORT 8080
#define WEB_ROOT "/home/tugmirk/c_/prof/web"

// Function declarations
enum MHD_Result handle_request(void *cls __attribute__((unused)), struct MHD_Connection *connection,
                   const char *url, const char *method,
                   const char *version __attribute__((unused)), const char *upload_data,
                   size_t *upload_data_size, void **con_cls);

// Helper functions
char* read_file_content(const char *filepath);
const char* get_content_type(const char *url);
json_object* get_coordinates_from_db();
json_object* get_user_locations_from_db();
json_object* get_friends_locations(const char* user_id);

// User management functions
char* create_user(const char* username);
int save_user_location(const char* user_id, double lat, double lon, int accuracy);
int add_friend(const char* user_id, const char* friend_username);
json_object* get_user_friends(const char* user_id);
char* get_user_id_by_username(const char* username);

// New functions for friends and locations
json_object* get_friends_list(const char* user_id);
json_object* get_friends_locations_from_db(const char* user_id);

// New user authentication and session management functions
char* generate_session_token();
int validate_session_token(const char* session_token, char** user_id);
int hash_password(const char* password, char* hash_buffer, size_t buffer_size);
int verify_password(const char* password, const char* hash);
char* register_user(const char* username, const char* password);
char* login_user(const char* username, const char* password);
int logout_user(const char* session_token);

// H3 and A* distance calculation functions
double calculate_h3_distance(const char* user1_id, const char* user2_id);
double calculate_astar_distance(const char* user1_id, const char* user2_id);

#endif // API_H