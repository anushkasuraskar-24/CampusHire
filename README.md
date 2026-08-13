# CampusHire 🎓

### AI-Powered Student Placement Portal

CampusHire is an AI-powered student placement portal designed to simplify and improve the placement journey for students, recruiters, and administrators.

The platform brings **job discovery, career guidance, interview preparation, resume management, recruiter interaction, and placement management** into one centralized system.

---

## 🚀 Project Overview

Finding the right job and preparing for placements can be difficult for students because job opportunities, preparation resources, resumes, and application tracking are often scattered across different platforms.

**CampusHire** provides a unified digital platform where students can:

- Discover relevant job opportunities
- Manage their profiles
- Create and manage resumes
- Prepare for interviews
- Get career guidance
- Track job applications
- Interact with an AI-powered chatbot

Recruiters can:

- Post job opportunities
- Manage applicants
- View candidate profiles
- Shortlist suitable candidates
- Manage recruitment activities

Administrators can:

- Manage students and recruiters
- Monitor placement activities
- Manage job listings
- Maintain the placement database

---

## 🎯 Objectives

The main objectives of CampusHire are:

1. To create a centralized student placement platform.
2. To simplify job discovery and application tracking.
3. To provide AI-based career and interview guidance.
4. To help students build and manage professional resumes.
5. To provide recruiters with an efficient candidate management system.
6. To provide administrators with centralized placement management.
7. To improve communication between students, recruiters, and placement administrators.

---

## ✨ Key Features

### 👨‍🎓 Student Module

- Student registration and login
- Student profile management
- Job search and discovery
- Job application management
- Resume management
- Career guidance
- Interview preparation
- AI-powered chatbot
- Student dashboard

### 🏢 Recruiter Module

- Recruiter registration and login
- Recruiter dashboard
- Create and manage job postings
- View applicants
- Candidate management
- Shortlisting of candidates
- Recruitment activity tracking

### 👨‍💼 Admin Module

- Admin dashboard
- Student management
- Recruiter management
- Job management
- Placement activity monitoring
- Database management

### 🤖 AI-Powered Features

- AI career guidance
- Interview preparation assistance
- Intelligent chatbot
- Personalized placement support

---

## 🏗️ System Architecture

```text
                    ┌──────────────────────┐
                    │      CampusHire      │
                    │   Placement Portal   │
                    └──────────┬───────────┘
                               │
             ┌─────────────────┼─────────────────┐
             │                 │                 │
             ▼                 ▼                 ▼
       ┌───────────┐     ┌────────────┐    ┌────────────┐
       │  Student  │     │ Recruiter  │    │   Admin    │
       │  Module   │     │   Module   │    │   Module   │
       └─────┬─────┘     └──────┬─────┘    └──────┬─────┘
             │                  │                  │
             └──────────────────┼──────────────────┘
                                ▼
                     ┌────────────────────┐
                     │   Backend System   │
                     │      C++           │
                     └─────────┬──────────┘
                               │
                    ┌──────────┴──────────┐
                    │                     │
                    ▼                     ▼
             ┌─────────────┐      ┌─────────────┐
             │   Database  │      │  AI / Chat  │
             │             │      │    Support  │
             └─────────────┘      └─────────────┘
