// ==========================================================
// database.hpp — Reusable database connection wrapper
// ==========================================================
// This class wraps SQLite3's C API so the rest of our backend 
// code doesn't need to deal with raw sqlite3* pointers and 
// manual error checking everywhere. Every route (register, 
// login, jobs, etc.) will use this same class to talk to 
// placement.db.

#pragma once   // prevents this header from being included twice in the same file

#include <sqlite3.h>
#include <string>
#include <vector>
#include <map>

// A single "row" of query results, represented as column-name -> value pairs.
// e.g., {"id": "1", "email": "test@example.com"}
using DBRow = std::map<std::string, std::string>;

class Database {
public:
    // Constructor: opens a connection to the given .db file.
    // Throws a std::runtime_error if the connection fails.
    explicit Database(const std::string& dbPath);

    // Destructor: automatically closes the connection when 
    // this object is destroyed (RAII pattern — no manual cleanup needed).
    ~Database();

    // Runs a SQL statement that doesn't return rows 
    // (INSERT, UPDATE, DELETE, CREATE TABLE, etc.)
    // Returns true on success, false on failure.
    bool execute(const std::string& sql);

    // Runs a SQL SELECT query and returns all matching rows.
    // Each row is a map of column name -> value (as strings).
    std::vector<DBRow> query(const std::string& sql);

    // Returns the ID of the most recently inserted row 
    // (useful right after an INSERT, e.g. to get a new user's ID).
    long long lastInsertRowId();

    // Returns the last error message from SQLite, useful for debugging.
    std::string lastError();

private:
    sqlite3* db;   // the raw SQLite3 connection handle
};