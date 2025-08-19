#define _GNU_SOURCE
#include "api_server.h"
#include "api.h"
#include "auth/auth.h"
#include "location/location.h"
#include "routing/routing.h"
#include "utils/utils.h"
#include "coordinate_logger.h"
#include <json-c/json.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <time.h>

// Start the API server
struct MHD_Daemon* start_api_server(void) {
    printf("DEBUG: Starting API server on port %d\n", PORT);
    fflush(stdout);
    
    struct MHD_Daemon* daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD, PORT, NULL, NULL,
                           &handle_request, NULL, MHD_OPTION_END);
    
    if (daemon == NULL) {
        fprintf(stderr, "DEBUG: MHD_start_daemon failed\n");
        fflush(stderr);
    } else {
        printf("DEBUG: API server started successfully\n");
        fflush(stdout);
    }
    
    return daemon;
}
 
 
 
 // Handle HTTP requests
enum MHD_Result handle_request(void *cls __attribute__((unused)), struct MHD_Connection *connection,
                   const char *url, const char *method,
                   const char *version __attribute__((unused)), const char *upload_data,
                   size_t *upload_data_size, void **con_cls) {
    
    // Debug print
    printf("Received request: %s %s\n", method, url);
    fflush(stdout);
    
    // Handle GET requests
    if (strcmp(method, "GET") == 0) {
        return handle_get_request(connection, url);
    }
    
    // Handle POST requests
    if (strcmp(method, "POST") == 0) {
        return handle_post_request(connection, url, upload_data, upload_data_size, con_cls);
    }
    
    // Handle OPTIONS requests for CORS preflight
    if (strcmp(method, "OPTIONS") == 0) {
        return handle_options_request(connection);
    }
    
    // Method not allowed
    struct MHD_Response *response = create_error_response("Method not allowed", MHD_HTTP_METHOD_NOT_ALLOWED);
    enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_METHOD_NOT_ALLOWED, response);
                MHD_destroy_response(response);
                return ret;
            }
            
// Handle GET requests
enum MHD_Result handle_get_request(struct MHD_Connection *connection, const char *url) {
    // Serve index.html for root path
    if (strcmp(url, "/") == 0) {
        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s/index.html", WEB_ROOT);
        return serve_static_file(connection, filepath, "text/html");
    }
    
    // API endpoints
        if (strcmp(url, "/api/user") == 0) {
        return handle_get_user_info(connection);
    }
    
    if (strcmp(url, "/api/friends") == 0) {
        return handle_get_friends(connection);
    }
    
    if (strcmp(url, "/api/friends/locations") == 0) {
        return handle_get_friends_locations(connection);
    }
    
    if (strcmp(url, "/api/route") == 0) {
        return handle_get_route(connection);
    }
    
    if (strcmp(url, "/api/distance/h3") == 0) {
        return handle_get_h3_distance(connection);
    }
    
    if (strcmp(url, "/api/distance/astar") == 0) {
        return handle_get_astar_distance(connection);
    }
    
    // Serve static files from web directory
    if (strncmp(url, "/web/", 5) == 0) {
        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s%s", WEB_ROOT, url + 4); // Remove /web prefix
        return serve_static_file(connection, filepath, get_content_type(url));
    }
    
    // Not found
    struct MHD_Response *response = create_error_response("Not found", MHD_HTTP_NOT_FOUND);
    enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_NOT_FOUND, response);
                MHD_destroy_response(response);
                return ret;
            }
            




// Handle POST requests
enum MHD_Result handle_post_request(struct MHD_Connection *connection, const char *url, 
                                   const char *upload_data, size_t *upload_data_size, void **con_cls) {
    printf("DEBUG: POST request - upload_data_size: %zu, con_cls: %p\n", *upload_data_size, (void*)*con_cls);
    
    // For now, just return a success response for testing
    printf("DEBUG: Returning test success response\n");
    
    const char *response_text = "{\"success\": \"Test response\"}";
    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(response_text), 
                                                                   (void *)response_text, 
                                                                   MHD_RESPMEM_PERSISTENT);
    MHD_add_response_header(response, "Content-Type", "application/json");
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
    MHD_add_response_header(response, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    MHD_add_response_header(response, "Access-Control-Allow-Headers", "Content-Type, Authorization");
    MHD_add_response_header(response, "Access-Control-Allow-Credentials", "true");
    
    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    printf("DEBUG: Queued test response, result: %d\n", ret);
    
    return ret;
}

