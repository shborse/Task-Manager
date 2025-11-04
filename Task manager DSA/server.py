import ctypes
import os
import json
from flask import Flask, request, jsonify, send_from_directory

app = Flask(__name__, static_folder="static")

# Load DLL (must be compiled and present)
dll_path = os.path.join(os.getcwd(), "task_manager_api.dll")
if not os.path.exists(dll_path):
    raise FileNotFoundError(f"{dll_path} not found. Compile your DLL first.")

task_api = ctypes.CDLL(dll_path)

# ----------------------------------------------------------------
# Tell Python about the functions and argument/return types.
# These names match the functions in the C file you posted.
# ----------------------------------------------------------------
task_api.add_task_api.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int, ctypes.c_char_p, ctypes.c_char_p]
task_api.add_task_api.restype  = ctypes.c_int

task_api.remove_task_api.argtypes = [ctypes.c_char_p, ctypes.c_int]
task_api.remove_task_api.restype  = ctypes.c_int

task_api.undo_api.argtypes = [ctypes.c_char_p]
task_api.undo_api.restype  = ctypes.c_int

task_api.redo_api.argtypes = [ctypes.c_char_p]
task_api.redo_api.restype  = ctypes.c_int

task_api.list_tasks_api.argtypes = [ctypes.c_char_p]
task_api.list_tasks_api.restype  = ctypes.c_char_p

task_api.notifications_api.argtypes = [ctypes.c_char_p]
task_api.notifications_api.restype  = ctypes.c_char_p

# ---------- Routes ----------
@app.route("/")
def index():
    # serve static/index.html
    return app.send_static_file("index.html")

# Get tasks for a username (returns JSON array)
@app.route("/api/tasks", methods=["GET"])
def get_tasks():
    username = request.args.get("username", "")
    buf = task_api.list_tasks_api(username.encode('utf-8'))
    if not buf:
        return jsonify([])
    # list_tasks_api returns a C string (JSON array) -> decode and parse
    try:
        text = ctypes.cast(buf, ctypes.c_char_p).value.decode('utf-8')
        return jsonify(json.loads(text))
    except Exception:
        return jsonify([])

# Add a task: expects JSON { username, title, due, priority, status }
@app.route("/api/tasks", methods=["POST"])
def add_task():
    data = request.get_json(force=True)
    username = data.get("username","").strip()
    title    = data.get("title","").strip()
    due      = data.get("due","2025-12-31")
    priority = int(data.get("priority", 1))
    status   = data.get("status","Pending")

    if not username or not title:
        return jsonify({"error":"username and title required"}), 400

    res = task_api.add_task_api(
        username.encode('utf-8'),
        title.encode('utf-8'),
        priority,
        due.encode('utf-8'),
        status.encode('utf-8')
    )
    if res < 0:
        return jsonify({"error":"failed to add task"}), 500
    return jsonify({"message":"Task added", "id": res})

# Delete a task: expects JSON { username, id }
@app.route("/api/tasks/delete", methods=["POST"])
def delete_task():
    data = request.get_json(force=True)
    username = data.get("username","").strip()
    task_id = int(data.get("id", 0))
    if not username or task_id <= 0:
        return jsonify({"error":"username and id required"}), 400
    ok = task_api.remove_task_api(username.encode('utf-8'), task_id)
    return jsonify({"success": bool(ok)})

# Undo for a username. JSON { username }
@app.route("/api/undo", methods=["POST"])
def undo():
    data = request.get_json(force=True)
    username = data.get("username","").strip()
    if not username:
        return jsonify({"error":"username required"}), 400
    ok = task_api.undo_api(username.encode('utf-8'))
    return jsonify({"success": bool(ok)})

# Redo for a username. JSON { username }
@app.route("/api/redo", methods=["POST"])
def redo():
    data = request.get_json(force=True)
    username = data.get("username","").strip()
    if not username:
        return jsonify({"error":"username required"}), 400
    ok = task_api.redo_api(username.encode('utf-8'))
    return jsonify({"success": bool(ok)})

# Notifications for a username (returns array)
@app.route("/api/notifications", methods=["GET"])
def notifications():
    username = request.args.get("username","").strip()
    buf = task_api.notifications_api(username.encode('utf-8'))
    if not buf:
        return jsonify([])
    try:
        text = ctypes.cast(buf, ctypes.c_char_p).value.decode('utf-8')
        # notifications_api returns JSON array string (same format as list_tasks_api),
        # but if you used a different format, adjust parsing here.
        return jsonify(json.loads(text))
    except Exception:
        # fallback: return raw string
        return jsonify([ctypes.cast(buf, ctypes.c_char_p).value.decode('utf-8')])

# static files (JS/CSS served automatically by Flask under /static/)

if __name__ == "__main__":
    # helpful reminder to the user
    print("Starting server on http://127.0.0.1:5000/ - serving static files from ./static")
    app.run(debug=True)
