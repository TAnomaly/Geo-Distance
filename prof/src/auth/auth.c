#define _GNU_SOURCE
#include "auth.h"
#include "../api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <libpq-fe.h>

// Generate a random session token
char* generate_session_token(void) {
    unsigned char random_bytes[32];
    if (RAND_bytes(random_bytes, sizeof(random_bytes)) != 1) {
        fprintf(stderr, "Failed to generate random bytes\n");
        return NULL;
    }
    
    char* token = malloc(65); // 64 hex chars + null terminator
    if (!token) {
        return NULL;
    }
    
    for (int i = 0; i < 32; i++) {
        sprintf(&token[i * 2], "%02x", random_bytes[i]);
    }
    token[64] = '\0';
    
    return token;
}

// Hash a password using SHA-256
int hash_password(const char* password, char* hash_buffer, size_t buffer_size) {
    if (!password || !hash_buffer || buffer_size < 65) {
        return -1; // Invalid input or insufficient buffer size
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, password, strlen(password));
    SHA256_Final(hash, &sha256);

    // Convert to hex string
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(&hash_buffer[i * 2], "%02x", hash[i]);
    }
    hash_buffer[64] = '\0';
    
    return 0; // Success
}

// Verify a password against a hash
int verify_password(const char* password, const char* hash) {
    if (!password || !hash) {
        return -1; // Invalid input
    }

    char computed_hash[65];
    if (hash_password(password, computed_hash, sizeof(computed_hash)) != 0) {
        return -1; // Hashing failed
    }

    return strcmp(computed_hash, hash) == 0 ? 0 : -1; // 0 if match, -1 if not
}

// Register a new user
char* register_user(const char* username, const char* password) {
    if (!username || !password) {
        fprintf(stderr, "Username or password is NULL\n");
        return NULL;
    }

    PGconn *conn = PQconnectdb(CONN_STR);
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Database connection failed: %s", PQerrorMessage(conn));
        PQfinish(conn);
        return NULL;
    }

    // Check if user already exists
    char query[512];
    snprintf(query, sizeof(query), 
             "SELECT id FROM users WHERE username = '%s';", username);
    
    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Query failed: %s", PQerrorMessage(conn));
        PQclear(res);
        PQfinish(conn);
        return NULL;
    }

    if (PQntuples(res) > 0) {
        // User already exists
        fprintf(stderr, "User '%s' already exists\n", username);
        PQclear(res);
        PQfinish(conn);
        return NULL;
    }
    PQclear(res);

    // Hash the password
    char password_hash[65];
    if (hash_password(password, password_hash, sizeof(password_hash)) != 0) {
        fprintf(stderr, "Failed to hash password\n");
        PQfinish(conn);
        return NULL;
    }

    // Insert new user
    snprintf(query, sizeof(query), 
             "INSERT INTO users (username, password_hash) VALUES ('%s', '%s') RETURNING id;", 
             username, password_hash);
    
    printf("DEBUG: Executing query: %s\n", query);
    
    res = PQexec(conn, query);
    printf("DEBUG: Query result status: %d\n", PQresultStatus(res));
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Insert failed: %s", PQerrorMessage(conn));
        PQclear(res);
        PQfinish(conn);
        return NULL;
    }

    if (PQntuples(res) == 0) {
        fprintf(stderr, "Insert returned no rows\n");
        PQclear(res);
        PQfinish(conn);
        return NULL;
    }

    // Get the new user's ID
    printf("DEBUG: Getting user ID from result\n");
    const char* db_user_id = PQgetvalue(res, 0, 0);
    printf("DEBUG: db_user_id: %s\n", db_user_id ? db_user_id : "NULL");
    if (!db_user_id) {
        fprintf(stderr, "Failed to get user ID from database\n");
        PQclear(res);
        PQfinish(conn);
        return NULL;
    }
    printf("DEBUG: About to strdup\n");
    char* user_id = strdup(db_user_id);
    printf("DEBUG: strdup result: %s\n", user_id ? user_id : "NULL");
    PQclear(res);
    PQfinish(conn);

    return user_id;
}