// Process complete POST data
enum MHD_Result process_post_data(struct MHD_Connection *connection, const char *url, 
                                 const char *post_data, size_t post_data_size) {
    // API endpoints
    if (strcmp(url, "/api/register") == 0) {
        return handle_post_register(connection, post_data, post_data_size);
    }
    
    if (strcmp(url, "/api/login") == 0) {
        return handle_post_login(connection, post_data, post_data_size);
    }
    
    if (strcmp(url, "/api/logout") == 0) {
        return handle_post_logout(connection);
    }
    
    if (strcmp(url, "/api/save-location") == 0) {
        return handle_post_save_location(connection, post_data, post_data_size);
    }
    
    if (strcmp(url, "/api/add-friend") == 0) {
        return handle_post_add_friend(connection, post_data, post_data_size);
    }
    
    if (strcmp(url, "/calculate-distance") == 0) {
        return handle_post_calculate_distance(connection, post_data, post_data_size);
    }
    
    // Not found
    struct MHD_Response *response = create_error_response("Not found", MHD_HTTP_NOT_FOUND);
    enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_NOT_FOUND, response);
                    MHD_destroy_response(response);
                    return ret;
                }
                
// Handle OPTIONS requests
enum MHD_Result handle_options_request(struct MHD_Connection *connection) {
    struct MHD_Response *response = MHD_create_response_from_buffer(0, NULL, MHD_RESPMEM_PERSISTENT);
    enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_OK, response);
                MHD_destroy_response(response);
                return ret;
}

// Serve static file
enum MHD_Result serve_static_file(struct MHD_Connection *connection, const char *filepath, const char *content_type) {
            char *content = read_file_content(filepath);
            if (!content) {
        struct MHD_Response *response = create_error_response("File not found", MHD_HTTP_NOT_FOUND);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_NOT_FOUND, response);
                MHD_destroy_response(response);
                return ret;
            }
            
            struct MHD_Response *response = MHD_create_response_from_buffer(strlen(content), 
                                                                           (void *)content, 
                                                                           MHD_RESPMEM_MUST_FREE);
    MHD_add_response_header(response, "Content-Type", content_type);
    enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_OK, response);
            MHD_destroy_response(response);
            return ret;
        }

// Handle user registration
enum MHD_Result handle_post_register(struct MHD_Connection *connection, const char *post_data, size_t post_data_size) {
    if (!post_data || post_data_size == 0) {
        struct MHD_Response *response = create_error_response("No data received", MHD_HTTP_BAD_REQUEST);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_BAD_REQUEST, response);
        MHD_destroy_response(response);
        return ret;
    }
    
    printf("DEBUG: Received post_data: %.*s\n", (int)post_data_size, post_data);
            json_object *json_obj = json_tokener_parse(post_data);
    printf("DEBUG: JSON parse result: %p\n", (void*)json_obj);
            if (!json_obj) {
        struct MHD_Response *response = create_error_response("Invalid JSON", MHD_HTTP_BAD_REQUEST);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_BAD_REQUEST, response);
                MHD_destroy_response(response);
                return ret;
            }
            
            json_object *username_obj, *password_obj;
            if (!json_object_object_get_ex(json_obj, "username", &username_obj) ||
                !json_object_object_get_ex(json_obj, "password", &password_obj)) {
        struct MHD_Response *response = create_error_response("Username and password required", MHD_HTTP_BAD_REQUEST);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_BAD_REQUEST, response);
                MHD_destroy_response(response);
                json_object_put(json_obj);
                return ret;
            }
            
            const char* username = json_object_get_string(username_obj);
            const char* password = json_object_get_string(password_obj);
            
            char* user_id = register_user(username, password);
    printf("DEBUG: register_user returned: %s\n", user_id ? user_id : "NULL");
            
            if (!user_id) {
        // Check if it's because user already exists
        PGconn *conn = PQconnectdb(CONN_STR);
        if (PQstatus(conn) == CONNECTION_OK) {
            char query[512];
            snprintf(query, sizeof(query), 
                     "SELECT id FROM users WHERE username = '%s';", username);
            
            PGresult *res = PQexec(conn, query);
            if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
                // User already exists
                struct MHD_Response *response = create_error_response("Username already exists", MHD_HTTP_CONFLICT);
                enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_CONFLICT, response);
                MHD_destroy_response(response);
                PQclear(res);
                PQfinish(conn);
                json_object_put(json_obj);
                return ret;
            }
            PQclear(res);
            PQfinish(conn);
        }
        
        struct MHD_Response *response = create_error_response("Failed to register user", MHD_HTTP_INTERNAL_SERVER_ERROR);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
        MHD_destroy_response(response);
        json_object_put(json_obj);
        return ret;
    }
            
                // Create success response
    printf("DEBUG: Creating success response\n");
    const char *response_text = "{\"success\": \"User registered successfully\"}";
    
    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(response_text), 
                                                                   (void *)response_text, 
                                                                   MHD_RESPMEM_PERSISTENT);
    MHD_add_response_header(response, "Content-Type", "application/json");
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
    MHD_add_response_header(response, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    MHD_add_response_header(response, "Access-Control-Allow-Headers", "Content-Type, Authorization");
    MHD_add_response_header(response, "Access-Control-Allow-Credentials", "true");
    
    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    printf("DEBUG: Queued success response, result: %d\n", ret);
    
    // Clean up
    json_object_put(json_obj);
    free(user_id);
    
    // Return the result from MHD_queue_response
    return ret;
    }
    
