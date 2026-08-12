#include "crow.h"
#include "crow/middlewares/cors.h"
#include "database.hpp"
#include "auth.hpp"
#include <sstream>
#include <cctype>

int main() {
    Database db("database/placement.db");

    crow::App<crow::CORSHandler> app;

    app.get_middleware<crow::CORSHandler>().global()
        .headers("Content-Type")
        .methods("GET"_method, "POST"_method, "PUT"_method, "DELETE"_method, "OPTIONS"_method)
        .origin("*");

    // ==========================================================
    // HEALTH CHECK
    // ==========================================================
    CROW_ROUTE(app, "/api/health")([](){
        crow::json::wvalue response;
        response["status"] = "ok";
        response["message"] = "Backend is healthy";
        return response;
    });

    // ==========================================================
    // AUTH ROUTES
    // ==========================================================
    CROW_ROUTE(app, "/api/auth/register").methods("POST"_method)
    ([&db](const crow::request& req) {
        auto body = crow::json::load(req.body);
        crow::json::wvalue response;

        if (!body || !body.has("fullName") || !body.has("email")
                    || !body.has("password") || !body.has("role")) {
            response["success"] = false;
            response["message"] = "Missing required fields.";
            return crow::response(400, response);
        }

        std::string fullName = body["fullName"].s();
        std::string email = body["email"].s();
        std::string password = body["password"].s();
        std::string role = body["role"].s();

        auto existing = db.query("SELECT id FROM Users WHERE email = '" + email + "';");
        if (!existing.empty()) {
            response["success"] = false;
            response["message"] = "An account with this email already exists.";
            return crow::response(409, response);
        }

        std::string hashedPassword = hashPassword(password);
        std::string insertSql = "INSERT INTO Users (full_name, email, password_hash, role) VALUES ('"
            + fullName + "', '" + email + "', '" + hashedPassword + "', '" + role + "');";

        if (!db.execute(insertSql)) {
            response["success"] = false;
            response["message"] = "Failed to create account: " + db.lastError();
            return crow::response(500, response);
        }

        long long newUserId = db.lastInsertRowId();

        if (role == "student") {
            db.execute("INSERT INTO StudentProfiles (user_id) VALUES (" + std::to_string(newUserId) + ");");
        }

        response["success"] = true;
        response["message"] = "Account created successfully.";
        response["userId"] = newUserId;
        return crow::response(201, response);
    });

    CROW_ROUTE(app, "/api/auth/login").methods("POST"_method)
    ([&db](const crow::request& req) {
        auto body = crow::json::load(req.body);
        crow::json::wvalue response;

        if (!body || !body.has("email") || !body.has("password")) {
            response["success"] = false;
            response["message"] = "Missing email or password.";
            return crow::response(400, response);
        }

        std::string email = body["email"].s();
        std::string password = body["password"].s();

        auto results = db.query("SELECT id, full_name, email, password_hash, role FROM Users WHERE email = '" + email + "';");

        if (results.empty()) {
            response["success"] = false;
            response["message"] = "Invalid email or password.";
            return crow::response(401, response);
        }

        DBRow user = results[0];

        if (!verifyPassword(password, user["password_hash"])) {
            response["success"] = false;
            response["message"] = "Invalid email or password.";
            return crow::response(401, response);
        }

        response["success"] = true;
        response["message"] = "Login successful.";
        response["user"]["id"] = user["id"];
        response["user"]["fullName"] = user["full_name"];
        response["user"]["email"] = user["email"];
        response["user"]["role"] = user["role"];

        return crow::response(200, response);
    });

    // ==========================================================
    // JOBS — list all active jobs (with company name + required skills)
    // ==========================================================
    CROW_ROUTE(app, "/api/jobs")
    ([&db](){
        crow::json::wvalue response;
        std::vector<crow::json::wvalue> jobList;

        auto jobs = db.query(
            "SELECT J.id, J.title, J.role_type, J.description, J.salary_min, J.salary_max, "
            "J.min_cgpa, J.eligible_branches, J.application_deadline, C.name as company_name, C.id as company_id "
            "FROM JobOpenings J JOIN Companies C ON J.company_id = C.id "
            "WHERE J.status = 'active' ORDER BY J.created_at DESC;"
        );

        for (auto& job : jobs) {
            crow::json::wvalue jobJson;
            jobJson["id"] = job["id"];
            jobJson["title"] = job["title"];
            jobJson["roleType"] = job["role_type"];
            jobJson["description"] = job["description"];
            jobJson["salaryMin"] = job["salary_min"];
            jobJson["salaryMax"] = job["salary_max"];
            jobJson["minCgpa"] = job["min_cgpa"];
            jobJson["eligibleBranches"] = job["eligible_branches"];
            jobJson["deadline"] = job["application_deadline"];
            jobJson["companyName"] = job["company_name"];
            jobJson["companyId"] = job["company_id"];

            auto skills = db.query(
                "SELECT S.name FROM RequiredSkills RS JOIN Skills S ON RS.skill_id = S.id WHERE RS.job_id = " + job["id"] + ";"
            );
            std::vector<std::string> skillNames;
            for (auto& s : skills) skillNames.push_back(s["name"]);

            std::vector<crow::json::wvalue> skillsJson;
            for (auto& sk : skillNames) skillsJson.push_back(sk);
            jobJson["requiredSkills"] = std::move(skillsJson);

            jobList.push_back(std::move(jobJson));
        }

        response["success"] = true;
        response["jobs"] = std::move(jobList);
        return response;
    });

    // ==========================================================
    // STUDENT PROFILE — get full profile by userId
    // ==========================================================
    CROW_ROUTE(app, "/api/student/profile")
    ([&db](const crow::request& req){
        crow::json::wvalue response;
        auto userIdParam = req.url_params.get("userId");

        if (!userIdParam) {
            response["success"] = false;
            response["message"] = "Missing userId parameter.";
            return crow::response(400, response);
        }

        std::string userId = userIdParam;

        auto userRows = db.query("SELECT full_name, email FROM Users WHERE id = " + userId + ";");
        auto profileRows = db.query("SELECT id, branch, cgpa, graduation_year, phone, resume_filename FROM StudentProfiles WHERE user_id = " + userId + ";");

        if (userRows.empty() || profileRows.empty()) {
            response["success"] = false;
            response["message"] = "Student profile not found.";
            return crow::response(404, response);
        }

        DBRow user = userRows[0];
        DBRow profile = profileRows[0];

        auto skillRows = db.query(
            "SELECT S.name FROM StudentSkills SS JOIN Skills S ON SS.skill_id = S.id WHERE SS.student_id = " + profile["id"] + ";"
        );
        std::vector<crow::json::wvalue> skillsJson;
        for (auto& s : skillRows) skillsJson.push_back(s["name"]);

        response["success"] = true;
        response["profile"]["studentId"] = profile["id"];
        response["profile"]["fullName"] = user["full_name"];
        response["profile"]["email"] = user["email"];
        response["profile"]["branch"] = profile["branch"];
        response["profile"]["cgpa"] = profile["cgpa"];
        response["profile"]["graduationYear"] = profile["graduation_year"];
        response["profile"]["phone"] = profile["phone"];
        response["profile"]["resumeFilename"] = profile["resume_filename"];
        response["profile"]["skills"] = std::move(skillsJson);

        return crow::response(200, response);
    });

    // ==========================================================
    // STUDENT PROFILE — update profile fields + skills
    // ==========================================================
    CROW_ROUTE(app, "/api/student/profile").methods("PUT"_method)
    ([&db](const crow::request& req){
        auto body = crow::json::load(req.body);
        crow::json::wvalue response;

        if (!body || !body.has("userId")) {
            response["success"] = false;
            response["message"] = "Missing userId.";
            return crow::response(400, response);
        }

        std::string userId = std::to_string(body["userId"].i());

        auto profileRows = db.query("SELECT id FROM StudentProfiles WHERE user_id = " + userId + ";");
        if (profileRows.empty()) {
            response["success"] = false;
            response["message"] = "Student profile not found.";
            return crow::response(404, response);
        }
        std::string profileId = profileRows[0]["id"];

        if (body.has("fullName")) {
            std::string fullName = body["fullName"].s();
            db.execute("UPDATE Users SET full_name = '" + fullName + "' WHERE id = " + userId + ";");
        }

        std::string branch = body.has("branch") ? std::string(body["branch"].s()) : "";
        std::string phone = body.has("phone") ? std::string(body["phone"].s()) : "";
        double cgpa = body.has("cgpa") ? body["cgpa"].d() : 0.0;
        int gradYear = body.has("gradYear") ? body["gradYear"].i() : 0;

        std::string updateSql = "UPDATE StudentProfiles SET branch = '" + branch
            + "', phone = '" + phone
            + "', cgpa = " + std::to_string(cgpa)
            + ", graduation_year = " + std::to_string(gradYear)
            + " WHERE user_id = " + userId + ";";

        if (!db.execute(updateSql)) {
            response["success"] = false;
            response["message"] = "Failed to update profile: " + db.lastError();
            return crow::response(500, response);
        }

        if (body.has("skills")) {
            db.execute("DELETE FROM StudentSkills WHERE student_id = " + profileId + ";");

            std::string skillsStr = body["skills"].s();
            std::stringstream ss(skillsStr);
            std::string skillName;
            while (std::getline(ss, skillName, ',')) {
                size_t start = skillName.find_first_not_of(" ");
                size_t end = skillName.find_last_not_of(" ");
                if (start == std::string::npos) continue;
                skillName = skillName.substr(start, end - start + 1);

                db.execute("INSERT OR IGNORE INTO Skills (name) VALUES ('" + skillName + "');");
                auto skillRows = db.query("SELECT id FROM Skills WHERE name = '" + skillName + "';");
                if (!skillRows.empty()) {
                    db.execute("INSERT OR IGNORE INTO StudentSkills (student_id, skill_id) VALUES ("
                        + profileId + ", " + skillRows[0]["id"] + ");");
                }
            }
        }

        response["success"] = true;
        response["message"] = "Profile updated successfully.";
        return crow::response(200, response);
    });

    // ==========================================================
    // STUDENT APPLICATIONS — list this student's applications
    // ==========================================================
    CROW_ROUTE(app, "/api/student/applications")
    ([&db](const crow::request& req){
        crow::json::wvalue response;
        auto userIdParam = req.url_params.get("userId");

        if (!userIdParam) {
            response["success"] = false;
            response["message"] = "Missing userId parameter.";
            return crow::response(400, response);
        }

        std::string userId = userIdParam;

        auto profileRows = db.query("SELECT id FROM StudentProfiles WHERE user_id = " + userId + ";");
        if (profileRows.empty()) {
            response["success"] = false;
            response["message"] = "Student profile not found.";
            return crow::response(404, response);
        }
        std::string studentId = profileRows[0]["id"];

        auto apps = db.query(
            "SELECT A.id, A.status, A.applied_at, J.title, C.name as company_name "
            "FROM Applications A "
            "JOIN JobOpenings J ON A.job_id = J.id "
            "JOIN Companies C ON J.company_id = C.id "
            "WHERE A.student_id = " + studentId + " ORDER BY A.applied_at DESC;"
        );

        std::vector<crow::json::wvalue> appList;
        for (auto& a : apps) {
            crow::json::wvalue appJson;
            appJson["id"] = a["id"];
            appJson["status"] = a["status"];
            appJson["appliedAt"] = a["applied_at"];
            appJson["jobTitle"] = a["title"];
            appJson["companyName"] = a["company_name"];
            appList.push_back(std::move(appJson));
        }

        response["success"] = true;
        response["applications"] = std::move(appList);
        return crow::response(200, response);
    });

    // ==========================================================
    // APPLY TO A JOB
    // ==========================================================
    CROW_ROUTE(app, "/api/applications").methods("POST"_method)
    ([&db](const crow::request& req){
        auto body = crow::json::load(req.body);
        crow::json::wvalue response;

        if (!body || !body.has("userId") || !body.has("jobId")) {
            response["success"] = false;
            response["message"] = "Missing userId or jobId.";
            return crow::response(400, response);
        }

        std::string userId = std::to_string(body["userId"].i());
        std::string jobId = std::to_string(body["jobId"].i());

        auto profileRows = db.query("SELECT id FROM StudentProfiles WHERE user_id = " + userId + ";");
        if (profileRows.empty()) {
            response["success"] = false;
            response["message"] = "Student profile not found.";
            return crow::response(404, response);
        }
        std::string studentId = profileRows[0]["id"];

        auto existing = db.query("SELECT id FROM Applications WHERE student_id = " + studentId + " AND job_id = " + jobId + ";");
        if (!existing.empty()) {
            response["success"] = false;
            response["message"] = "You have already applied to this job.";
            return crow::response(409, response);
        }

        bool ok = db.execute("INSERT INTO Applications (student_id, job_id, status) VALUES (" + studentId + ", " + jobId + ", 'applied');");

        if (!ok) {
            response["success"] = false;
            response["message"] = "Failed to apply: " + db.lastError();
            return crow::response(500, response);
        }

        response["success"] = true;
        response["message"] = "Application submitted successfully.";
        return crow::response(201, response);
    });

    // ==========================================================
    // RECRUITER — get jobs posted by this recruiter
    // ==========================================================
    CROW_ROUTE(app, "/api/recruiter/jobs")
    ([&db](const crow::request& req){
        crow::json::wvalue response;
        auto userIdParam = req.url_params.get("userId");

        if (!userIdParam) {
            response["success"] = false;
            response["message"] = "Missing userId parameter.";
            return crow::response(400, response);
        }

        std::string userId = userIdParam;

        auto companyRows = db.query("SELECT id, name FROM Companies WHERE recruiter_user_id = " + userId + ";");
        if (companyRows.empty()) {
            response["success"] = false;
            response["message"] = "No company found for this recruiter.";
            return crow::response(404, response);
        }
        std::string companyId = companyRows[0]["id"];

        auto jobs = db.query(
            "SELECT J.id, J.title, J.role_type, J.status, J.application_deadline, "
            "(SELECT COUNT(*) FROM Applications WHERE job_id = J.id) as applicant_count "
            "FROM JobOpenings J WHERE J.company_id = " + companyId + " ORDER BY J.created_at DESC;"
        );

        std::vector<crow::json::wvalue> jobList;
        for (auto& j : jobs) {
            crow::json::wvalue jobJson;
            jobJson["id"] = j["id"];
            jobJson["title"] = j["title"];
            jobJson["roleType"] = j["role_type"];
            jobJson["status"] = j["status"];
            jobJson["deadline"] = j["application_deadline"];
            jobJson["applicantCount"] = j["applicant_count"];
            jobList.push_back(std::move(jobJson));
        }

        response["success"] = true;
        response["companyName"] = companyRows[0]["name"];
        response["jobs"] = std::move(jobList);
        return crow::response(200, response);
    });

    // ==========================================================
    // RECRUITER — post a new job
    // ==========================================================
    CROW_ROUTE(app, "/api/recruiter/jobs").methods("POST"_method)
    ([&db](const crow::request& req){
        auto body = crow::json::load(req.body);
        crow::json::wvalue response;

        if (!body || !body.has("userId") || !body.has("title") || !body.has("roleType")
                  || !body.has("description") || !body.has("salaryMin") || !body.has("salaryMax")
                  || !body.has("minCgpa") || !body.has("eligibleBranches")) {
            response["success"] = false;
            response["message"] = "Missing required job fields.";
            return crow::response(400, response);
        }

        std::string userId = std::to_string(body["userId"].i());

        auto companyRows = db.query("SELECT id FROM Companies WHERE recruiter_user_id = " + userId + ";");
        if (companyRows.empty()) {
            response["success"] = false;
            response["message"] = "No company found for this recruiter.";
            return crow::response(404, response);
        }
        std::string companyId = companyRows[0]["id"];

        std::string title = body["title"].s();
        std::string roleType = body["roleType"].s();
        std::string description = body["description"].s();
        double salaryMin = body["salaryMin"].d();
        double salaryMax = body["salaryMax"].d();
        double minCgpa = body["minCgpa"].d();
        std::string eligibleBranches = body["eligibleBranches"].s();

        std::string sql = "INSERT INTO JobOpenings "
            "(company_id, title, role_type, description, salary_min, salary_max, min_cgpa, eligible_branches, application_deadline) "
            "VALUES (" + companyId + ", '" + title + "', '" + roleType + "', '" + description + "', "
            + std::to_string(salaryMin) + ", " + std::to_string(salaryMax) + ", " + std::to_string(minCgpa)
            + ", '" + eligibleBranches + "', date('now', '+30 days'));";

        if (!db.execute(sql)) {
            response["success"] = false;
            response["message"] = "Failed to post job: " + db.lastError();
            return crow::response(500, response);
        }

        long long jobId = db.lastInsertRowId();

        if (body.has("requiredSkills")) {
            std::string skillsStr = body["requiredSkills"].s();
            std::stringstream ss(skillsStr);
            std::string skillName;
            while (std::getline(ss, skillName, ',')) {
                size_t start = skillName.find_first_not_of(" ");
                size_t end = skillName.find_last_not_of(" ");
                if (start == std::string::npos) continue;
                skillName = skillName.substr(start, end - start + 1);

                db.execute("INSERT OR IGNORE INTO Skills (name) VALUES ('" + skillName + "');");
                auto skillRows = db.query("SELECT id FROM Skills WHERE name = '" + skillName + "';");
                if (!skillRows.empty()) {
                    db.execute("INSERT OR IGNORE INTO RequiredSkills (job_id, skill_id) VALUES (" + std::to_string(jobId) + ", " + skillRows[0]["id"] + ");");
                }
            }
        }

        response["success"] = true;
        response["message"] = "Job posted successfully.";
        response["jobId"] = jobId;
        return crow::response(201, response);
    });

    // ==========================================================
    // RECRUITER — get applicants for a specific job
    // ==========================================================
    CROW_ROUTE(app, "/api/recruiter/applicants")
    ([&db](const crow::request& req){
        crow::json::wvalue response;
        auto jobIdParam = req.url_params.get("jobId");

        if (!jobIdParam) {
            response["success"] = false;
            response["message"] = "Missing jobId parameter.";
            return crow::response(400, response);
        }

        std::string jobId = jobIdParam;

        auto applicants = db.query(
            "SELECT A.id as application_id, A.status, A.applied_at, U.full_name, U.email, "
            "SP.branch, SP.cgpa "
            "FROM Applications A "
            "JOIN StudentProfiles SP ON A.student_id = SP.id "
            "JOIN Users U ON SP.user_id = U.id "
            "WHERE A.job_id = " + jobId + " ORDER BY A.applied_at DESC;"
        );

        std::vector<crow::json::wvalue> applicantList;
        for (auto& a : applicants) {
            crow::json::wvalue aJson;
            aJson["applicationId"] = a["application_id"];
            aJson["status"] = a["status"];
            aJson["appliedAt"] = a["applied_at"];
            aJson["fullName"] = a["full_name"];
            aJson["email"] = a["email"];
            aJson["branch"] = a["branch"];
            aJson["cgpa"] = a["cgpa"];
            applicantList.push_back(std::move(aJson));
        }

        response["success"] = true;
        response["applicants"] = std::move(applicantList);
        return crow::response(200, response);
    });

    // ==========================================================
    // RECRUITER — update an application's status
    // ==========================================================
    CROW_ROUTE(app, "/api/applications/status").methods("PUT"_method)
    ([&db](const crow::request& req){
        auto body = crow::json::load(req.body);
        crow::json::wvalue response;

        if (!body || !body.has("applicationId") || !body.has("status")) {
            response["success"] = false;
            response["message"] = "Missing applicationId or status.";
            return crow::response(400, response);
        }

        std::string applicationId = std::to_string(body["applicationId"].i());
        std::string status = body["status"].s();

        bool ok = db.execute(
            "UPDATE Applications SET status = '" + status + "', updated_at = CURRENT_TIMESTAMP WHERE id = " + applicationId + ";"
        );

        if (!ok) {
            response["success"] = false;
            response["message"] = "Failed to update status: " + db.lastError();
            return crow::response(500, response);
        }

        response["success"] = true;
        response["message"] = "Application status updated.";
        return crow::response(200, response);
    });

    // ==========================================================
    // ADMIN — basic analytics
    // ==========================================================
    CROW_ROUTE(app, "/api/admin/stats")
    ([&db](){
        crow::json::wvalue response;

        auto studentCount = db.query("SELECT COUNT(*) as count FROM Users WHERE role = 'student';");
        auto recruiterCount = db.query("SELECT COUNT(*) as count FROM Users WHERE role = 'recruiter';");
        auto companyCount = db.query("SELECT COUNT(*) as count FROM Companies;");
        auto jobCount = db.query("SELECT COUNT(*) as count FROM JobOpenings WHERE status = 'active';");
        auto applicationCount = db.query("SELECT COUNT(*) as count FROM Applications;");
        auto selectedCount = db.query("SELECT COUNT(*) as count FROM Applications WHERE status = 'selected';");

        response["success"] = true;
        response["stats"]["totalStudents"] = studentCount[0]["count"];
        response["stats"]["totalRecruiters"] = recruiterCount[0]["count"];
        response["stats"]["totalCompanies"] = companyCount[0]["count"];
        response["stats"]["activeJobs"] = jobCount[0]["count"];
        response["stats"]["totalApplications"] = applicationCount[0]["count"];
        response["stats"]["totalSelected"] = selectedCount[0]["count"];

        return response;
    });

    CROW_ROUTE(app, "/api/admin/students")
    ([&db](){
        crow::json::wvalue response;
        auto students = db.query(
            "SELECT U.id, U.full_name, U.email, SP.branch, SP.cgpa "
            "FROM Users U JOIN StudentProfiles SP ON U.id = SP.user_id "
            "WHERE U.role = 'student' ORDER BY U.full_name;"
        );

        std::vector<crow::json::wvalue> studentList;
        for (auto& s : students) {
            crow::json::wvalue sJson;
            sJson["id"] = s["id"];
            sJson["fullName"] = s["full_name"];
            sJson["email"] = s["email"];
            sJson["branch"] = s["branch"];
            sJson["cgpa"] = s["cgpa"];
            studentList.push_back(std::move(sJson));
        }

        response["success"] = true;
        response["students"] = std::move(studentList);
        return response;
    });

    CROW_ROUTE(app, "/api/admin/companies")
    ([&db](){
        crow::json::wvalue response;
        auto companies = db.query(
            "SELECT C.id, C.name, C.industry, "
            "(SELECT COUNT(*) FROM JobOpenings WHERE company_id = C.id) as job_count "
            "FROM Companies C ORDER BY C.name;"
        );

        std::vector<crow::json::wvalue> companyList;
        for (auto& c : companies) {
            crow::json::wvalue cJson;
            cJson["id"] = c["id"];
            cJson["name"] = c["name"];
            cJson["industry"] = c["industry"];
            cJson["jobCount"] = c["job_count"];
            companyList.push_back(std::move(cJson));
        }

        response["success"] = true;
        response["companies"] = std::move(companyList);
        return response;
    });

    // ==========================================================
    // AI CHATBOT — rule-based, matches skills to jobs/companies
    // ==========================================================
    CROW_ROUTE(app, "/api/chatbot/ask").methods("POST"_method)
    ([&db](const crow::request& req){
        auto body = crow::json::load(req.body);
        crow::json::wvalue response;

        if (!body || !body.has("message")) {
            response["success"] = false;
            response["message"] = "Missing message.";
            return crow::response(400, response);
        }

        std::string userMessage = body["message"].s();

        std::string lowerMsg = userMessage;
        for (auto& c : lowerMsg) c = tolower((unsigned char)c);

        auto allSkills = db.query("SELECT name FROM Skills;");
        std::vector<std::string> mentionedSkills;

        for (auto& s : allSkills) {
            std::string skillLower = s["name"];
            for (auto& c : skillLower) c = tolower((unsigned char)c);

            if (lowerMsg.find(skillLower) != std::string::npos) {
                mentionedSkills.push_back(s["name"]);
            }
        }

        if (mentionedSkills.empty()) {
            response["success"] = true;
            response["type"] = "text";
            response["reply"] = "Tell me a bit about your skills (e.g. \"I know C++, SQL and Python\") and I'll suggest companies and roles that match!";
            return crow::response(200, response);
        }

        std::string skillListSql = "";
        for (size_t i = 0; i < mentionedSkills.size(); i++) {
            if (i > 0) skillListSql += ",";
            skillListSql += "'" + mentionedSkills[i] + "'";
        }

        auto matchingJobs = db.query(
            "SELECT DISTINCT J.id, J.title, J.salary_min, J.salary_max, C.name as company_name, "
            "(SELECT COUNT(*) FROM RequiredSkills RS2 JOIN Skills S2 ON RS2.skill_id = S2.id "
            " WHERE RS2.job_id = J.id AND S2.name IN (" + skillListSql + ")) as match_count "
            "FROM JobOpenings J "
            "JOIN Companies C ON J.company_id = C.id "
            "JOIN RequiredSkills RS ON RS.job_id = J.id "
            "JOIN Skills S ON RS.skill_id = S.id "
            "WHERE S.name IN (" + skillListSql + ") AND J.status = 'active' "
            "ORDER BY match_count DESC;"
        );

        if (matchingJobs.empty()) {
            response["success"] = true;
            response["type"] = "text";
            response["reply"] = "I noticed you mentioned " + mentionedSkills[0] + ", but I couldn't find any matching openings right now. Try checking the Job Listings page for the full list!";
            return crow::response(200, response);
        }

        std::vector<crow::json::wvalue> companiesJson;
        std::vector<std::string> seenCompanies;
        double minSalary = 999, maxSalary = 0;

        for (auto& job : matchingJobs) {
            std::string companyName = job["company_name"];
            bool alreadySeen = false;
            for (auto& c : seenCompanies) if (c == companyName) alreadySeen = true;

            if (!alreadySeen) {
                seenCompanies.push_back(companyName);
                companiesJson.push_back(companyName);
            }

            double salMin = std::stod(job["salary_min"]);
            double salMax = std::stod(job["salary_max"]);
            if (salMin < minSalary) minSalary = salMin;
            if (salMax > maxSalary) maxSalary = salMax;
        }

        std::vector<crow::json::wvalue> skillsJson;
        for (auto& s : mentionedSkills) skillsJson.push_back(s);

        response["success"] = true;
        response["type"] = "structured";
        response["matchedSkills"] = std::move(skillsJson);
        response["companies"] = std::move(companiesJson);
        response["salaryMin"] = minSalary;
        response["salaryMax"] = maxSalary;
        response["jobCount"] = (int)matchingJobs.size();

        return crow::response(200, response);
    });

    app.port(8080).multithreaded().run();
    return 0;
}