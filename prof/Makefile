# Makefile for coordinate_logger project

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I/usr/include/postgresql -I/usr/include/json-c -Isrc
LIBS_LOGGER = -lpq -lm
LIBS_API = -lpq -lm -ljson-c -lmicrohttpd

# Directories
SRC_DIR = src
BUILD_DIR = build
WEB_DIR = web

# Executable names
TARGET_LOGGER = coordinate_logger
TARGET_API = api_server

# Source files
SRCS_LOGGER_LIB = $(SRC_DIR)/coordinate_logger.c
SRCS_LOGGER_MAIN = $(SRC_DIR)/cli_main.c
SRCS_API = $(SRC_DIR)/api_server.c $(SRC_DIR)/coordinate_logger.c

# Default target
all: $(BUILD_DIR) $(TARGET_LOGGER) $(TARGET_API)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build the logger executable
$(TARGET_LOGGER): $(SRCS_LOGGER_LIB) $(SRCS_LOGGER_MAIN)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/$@ $^ $(LIBS_LOGGER)

# Build the API server executable
$(TARGET_API): $(SRCS_API)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/$@ $^ $(LIBS_API)

# Install web files
install-web:
	mkdir -p $(BUILD_DIR)/web
	cp -r $(WEB_DIR)/* $(BUILD_DIR)/web/

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

# Phony targets
.PHONY: all clean install-web