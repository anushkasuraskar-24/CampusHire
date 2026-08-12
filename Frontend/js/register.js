// ==========================================================
// register.js — Handles the registration form submission
// ==========================================================

// Wait until the page's HTML is fully loaded before running 
// this code, so we're sure the form elements actually exist.
document.addEventListener("DOMContentLoaded", function () {

    const registerForm = document.getElementById("registerForm");
    const errorMessageEl = document.getElementById("registerError");

    // If this page doesn't have a registerForm (shouldn't happen, 
    // but a safe check), stop here.
    if (!registerForm) return;

    registerForm.addEventListener("submit", async function (event) {

        // THIS is the key line that fixes the issue you saw earlier — 
        // it stops the browser's default "reload page with URL params" 
        // behavior, so we can handle the submission ourselves instead.
        event.preventDefault();

        // Clear any previous error message
        errorMessageEl.textContent = "";

        // Collect form values using their "name" attributes
        const fullName = document.getElementById("fullName").value.trim();
        const email = document.getElementById("email").value.trim();
        const password = document.getElementById("password").value;
        const confirmPassword = document.getElementById("confirmPassword").value;
        const role = document.getElementById("role").value;

        // ----------------------------------------------------
        // BASIC CLIENT-SIDE VALIDATION
        // ----------------------------------------------------
        // Quick checks before even contacting the server — saves 
        // an unnecessary network request for obvious mistakes.
        if (!fullName || !email || !password || !role) {
            errorMessageEl.textContent = "Please fill in all required fields.";
            return;
        }

        if (password !== confirmPassword) {
            errorMessageEl.textContent = "Passwords do not match.";
            return;
        }

        if (password.length < 6) {
            errorMessageEl.textContent = "Password must be at least 6 characters.";
            return;
        }

        // ----------------------------------------------------
        // CALL THE BACKEND
        // ----------------------------------------------------
        const result = await apiRequest("/api/auth/register", "POST", {
            fullName: fullName,
            email: email,
            password: password,
            role: role
        });

        if (result.ok) {
            // Success — show a message and redirect to login shortly after
            errorMessageEl.style.color = "#1e8e3e";  // green, since this is good news
            errorMessageEl.textContent = "Account created successfully! Redirecting to login...";

            setTimeout(function () {
                window.location.href = "login.html";
            }, 1500);

        } else {
            // Something went wrong (e.g., email already exists) — 
            // show the backend's error message directly.
            errorMessageEl.style.color = "#e94560";  // red, our usual error color
            errorMessageEl.textContent = result.data.message || "Registration failed. Please try again.";
        }
    });

});