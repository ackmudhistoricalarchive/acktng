-- Migration: rename mobiles.cast -> mobiles.cast_flags
--
-- Apply to databases created from the original docs/proposals/schema.sql
-- (schema version 2) where the column was named "cast" instead of "cast_flags".
-- "cast" is a SQL reserved word; the column was renamed to avoid conflicts.
--
-- Usage:
--   psql -U ack -d acktng -f migrate_v2_cast_rename.sql

ALTER TABLE mobiles RENAME COLUMN "cast" TO cast_flags;
