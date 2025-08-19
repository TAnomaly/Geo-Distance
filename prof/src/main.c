#include "api_server.h"
#include "api.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

static struct MHD_Daemon *daemon = NULL;

// Signal handler for graceful shutdown
void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down gracefully...\n", sig);
    if (daemon) {
        MHD_stop_daemon(daemon);
    }
    exit(0);
}

int main() {
    // Set up signal handlers for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Initialize the API server
    daemon = start_api_server();
    if (daemon == NULL) {
        fprintf(stderr, "Failed to start API server\n");
        return 1;
    }

    printf("Location Sharing System Server\n");
    printf("==============================\n");
    printf("Server running on port %d\n", PORT);
    printf("Web interface: http://localhost:%d/\n", PORT);
    printf("API endpoints:\n");
    printf("  - POST /api/register - User registration\n");
    printf("  - POST /api/login - User login\n");
    printf("  - POST /api/logout - User logout\n");
    printf("  - POST /api/save-location - Save user location\n");
    printf("  - GET  /api/friends - Get friends list\n");
    printf("  - GET  /api/friends/locations - Get friends locations\n");
    printf("  - GET  /api/route - Calculate route between points\n");
    printf("  - GET  /api/distance/h3 - H3 distance calculation\n");
    printf("  - GET  /api/distance/astar - A* distance calculation\n");
    printf("\nPress Ctrl+C to stop the server...\n");

    // Keep the server running
    while (1) {
        sleep(1);
    }

    return 0;
}
