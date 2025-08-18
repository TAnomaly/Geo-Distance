#include "api.h"
#include "coordinate_logger.h"
#include <json-c/json.h>

// Handle HTTP requests
enum MHD_Result handle_request(void *cls __attribute__((unused)), struct MHD_Connection *connection,
                   const char *url, const char *method,
                   const char *version __attribute__((unused)), const char *upload_data,
                   size_t *upload_data_size, void **con_cls) {
    
    // Debug print
    printf("Received request: %s %s\n", method, url);
    fflush(stdout);
    
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