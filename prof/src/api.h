#ifndef API_H
#define API_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <libpq-fe.h>
#include <microhttpd.h>

#define CONN_STR "host=localhost dbname=mydb user=myuser password=mypassword"
#define PORT 8080

// Function declarations
enum MHD_Result handle_request(void *cls __attribute__((unused)), struct MHD_Connection *connection,
                   const char *url, const char *method,
                   const char *version __attribute__((unused)), const char *upload_data,
                   size_t *upload_data_size, void **con_cls);

#endif // API_H