CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -O2 -I/usr/include/postgresql -I/usr/local/include
LDFLAGS = -lmicrohttpd -ljson-c -lpq -lssl -lcrypto -lh3 -lm -L/usr/local/lib

# Directories
SRCDIR = src
BUILDDIR = build
AUTHDIR = $(SRCDIR)/auth
LOCATIONDIR = $(SRCDIR)/location
ROUTINGDIR = $(SRCDIR)/routing
UTILSDIR = $(SRCDIR)/utils

# Source files
MAIN_SRC = $(SRCDIR)/main.c
API_SERVER_SRC = $(SRCDIR)/api_server.c
AUTH_SRC = $(AUTHDIR)/auth.c
LOCATION_SRC = $(LOCATIONDIR)/location.c
ROUTING_SRC = $(ROUTINGDIR)/routing.c
UTILS_SRC = $(UTILSDIR)/utils.c
COORDINATE_LOGGER_SRC = $(SRCDIR)/coordinate_logger.c

# Object files
MAIN_OBJ = $(BUILDDIR)/main.o
API_SERVER_OBJ = $(BUILDDIR)/api_server.o
AUTH_OBJ = $(BUILDDIR)/auth.o
LOCATION_OBJ = $(BUILDDIR)/location.o
ROUTING_OBJ = $(BUILDDIR)/routing.o
UTILS_OBJ = $(BUILDDIR)/utils.o
COORDINATE_LOGGER_OBJ = $(BUILDDIR)/coordinate_logger.o

# Target executable
TARGET = $(BUILDDIR)/location_sharing_system

# Default target
all: $(TARGET)

# Create build directory
$(BUILDDIR):
	mkdir -p $(BUILDDIR)

# Build main executable
$(TARGET): $(BUILDDIR) $(MAIN_OBJ) $(API_SERVER_OBJ) $(AUTH_OBJ) $(LOCATION_OBJ) $(ROUTING_OBJ) $(UTILS_OBJ) $(COORDINATE_LOGGER_OBJ)
	$(CC) $(MAIN_OBJ) $(API_SERVER_OBJ) $(AUTH_OBJ) $(LOCATION_OBJ) $(ROUTING_OBJ) $(UTILS_OBJ) $(COORDINATE_LOGGER_OBJ) -o $(TARGET) $(LDFLAGS)

# Compile main.c
$(MAIN_OBJ): $(MAIN_SRC) $(SRCDIR)/api_server.h $(SRCDIR)/api.h
	$(CC) $(CFLAGS) -c $(MAIN_SRC) -o $(MAIN_OBJ)

# Compile api_server.c
$(API_SERVER_OBJ): $(API_SERVER_SRC) $(SRCDIR)/api_server.h $(SRCDIR)/api.h $(AUTHDIR)/auth.h $(LOCATIONDIR)/location.h $(ROUTINGDIR)/routing.h $(UTILSDIR)/utils.h
	$(CC) $(CFLAGS) -c $(API_SERVER_SRC) -o $(API_SERVER_OBJ)

# Compile auth.c
$(AUTH_OBJ): $(AUTH_SRC) $(AUTHDIR)/auth.h $(SRCDIR)/api.h
	$(CC) $(CFLAGS) -c $(AUTH_SRC) -o $(AUTH_OBJ)

# Compile location.c
$(LOCATION_OBJ): $(LOCATION_SRC) $(LOCATIONDIR)/location.h $(SRCDIR)/api.h
	$(CC) $(CFLAGS) -c $(LOCATION_SRC) -o $(LOCATION_OBJ)

# Compile routing.c
$(ROUTING_OBJ): $(ROUTING_SRC) $(ROUTINGDIR)/routing.h $(SRCDIR)/api.h $(LOCATIONDIR)/location.h
	$(CC) $(CFLAGS) -c $(ROUTING_SRC) -o $(ROUTING_OBJ)

# Compile utils.c
$(UTILS_OBJ): $(UTILS_SRC) $(UTILSDIR)/utils.h
	$(CC) $(CFLAGS) -c $(UTILS_SRC) -o $(UTILS_OBJ)

# Compile coordinate_logger.c
$(COORDINATE_LOGGER_OBJ): $(COORDINATE_LOGGER_SRC) $(SRCDIR)/coordinate_logger.h $(SRCDIR)/api.h
	$(CC) $(CFLAGS) -c $(COORDINATE_LOGGER_SRC) -o $(COORDINATE_LOGGER_OBJ)

# Clean build files
clean:
	rm -rf $(BUILDDIR)

# Install dependencies (Ubuntu/Debian)
install-deps:
	sudo apt-get update
	sudo apt-get install -y libmicrohttpd-dev libjson-c-dev libpq-dev libssl-dev libh3-dev

# Install dependencies (CentOS/RHEL/Fedora)
install-deps-rpm:
	sudo yum install -y libmicrohttpd-devel json-c-devel postgresql-devel openssl-devel h3-devel

# Run the application
run: $(TARGET)
	./$(TARGET)

# Debug build
debug: CFLAGS += -DDEBUG -g3
debug: $(TARGET)

# Release build
release: CFLAGS += -DNDEBUG -O3
release: $(TARGET)

# Show help
help:
	@echo "Available targets:"
	@echo "  all          - Build the application (default)"
	@echo "  clean        - Remove build files"
	@echo "  install-deps - Install dependencies (Ubuntu/Debian)"
	@echo "  install-deps-rpm - Install dependencies (CentOS/RHEL/Fedora)"
	@echo "  run          - Build and run the application"
	@echo "  debug        - Build with debug flags"
	@echo "  release      - Build with release optimization"
	@echo "  help         - Show this help message"

.PHONY: all clean install-deps install-deps-rpm run debug release help
