#ifndef API_H
#define API_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <libpq-fe.h>
#include <microhttpd.h>
#include <json-c/json.h>

#define CONN_STR "host=localhost dbname=mydb user=myuser password=mypassword"
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

#endif // API_H