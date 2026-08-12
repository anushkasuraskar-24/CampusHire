// ==========================================================
// recruiter-dashboard.js — Recruiter CRUD dashboard
// ==========================================================

let currentUser = null;

document.addEventListener("DOMContentLoaded", async function () {

    const currentUserStr = localStorage.getItem("currentUser");
    if (!currentUserStr) {
        window.location.href = "login.html";
        return;
    }
    currentUser = JSON.parse(currentUserStr);

    await loadMyJobs();

    // ----------------------------------------------------
    // Post job form
    // ----------------------------------------------------
    document.getElementById("postJobForm").addEventListener("submit", async function (event) {
        event.preventDefault();

        const messageEl = document.getElementById("postJobMessage");

        const jobData = {
            userId: parseInt(currentUser.id),
            title: document.getElementById("jobTitle").value.trim(),
            roleType: document.getElementById("jobRoleType").value,
            description: document.getElementById("jobDescription").value.trim(),
            salaryMin: parseFloat(document.getElementById("jobSalaryMin").value),
            salaryMax: parseFloat(document.getElementById("jobSalaryMax").value),
            minCgpa: parseFloat(document.getElementById("jobMinCgpa").value),
            eligibleBranches: document.getElementById("jobEligibleBranches").value.trim(),
            requiredSkills: document.getElementById("jobRequiredSkills").value.trim()
        };

        const result = await apiRequest("/api/recruiter/jobs", "POST", jobData);

        if (result.ok) {
            messageEl.style.color = "#1e8e3e";
            messageEl.textContent = "Job posted successfully!";
            document.getElementById("postJobForm").reset();
            await loadMyJobs();
        } else {
            messageEl.style.color = "#e94560";
            messageEl.textContent = result.data.message || "Failed to post job.";
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

// ----------------------------------------------------
// Load this recruiter's jobs + stats
// ----------------------------------------------------
async function loadMyJobs() {
    const result = await apiRequest("/api/recruiter/jobs?userId=" + currentUser.id, "GET");

    if (!result.ok) {
        document.getElementById("myPostingsList").innerHTML = "<p>" + (result.data.message || "Could not load jobs.") + "</p>";
        return;
    }

    const jobs = result.data.jobs;
    document.getElementById("companyNameHeader").textContent = result.data.companyName + " — Recruiter Dashboard";
    document.getElementById("statTotalJobs").textContent = jobs.length;

    let totalApplicants = 0;
    jobs.forEach(j => totalApplicants += parseInt(j.applicantCount));
    document.getElementById("statTotalApplicants").textContent = totalApplicants;

    const listEl = document.getElementById("myPostingsList");
    listEl.innerHTML = "";

    if (jobs.length === 0) {
        listEl.innerHTML = "<p>You haven't posted any jobs yet.</p>";
        return;
    }

    jobs.forEach(job => {
        const item = document.createElement("div");
        item.className = "posting-item";
        item.innerHTML = `
            <div class="posting-summary">
                <h3>${job.title}</h3>
                <p>${job.roleType} • Status: ${job.status} • Deadline: ${job.deadline}</p>
            </div>
            <span class="applicant-count-badge">${job.applicantCount} applicants</span>
            <button class="btn btn-secondary btn-small view-applicants-btn" data-job-id="${job.id}" data-job-title="${job.title}">
                View Applicants
            </button>
        `;
        listEl.appendChild(item);
    });

    document.querySelectorAll(".view-applicants-btn").forEach(btn => {
        btn.addEventListener("click", function () {
            const jobId = this.getAttribute("data-job-id");
            const jobTitle = this.getAttribute("data-job-title");
            loadApplicants(jobId, jobTitle);
        });
    });
}

// ----------------------------------------------------
// Load applicants for a specific job
// ----------------------------------------------------
async function loadApplicants(jobId, jobTitle) {
    const result = await apiRequest("/api/recruiter/applicants?jobId=" + jobId, "GET");

    document.getElementById("applicantsSection").style.display = "block";
    document.getElementById("selectedJobTitle").textContent = jobTitle;

    const tbody = document.getElementById("applicantsTableBody");
    tbody.innerHTML = "";

    if (!result.ok || result.data.applicants.length === 0) {
        tbody.innerHTML = "<tr><td colspan='6'>No applicants yet.</td></tr>";
        return;
    }

    result.data.applicants.forEach(app => {
        const row = document.createElement("tr");
        row.innerHTML = `
            <td>${app.fullName}</td>
            <td>${app.email}</td>
            <td>${app.branch}</td>
            <td>${app.cgpa}</td>
            <td><span class="status-badge status-${app.status}">${app.status}</span></td>
            <td>
                <select class="status-update-select" data-application-id="${app.applicationId}">
                    <option value="applied" ${app.status === 'applied' ? 'selected' : ''}>Applied</option>
                    <option value="shortlisted" ${app.status === 'shortlisted' ? 'selected' : ''}>Shortlisted</option>
                    <option value="interview" ${app.status === 'interview' ? 'selected' : ''}>Interview</option>
                    <option value="selected" ${app.status === 'selected' ? 'selected' : ''}>Selected</option>
                    <option value="rejected" ${app.status === 'rejected' ? 'selected' : ''}>Rejected</option>
                </select>
            </td>
        `;
        tbody.appendChild(row);
    });

    // Attach change listeners to update status dropdowns
    document.querySelectorAll(".status-update-select").forEach(select => {
        select.addEventListener("change", async function () {
            const applicationId = this.getAttribute("data-application-id");
            const newStatus = this.value;

            await apiRequest("/api/applications/status", "PUT", {
                applicationId: parseInt(applicationId),
                status: newStatus
            });

            // Reload applicants to reflect the update
            loadApplicants(jobId, jobTitle);
        });
    });
}