#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

// Helper function to queue HTTP responses with CORS headers
enum MHD_Result queue_response_with_cors(struct MHD_Connection *connection, 
                                        unsigned int status, 
                                        struct MHD_Response *response) {
    // Add CORS headers to allow requests from any origin
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
    MHD_add_response_header(response, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    MHD_add_response_header(response, "Access-Control-Allow-Headers", "Content-Type, Authorization");
    MHD_add_response_header(response, "Access-Control-Allow-Credentials", "true");
    
    return MHD_queue_response(connection, status, response);
}

// Create a JSON response
struct MHD_Response* create_json_response(const char* json_str, int status_code) {
    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(json_str), 
                                                                   (void *)json_str, 
                                                                   MHD_RESPMEM_MUST_COPY);
    MHD_add_response_header(response, "Content-Type", "application/json");
    return response;
}

// Create an error response
struct MHD_Response* create_error_response(const char* error_msg, int status_code) {
    char json_error[256];
    snprintf(json_error, sizeof(json_error), "{\"error\": \"%s\"}", error_msg);
    return create_json_response(json_error, status_code);
}

// Create a success response
struct MHD_Response* create_success_response(const char* success_msg, int status_code) {
    char json_success[256];
    snprintf(json_success, sizeof(json_success), "{\"success\": \"%s\"}", success_msg);
    return create_json_response(json_success, status_code);
}
