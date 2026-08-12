// ==========================================================
// admin-dashboard.js — Platform analytics for admin
// ==========================================================

let allStudents = [];

document.addEventListener("DOMContentLoaded", async function () {

    const currentUserStr = localStorage.getItem("currentUser");
    if (!currentUserStr) {
        window.location.href = "login.html";
        return;
    }

    await loadStats();
    await loadStudents();
    await loadCompanies();

    document.getElementById("studentSearch").addEventListener("input", function () {
        const term = this.value.toLowerCase();
        const filtered = allStudents.filter(s =>
            s.fullName.toLowerCase().includes(term) || s.email.toLowerCase().includes(term)
        );
        renderStudents(filtered);
    });

    document.getElementById("logoutBtn").addEventListener("click", function () {
        localStorage.removeItem("currentUser");
        window.location.href = "login.html";
    });
});

async function loadStats() {
    const result = await apiRequest("/api/admin/stats", "GET");
    if (!result.ok) return;

    const stats = result.data.stats;
    document.getElementById("statStudents").textContent = stats.totalStudents;
    document.getElementById("statRecruiters").textContent = stats.totalRecruiters;
    document.getElementById("statCompanies").textContent = stats.totalCompanies;
    document.getElementById("statJobs").textContent = stats.activeJobs;
    document.getElementById("statApplications").textContent = stats.totalApplications;
    document.getElementById("statSelected").textContent = stats.totalSelected;
}

async function loadStudents() {
    const result = await apiRequest("/api/admin/students", "GET");
    if (!result.ok) return;

    allStudents = result.data.students;
    renderStudents(allStudents);
}

function renderStudents(students) {
    const tbody = document.getElementById("studentsTableBody");
    tbody.innerHTML = "";

    students.forEach(s => {
        const row = document.createElement("tr");
        row.innerHTML = `
            <td>${s.fullName}</td>
            <td>${s.email}</td>
            <td>${s.branch || "-"}</td>
            <td>${s.cgpa || "-"}</td>
        `;
        tbody.appendChild(row);
    });
}

async function loadCompanies() {
    const result = await apiRequest("/api/admin/companies", "GET");
    if (!result.ok) return;

    const tbody = document.getElementById("companiesTableBody");
    tbody.innerHTML = "";

    result.data.companies.forEach(c => {
        const row = document.createElement("tr");
        row.innerHTML = `
            <td>${c.name}</td>
            <td>${c.industry}</td>
            <td>${c.jobCount}</td>
        `;
        tbody.appendChild(row);
    });
}