// Login a user
char* login_user(const char* username, const char* password) {
    if (!username || !password) {
        fprintf(stderr, "Username or password is NULL\n");
        return NULL;
    }

    PGconn *conn = PQconnectdb(CONN_STR);
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Database connection failed: %s", PQerrorMessage(conn));
        PQfinish(conn);
        return NULL;
    }

    // Get user's password hash
    char query[512];
    snprintf(query, sizeof(query), 
             "SELECT id, password_hash FROM users WHERE username = '%s';", username);
    
    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Query failed: %s", PQerrorMessage(conn));
        PQclear(res);
        PQfinish(conn);
        return NULL;
    }

    if (PQntuples(res) == 0) {
        // User not found
        PQclear(res);
        PQfinish(conn);
        return NULL;
    }

    const char* stored_hash = PQgetvalue(res, 0, 1);
    const char* db_user_id = PQgetvalue(res, 0, 0);
    if (!stored_hash || !db_user_id) {
        fprintf(stderr, "Failed to get user data from database\n");
        PQclear(res);
        PQfinish(conn);
        return NULL;
    }
    char* user_id = strdup(db_user_id);
    PQclear(res);

    // Verify password
    if (verify_password(password, stored_hash) != 0) {
        // Password incorrect
        free(user_id);
        PQfinish(conn);
        return NULL;
    }

    // Generate session token
    char* session_token = generate_session_token();
    if (!session_token) {
        free(user_id);
        PQfinish(conn);
        return NULL;
    }

    // Insert session into database (expires in 1 hour)
    time_t now = time(NULL);
    time_t expires_at = now + 3600; // 1 hour
    
    char expires_at_str[20];
    strftime(expires_at_str, sizeof(expires_at_str), "%Y-%m-%d %H:%M:%S", gmtime(&expires_at));

    snprintf(query, sizeof(query), 
             "INSERT INTO user_sessions (session_token, user_id, expires_at) VALUES ('%s', %s, '%s');", 
             session_token, user_id, expires_at_str);
    
    res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Insert session failed: %s", PQerrorMessage(conn));
        free(session_token);
        free(user_id);
        PQclear(res);
        PQfinish(conn);
        return NULL;
    }
    PQclear(res);
    PQfinish(conn);
    free(user_id); // We don't need user_id anymore, we return session_token

    return session_token;
}

// Logout a user
int logout_user(const char* session_token) {
    if (!session_token) {
        return -1; // Invalid input
    }

    PGconn *conn = PQconnectdb(CONN_STR);
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Database connection failed: %s", PQerrorMessage(conn));
        PQfinish(conn);
        return -1;
    }

    // Delete session from database
    char query[512];
    snprintf(query, sizeof(query), 
             "DELETE FROM user_sessions WHERE session_token = '%s';", session_token);
    
    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Delete session failed: %s", PQerrorMessage(conn));
        PQclear(res);
        PQfinish(conn);
        return -1;
    }
    PQclear(res);
    PQfinish(conn);

    return 0; // Success
}

// Validate a session token and get user ID
int validate_session_token(const char* session_token, char** user_id) {
    if (!session_token || !user_id) {
        return -1; // Invalid input
    }
    
    PGconn *conn = PQconnectdb(CONN_STR);
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Database connection failed: %s", PQerrorMessage(conn));
        PQfinish(conn);
        return -1;
    }
    
    // Get user ID and expiration time from database
    char query[512];
    snprintf(query, sizeof(query), 
             "SELECT user_id, expires_at FROM user_sessions WHERE session_token = '%s';", session_token);
    
    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Query failed: %s", PQerrorMessage(conn));
        PQclear(res);
        PQfinish(conn);
        return -1;
    }
    
    if (PQntuples(res) == 0) {
        // Session token not found
        PQclear(res);
        PQfinish(conn);
        return -1;
    }
    
    const char* db_user_id = PQgetvalue(res, 0, 0);
    const char* expires_at_str = PQgetvalue(res, 0, 1);
    PQclear(res);
    
    // Parse expiration time
    struct tm tm_expires;
    memset(&tm_expires, 0, sizeof(struct tm));
    if (strptime(expires_at_str, "%Y-%m-%d %H:%M:%S", &tm_expires) == NULL) {
        fprintf(stderr, "Failed to parse expiration time: %s\n", expires_at_str);
        PQfinish(conn);
        return -1;
    }
    
    time_t expires_at = timegm(&tm_expires);
    if (expires_at == -1) {
        fprintf(stderr, "Failed to convert expiration time to time_t\n");
        PQfinish(conn);
        return -1;
    }
    
    // Check if session has expired
    time_t now = time(NULL);
    if (now > expires_at) {
        // Session expired, delete it from database
        snprintf(query, sizeof(query), 
                 "DELETE FROM user_sessions WHERE session_token = '%s';", session_token);
        
        PGresult *del_res = PQexec(conn, query);
        if (PQresultStatus(del_res) != PGRES_COMMAND_OK) {
            fprintf(stderr, "Delete expired session failed: %s", PQerrorMessage(conn));
            PQclear(del_res);
            PQfinish(conn);
            return -1;
        }
        PQclear(del_res);
        PQfinish(conn);
        return -1; // Session expired
    }
    
    // Session is valid, return user ID
    if (!db_user_id) {
        fprintf(stderr, "Failed to get user ID from database\n");
        PQfinish(conn);
        return -1;
    }
    *user_id = strdup(db_user_id);
    PQfinish(conn);
    
    return 0; // Success
}

