# ==========================================================
# CMAKE MINIMUM VERSION & PROJECT DECLARATION
# ==========================================================
# Requires at least CMake 3.16 (a safe modern baseline).
cmake_minimum_required(VERSION 3.16)

# Declares our project's name — this can be anything descriptive.
project(PlacementPortalBackend)

# ==========================================================
# C++ STANDARD
# ==========================================================
# Crow requires C++17 features. This tells the compiler to 
# use the C++17 standard, and makes it a hard requirement 
# (the build will fail loudly instead of silently using an 
# older standard if C++17 isn't supported).
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==========================================================
# FIND DEPENDENCIES (installed via vcpkg)
# ==========================================================
# These commands tell CMake to look for the Crow and SQLite3 
# packages. They were installed earlier using:
#   vcpkg install crow sqlite3 --triplet x64-mingw-dynamic
#
# For CMake to actually FIND these packages, we must pass a 
# special flag when running cmake (the "toolchain file"). 
# We'll do that in the terminal command itself later — this 
# file doesn't need to reference that path directly.
find_package(Crow CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)

# ==========================================================
# DEFINE OUR EXECUTABLE
# ==========================================================
# This creates our final program, named "server", built from 
# main.cpp. As we add more .cpp files later (database.cpp, 
# routes.cpp, etc.), we'll add them to this list too.
add_executable(server main.cpp database.cpp auth.cpp)

# One-time database setup utility
add_executable(init_db init_db.cpp database.cpp auth.cpp)
target_link_libraries(init_db PRIVATE unofficial::sqlite3::sqlite3)

# One-time database seeding utility
add_executable(seed_data seed_data.cpp database.cpp auth.cpp)
target_link_libraries(seed_data PRIVATE unofficial::sqlite3::sqlite3)

# ==========================================================
# LINK LIBRARIES
# ==========================================================
# This tells the linker to connect our program with the 
# actual Crow and SQLite3 library code, so we can #include 
# and use them in main.cpp.
target_link_libraries(server PRIVATE Crow::Crow)
target_link_libraries(server PRIVATE unofficial::sqlite3::sqlite3)

# Windows-specific networking libraries required by Asio (used internally by Crow)
if(WIN32)
    target_link_libraries(server PRIVATE ws2_32 mswsock)
endif()