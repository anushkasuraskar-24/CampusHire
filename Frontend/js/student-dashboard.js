// ==========================================================
// student-dashboard.js — Loads real student data + jobs
// ==========================================================

document.addEventListener("DOMContentLoaded", async function () {

    const currentUserStr = localStorage.getItem("currentUser");
    if (!currentUserStr) {
        window.location.href = "login.html";
        return;
    }
    const currentUser = JSON.parse(currentUserStr);

    // ----------------------------------------------------
    // Load profile info
    // ----------------------------------------------------
    const profileResult = await apiRequest("/api/student/profile?userId=" + currentUser.id, "GET");

    if (profileResult.ok) {
        const profile = profileResult.data.profile;

        const nameEl = document.getElementById("studentName");
        const nameDetailEl = document.getElementById("studentNameDetail");
        if (nameDetailEl) nameDetailEl.textContent = profile.fullName;
        if (nameEl) nameEl.textContent = profile.fullName;

        const branchEl = document.getElementById("studentBranch");
        if (branchEl) branchEl.textContent = profile.branch || "Not set";

        const cgpaEl = document.getElementById("studentCgpa");
        if (cgpaEl) cgpaEl.textContent = profile.cgpa || "Not set";

        const skillsEl = document.getElementById("studentSkills");
        if (skillsEl) {
            skillsEl.innerHTML = "";
            (profile.skills || []).forEach(skill => {
                const tag = document.createElement("span");
                tag.className = "skill-tag";
                tag.textContent = skill;
                skillsEl.appendChild(tag);
            });
        }
    }

    // ----------------------------------------------------
    // Load recent jobs (show a preview, e.g. first 3)
    // ----------------------------------------------------
    const jobsResult = await apiRequest("/api/jobs", "GET");
    const jobsContainer = document.getElementById("jobsPreview");

    if (jobsResult.ok && jobsContainer) {
        jobsContainer.innerHTML = "";
        const jobsToShow = jobsResult.data.jobs.slice(0, 3);

        jobsToShow.forEach(job => {
            const card = document.createElement("div");
            card.className = "job-card-full";
            card.innerHTML = `
                <h3>${job.title}</h3>
                <p class="company-name">${job.companyName}</p>
                <p>${job.description}</p>
                <span class="salary-tag">₹${job.salaryMin} - ₹${job.salaryMax} LPA</span>
            `;
            jobsContainer.appendChild(card);
        });
    }

    // ----------------------------------------------------
    // Load applications table
    // ----------------------------------------------------
    const appsResult = await apiRequest("/api/student/applications?userId=" + currentUser.id, "GET");
    const appsTableBody = document.getElementById("applicationsTableBody");

    if (appsResult.ok && appsTableBody) {
        appsTableBody.innerHTML = "";

        appsResult.data.applications.forEach(app => {
            const row = document.createElement("tr");
            row.innerHTML = `
                <td>${app.jobTitle}</td>
                <td>${app.companyName}</td>
                <td><span class="status-badge status-${app.status}">${app.status}</span></td>
                <td>${app.appliedAt}</td>
            `;
            appsTableBody.appendChild(row);
        });
    }

    // ----------------------------------------------------
    // Logout button (if present)
    // ----------------------------------------------------
    const logoutBtn = document.getElementById("logoutBtn");
    if (logoutBtn) {
        logoutBtn.addEventListener("click", function () {
            localStorage.removeItem("currentUser");
            window.location.href = "login.html";
        });
    }
});