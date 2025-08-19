#ifndef UTILS_H
#define UTILS_H

#include <microhttpd.h>
#include <json-c/json.h>

// File utility functions
char* read_file_content(const char *filepath);
const char* get_content_type(const char *url);

// HTTP response utility functions
enum MHD_Result queue_response_with_cors(struct MHD_Connection *connection, 
                                        unsigned int status, 
                                        struct MHD_Response *response);

// JSON response utility functions
struct MHD_Response* create_json_response(const char* json_str, int status_code);
struct MHD_Response* create_error_response(const char* error_msg, int status_code);
struct MHD_Response* create_success_response(const char* success_msg, int status_code);

#endif // UTILS_H
