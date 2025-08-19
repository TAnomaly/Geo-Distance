-- Migration script to add H3 columns to coordinates table

-- Add first_h3 column
ALTER TABLE coordinates ADD COLUMN IF NOT EXISTS first_h3 VARCHAR(16);

-- Add second_h3 column
ALTER TABLE coordinates ADD COLUMN IF NOT EXISTS second_h3 VARCHAR(16);

-- Update existing rows to have H3 indices (optional, for backward compatibility)
-- This would require calculating H3 indices for existing data