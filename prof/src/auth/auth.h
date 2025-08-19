#ifndef AUTH_H
#define AUTH_H

#include <json-c/json.h>

// User registration and authentication functions
char* register_user(const char* username, const char* password);
char* login_user(const char* username, const char* password);
int logout_user(const char* session_token);
int validate_session_token(const char* session_token, char** user_id);

// Password utilities
int hash_password(const char* password, char* hash_buffer, size_t buffer_size);
int verify_password(const char* password, const char* hash);

// Session token generation
char* generate_session_token(void);

// Friend management
int add_friend(const char* user_id, const char* friend_username);
json_object* get_friends_list(const char* user_id);

#endif // AUTH_H