// Handle user login
enum MHD_Result handle_post_login(struct MHD_Connection *connection, const char *post_data, size_t post_data_size) {
            json_object *json_obj = json_tokener_parse(post_data);
            if (!json_obj) {
        struct MHD_Response *response = create_error_response("Invalid JSON", MHD_HTTP_BAD_REQUEST);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_BAD_REQUEST, response);
                MHD_destroy_response(response);
                return ret;
            }
            
            json_object *username_obj, *password_obj;
            if (!json_object_object_get_ex(json_obj, "username", &username_obj) ||
                !json_object_object_get_ex(json_obj, "password", &password_obj)) {
        struct MHD_Response *response = create_error_response("Username and password required", MHD_HTTP_BAD_REQUEST);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_BAD_REQUEST, response);
                MHD_destroy_response(response);
                json_object_put(json_obj);
                return ret;
            }
            
            const char* username = json_object_get_string(username_obj);
            const char* password = json_object_get_string(password_obj);
            
            char* session_token = login_user(username, password);
            
            if (!session_token) {
        struct MHD_Response *response = create_error_response("Invalid username or password", MHD_HTTP_UNAUTHORIZED);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_UNAUTHORIZED, response);
                MHD_destroy_response(response);
                json_object_put(json_obj);
                return ret;
            }
            
            // Create JSON response
            json_object *response_obj = json_object_new_object();
            json_object_object_add(response_obj, "session_token", json_object_new_string(session_token));
            
            const char *json_str = json_object_to_json_string(response_obj);
    struct MHD_Response *response = create_json_response(json_str, MHD_HTTP_OK);
    enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_OK, response);
            MHD_destroy_response(response);
            json_object_put(response_obj);
            json_object_put(json_obj);
            free(session_token);
            return ret;
    }
    
// Handle user logout
enum MHD_Result handle_post_logout(struct MHD_Connection *connection) {
        const char* session_token = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Authorization");
        if (!session_token || strncmp(session_token, "Bearer ", 7) != 0) {
        struct MHD_Response *response = create_error_response("Missing or invalid Authorization header", MHD_HTTP_UNAUTHORIZED);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_UNAUTHORIZED, response);
            MHD_destroy_response(response);
            return ret;
        }
        
    session_token += 7; // Skip "Bearer " prefix
        
        if (logout_user(session_token) != 0) {
        struct MHD_Response *response = create_error_response("Failed to logout", MHD_HTTP_INTERNAL_SERVER_ERROR);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
            MHD_destroy_response(response);
            return ret;
        }
        
    struct MHD_Response *response = create_success_response("Logged out successfully", MHD_HTTP_OK);
    enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    }
    