// Add a friend
int add_friend(const char* user_id, const char* friend_username) {
    if (!user_id || !friend_username) {
        return -1;
    }

    PGconn *conn = PQconnectdb(CONN_STR);
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Database connection failed: %s", PQerrorMessage(conn));
        PQfinish(conn);
        return -1;
    }

    // Get friend's user ID
    char query[512];
    snprintf(query, sizeof(query), 
             "SELECT id FROM users WHERE username = '%s';", friend_username);
    
    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Query failed: %s", PQerrorMessage(conn));
        PQclear(res);
        PQfinish(conn);
        return -1;
    }

    if (PQntuples(res) == 0) {
        // Friend not found
        PQclear(res);
        PQfinish(conn);
        return -1;
    }

    const char* friend_id = PQgetvalue(res, 0, 0);
    PQclear(res);

    // Check if friendship already exists
    snprintf(query, sizeof(query), 
             "SELECT id FROM friendships WHERE (user_id = %s AND friend_id = %s) OR (user_id = %s AND friend_id = %s);", 
             user_id, friend_id, friend_id, user_id);
    
    res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Query failed: %s", PQerrorMessage(conn));
        PQclear(res);
        PQfinish(conn);
        return -1;
    }

    if (PQntuples(res) > 0) {
        // Friendship already exists
        PQclear(res);
        PQfinish(conn);
        return -1;
    }
    PQclear(res);

    // Insert friendship
    snprintf(query, sizeof(query), 
             "INSERT INTO friendships (user_id, friend_id, status) VALUES (%s, %s, 'accepted');", 
             user_id, friend_id);
    
    res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Insert friendship failed: %s", PQerrorMessage(conn));
        PQclear(res);
        PQfinish(conn);
        return -1;
    }
    PQclear(res);
    PQfinish(conn);

    return 0; // Success
}

// Get friends list for a user
json_object* get_friends_list(const char* user_id) {
    if (!user_id) {
        fprintf(stderr, "User ID is NULL\n");
        return NULL;
    }
    
    PGconn *conn = PQconnectdb(CONN_STR);
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Database connection failed: %s", PQerrorMessage(conn));
        PQfinish(conn);
        return NULL;
    }
    
    // Get accepted friends (where user is either user_id or friend_id)
    char query[1024];
    snprintf(query, sizeof(query), 
             "SELECT u.id, u.username "
             "FROM friendships f "
             "JOIN users u ON (CASE "
             "    WHEN f.user_id = %s THEN f.friend_id "
             "    WHEN f.friend_id = %s THEN f.user_id "
             "END) = u.id "
             "WHERE (f.user_id = %s OR f.friend_id = %s) "
             "AND f.status = 'accepted';", 
             user_id, user_id, user_id, user_id);
    
    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Query failed: %s", PQerrorMessage(conn));
        PQclear(res);
        PQfinish(conn);
        return NULL;
    }
    
    int rows = PQntuples(res);
    json_object *friends_array = json_object_new_array();
    
    for (int i = 0; i < rows; i++) {
        json_object *friend_obj = json_object_new_object();
        json_object_object_add(friend_obj, "id", json_object_new_string(PQgetvalue(res, i, 0)));
        json_object_object_add(friend_obj, "username", json_object_new_string(PQgetvalue(res, i, 1)));
        // For simplicity, we'll assume all friends are offline
        // In a real application, you would check their last location update time
        json_object_object_add(friend_obj, "online", json_object_new_boolean(0));
        
        json_object_array_add(friends_array, friend_obj);
    }
    
    PQclear(res);
    PQfinish(conn);
    
    return friends_array;
}
