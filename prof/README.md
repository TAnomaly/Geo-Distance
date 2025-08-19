# Location Sharing System - Modular Architecture

A comprehensive location-sharing and route-finding system that uses Uber H3 Algorithm, Kring, and A* pathfinding to help friends share live locations and find optimal routes.

## 🏗️ Architecture Overview

The system has been reorganized into a clean, modular architecture following the principle of separation of concerns:

```
prof/
├── src/
│   ├── main.c                    # Main entry point
│   ├── api_server.h              # API server interface
│   ├── api_server_new.c          # Clean HTTP server implementation
│   ├── api.h                     # Configuration and constants
│   ├── coordinate_logger.c       # Coordinate logging utilities
│   ├── coordinate_logger.h       # Coordinate logging interface
│   ├── auth/                     # Authentication module
│   │   ├── auth.h               # Authentication interface
│   │   └── auth.c               # User management & authentication
│   ├── location/                 # Location management module
│   │   ├── location.h           # Location interface
│   │   └── location.c           # Location operations & H3 integration
│   ├── routing/                  # Route finding module
│   │   ├── routing.h            # Routing interface
│   │   ├── routing.c            # Route calculation algorithms
│   │   ├── h3_routing.c         # H3-based routing (future)
│   │   ├── astar_routing.c      # A* algorithm (future)
│   │   └── kring_routing.c      # Kring algorithm (future)
│   └── utils/                    # Utility functions
│       ├── utils.h              # Utilities interface
│       └── utils.c              # Common utilities
├── web/                          # Frontend files
│   ├── index.html
│   ├── style.css
│   └── script.js
├── build/                        # Build output directory
├── Makefile_new                  # Build system for modular structure
├── schema.sql                    # Database schema
└── README_NEW_STRUCTURE.md       # This file
```

## 🚀 Key Features

### Core Functionality
- **User Authentication**: Secure registration, login, and session management
- **Location Sharing**: Real-time location updates with H3 geospatial indexing
- **Friend Management**: Add friends and view their locations
- **Route Finding**: Multiple algorithms for optimal path calculation
- **Distance Calculation**: H3 and A* based distance measurements

### Algorithms Implemented
1. **Uber H3 Algorithm**: Geospatial indexing for efficient location queries
2. **A* Pathfinding**: Optimal route calculation between points
3. **Kring Algorithm**: Finding nearby places and points of interest
4. **Haversine Distance**: Accurate distance calculations on Earth's surface

## 🛠️ Installation & Setup

### Prerequisites
- GCC compiler
- PostgreSQL database
- Required libraries (see dependencies below)

### Dependencies
```bash
# Ubuntu/Debian
sudo apt-get install libmicrohttpd-dev libjson-c-dev libpq-dev libssl-dev libh3-dev

# CentOS/RHEL/Fedora
sudo yum install libmicrohttpd-devel json-c-devel postgresql-devel openssl-devel h3-devel
```

### Build Instructions
```bash
# Clone and navigate to project
cd prof

# Install dependencies
make -f Makefile_new install-deps

# Build the application
make -f Makefile_new

# Run the application
make -f Makefile_new run
```

## 📊 Database Setup

1. Create a PostgreSQL database
2. Run the schema file:
```bash
psql -d your_database_name -f schema.sql
```

3. Update the connection string in `src/api.h`:
```c
#define CONN_STR "host=localhost dbname=your_database_name user=your_username password=your_password"
```

## 🔌 API Endpoints

### Authentication
- `POST /api/register` - User registration
- `POST /api/login` - User login
- `POST /api/logout` - User logout
- `GET /api/user` - Get user information

### Location Management
- `POST /api/save-location` - Save user location
- `GET /api/friends/locations` - Get friends' locations

### Social Features
- `POST /api/add-friend` - Add a friend
- `GET /api/friends` - Get friends list

### Route Finding
- `GET /api/route` - Calculate route between points
- `GET /api/distance/h3` - H3 distance calculation
- `GET /api/distance/astar` - A* distance calculation

### Legacy Support
- `POST /calculate-distance` - Legacy distance calculation endpoint

## 🧩 Module Details

