// ==========================================================
// database.cpp — Implementation of the Database class
// ==========================================================

#include "database.hpp"
#include <stdexcept>

// ----------------------------------------------------
// CONSTRUCTOR: Opens the database connection
// ----------------------------------------------------
Database::Database(const std::string& dbPath) {
    int result = sqlite3_open(dbPath.c_str(), &db);

    if (result != SQLITE_OK) {
        // If opening fails, throw an exception — this stops 
        // the program immediately with a clear error, rather 
        // than silently continuing with a broken connection.
        std::string error = sqlite3_errmsg(db);
        sqlite3_close(db);
        throw std::runtime_error("Failed to open database: " + error);
    }

    // Enable foreign key constraint enforcement for this connection 
    // (SQLite has this OFF by default, even though our schema defines them).
    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
}

// ----------------------------------------------------
// DESTRUCTOR: Closes the connection automatically
// ----------------------------------------------------
Database::~Database() {
    sqlite3_close(db);
}

// ----------------------------------------------------
// EXECUTE: For INSERT / UPDATE / DELETE / CREATE statements
// ----------------------------------------------------
bool Database::execute(const std::string& sql) {
    char* errorMessage = nullptr;
    int result = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errorMessage);

    if (result != SQLITE_OK) {
        sqlite3_free(errorMessage);
        return false;
    }
    return true;
}

// ----------------------------------------------------
// QUERY: For SELECT statements — returns all matching rows
// ----------------------------------------------------
std::vector<DBRow> Database::query(const std::string& sql) {
    std::vector<DBRow> results;
    sqlite3_stmt* statement;

    // Prepare the SQL statement for execution
    int result = sqlite3_prepare_v2(db, sql.c_str(), -1, &statement, nullptr);
    if (result != SQLITE_OK) {
        return results;   // return empty results on failure
    }

    // sqlite3_step() executes the query one row at a time.
    // We keep looping until there are no more rows (SQLITE_DONE).
    while (sqlite3_step(statement) == SQLITE_ROW) {
        DBRow row;
        int columnCount = sqlite3_column_count(statement);

        // For each column in this row, grab its name and value
        for (int i = 0; i < columnCount; i++) {
            std::string columnName = sqlite3_column_name(statement, i);
            const unsigned char* value = sqlite3_column_text(statement, i);

            // sqlite3_column_text returns nullptr for NULL values — 
            // we convert that to an empty string instead.
            row[columnName] = value ? reinterpret_cast<const char*>(value) : "";
        }

        results.push_back(row);
    }

    // Clean up the prepared statement — required to avoid memory leaks
    sqlite3_finalize(statement);

    return results;
}

// ----------------------------------------------------
// LAST INSERT ROW ID: Get the ID of the row we just inserted
// ----------------------------------------------------
long long Database::lastInsertRowId() {
    return sqlite3_last_insert_rowid(db);
}

// ----------------------------------------------------
// LAST ERROR: For debugging failed queries
// ----------------------------------------------------
std::string Database::lastError() {
    return sqlite3_errmsg(db);
}