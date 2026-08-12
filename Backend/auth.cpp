// ==========================================================
// auth.cpp — Implementation of password hashing
// ==========================================================

#include "auth.hpp"
#include <random>
#include <sstream>
#include <iomanip>
#include <functional>

// ----------------------------------------------------
// Generates a random salt string (adds randomness so 
// identical passwords don't produce identical hashes).
// ----------------------------------------------------
static std::string generateSalt() {
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<int> distribution(0, 255);

    std::stringstream saltStream;
    for (int i = 0; i < 16; i++) {
        saltStream << std::hex << std::setw(2) << std::setfill('0') << distribution(generator);
    }
    return saltStream.str();
}

// ----------------------------------------------------
// A simple hash function combining the password + salt.
// Uses std::hash repeatedly to make it slightly harder 
// to reverse than a single pass (not cryptographically 
// strong — see note in auth.hpp).
// ----------------------------------------------------
static std::string simpleHash(const std::string& input) {
    std::hash<std::string> hasher;
    size_t hashValue = hasher(input);

    // Run the hash through itself a few more times ("stretching") 
    // to make brute-forcing slightly slower than a single hash pass.
    for (int i = 0; i < 1000; i++) {
        std::stringstream ss;
        ss << hashValue;
        hashValue = hasher(ss.str());
    }

    std::stringstream result;
    result << std::hex << hashValue;
    return result.str();
}

// ----------------------------------------------------
// PUBLIC: hashPassword
// ----------------------------------------------------
std::string hashPassword(const std::string& plainPassword) {
    std::string salt = generateSalt();
    std::string hashed = simpleHash(plainPassword + salt);

    // We store the salt alongside the hash (separated by '$'), 
    // since we need the SAME salt again later to verify a login attempt.
    return salt + "$" + hashed;
}

// ----------------------------------------------------
// PUBLIC: verifyPassword
// ----------------------------------------------------
bool verifyPassword(const std::string& plainPassword, const std::string& storedHash) {
    // Split the stored value back into its salt and hash parts
    size_t separatorPos = storedHash.find('$');
    if (separatorPos == std::string::npos) {
        return false;   // malformed stored hash
    }

    std::string salt = storedHash.substr(0, separatorPos);
    std::string originalHash = storedHash.substr(separatorPos + 1);

    // Re-hash the attempted password with the SAME salt, 
    // then check if it matches what's stored.
    std::string attemptHash = simpleHash(plainPassword + salt);

    return attemptHash == originalHash;
}