// Handle save location
enum MHD_Result handle_post_save_location(struct MHD_Connection *connection, const char *post_data, size_t post_data_size) {
    json_object *json_obj = json_tokener_parse(post_data);
    if (!json_obj) {
        struct MHD_Response *response = create_error_response("Invalid JSON", MHD_HTTP_BAD_REQUEST);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_BAD_REQUEST, response);
            MHD_destroy_response(response);
            return ret;
        }
        
    json_object *user_id_obj, *lat_obj, *lon_obj, *accuracy_obj;
    if (!json_object_object_get_ex(json_obj, "user_id", &user_id_obj) ||
        !json_object_object_get_ex(json_obj, "latitude", &lat_obj) ||
        !json_object_object_get_ex(json_obj, "longitude", &lon_obj)) {
        struct MHD_Response *response = create_error_response("User ID, latitude and longitude required", MHD_HTTP_BAD_REQUEST);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_BAD_REQUEST, response);
            MHD_destroy_response(response);
        json_object_put(json_obj);
            return ret;
        }
        
    const char* user_id = json_object_get_string(user_id_obj);
    double latitude = json_object_get_double(lat_obj);
    double longitude = json_object_get_double(lon_obj);
    int accuracy = 50; // Default accuracy
    
    if (json_object_object_get_ex(json_obj, "accuracy", &accuracy_obj)) {
        accuracy = json_object_get_int(accuracy_obj);
    }
    
    int result = save_user_location(user_id, latitude, longitude, accuracy);
    
    if (result != 0) {
        struct MHD_Response *response = create_error_response("Failed to save location", MHD_HTTP_INTERNAL_SERVER_ERROR);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
            MHD_destroy_response(response);
        json_object_put(json_obj);
            return ret;
        }
        
    struct MHD_Response *response = create_success_response("Location saved successfully", MHD_HTTP_OK);
    enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    json_object_put(json_obj);
    return ret;
}

// Handle add friend
enum MHD_Result handle_post_add_friend(struct MHD_Connection *connection, const char *post_data, size_t post_data_size) {
    json_object *json_obj = json_tokener_parse(post_data);
    if (!json_obj) {
        struct MHD_Response *response = create_error_response("Invalid JSON", MHD_HTTP_BAD_REQUEST);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_BAD_REQUEST, response);
            MHD_destroy_response(response);
            return ret;
        }
        
    json_object *user_id_obj, *friend_username_obj;
    if (!json_object_object_get_ex(json_obj, "user_id", &user_id_obj) ||
        !json_object_object_get_ex(json_obj, "friend_username", &friend_username_obj)) {
        struct MHD_Response *response = create_error_response("User ID and friend username required", MHD_HTTP_BAD_REQUEST);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_BAD_REQUEST, response);
        MHD_destroy_response(response);
        json_object_put(json_obj);
        return ret;
    }
    
    const char* user_id = json_object_get_string(user_id_obj);
    const char* friend_username = json_object_get_string(friend_username_obj);
    
    int result = add_friend(user_id, friend_username);
    
    if (result != 0) {
        struct MHD_Response *response = create_error_response("Failed to add friend", MHD_HTTP_INTERNAL_SERVER_ERROR);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
        MHD_destroy_response(response);
        json_object_put(json_obj);
        return ret;
    }
    
    struct MHD_Response *response = create_success_response("Friend added successfully", MHD_HTTP_OK);
    enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    json_object_put(json_obj);
    return ret;
}

