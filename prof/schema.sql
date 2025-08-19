-- schema.sql: Friend Locator Application Database Schema

-- Enable PostGIS extension if not already enabled
CREATE EXTENSION IF NOT EXISTS postgis;

-- Table: users
-- Stores user account information.
CREATE TABLE IF NOT EXISTS users (
    id SERIAL PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    password_hash TEXT NOT NULL, -- Store hashed passwords
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- Index on username for faster lookups
CREATE INDEX IF NOT EXISTS idx_users_username ON users(username);

-- Table: user_sessions
-- Manages user login sessions.
CREATE TABLE IF NOT EXISTS user_sessions (
    session_token VARCHAR(128) PRIMARY KEY, -- Secure random token
    user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    expires_at TIMESTAMP WITH TIME ZONE NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- Index on session_token for fast retrieval
CREATE INDEX IF NOT EXISTS idx_user_sessions_token ON user_sessions(session_token);

-- Index on user_id for faster lookups by user
CREATE INDEX IF NOT EXISTS idx_user_sessions_user_id ON user_sessions(user_id);

-- Table: user_locations
-- Stores the latest known location for each user.
CREATE TABLE IF NOT EXISTS user_locations (
    user_id INTEGER PRIMARY KEY REFERENCES users(id) ON DELETE CASCADE,
    -- Using PostGIS geometry type for storing point coordinates (SRID 4326 for WGS84)
    location GEOMETRY(Point, 4326) NOT NULL,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- Index on location for spatial queries (e.g., finding nearby users)
CREATE INDEX IF NOT EXISTS idx_user_locations_location ON user_locations USING GIST(location);

-- Index on updated_at for time-based queries
CREATE INDEX IF NOT EXISTS idx_user_locations_updated_at ON user_locations(updated_at);

-- Table: friendships
-- Represents friendship requests and relationships between users.
CREATE TABLE IF NOT EXISTS friendships (
    user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    friend_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    status VARCHAR(20) NOT NULL DEFAULT 'pending', -- 'pending', 'accepted', 'blocked'
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (user_id, friend_id),
    -- Ensure user_id < friend_id to prevent duplicate pairs (e.g., (1,2) and (2,1))
    CONSTRAINT chk_friendship_order CHECK (user_id < friend_id),
    -- Ensure status is one of the allowed values
    CONSTRAINT chk_friendship_status CHECK (status IN ('pending', 'accepted', 'blocked'))
);

-- Index on user_id for querying friendships of a user
CREATE INDEX IF NOT EXISTS idx_friendships_user_id ON friendships(user_id);

-- Index on friend_id for querying who has a specific user as a friend
CREATE INDEX IF NOT EXISTS idx_friendships_friend_id ON friendships(friend_id);

-- Index on status for filtering by friendship status
CREATE INDEX IF NOT EXISTS idx_friendships_status ON friendships(status);

-- Trigger function to update 'updated_at' column on row update
CREATE OR REPLACE FUNCTION update_updated_at_column()
RETURNS TRIGGER AS $$
BEGIN
   NEW.updated_at = CURRENT_TIMESTAMP;
   RETURN NEW;
END;
$$ language 'plpgsql';

-- Trigger to automatically update 'updated_at' in the friendships table
DROP TRIGGER IF EXISTS update_friendships_updated_at ON friendships;
CREATE TRIGGER update_friendships_updated_at
BEFORE UPDATE ON friendships
FOR EACH ROW
EXECUTE FUNCTION update_updated_at_column();

-- Trigger to automatically update 'updated_at' in the user_locations table
DROP TRIGGER IF EXISTS update_user_locations_updated_at ON user_locations;
CREATE TRIGGER update_user_locations_updated_at
BEFORE UPDATE ON user_locations
FOR EACH ROW
EXECUTE FUNCTION update_updated_at_column();