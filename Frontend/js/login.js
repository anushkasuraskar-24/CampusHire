// ==========================================================
// login.js — Handles the login form submission
// ==========================================================

document.addEventListener("DOMContentLoaded", function () {

    const loginForm = document.getElementById("loginForm");
    const errorMessageEl = document.getElementById("loginError");

    if (!loginForm) return;

    loginForm.addEventListener("submit", async function (event) {
        event.preventDefault();   // stops the default page-reload behavior

        errorMessageEl.textContent = "";

        const email = document.getElementById("email").value.trim();
        const password = document.getElementById("password").value;
        const selectedRole = document.getElementById("role").value;

        if (!email || !password || !selectedRole) {
            errorMessageEl.textContent = "Please fill in all fields.";
            return;
        }

        // Call the backend
        const result = await apiRequest("/api/auth/login", "POST", {
            email: email,
            password: password
        });

        if (result.ok) {
            const user = result.data.user;

            // Make sure the role they selected matches their actual account role
            if (user.role !== selectedRole) {
                errorMessageEl.textContent = "This account is not registered as a " + selectedRole + ".";
                return;
            }

            // Store basic user info so other pages (dashboard, etc.) 
            // can know who's logged in. We'll build on this later.
            localStorage.setItem("currentUser", JSON.stringify(user));

            errorMessageEl.style.color = "#1e8e3e";
            errorMessageEl.textContent = "Login successful! Redirecting...";

            // Redirect to the correct dashboard based on role
            setTimeout(function () {
                if (user.role === "student") {
                    window.location.href = "student-dashboard.html";
                } else if (user.role === "recruiter") {
                    window.location.href = "recruiter-dashboard.html";
                } else if (user.role === "admin") {
                    window.location.href = "admin-dashboard.html";
                }
            }, 1000);

        } else {
            errorMessageEl.style.color = "#e94560";
            errorMessageEl.textContent = result.data.message || "Login failed. Please try again.";
        }
    });

});