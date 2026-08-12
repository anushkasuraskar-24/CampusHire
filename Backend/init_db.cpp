// ==========================================================
// init_db.cpp — One-time database setup utility
// ==========================================================
// Run this once to create placement.db from schema.sql.
// After running, the real server (main.cpp) will just OPEN 
// the existing database — it won't need this file again.

#include <sqlite3.h>
#include <fstream>
#include <sstream>
#include <iostream>

int main() {

    // ----------------------------------------------------
    // STEP 1: Read schema.sql into a string
    // ----------------------------------------------------
    std::ifstream schemaFile("database/schema.sql");
    if (!schemaFile.is_open()) {
        std::cerr << "ERROR: Could not open database/schema.sql. "
                  << "Make sure you run this from the Backend folder." << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << schemaFile.rdbuf();   // reads the whole file into buffer
    std::string schemaSQL = buffer.str();
    schemaFile.close();

    // ----------------------------------------------------
    // STEP 2: Open (or create) the database file
    // ----------------------------------------------------
    sqlite3* db;
    int result = sqlite3_open("database/placement.db", &db);

    if (result != SQLITE_OK) {
        std::cerr << "ERROR: Could not open database: " 
                  << sqlite3_errmsg(db) << std::endl;
        return 1;
    }

    std::cout << "Database opened successfully at database/placement.db" << std::endl;

    // ----------------------------------------------------
    // STEP 3: Execute the schema SQL
    // ----------------------------------------------------
    char* errorMessage = nullptr;
    result = sqlite3_exec(db, schemaSQL.c_str(), nullptr, nullptr, &errorMessage);

    if (result != SQLITE_OK) {
        std::cerr << "ERROR executing schema: " << errorMessage << std::endl;
        sqlite3_free(errorMessage);
        sqlite3_close(db);
        return 1;
    }

    std::cout << "All tables created successfully!" << std::endl;

    // ----------------------------------------------------
    // STEP 4: Close the database
    // ----------------------------------------------------
    sqlite3_close(db);

    return 0;
}