// Handle get user info
enum MHD_Result handle_get_user_info(struct MHD_Connection *connection) {
        const char* session_token = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Authorization");
        if (!session_token || strncmp(session_token, "Bearer ", 7) != 0) {
        struct MHD_Response *response = create_error_response("Missing or invalid Authorization header", MHD_HTTP_UNAUTHORIZED);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_UNAUTHORIZED, response);
            MHD_destroy_response(response);
            return ret;
        }
        
    session_token += 7; // Skip "Bearer " prefix
        
        char* user_id = NULL;
        if (validate_session_token(session_token, &user_id) != 0) {
        struct MHD_Response *response = create_error_response("Invalid or expired session token", MHD_HTTP_UNAUTHORIZED);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_UNAUTHORIZED, response);
            MHD_destroy_response(response);
            return ret;
        }
        
    // Get user info from database
    PGconn *conn = PQconnectdb(CONN_STR);
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Database connection failed: %s", PQerrorMessage(conn));
        PQfinish(conn);
            free(user_id);
        struct MHD_Response *response = create_error_response("Database connection failed", MHD_HTTP_INTERNAL_SERVER_ERROR);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
        MHD_destroy_response(response);
            return ret;
        }
        
    char query[512];
    snprintf(query, sizeof(query), 
             "SELECT username FROM users WHERE id = %s;", user_id);
    
    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Query failed: %s", PQerrorMessage(conn));
        PQclear(res);
        PQfinish(conn);
        free(user_id);
        struct MHD_Response *response = create_error_response("Failed to retrieve user info", MHD_HTTP_INTERNAL_SERVER_ERROR);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
            MHD_destroy_response(response);
        return ret;
    }
    
    if (PQntuples(res) == 0) {
        PQclear(res);
        PQfinish(conn);
            free(user_id);
        struct MHD_Response *response = create_error_response("User not found", MHD_HTTP_NOT_FOUND);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_NOT_FOUND, response);
        MHD_destroy_response(response);
            return ret;
        }
    
    const char* username = PQgetvalue(res, 0, 0);
        
        // Create JSON response
    json_object *response_obj = json_object_new_object();
    json_object_object_add(response_obj, "user_id", json_object_new_string(user_id));
    json_object_object_add(response_obj, "username", json_object_new_string(username));
    
    const char *json_str = json_object_to_json_string(response_obj);
    struct MHD_Response *response = create_json_response(json_str, MHD_HTTP_OK);
    enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
    json_object_put(response_obj);
    PQclear(res);
    PQfinish(conn);
        free(user_id);
        return ret;
    }
    
// Handle get friends
enum MHD_Result handle_get_friends(struct MHD_Connection *connection) {
        const char* session_token = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Authorization");
        if (!session_token || strncmp(session_token, "Bearer ", 7) != 0) {
        struct MHD_Response *response = create_error_response("Missing or invalid Authorization header", MHD_HTTP_UNAUTHORIZED);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_UNAUTHORIZED, response);
            MHD_destroy_response(response);
            return ret;
        }
        
    session_token += 7; // Skip "Bearer " prefix
        
        char* user_id = NULL;
        if (validate_session_token(session_token, &user_id) != 0) {
        struct MHD_Response *response = create_error_response("Invalid or expired session token", MHD_HTTP_UNAUTHORIZED);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_UNAUTHORIZED, response);
            MHD_destroy_response(response);
            return ret;
        }
        
        json_object *friends = get_friends_list(user_id);
        
        if (!friends) {
        struct MHD_Response *response = create_error_response("Failed to retrieve friends list", MHD_HTTP_INTERNAL_SERVER_ERROR);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
            MHD_destroy_response(response);
            free(user_id);
            return ret;
        }
        
        const char *json_str = json_object_to_json_string(friends);
    struct MHD_Response *response = create_json_response(json_str, MHD_HTTP_OK);
    enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        json_object_put(friends);
        free(user_id);
        return ret;
    }
    
// Handle get friends locations
enum MHD_Result handle_get_friends_locations(struct MHD_Connection *connection) {
        const char* session_token = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Authorization");
        if (!session_token || strncmp(session_token, "Bearer ", 7) != 0) {
        struct MHD_Response *response = create_error_response("Missing or invalid Authorization header", MHD_HTTP_UNAUTHORIZED);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_UNAUTHORIZED, response);
            MHD_destroy_response(response);
            return ret;
        }
        
    session_token += 7; // Skip "Bearer " prefix
        
        char* user_id = NULL;
        if (validate_session_token(session_token, &user_id) != 0) {
        struct MHD_Response *response = create_error_response("Invalid or expired session token", MHD_HTTP_UNAUTHORIZED);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_UNAUTHORIZED, response);
            MHD_destroy_response(response);
            return ret;
        }
        
        json_object *locations = get_friends_locations_from_db(user_id);
        
        if (!locations) {
        struct MHD_Response *response = create_error_response("Failed to retrieve friends locations", MHD_HTTP_INTERNAL_SERVER_ERROR);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
            MHD_destroy_response(response);
            free(user_id);
            return ret;
        }
        
        const char *json_str = json_object_to_json_string(locations);
    struct MHD_Response *response = create_json_response(json_str, MHD_HTTP_OK);
    enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        json_object_put(locations);
        free(user_id);
        return ret;
    }
    