### Authentication Module (`auth/`)
- **Purpose**: User management and session handling
- **Key Functions**:
  - `register_user()` - Create new user accounts
  - `login_user()` - Authenticate users
  - `validate_session_token()` - Verify session validity
  - `add_friend()` - Manage friend relationships

### Location Module (`location/`)
- **Purpose**: Location storage and retrieval with H3 integration
- **Key Functions**:
  - `save_user_location()` - Store user locations with H3 indexing
  - `get_friends_locations_from_db()` - Retrieve friends' locations
  - `calculate_h3_distance()` - H3-based distance calculation
  - `calculate_astar_distance()` - A* pathfinding distance

### Routing Module (`routing/`)
- **Purpose**: Route calculation and pathfinding algorithms
- **Key Functions**:
  - `calculate_route()` - Main route calculation function
  - `find_nearby_places()` - Kring-based nearby place discovery
  - `get_kring_cells()` - Generate H3 kring cells

### Utilities Module (`utils/`)
- **Purpose**: Common utility functions
- **Key Functions**:
  - `read_file_content()` - File reading utilities
  - `create_json_response()` - HTTP response helpers
  - `queue_response_with_cors()` - CORS handling

## 🔧 Configuration

### Environment Variables
Set these in `src/api.h`:
```c
#define PORT 8080                    // Server port
#define WEB_ROOT "./web"             // Web files directory
#define CONN_STR "your_db_connection_string"
```

### H3 Configuration
- **Resolution**: Currently set to 9 (adjustable in location.c)
- **Indexing**: Automatic H3 index generation for all locations
- **Distance**: H3-based distance calculations for efficiency

## 🚀 Usage Examples

### Register a New User
```bash
curl -X POST http://localhost:8080/api/register \
  -H "Content-Type: application/json" \
  -d '{"username": "john_doe", "password": "secure_password"}'
```

### Save User Location
```bash
curl -X POST http://localhost:8080/api/save-location \
  -H "Content-Type: application/json" \
  -d '{"user_id": "123", "latitude": 41.0151, "longitude": 28.9795, "accuracy": 50}'
```

### Calculate Route
```bash
curl -X GET "http://localhost:8080/api/route?start_lat=41.0151&start_lon=28.9795&end_id=456" \
  -H "Authorization: Bearer your_session_token"
```

## 🔍 Algorithm Details

### H3 Algorithm
- **Purpose**: Geospatial indexing for efficient location queries
- **Benefits**: 
  - Fast proximity searches
  - Hierarchical resolution support
  - Consistent hexagonal grid system

### A* Pathfinding
- **Purpose**: Optimal route calculation
- **Implementation**: Simplified version using H3 grid
- **Future**: Full A* implementation with obstacle avoidance

### Kring Algorithm
- **Purpose**: Finding nearby places and points of interest
- **Usage**: Generates concentric rings of H3 cells around a point
- **Applications**: Nearby restaurant discovery, meeting point suggestions

## 🧪 Testing

### Build and Test
```bash
# Debug build with extra logging
make -f Makefile_new debug

# Run with debug output
./build/location_sharing_system
```

### API Testing
Use the provided web interface at `http://localhost:8080/` or test endpoints directly with curl.

## 🔮 Future Enhancements

### Planned Features
1. **Enhanced A* Implementation**: Full pathfinding with real-world constraints
2. **Kring Optimization**: Improved nearby place discovery
3. **Real-time Updates**: WebSocket support for live location updates
4. **Mobile App**: Native mobile applications
5. **Advanced Analytics**: Location history and movement patterns

### Algorithm Improvements
1. **Multi-resolution H3**: Dynamic resolution based on zoom level
2. **Traffic Integration**: Real-time traffic data for route optimization
3. **Public Transport**: Multi-modal route planning
4. **Geofencing**: Location-based notifications and alerts

## 🤝 Contributing

1. Follow the modular architecture
2. Add new features in appropriate modules
3. Update documentation for new endpoints
4. Test thoroughly before submitting

## 📝 License

This project is open source. Please refer to the license file for details.

## 🆘 Support

For issues and questions:
1. Check the documentation
2. Review the code structure
3. Test with the provided examples
4. Create an issue with detailed information

---

**Note**: This modular structure makes the codebase much more maintainable and follows software engineering best practices. Each module has a single responsibility and clear interfaces, making it easier to extend and modify the system.
