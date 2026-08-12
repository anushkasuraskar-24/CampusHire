// ==========================================================
// chatbot.js — Rule-based AI career assistant
// ==========================================================

document.addEventListener("DOMContentLoaded", function () {

    const currentUserStr = localStorage.getItem("currentUser");
    if (!currentUserStr) {
        window.location.href = "login.html";
        return;
    }

    const chatWindow = document.getElementById("chatWindow");
    const chatInput = document.getElementById("chatInput");
    const sendBtn = document.getElementById("sendBtn");

    sendBtn.addEventListener("click", sendMessage);
    chatInput.addEventListener("keypress", function (e) {
        if (e.key === "Enter") sendMessage();
    });

    document.querySelectorAll(".suggestion-chip").forEach(chip => {
        chip.addEventListener("click", function () {
            chatInput.value = this.getAttribute("data-msg");
            sendMessage();
        });
    });

    async function sendMessage() {
        const message = chatInput.value.trim();
        if (!message) return;

        addUserMessage(message);
        chatInput.value = "";

        const result = await apiRequest("/api/chatbot/ask", "POST", { message: message });

        if (result.ok) {
            if (result.data.type === "structured") {
                addBotStructuredMessage(result.data);
            } else {
                addBotTextMessage(result.data.reply);
            }
        } else {
            addBotTextMessage("Sorry, I'm having trouble connecting right now.");
        }
    }

    function addUserMessage(text) {
        const div = document.createElement("div");
        div.className = "message user-message";
        div.innerHTML = `<div class="message-bubble">${text}</div>`;
        chatWindow.appendChild(div);
        chatWindow.scrollTop = chatWindow.scrollHeight;
    }

    function addBotTextMessage(text) {
        const div = document.createElement("div");
        div.className = "message bot-message";
        div.innerHTML = `<div class="message-bubble">${text}</div>`;
        chatWindow.appendChild(div);
        chatWindow.scrollTop = chatWindow.scrollHeight;
    }

    function addBotStructuredMessage(data) {
        const skillTags = data.matchedSkills.map(s => `<span class="response-tag skill">${s}</span>`).join("");
        const companyTags = data.companies.map(c => `<span class="response-tag">${c}</span>`).join("");

        const div = document.createElement("div");
        div.className = "message bot-message";
        div.innerHTML = `
            <div class="message-bubble structured-response">
                <div class="response-section">
                    <strong>Matched Skills:</strong><br>${skillTags}
                </div>
                <div class="response-section">
                    <strong>Companies Hiring:</strong><br>${companyTags}
                </div>
                <div class="salary-highlight">
                    <span class="salary-value">₹${data.salaryMin} - ₹${data.salaryMax} LPA</span>
                    <p>${data.jobCount} matching job(s) found</p>
                </div>
            </div>
        `;
        chatWindow.appendChild(div);
        chatWindow.scrollTop = chatWindow.scrollHeight;
    }
});