// Handle get route
enum MHD_Result handle_get_route(struct MHD_Connection *connection) {
        const char* session_token = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Authorization");
        if (!session_token || strncmp(session_token, "Bearer ", 7) != 0) {
        struct MHD_Response *response = create_error_response("Missing or invalid Authorization header", MHD_HTTP_UNAUTHORIZED);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_UNAUTHORIZED, response);
            MHD_destroy_response(response);
            return ret;
        }
        
    session_token += 7; // Skip "Bearer " prefix
        
        char* user_id = NULL;
        if (validate_session_token(session_token, &user_id) != 0) {
        struct MHD_Response *response = create_error_response("Invalid or expired session token", MHD_HTTP_UNAUTHORIZED);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_UNAUTHORIZED, response);
            MHD_destroy_response(response);
            return ret;
        }
        
        const char* start_lat_str = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "start_lat");
        const char* start_lon_str = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "start_lon");
        const char* end_id = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "end_id");
        
        if (!start_lat_str || !start_lon_str || !end_id) {
        struct MHD_Response *response = create_error_response("start_lat, start_lon, and end_id query parameters required", MHD_HTTP_BAD_REQUEST);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_BAD_REQUEST, response);
            MHD_destroy_response(response);
            free(user_id);
            return ret;
        }
        
        double start_lat = atof(start_lat_str);
        double start_lon = atof(start_lon_str);
        
    json_object *route = calculate_route(start_lat, start_lon, end_id);
    
    if (!route) {
        struct MHD_Response *response = create_error_response("Failed to calculate route", MHD_HTTP_INTERNAL_SERVER_ERROR);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
            MHD_destroy_response(response);
            free(user_id);
            return ret;
        }
        
    const char *json_str = json_object_to_json_string(route);
    struct MHD_Response *response = create_json_response(json_str, MHD_HTTP_OK);
    enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_OK, response);
            MHD_destroy_response(response);
    json_object_put(route);
            free(user_id);
            return ret;
        }
        
// Handle get H3 distance
enum MHD_Result handle_get_h3_distance(struct MHD_Connection *connection) {
    const char* session_token = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Authorization");
    if (!session_token || strncmp(session_token, "Bearer ", 7) != 0) {
        struct MHD_Response *response = create_error_response("Missing or invalid Authorization header", MHD_HTTP_UNAUTHORIZED);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_UNAUTHORIZED, response);
        MHD_destroy_response(response);
        return ret;
    }
    
    session_token += 7; // Skip "Bearer " prefix
    
    char* user_id = NULL;
    if (validate_session_token(session_token, &user_id) != 0) {
        struct MHD_Response *response = create_error_response("Invalid or expired session token", MHD_HTTP_UNAUTHORIZED);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_UNAUTHORIZED, response);
        MHD_destroy_response(response);
        return ret;
    }
    
    const char* user1_id = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "user1");
    const char* user2_id = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "user2");
    
    if (!user1_id || !user2_id) {
        struct MHD_Response *response = create_error_response("user1 and user2 query parameters required", MHD_HTTP_BAD_REQUEST);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_BAD_REQUEST, response);
            MHD_destroy_response(response);
            free(user_id);
            return ret;
        }
        
    double distance = calculate_h3_distance(user1_id, user2_id);
    
    if (distance < 0) {
        struct MHD_Response *response = create_error_response("Failed to calculate H3 distance", MHD_HTTP_INTERNAL_SERVER_ERROR);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
        MHD_destroy_response(response);
        free(user_id);
        return ret;
    }
    
    char response_str[256];
    snprintf(response_str, sizeof(response_str), 
             "{\"distance\": %.2f, \"unit\": \"meters\", \"algorithm\": \"H3\"}", distance);
    
    struct MHD_Response *response = create_json_response(response_str, MHD_HTTP_OK);
    enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    free(user_id);
    return ret;
}

