// ==========================================================
// jobs.js — Browse jobs, filter, and apply
// ==========================================================

let allJobs = [];   // cache of all jobs, so filtering doesn't need a new API call
let currentUser = null;
let appliedJobIds = new Set();   // job IDs the student already applied to

document.addEventListener("DOMContentLoaded", async function () {

    const currentUserStr = localStorage.getItem("currentUser");
    if (!currentUserStr) {
        window.location.href = "login.html";
        return;
    }
    currentUser = JSON.parse(currentUserStr);

    // Load jobs and this student's existing applications in parallel
    const [jobsResult, appsResult] = await Promise.all([
        apiRequest("/api/jobs", "GET"),
        apiRequest("/api/student/applications?userId=" + currentUser.id, "GET")
    ]);

    if (jobsResult.ok) {
        allJobs = jobsResult.data.jobs;
    }

    if (appsResult.ok) {
        // We only have jobTitle/companyName from this endpoint, not jobId directly,
        // so instead we'll just re-check on apply (server prevents duplicates anyway).
    }

    renderJobs(allJobs);

    // ----------------------------------------------------
    // Search + filter listeners
    // ----------------------------------------------------
    document.getElementById("searchInput").addEventListener("input", applyFilters);
    document.getElementById("branchFilter").addEventListener("change", applyFilters);

    // ----------------------------------------------------
    // Logout
    // ----------------------------------------------------
    const logoutBtn = document.getElementById("logoutBtn");
    if (logoutBtn) {
        logoutBtn.addEventListener("click", function () {
            localStorage.removeItem("currentUser");
            window.location.href = "login.html";
        });
    }
});

function applyFilters() {
    const searchTerm = document.getElementById("searchInput").value.toLowerCase();
    const branch = document.getElementById("branchFilter").value;

    const filtered = allJobs.filter(job => {
        const matchesSearch = job.title.toLowerCase().includes(searchTerm) 
            || job.companyName.toLowerCase().includes(searchTerm);
        const matchesBranch = !branch || job.eligibleBranches.includes(branch);
        return matchesSearch && matchesBranch;
    });

    renderJobs(filtered);
}

function renderJobs(jobs) {
    const grid = document.getElementById("jobsGrid");
    grid.innerHTML = "";

    if (jobs.length === 0) {
        grid.innerHTML = "<p>No jobs match your search.</p>";
        return;
    }

    jobs.forEach(job => {
        const card = document.createElement("div");
        card.className = "job-card-full";

        const skillsHtml = (job.requiredSkills || [])
            .map(s => `<span class="skill-tag">${s}</span>`)
            .join("");

        card.innerHTML = `
            <h3>${job.title}</h3>
            <p class="company-name">${job.companyName}</p>
            <p>${job.description}</p>
            <div class="skills-list">${skillsHtml}</div>
            <span class="salary-tag">₹${job.salaryMin} - ₹${job.salaryMax} LPA</span>
            <p><strong>Min CGPA:</strong> ${job.minCgpa} | <strong>Branches:</strong> ${job.eligibleBranches}</p>
            <p><strong>Deadline:</strong> ${job.deadline}</p>
            <button class="btn btn-primary apply-btn" data-job-id="${job.id}">Apply</button>
        `;

        grid.appendChild(card);
    });

    // Attach click listeners to all Apply buttons
    document.querySelectorAll(".apply-btn").forEach(btn => {
        btn.addEventListener("click", async function () {
            const jobId = this.getAttribute("data-job-id");
            await applyToJob(jobId, this);
        });
    });
}

async function applyToJob(jobId, buttonEl) {
    const messageEl = document.getElementById("applyMessage");

    buttonEl.disabled = true;
    buttonEl.textContent = "Applying...";

    const result = await apiRequest("/api/applications", "POST", {
        userId: parseInt(currentUser.id),
        jobId: parseInt(jobId)
    });

    if (result.ok) {
        messageEl.style.color = "#1e8e3e";
        messageEl.textContent = "Application submitted successfully!";
        buttonEl.textContent = "Applied ✓";
        buttonEl.disabled = true;
    } else {
        messageEl.style.color = "#e94560";
        messageEl.textContent = result.data.message || "Failed to apply.";
        buttonEl.textContent = "Apply";
        buttonEl.disabled = false;
    }
}