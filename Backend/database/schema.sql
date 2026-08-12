-- ==========================================================
-- CampusHire Database Schema
-- ==========================================================
-- This file defines every table in our placement portal.
-- Run once to create placement.db with this structure.

-- Enable foreign key constraints (SQLite has this off by default)
PRAGMA foreign_keys = ON;

-- ----------------------------------------------------------
-- USERS TABLE
-- ----------------------------------------------------------
-- Stores login credentials and role for ALL user types 
-- (student, recruiter, admin). The "role" column determines 
-- which dashboard/permissions they get.
CREATE TABLE IF NOT EXISTS Users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    full_name TEXT NOT NULL,
    email TEXT NOT NULL UNIQUE,
    password_hash TEXT NOT NULL,   -- never store plain-text passwords
    role TEXT NOT NULL CHECK(role IN ('student', 'recruiter', 'admin')),
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- ----------------------------------------------------------
-- STUDENT PROFILES TABLE
-- ----------------------------------------------------------
-- Extra details specific to students only. Linked 1-to-1 
-- with a Users row via user_id.
CREATE TABLE IF NOT EXISTS StudentProfiles (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL UNIQUE,
    branch TEXT,
    cgpa REAL,
    graduation_year INTEGER,
    phone TEXT,
    resume_filename TEXT,
    FOREIGN KEY (user_id) REFERENCES Users(id) ON DELETE CASCADE
);

-- ----------------------------------------------------------
-- SKILLS TABLE
-- ----------------------------------------------------------
-- A master list of all known skills (e.g., "C++", "SQL").
-- Both students and jobs reference this table, so skill 
-- names stay consistent (no typos like "c++" vs "C++").
CREATE TABLE IF NOT EXISTS Skills (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE
);

-- ----------------------------------------------------------
-- STUDENT SKILLS (many-to-many link table)
-- ----------------------------------------------------------
-- One student can have many skills; one skill can belong to 
-- many students. This junction table connects them.
CREATE TABLE IF NOT EXISTS StudentSkills (
    student_id INTEGER NOT NULL,
    skill_id INTEGER NOT NULL,
    PRIMARY KEY (student_id, skill_id),
    FOREIGN KEY (student_id) REFERENCES StudentProfiles(id) ON DELETE CASCADE,
    FOREIGN KEY (skill_id) REFERENCES Skills(id) ON DELETE CASCADE
);

-- ----------------------------------------------------------
-- COMPANIES TABLE
-- ----------------------------------------------------------
-- Each recruiter belongs to a company. Admins can also view 
-- all companies here.
CREATE TABLE IF NOT EXISTS Companies (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    industry TEXT,
    recruiter_user_id INTEGER,   -- which recruiter account manages this company
    FOREIGN KEY (recruiter_user_id) REFERENCES Users(id) ON DELETE SET NULL
);

-- ----------------------------------------------------------
-- JOB OPENINGS TABLE
-- ----------------------------------------------------------
-- Each job posting belongs to one company.
CREATE TABLE IF NOT EXISTS JobOpenings (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    company_id INTEGER NOT NULL,
    title TEXT NOT NULL,
    role_type TEXT,              -- 'fulltime', 'internship', etc.
    description TEXT,
    salary_min REAL,
    salary_max REAL,
    min_cgpa REAL,
    eligible_branches TEXT,       -- comma-separated for simplicity (e.g. "CSE,ECE")
    application_deadline DATE,
    status TEXT DEFAULT 'active' CHECK(status IN ('active', 'closed')),
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (company_id) REFERENCES Companies(id) ON DELETE CASCADE
);

-- ----------------------------------------------------------
-- REQUIRED SKILLS (many-to-many link table for jobs)
-- ----------------------------------------------------------
-- Which skills a job requires. Same pattern as StudentSkills.
CREATE TABLE IF NOT EXISTS RequiredSkills (
    job_id INTEGER NOT NULL,
    skill_id INTEGER NOT NULL,
    PRIMARY KEY (job_id, skill_id),
    FOREIGN KEY (job_id) REFERENCES JobOpenings(id) ON DELETE CASCADE,
    FOREIGN KEY (skill_id) REFERENCES Skills(id) ON DELETE CASCADE
);

-- ----------------------------------------------------------
-- APPLICATIONS TABLE
-- ----------------------------------------------------------
-- Tracks which student applied to which job, and the status 
-- of that application over time (Applied -> Shortlisted -> 
-- Interview -> Selected/Rejected).
CREATE TABLE IF NOT EXISTS Applications (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    student_id INTEGER NOT NULL,
    job_id INTEGER NOT NULL,
    status TEXT DEFAULT 'applied' CHECK(status IN ('applied', 'shortlisted', 'interview', 'selected', 'rejected')),
    applied_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (student_id) REFERENCES StudentProfiles(id) ON DELETE CASCADE,
    FOREIGN KEY (job_id) REFERENCES JobOpenings(id) ON DELETE CASCADE,
    UNIQUE(student_id, job_id)   -- prevents applying to the same job twice
);

-- ----------------------------------------------------------
-- CHAT SESSIONS & MESSAGES (for the AI Assistant)
-- ----------------------------------------------------------
CREATE TABLE IF NOT EXISTS ChatSessions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    student_id INTEGER NOT NULL,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (student_id) REFERENCES StudentProfiles(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS ChatMessages (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    session_id INTEGER NOT NULL,
    sender TEXT NOT NULL CHECK(sender IN ('user', 'bot')),
    message TEXT NOT NULL,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (session_id) REFERENCES ChatSessions(id) ON DELETE CASCADE
);

-- ----------------------------------------------------------
-- RESUMES TABLE
-- ----------------------------------------------------------
-- Stores each uploaded resume file plus what our AI Resume 
-- Analyzer extracted from it. A student can have multiple 
-- resumes over time (e.g., updated versions); is_active marks 
-- which one is their current/default resume.
CREATE TABLE IF NOT EXISTS Resumes (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    student_id INTEGER NOT NULL,
    filename TEXT NOT NULL,
    file_path TEXT NOT NULL,
    extracted_text TEXT,              -- raw text pulled from the PDF/doc
    extracted_skills TEXT,            -- comma-separated skills the analyzer found
    analysis_summary TEXT,            -- AI-generated summary/feedback
    analysis_score INTEGER,           -- e.g. 0-100 "resume strength" score
    is_active INTEGER DEFAULT 1 CHECK(is_active IN (0, 1)),
    uploaded_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (student_id) REFERENCES StudentProfiles(id) ON DELETE CASCADE
);

-- ----------------------------------------------------------
-- NOTIFICATIONS TABLE
-- ----------------------------------------------------------
-- Every user (student, recruiter, or admin) can receive 
-- notifications — e.g. "Your application was shortlisted", 
-- "New job matches your profile", "New applicant for your job".
CREATE TABLE IF NOT EXISTS Notifications (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    title TEXT NOT NULL,
    message TEXT NOT NULL,
    type TEXT DEFAULT 'info' CHECK(type IN ('info', 'success', 'warning', 'application_update')),
    is_read INTEGER DEFAULT 0 CHECK(is_read IN (0, 1)),
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES Users(id) ON DELETE CASCADE
);