// Handle get A* distance
enum MHD_Result handle_get_astar_distance(struct MHD_Connection *connection) {
    const char* session_token = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Authorization");
    if (!session_token || strncmp(session_token, "Bearer ", 7) != 0) {
        struct MHD_Response *response = create_error_response("Missing or invalid Authorization header", MHD_HTTP_UNAUTHORIZED);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_UNAUTHORIZED, response);
        MHD_destroy_response(response);
        return ret;
    }
    
    session_token += 7; // Skip "Bearer " prefix
    
    char* user_id = NULL;
    if (validate_session_token(session_token, &user_id) != 0) {
        struct MHD_Response *response = create_error_response("Invalid or expired session token", MHD_HTTP_UNAUTHORIZED);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_UNAUTHORIZED, response);
        MHD_destroy_response(response);
        return ret;
    }
    
    const char* user1_id = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "user1");
    const char* user2_id = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "user2");
    
    if (!user1_id || !user2_id) {
        struct MHD_Response *response = create_error_response("user1 and user2 query parameters required", MHD_HTTP_BAD_REQUEST);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_BAD_REQUEST, response);
        MHD_destroy_response(response);
        free(user_id);
        return ret;
    }
    
    double distance = calculate_astar_distance(user1_id, user2_id);
    
    if (distance < 0) {
        struct MHD_Response *response = create_error_response("Failed to calculate A* distance", MHD_HTTP_INTERNAL_SERVER_ERROR);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
        MHD_destroy_response(response);
        free(user_id);
        return ret;
    }
    
    char response_str[256];
    snprintf(response_str, sizeof(response_str), 
             "{\"distance\": %.2f, \"unit\": \"meters\", \"algorithm\": \"A*\"}", distance);
    
    struct MHD_Response *response = create_json_response(response_str, MHD_HTTP_OK);
    enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    free(user_id);
    return ret;
}

// Handle calculate distance (legacy endpoint)
enum MHD_Result handle_post_calculate_distance(struct MHD_Connection *connection, const char *post_data, size_t post_data_size) {
    json_object *json_obj = json_tokener_parse(post_data);
            if (!json_obj) {
        struct MHD_Response *response = create_error_response("Invalid JSON", MHD_HTTP_BAD_REQUEST);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_BAD_REQUEST, response);
                MHD_destroy_response(response);
                return ret;
            }

            json_object *name1_obj, *lat1_obj, *lon1_obj, *name2_obj, *lat2_obj, *lon2_obj;
            if (!json_object_object_get_ex(json_obj, "name1", &name1_obj) ||
                !json_object_object_get_ex(json_obj, "lat1", &lat1_obj) ||
                !json_object_object_get_ex(json_obj, "lon1", &lon1_obj) ||
                !json_object_object_get_ex(json_obj, "name2", &name2_obj) ||
                !json_object_object_get_ex(json_obj, "lat2", &lat2_obj) ||
                !json_object_object_get_ex(json_obj, "lon2", &lon2_obj)) {
                
        struct MHD_Response *response = create_error_response("Missing required fields", MHD_HTTP_BAD_REQUEST);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_BAD_REQUEST, response);
                MHD_destroy_response(response);
                json_object_put(json_obj);
                return ret;
            }

            const char *name1 = json_object_get_string(name1_obj);
            double lat1 = json_object_get_double(lat1_obj);
            double lon1 = json_object_get_double(lon1_obj);
            const char *name2 = json_object_get_string(name2_obj);
            double lat2 = json_object_get_double(lat2_obj);
            double lon2 = json_object_get_double(lon2_obj);

            double distance = haversine_distance(lat1, lon1, lat2, lon2);

            PGconn *conn = PQconnectdb(CONN_STR);
            if (PQstatus(conn) != CONNECTION_OK) {
        struct MHD_Response *response = create_error_response("Database connection failed", MHD_HTTP_INTERNAL_SERVER_ERROR);
        enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
                MHD_destroy_response(response);
                PQfinish(conn);
                json_object_put(json_obj);
                return ret;
            }

            save_location_pair_to_db(conn, name1, lat1, lon1, name2, lat2, lon2, distance);
            PQfinish(conn);

            char response_str[256];
            snprintf(response_str, sizeof(response_str), 
                     "{\"distance\": %.2f, \"unit\": \"km\"}", distance);

    struct MHD_Response *response = create_json_response(response_str, MHD_HTTP_OK);
    enum MHD_Result ret = queue_response_with_cors(connection, MHD_HTTP_OK, response);
            MHD_destroy_response(response);
            json_object_put(json_obj);
            return ret;
        }
