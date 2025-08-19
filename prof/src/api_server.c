#include "api.h"
#include "coordinate_logger.h"
#include <json-c/json.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>

// Helper function to read file content
char* read_file_content(const char *filepath) {
    FILE *file = fopen(filepath, "rb");
    if (!file) {
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *content = malloc(length + 1);
    if (!content) {
        fclose(file);
        return NULL;
    }
    
    fread(content, 1, length, file);
    content[length] = '\0';
    fclose(file);
    
    return content;
}

// Helper function to get content type based on file extension
const char* get_content_type(const char *url) {
    const char *dot = strrchr(url, '.');
    if (!dot) return "text/plain";
    
    if (strcmp(dot, ".html") == 0) return "text/html";
    if (strcmp(dot, ".css") == 0) return "text/css";
    if (strcmp(dot, ".js") == 0) return "application/javascript";
    if (strcmp(dot, ".json") == 0) return "application/json";
    if (strcmp(dot, ".png") == 0) return "image/png";
    if (strcmp(dot, ".jpg") == 0) return "image/jpeg";
    if (strcmp(dot, ".gif") == 0) return "image/gif";
    
    return "text/plain";
}

// Function to retrieve coordinates from database
json_object* get_coordinates_from_db() {
    PGconn *conn = PQconnectdb(CONN_STR);
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Database connection failed: %s", PQerrorMessage(conn));
        PQfinish(conn);
        return NULL;
    }
    
    const char *query = "SELECT first_name, first_lat, first_lon, first_h3, second_name, second_lat, second_lon, second_h3, distance FROM coordinates ORDER BY id DESC LIMIT 100;";
    PGresult *res = PQexec(conn, query);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Query failed: %s", PQerrorMessage(conn));
        PQclear(res);
        PQfinish(conn);
        return NULL;
    }
    
    int rows = PQntuples(res);
    json_object *coordinates_array = json_object_new_array();
    
    for (int i = 0; i < rows; i++) {
        json_object *coord_obj = json_object_new_object();
        
        json_object_object_add(coord_obj, "first_name", json_object_new_string(PQgetvalue(res, i, 0)));
        json_object_object_add(coord_obj, "first_lat", json_object_new_double(atof(PQgetvalue(res, i, 1))));
        json_object_object_add(coord_obj, "first_lon", json_object_new_double(atof(PQgetvalue(res, i, 2))));
        json_object_object_add(coord_obj, "first_h3", json_object_new_string(PQgetvalue(res, i, 3)));
        json_object_object_add(coord_obj, "second_name", json_object_new_string(PQgetvalue(res, i, 4)));
        json_object_object_add(coord_obj, "second_lat", json_object_new_double(atof(PQgetvalue(res, i, 5))));
        json_object_object_add(coord_obj, "second_lon", json_object_new_double(atof(PQgetvalue(res, i, 6))));
        json_object_object_add(coord_obj, "second_h3", json_object_new_string(PQgetvalue(res, i, 7)));
        json_object_object_add(coord_obj, "distance", json_object_new_double(atof(PQgetvalue(res, i, 8))));
        
        json_object_array_add(coordinates_array, coord_obj);
    }
    
    PQclear(res);
    PQfinish(conn);
    
    return coordinates_array;
}

