// ==========================================================
// api.js — Shared helper for talking to our backend
// ==========================================================

const API_BASE_URL = "http://localhost:8080";

/**
 * Sends a request to our backend API.
 * @param {string} endpoint - e.g. "/api/auth/login"
 * @param {string} method - "GET", "POST", etc.
 * @param {object|null} body - data to send (for POST requests)
 * @returns {Promise<object>} - the parsed JSON response
 */
async function apiRequest(endpoint, method = "GET", body = null) {
    const options = {
        method: method,
        headers: { "Content-Type": "application/json" }
    };

    if (body) {
        options.body = JSON.stringify(body);
    }

    try {
        const response = await fetch(API_BASE_URL + endpoint, options);
        const data = await response.json();
        return { ok: response.ok, status: response.status, data: data };

    } catch (error) {
        console.error("API request failed:", error);
        return { ok: false, status: 0, data: { message: "Could not connect to the server. Please try again." } };
    }
}