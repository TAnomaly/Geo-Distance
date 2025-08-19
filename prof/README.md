# Coordinate Distance Calculator with H3 Indexing

A C-based REST API and CLI tool to calculate the distance between two geographic coordinates using the Haversine formula. Results are stored in a PostgreSQL database with H3 spatial indexing for advanced geospatial operations.

## Features

- Calculate distance between two geographic points (km)
- Store results in PostgreSQL with H3 spatial indexing
- Interactive map visualization using Leaflet.js
- REST API interface for programmatic access
- CLI interface for interactive terminal usage
- Real-time map updates with coordinate visualization

## Tech Stack

- **Language**: C (ISO C99)
- **Web Framework**: libmicrohttpd (for REST API)
- **Database**: PostgreSQL
- **Spatial Indexing**: H3 (Uber's Hexagonal Hierarchical Spatial Index)
- **JSON Library**: json-c
- **Map Visualization**: Leaflet.js with OpenStreetMap
- **Build System**: Make

## Project Structure

```
prof/
├── Makefile
├── src/
│   ├── api_server.c       # HTTP server implementation
│   ├── cli_main.c         # CLI entry point
│   ├── coordinate_logger.c # Core logic (Haversine + DB + H3)
│   ├── coordinate_logger.h # Function declarations
│   └── api.h              # API headers
├── web/
│   └── index.html         # Map UI interface
├── build/
│   ├── coordinate_logger  # CLI executable
│   └── api_server         # API executable
├── migrate_h3.sql         # Database migration script
└── README.md
```

## Database Schema

```sql
CREATE TABLE coordinates (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    first_name VARCHAR(100),
    first_lat DOUBLE PRECISION,
    first_lon DOUBLE PRECISION,
    first_h3 VARCHAR(16),
    second_name VARCHAR(100),
    second_lat DOUBLE PRECISION,
    second_lon DOUBLE PRECISION,
    second_h3 VARCHAR(16),
    distance DOUBLE PRECISION
);
```

## Prerequisites

Before building and running the application, ensure you have the following dependencies installed:

- **PostgreSQL** (`postgresql`, `postgresql-contrib`)
- **PostgreSQL Development Libraries** (`libpq-dev`)
- **libmicrohttpd** (`libmicrohttpd-dev`)
- **json-c** (`libjson-c-dev`)
- **H3 Library** (`libh3-dev` or build from source)
- **GCC** (or another C99 compatible compiler)
- **Make**

### Installing Dependencies on Ubuntu/Debian:

```bash
# Update package list
sudo apt update

# Install PostgreSQL and development libraries
sudo apt install postgresql postgresql-contrib libpq-dev

# Install project dependencies
sudo apt install libmicrohttpd-dev libjson-c-dev build-essential

# Install H3 library
# Option 1: Install from package manager (if available)
sudo apt install libh3-dev

# Option 2: Build from source
git clone https://github.com/uber/h3.git
cd h3
cmake -DBUILD_SHARED_LIBS=ON .
make
sudo make install
```

## Database Setup

1. Start PostgreSQL service:
```bash
sudo systemctl start postgresql
```

2. Create database and user:
```bash
sudo -u postgres createuser myuser
sudo -u postgres createdb mydb
sudo -u postgres psql -c "ALTER USER myuser WITH PASSWORD 'mypassword';"
sudo -u postgres psql -c "ALTER USER myuser WITH SUPERUSER;"
```

3. Create the coordinates table:
```bash
psql "host=localhost dbname=mydb user=myuser password=mypassword" -c "
CREATE TABLE coordinates (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    first_name VARCHAR(100),
    first_lat DOUBLE PRECISION,
    first_lon DOUBLE PRECISION,
    first_h3 VARCHAR(16),
    second_name VARCHAR(100),
    second_lat DOUBLE PRECISION,
    second_lon DOUBLE PRECISION,
    second_h3 VARCHAR(16),
    distance DOUBLE PRECISION
);"
```

## Build Instructions

```bash
# Clean previous builds
make clean

# Build both executables
make

# Install web files (optional, for map UI)
make install-web
```

## Usage

### Start API Server

```bash
./build/api_server
```

After starting, the server will display:
```
Server running on port 8080
POST requests to http://localhost:8080/calculate-distance
Map UI available at http://localhost:8080/
API endpoint for coordinates: http://localhost:8080/api/coordinates
```

### Use CLI Interface

```bash
./build/coordinate_logger
```

The CLI will prompt for two locations with their names and coordinates.

### API Endpoints

#### POST `/calculate-distance`

Calculate the distance between two geographic points and store in database.

##### Request Body (JSON)

```json
{
  "name1": "Istanbul",
  "lat1": 41.0151,
  "lon1": 28.9795,
  "name2": "Ankara",
  "lat2": 39.9334,
  "lon2": 32.8597
}
```

##### Response (JSON)

```json
{
  "distance": 419.45,
  "unit": "km"
}
```

#### GET `/`

Serve the interactive map interface for visualizing coordinate data.

#### GET `/api/coordinates`

Retrieve all stored coordinate pairs with their H3 indices.

##### Response (JSON)

```json
[
  {
    "first_name": "Istanbul",
    "first_lat": 41.0151,
    "first_lon": 28.9795,
    "first_h3": "891ec90243bffff",
    "second_name": "Ankara",
    "second_lat": 39.9334,
    "second_lon": 32.8597,
    "second_h3": "891e9462833ffff",
    "distance": 419.45
  }
]
```

### Testing with cURL

```bash
curl -X POST http://localhost:8080/calculate-distance \
  -H "Content-Type: application/json" \
  -d '{ "name1": "Istanbul", "lat1": 41.0151, "lon1": 28.9795, "name2": "Ankara", "lat2": 39.9334, "lon2": 32.8597 }'
```

### Map Interface

Navigate to `http://localhost:8080/` in your browser to view the interactive map interface. The map will:

1. Display markers for each coordinate pair
2. Show lines connecting the pairs
3. Display location names and H3 indices in popups
4. Show a panel with all coordinate data
5. Auto-refresh data every 30 seconds

## H3 Spatial Indexing

This project uses Uber's H3 library for spatial indexing. H3 provides:

- Hierarchical spatial indexing system
- Hexagonal grid tiling
- Fast spatial operations
- Consistent cell sizes

When coordinates are stored, the system automatically calculates and stores their H3 indices at resolution 9. These indices can be used for:

- Fast spatial lookups
- Proximity searches
- Spatial joins
- Geofencing operations

## Troubleshooting

### Database Connection Issues

If you encounter database connection errors:

1. Verify PostgreSQL is running:
```bash
sudo systemctl status postgresql
```

2. Check connection string in `src/api.h`:
```c
#define CONN_STR "host=localhost dbname=mydb user=myuser password=mypassword"
```

3. Test connection manually:
```bash
psql "host=localhost dbname=mydb user=myuser password=mypassword"
```

### H3 Library Issues

If you encounter issues with the H3 library:

1. Ensure it's properly installed:
```bash
ldconfig -p | grep h3
```

2. Check include paths in Makefile:
```makefile
CFLAGS = -Wall -Wextra -std=c99 -I/usr/include/postgresql -I/usr/include/json-c -I/usr/local/include -Isrc
LIBS_LOGGER = -lpq -lm -lh3
LIBS_API = -lpq -lm -ljson-c -lmicrohttpd -lh3
```

### Common Build Errors

1. **Missing headers**: Ensure all development packages are installed
2. **Linking errors**: Verify all libraries are installed and accessible
3. **Database errors**: Ensure the database schema matches the expected structure

## License

MIT

## Contributing

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request