// Handle HTTP requests
enum MHD_Result handle_request(void *cls __attribute__((unused)), struct MHD_Connection *connection,
                   const char *url, const char *method,
                   const char *version __attribute__((unused)), const char *upload_data,
                   size_t *upload_data_size, void **con_cls) {
    
    // Debug print
    printf("Received request: %s %s\n", method, url);
    fflush(stdout);
    
    // Handle GET requests for web files
    if (strcmp(method, "GET") == 0) {
        // Serve index.html for root path
        if (strcmp(url, "/") == 0) {
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s/index.html", WEB_ROOT);
            
            char *content = read_file_content(filepath);
            if (!content) {
                const char *error_msg = "{\"error\": \"File not found\"}";
                struct MHD_Response *response = MHD_create_response_from_buffer(strlen(error_msg), 
                                                                               (void *)error_msg, 
                                                                               MHD_RESPMEM_MUST_COPY);
                enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
                MHD_destroy_response(response);
                return ret;
            }
            
            struct MHD_Response *response = MHD_create_response_from_buffer(strlen(content), 
                                                                           (void *)content, 
                                                                           MHD_RESPMEM_MUST_FREE);
            MHD_add_response_header(response, "Content-Type", "text/html");
            enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
            MHD_destroy_response(response);
            return ret;
        }
        
        // Serve coordinates API endpoint
        if (strcmp(url, "/api/coordinates") == 0) {
            json_object *coordinates = get_coordinates_from_db();
            if (!coordinates) {
                const char *error_msg = "{\"error\": \"Failed to retrieve coordinates\"}";
                struct MHD_Response *response = MHD_create_response_from_buffer(strlen(error_msg), 
                                                                               (void *)error_msg, 
                                                                               MHD_RESPMEM_MUST_COPY);
                enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
                MHD_destroy_response(response);
                return ret;
            }
            
            const char *json_str = json_object_to_json_string(coordinates);
            struct MHD_Response *response = MHD_create_response_from_buffer(strlen(json_str), 
                                                                           (void *)json_str, 
                                                                           MHD_RESPMEM_MUST_COPY);
            MHD_add_response_header(response, "Content-Type", "application/json");
            enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
            MHD_destroy_response(response);
            json_object_put(coordinates);
            return ret;
        }
        
        // Serve static files from web directory
        if (strncmp(url, "/web/", 5) == 0) {
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s%s", WEB_ROOT, url + 4); // Remove /web prefix
            
            char *content = read_file_content(filepath);
            if (!content) {
                const char *error_msg = "{\"error\": \"File not found\"}";
                struct MHD_Response *response = MHD_create_response_from_buffer(strlen(error_msg), 
                                                                               (void *)error_msg, 
                                                                               MHD_RESPMEM_MUST_COPY);
                enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
                MHD_destroy_response(response);
                return ret;
            }
            
            struct MHD_Response *response = MHD_create_response_from_buffer(strlen(content), 
                                                                           (void *)content, 
                                                                           MHD_RESPMEM_MUST_FREE);
            MHD_add_response_header(response, "Content-Type", get_content_type(url));
            enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
            MHD_destroy_response(response);
            return ret;
        }
    }
    
    // Only accept POST requests to /calculate-distance
    if (strcmp(method, "POST") != 0 || strcmp(url, "/calculate-distance") != 0) {
        const char *error_msg = "{\"error\": \"Invalid method or endpoint\"}";
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(error_msg), 
                                                                       (void *)error_msg, 
                                                                       MHD_RESPMEM_MUST_COPY);
        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
        MHD_destroy_response(response);
        return ret;
    }

    // If no data, wait for it
    if (*upload_data_size == 0) {
        *con_cls = (void *)1; // Mark that we're waiting for data
        return MHD_YES;
    }

    // Get JSON data from request body
    const char *data = upload_data;
    if (*upload_data_size != 0) {
        // Parse JSON
        json_object *json_obj = json_tokener_parse(data);
        if (!json_obj) {
            const char *error_msg = "{\"error\": \"Invalid JSON\"}";
            struct MHD_Response *response = MHD_create_response_from_buffer(strlen(error_msg), 
                                                                           (void *)error_msg, 
                                                                           MHD_RESPMEM_MUST_COPY);
            enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
            MHD_destroy_response(response);
            return ret;
        }

        // Extract values from JSON
        json_object *name1_obj, *lat1_obj, *lon1_obj, *name2_obj, *lat2_obj, *lon2_obj;
        if (!json_object_object_get_ex(json_obj, "name1", &name1_obj) ||
            !json_object_object_get_ex(json_obj, "lat1", &lat1_obj) ||
            !json_object_object_get_ex(json_obj, "lon1", &lon1_obj) ||
            !json_object_object_get_ex(json_obj, "name2", &name2_obj) ||
            !json_object_object_get_ex(json_obj, "lat2", &lat2_obj) ||
            !json_object_object_get_ex(json_obj, "lon2", &lon2_obj)) {
            
            const char *error_msg = "{\"error\": \"Missing required fields\"}";
            struct MHD_Response *response = MHD_create_response_from_buffer(strlen(error_msg), 
                                                                           (void *)error_msg, 
                                                                           MHD_RESPMEM_MUST_COPY);
            enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
            MHD_destroy_response(response);
            json_object_put(json_obj);
            return ret;
        }

        // Get values
        const char *name1 = json_object_get_string(name1_obj);
        double lat1 = json_object_get_double(lat1_obj);
        double lon1 = json_object_get_double(lon1_obj);
        const char *name2 = json_object_get_string(name2_obj);
        double lat2 = json_object_get_double(lat2_obj);
        double lon2 = json_object_get_double(lon2_obj);

        // Calculate distance (using coordinate_logger.c function)
        double distance = haversine_distance(lat1, lon1, lat2, lon2);

        // Connect to database and save (using coordinate_logger.c function)
        PGconn *conn = PQconnectdb(CONN_STR);
        if (PQstatus(conn) != CONNECTION_OK) {
            const char *error_msg = "{\"error\": \"Database connection failed\"}";
            struct MHD_Response *response = MHD_create_response_from_buffer(strlen(error_msg), 
                                                                           (void *)error_msg, 
                                                                           MHD_RESPMEM_MUST_COPY);
            enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
            MHD_destroy_response(response);
            PQfinish(conn);
            json_object_put(json_obj);
            return ret;
        }

        save_location_pair_to_db(conn, name1, lat1, lon1, name2, lat2, lon2, distance);
        PQfinish(conn);

        // Create JSON response
        char response_str[256];
        snprintf(response_str, sizeof(response_str), 
                 "{\"distance\": %.2f, \"unit\": \"km\"}", distance);

        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(response_str), 
                                                                       (void *)response_str, 
                                                                       MHD_RESPMEM_MUST_COPY);
        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        json_object_put(json_obj);
        *upload_data_size = 0; // Mark data as processed
        return ret;
    }

    return MHD_YES;
}

int main() {
    struct MHD_Daemon *daemon;

    daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD, PORT, NULL, NULL,
                              &handle_request, NULL, MHD_OPTION_END);
    if (NULL == daemon) {
        fprintf(stderr, "Failed to start daemon\n");
        return 1;
    }

    printf("Server running on port %d\n", PORT);
    printf("POST requests to http://localhost:%d/calculate-distance\n", PORT);
    printf("Map UI available at http://localhost:%d/\n", PORT);
    printf("API endpoint for coordinates: http://localhost:%d/api/coordinates\n", PORT);
    printf("Example JSON payload:\n");
    printf("{\n");
    printf("  \"name1\": \"Istanbul\",\n");
    printf("  \"lat1\": 41.0151,\n");
    printf("  \"lon1\": 28.9795,\n");
    printf("  \"name2\": \"Ankara\",\n");
    printf("  \"lat2\": 39.9334,\n");
    printf("  \"lon2\": 32.8597\n");
    printf("}\n");
    printf("Press enter to stop the server...\n");
    getchar();

    MHD_stop_daemon(daemon);
    return 0;
}