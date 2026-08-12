// ==========================================================
// auth.hpp — Password hashing utilities
// ==========================================================
// NOTE: This uses a simplified salted-hash approach for 
// learning/demo purposes. Production systems should use a 
// dedicated library like bcrypt or Argon2, which are designed 
// to resist brute-force attacks far better than a hand-rolled 
// hash. This is a reasonable simplification for a student 
// resume project, but worth knowing the real-world standard.

#pragma once

#include <string>

// Turns a plain-text password into a salted hash string, 
// safe to store in the database.
std::string hashPassword(const std::string& plainPassword);

// Checks whether a plain-text password matches a previously 
// stored hash. Used when a user tries to log in.
bool verifyPassword(const std::string& plainPassword, const std::string& storedHash);