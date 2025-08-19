#ifndef API_SERVER_H
#define API_SERVER_H

#include <microhttpd.h>

// Start the API server
struct MHD_Daemon* start_api_server(void);

// Handle HTTP requests
enum MHD_Result handle_request(void *cls, struct MHD_Connection *connection,
                              const char *url, const char *method,
                              const char *version, const char *upload_data,
                              size_t *upload_data_size, void **con_cls);

// Request handler function prototypes
enum MHD_Result handle_get_request(struct MHD_Connection *connection, const char *url);
enum MHD_Result handle_post_request(struct MHD_Connection *connection, const char *url, 
                                   const char *upload_data, size_t *upload_data_size, void **con_cls);
enum MHD_Result process_post_data(struct MHD_Connection *connection, const char *url, 
                                 const char *post_data, size_t post_data_size);
enum MHD_Result handle_options_request(struct MHD_Connection *connection);
enum MHD_Result serve_static_file(struct MHD_Connection *connection, const char *filepath, const char *content_type);

// POST request handlers
enum MHD_Result handle_post_register(struct MHD_Connection *connection, const char *post_data, size_t post_data_size);
enum MHD_Result handle_post_login(struct MHD_Connection *connection, const char *post_data, size_t post_data_size);
enum MHD_Result handle_post_logout(struct MHD_Connection *connection);
enum MHD_Result handle_post_save_location(struct MHD_Connection *connection, const char *post_data, size_t post_data_size);
enum MHD_Result handle_post_add_friend(struct MHD_Connection *connection, const char *post_data, size_t post_data_size);
enum MHD_Result handle_post_calculate_distance(struct MHD_Connection *connection, const char *post_data, size_t post_data_size);

// GET request handlers
enum MHD_Result handle_get_user_info(struct MHD_Connection *connection);
enum MHD_Result handle_get_friends(struct MHD_Connection *connection);
enum MHD_Result handle_get_friends_locations(struct MHD_Connection *connection);
enum MHD_Result handle_get_route(struct MHD_Connection *connection);
enum MHD_Result handle_get_h3_distance(struct MHD_Connection *connection);
enum MHD_Result handle_get_astar_distance(struct MHD_Connection *connection);

#endif // API_SERVER_H
