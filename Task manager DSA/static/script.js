const taskList = document.getElementById("taskList");
const usernameInput = document.getElementById("username");
const taskInput = document.getElementById("taskInput");
const dueInput = document.getElementById("dueInput");
const priorityInput = document.getElementById("priorityInput");
const statusInput = document.getElementById("statusInput");
const addBtn = document.getElementById("addBtn");
const undoBtn = document.getElementById("undoBtn");
const redoBtn = document.getElementById("redoBtn");
const notifyBtn = document.getElementById("notifyBtn");
const notificationsDiv = document.getElementById("notifications");

async function loadTasks() {
    const username = usernameInput.value.trim();
    if (!username) {
        taskList.innerHTML = "<li style='color:#777'>Enter username to see tasks</li>";
        return;
    }
    const res = await fetch(`/api/tasks?username=${encodeURIComponent(username)}`);
    const tasks = await res.json();
    taskList.innerHTML = "";
    if (!tasks || tasks.length === 0) {
        taskList.innerHTML = "<li style='color:#777'>No tasks yet</li>";
        return;
    }
    tasks.forEach(t => {
        const li = document.createElement("li");
        li.innerHTML = `<div><strong>${escapeHtml(t.title)}</strong></div>
                        <div style="font-size:.9rem;color:#555">Due: ${escapeHtml(t.due)} • Priority: ${t.priority} • Status: ${escapeHtml(t.status)} • Added: ${escapeHtml(t.time)}</div>`;
        taskList.appendChild(li);
    });
}

addBtn.onclick = async () => {
    const username = usernameInput.value.trim();
    const title = taskInput.value.trim();
    const due = dueInput.value || "2025-12-31";
    const priority = parseInt(priorityInput.value || "1");
    const status = statusInput.value || "Pending";
    if (!username || !title) { alert("Enter username and task title"); return; }

    await fetch("/api/tasks", {
        method: "POST",
        headers: {"Content-Type": "application/json"},
        body: JSON.stringify({ username, title, due, priority, status })
    });
    taskInput.value = ""; dueInput.value = ""; loadTasks();
}

undoBtn.onclick = async () => {
    const username = usernameInput.value.trim();
    if (!username) { alert("Enter username"); return; }
    await fetch("/api/undo", {
        method: "POST",
        headers: {"Content-Type":"application/json"},
        body: JSON.stringify({ username })
    });
    loadTasks();
    loadNotifications();
}

redoBtn.onclick = async () => {
    const username = usernameInput.value.trim();
    if (!username) { alert("Enter username"); return; }
    await fetch("/api/redo", {
        method: "POST",
        headers: {"Content-Type":"application/json"},
        body: JSON.stringify({ username })
    });
    loadTasks();
    loadNotifications();
}

notifyBtn.onclick = loadNotifications;

async function loadNotifications() {
    const username = usernameInput.value.trim();
    if (!username) { notificationsDiv.innerHTML = "<p style='color:#777'>Enter username</p>"; return; }
    const res = await fetch(`/api/notifications?username=${encodeURIComponent(username)}`);
    const notes = await res.json();
    notificationsDiv.innerHTML = "";
    if (!notes || notes.length === 0) {
        notificationsDiv.innerHTML = "<p style='color:#777'>No notifications</p>";
        return;
    }
    notificationsDiv.innerHTML = notes.map(n => {
        // n may be an object (task) or string - handle both
        if (typeof n === 'object') {
            return `<div class="notif-item"><strong>${escapeHtml(n.title || "")}</strong> — ${escapeHtml(n.status || "")} (Due: ${escapeHtml(n.due || "")})</div>`;
        }
        return `<div class="notif-item">${escapeHtml(String(n))}</div>`;
    }).join("");
}

// small helper to avoid XSS when injecting text
function escapeHtml(s) {
    if (!s) return "";
    return s.replaceAll("&", "&amp;").replaceAll("<","&lt;").replaceAll(">","&gt;").replaceAll('"',"&quot;");
}
// Show notifications
document.getElementById("notifyBtn").addEventListener("click", () => {
  const notificationsDiv = document.getElementById("notifications");

  if (!notificationsDiv.innerHTML.trim()) {
    notificationsDiv.innerHTML = "<p>No recent notifications.</p>";
  }

  notificationsDiv.style.display =
    notificationsDiv.style.display === "block" ? "none" : "block";
});

// auto-load when username changes
usernameInput.addEventListener("change", loadTasks);
usernameInput.addEventListener("keyup", e => { if(e.key === "Enter") loadTasks(); });

// initial hint
taskList.innerHTML = "<li style='color:#777'>Enter username to see tasks</li>";
