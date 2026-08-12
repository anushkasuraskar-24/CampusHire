// ==========================================================
// profile.js — Load and save student profile
// ==========================================================

let currentUser = null;

document.addEventListener("DOMContentLoaded", async function () {

    const currentUserStr = localStorage.getItem("currentUser");
    if (!currentUserStr) {
        window.location.href = "login.html";
        return;
    }
    currentUser = JSON.parse(currentUserStr);

    // ----------------------------------------------------
    // Load existing profile data into the form
    // ----------------------------------------------------
    const result = await apiRequest("/api/student/profile?userId=" + currentUser.id, "GET");

    if (result.ok) {
        const profile = result.data.profile;

        document.getElementById("fullName").value = profile.fullName || "";
        document.getElementById("email").value = profile.email || "";
        document.getElementById("branch").value = profile.branch || "";
        document.getElementById("cgpa").value = profile.cgpa || "";
        document.getElementById("gradYear").value = profile.graduationYear || "";
        document.getElementById("phone").value = profile.phone || "";
        document.getElementById("skills").value = (profile.skills || []).join(", ");

        const initial = (profile.fullName || "A").charAt(0).toUpperCase();
        document.getElementById("avatarInitial").textContent = initial;
    }

    // ----------------------------------------------------
    // Handle form submit — save changes
    // ----------------------------------------------------
    document.getElementById("profileForm").addEventListener("submit", async function (event) {
        event.preventDefault();

        const messageEl = document.getElementById("profileMessage");

        const updateData = {
            userId: parseInt(currentUser.id),
            fullName: document.getElementById("fullName").value.trim(),
            branch: document.getElementById("branch").value,
            cgpa: parseFloat(document.getElementById("cgpa").value),
            gradYear: parseInt(document.getElementById("gradYear").value),
            phone: document.getElementById("phone").value.trim(),
            skills: document.getElementById("skills").value.trim()
        };

        const saveResult = await apiRequest("/api/student/profile", "PUT", updateData);

        if (saveResult.ok) {
            messageEl.style.color = "#1e8e3e";
            messageEl.textContent = "Profile updated successfully!";

            // Update localStorage's cached name too, so the dashboard 
            // greeting shows the updated name immediately
            currentUser.fullName = updateData.fullName;
            localStorage.setItem("currentUser", JSON.stringify(currentUser));
        } else {
            messageEl.style.color = "#e94560";
            messageEl.textContent = saveResult.data.message || "Failed to update profile.";
        }
    });

    // ----------------------------------------------------
    // Logout
    // ----------------------------------------------------
    document.getElementById("logoutBtn").addEventListener("click", function () {
        localStorage.removeItem("currentUser");
        window.location.href = "login.html";
    });
});