// ==========================================================
// seed_data.cpp — Populates the database with realistic 
// sample data for development/demo purposes.
// ==========================================================
// Run this ONCE after init_db.exe, to fill placement.db with 
// sample companies, recruiters, students, jobs, skills, and 
// a few applications — so the dashboards have real data to 
// display while we build them.

#include "database.hpp"
#include "auth.hpp"
#include <iostream>
#include <vector>
#include <string>

int main() {
    Database db("database/placement.db");
    std::cout << "Starting database seeding...\n" << std::endl;

    // ----------------------------------------------------
    // 1. SKILLS — master list
    // ----------------------------------------------------
    std::vector<std::string> skills = {
        "C++", "C", "Python", "Java", "JavaScript", "SQL",
        "Data Structures", "Algorithms", "HTML", "CSS",
        "React", "Node.js", "Embedded Systems", "Machine Learning",
        "Git", "Linux", "DBMS", "Operating Systems", "Networking", "AWS"
    };

    for (const auto& skill : skills) {
        db.execute("INSERT OR IGNORE INTO Skills (name) VALUES ('" + skill + "');");
    }
    std::cout << "Inserted " << skills.size() << " skills." << std::endl;

    // ----------------------------------------------------
    // 2. RECRUITER USER ACCOUNTS (created first, so Companies 
    // can reference them via recruiter_user_id)
    // ----------------------------------------------------
    struct RecruiterSeed {
        std::string fullName, email, password;
    };

    std::vector<RecruiterSeed> recruiters = {
        {"Rohan Mehta", "rohan.mehta@qualcomm.com", "recruiter123"},
        {"Priya Nair", "priya.nair@bosch.com", "recruiter123"},
        {"Arjun Kapoor", "arjun.kapoor@infosys.com", "recruiter123"}
    };

    std::vector<long long> recruiterUserIds;

    for (const auto& r : recruiters) {
        std::string hashedPw = hashPassword(r.password);
        std::string sql = "INSERT INTO Users (full_name, email, password_hash, role) VALUES ('"
            + r.fullName + "', '" + r.email + "', '" + hashedPw + "', 'recruiter');";
        db.execute(sql);
        recruiterUserIds.push_back(db.lastInsertRowId());
    }
    std::cout << "Inserted " << recruiters.size() << " recruiter accounts." << std::endl;

    // ----------------------------------------------------
    // 3. COMPANIES
    // ----------------------------------------------------
    struct CompanySeed {
        std::string name, industry;
        int recruiterIndex;   // which recruiter (by index above) manages this company
    };

    std::vector<CompanySeed> companies = {
        {"Qualcomm", "Semiconductors & Telecom", 0},
        {"Bosch", "Automotive & Engineering", 1},
        {"Infosys", "IT Services & Consulting", 2},
        {"Tata Technologies", "Engineering Services", 2},
        {"Accenture", "IT Services & Consulting", 2}
    };

    std::vector<long long> companyIds;

    for (const auto& c : companies) {
        long long recruiterUserId = recruiterUserIds[c.recruiterIndex];
        std::string sql = "INSERT INTO Companies (name, industry, recruiter_user_id) VALUES ('"
            + c.name + "', '" + c.industry + "', " + std::to_string(recruiterUserId) + ");";
        db.execute(sql);
        companyIds.push_back(db.lastInsertRowId());
    }
    std::cout << "Inserted " << companies.size() << " companies." << std::endl;

    // ----------------------------------------------------
    // 4. JOB OPENINGS
    // ----------------------------------------------------
    struct JobSeed {
        int companyIndex;
        std::string title, roleType, description, eligibleBranches;
        double salaryMin, salaryMax, minCgpa;
        std::vector<std::string> requiredSkills;
    };

    std::vector<JobSeed> jobs = {
        {0, "Software Engineer", "fulltime",
         "Work on embedded software for next-generation chipsets.",
         "CSE,ECE", 8.0, 12.0, 7.0,
         {"C++", "Data Structures", "Embedded Systems"}},

        {0, "Firmware Engineer Intern", "internship",
         "Assist in developing and testing firmware for mobile SoCs.",
         "ECE,CSE", 3.0, 5.0, 6.5,
         {"C", "Embedded Systems"}},

        {1, "Embedded Systems Engineer", "fulltime",
         "Develop embedded control systems for automotive applications.",
         "ECE,EEE", 6.0, 10.0, 7.0,
         {"C++", "Embedded Systems", "Operating Systems"}},

        {2, "Backend Developer", "fulltime",
         "Build and maintain scalable backend services for enterprise clients.",
         "CSE,IT", 4.0, 7.0, 6.0,
         {"Java", "SQL", "DBMS"}},

        {2, "Data Analyst", "fulltime",
         "Analyze business data to generate actionable insights for clients.",
         "CSE,IT,ECE", 4.5, 6.5, 6.5,
         {"Python", "SQL", "Machine Learning"}},

        {3, "Systems Engineer", "fulltime",
         "Support engineering design systems for automotive and aerospace clients.",
         "ME,CSE,ECE", 5.0, 9.0, 6.5,
         {"C++", "Algorithms"}},

        {4, "Associate Software Engineer", "fulltime",
         "Entry-level role building web applications for global clients.",
         "CSE,IT", 4.0, 7.0, 6.0,
         {"JavaScript", "React", "Node.js"}},

        {4, "Cloud Support Intern", "internship",
         "Support cloud infrastructure projects using AWS.",
         "CSE,IT", 2.5, 4.0, 6.0,
         {"AWS", "Linux", "Networking"}}
    };

    for (const auto& j : jobs) {
        long long companyId = companyIds[j.companyIndex];

        std::string sql = "INSERT INTO JobOpenings "
            "(company_id, title, role_type, description, salary_min, salary_max, min_cgpa, eligible_branches, application_deadline) "
            "VALUES (" + std::to_string(companyId) + ", '" + j.title + "', '" + j.roleType + "', '"
            + j.description + "', " + std::to_string(j.salaryMin) + ", " + std::to_string(j.salaryMax)
            + ", " + std::to_string(j.minCgpa) + ", '" + j.eligibleBranches + "', date('now', '+30 days'));";

        db.execute(sql);
        long long jobId = db.lastInsertRowId();

        // Link required skills for this job
        for (const auto& skillName : j.requiredSkills) {
            auto skillRows = db.query("SELECT id FROM Skills WHERE name = '" + skillName + "';");
            if (!skillRows.empty()) {
                std::string skillId = skillRows[0]["id"];
                db.execute("INSERT OR IGNORE INTO RequiredSkills (job_id, skill_id) VALUES ("
                    + std::to_string(jobId) + ", " + skillId + ");");
            }
        }
    }
    std::cout << "Inserted " << jobs.size() << " job openings (with required skills)." << std::endl;

    // ----------------------------------------------------
    // 5. STUDENT USER ACCOUNTS + PROFILES
    // ----------------------------------------------------
    struct StudentSeed {
        std::string fullName, email, password, branch;
        double cgpa;
        int gradYear;
        std::vector<std::string> skillsList;
    };

    std::vector<StudentSeed> students = {
        {"Anushka Suraskar", "anushka.suraskar@example.com", "student123", "CSE", 8.4, 2026,
         {"C++", "SQL", "HTML", "CSS", "Data Structures"}},

        {"Rahul Verma", "rahul.verma@example.com", "student123", "ECE", 7.8, 2026,
         {"C", "Embedded Systems", "C++"}},

        {"Sneha Iyer", "sneha.iyer@example.com", "student123", "IT", 8.9, 2026,
         {"Python", "SQL", "Machine Learning", "Data Structures"}},

        {"Karan Singh", "karan.singh@example.com", "student123", "CSE", 6.9, 2027,
         {"Java", "SQL", "DBMS"}},

        {"Divya Patel", "divya.patel@example.com", "student123", "IT", 7.5, 2026,
         {"JavaScript", "React", "Node.js", "HTML", "CSS"}}
    };

    std::vector<long long> studentProfileIds;

    for (const auto& s : students) {
        std::string hashedPw = hashPassword(s.password);
        std::string userSql = "INSERT INTO Users (full_name, email, password_hash, role) VALUES ('"
            + s.fullName + "', '" + s.email + "', '" + hashedPw + "', 'student');";
        db.execute(userSql);
        long long userId = db.lastInsertRowId();

        std::string profileSql = "INSERT INTO StudentProfiles (user_id, branch, cgpa, graduation_year) VALUES ("
            + std::to_string(userId) + ", '" + s.branch + "', " + std::to_string(s.cgpa)
            + ", " + std::to_string(s.gradYear) + ");";
        db.execute(profileSql);
        long long profileId = db.lastInsertRowId();
        studentProfileIds.push_back(profileId);

        // Link this student's skills
        for (const auto& skillName : s.skillsList) {
            auto skillRows = db.query("SELECT id FROM Skills WHERE name = '" + skillName + "';");
            if (!skillRows.empty()) {
                std::string skillId = skillRows[0]["id"];
                db.execute("INSERT OR IGNORE INTO StudentSkills (student_id, skill_id) VALUES ("
                    + std::to_string(profileId) + ", " + skillId + ");");
            }
        }
    }
    std::cout << "Inserted " << students.size() << " student accounts (with profiles and skills)." << std::endl;

    // ----------------------------------------------------
    // 6. SAMPLE APPLICATIONS (a few, with varied statuses)
    // ----------------------------------------------------
    // studentProfileIds[0] = Anushka, [1] = Rahul, [2] = Sneha, 
    // [3] = Karan, [4] = Divya. Job IDs are 1-8 in insertion order.
    struct ApplicationSeed {
        int studentIndex;
        int jobId;
        std::string status;
    };

    std::vector<ApplicationSeed> applications = {
        {0, 1, "shortlisted"},   // Anushka -> Software Engineer @ Qualcomm
        {0, 4, "applied"},       // Anushka -> Backend Developer @ Infosys
        {1, 3, "interview"},     // Rahul -> Embedded Systems Engineer @ Bosch
        {2, 5, "selected"},      // Sneha -> Data Analyst @ Infosys
        {3, 4, "rejected"},      // Karan -> Backend Developer @ Infosys
        {4, 7, "applied"}        // Divya -> Associate Software Engineer @ Accenture
    };

    for (const auto& a : applications) {
        long long studentId = studentProfileIds[a.studentIndex];
        std::string sql = "INSERT OR IGNORE INTO Applications (student_id, job_id, status) VALUES ("
            + std::to_string(studentId) + ", " + std::to_string(a.jobId) + ", '" + a.status + "');";
        db.execute(sql);
    }
    std::cout << "Inserted " << applications.size() << " sample applications." << std::endl;

    std::cout << "\nDatabase seeding completed successfully!" << std::endl;
    return 0;
}