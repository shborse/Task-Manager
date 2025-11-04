Collaborative Task Manager
1. Introduction

The Collaborative Task Manager is a full-stack project designed to manage and share tasks among multiple users.
It integrates C, Python, and Web technologies (HTML, CSS, JavaScript) to demonstrate practical applications of Data Structures and Algorithms (DSA) in a real-world collaborative system.

This project allows users to create, assign, update, and delete tasks with real-time communication between the frontend and backend using a C-based API library.

2. Features

Add, edit, delete, and view tasks.

Shared task management for multiple users.

Task prioritization and due dates.

Efficient task storage using data structures (linked lists, queues, etc.).

Backend written in Python with integration to C via a DLL.

Frontend built using HTML, CSS, and JavaScript.

Persistent data handling through the task API.

3. System Architecture

The system consists of three main layers:

Frontend (Presentation Layer)

User interface built with HTML, CSS, and JavaScript.

Provides task creation, modification, and display options.

Backend (Application Layer)

Implemented in Python (server.py).

Handles requests from the frontend and interacts with the compiled C library.

Task API (Logic Layer)

Implemented in C (task_api.c).

Compiled into a dynamic library (task_api.dll).

Performs task operations efficiently using DSA techniques.

4. Project Structure
Task Manager DSA/
│
├── task_api.c           # Core data structure and task management logic
├── task_api.dll         # Compiled shared library (C API)
│
├── server.py            # Python backend (Flask-based HTTP server)
│
├── frontend/
│   ├── index.html       # User interface
│   ├── style.css        # Stylesheet for layout and design
│   ├── script.js        # Client-side logic and API requests
│
└── README.md            # Project documentation

5. Technologies Used
Component	Technology Used
Frontend	HTML, CSS, JavaScript
Backend	Python (Flask or SimpleHTTPServer)
Task API	C (compiled as DLL using GCC)
Data Handling	Linked lists, queues, and file handling
Operating System	Windows / Linux compatible
6. Setup Instructions
Step 1: Clone or Extract the Project

Download or extract the folder Task Manager DSA to your local system.

Step 2: Compile the C API

Use a C compiler (e.g., GCC) to build the shared library:

For Windows:

gcc -shared -o task_api.dll -fPIC task_api.c


For Linux/macOS:

gcc -shared -o task_api.so -fPIC task_api.c

Step 3: Install Required Python Packages

Ensure Python is installed and install Flask (if not already installed):

pip install flask

Step 4: Run the Server

Start the backend server:

python server.py


The server will run on http://localhost:5000

Step 5: Launch the Frontend

Open the frontend/index.html file in a web browser to access the interface.

7. API Endpoints
Method	Endpoint	Description
GET	/tasks	Retrieve all tasks
POST	/add	Add a new task
PUT	/update/<id>	Update task details
DELETE	/delete/<id>	Delete a task by ID
8. Data Structures and Algorithms Used

The C API (task_api.c) implements the following:

Linked List: For storing dynamic task records.

Queue or Stack: For scheduling and priority handling.

File Handling: For saving and loading tasks.

Pointers and Structures: For efficient data management and memory utilization.

9. Future Enhancements

Integration of user authentication and login system.

Database connectivity (MySQL or SQLite) for persistent storage.

Enhanced frontend with task filtering and sorting features.

Notification system for task reminders.

10. Conclusion

The Collaborative Task Manager demonstrates how core Data Structures and Algorithms can be integrated with real-world applications using modern web technologies.
It serves as a practical implementation of a multi-language system that combines the efficiency of C with the flexibility of Python and the accessibility of web technologies.

11. Author

Name: Shreya Borse, Palak Chandankhede, Gayatri Chaudhari
Course: Data Structures and Algorithms
Project Title: Collaborative Task Manager
Languages Used: C, Python, HTML, CSS, JavaScript

Would you like me to generate this as a README.md
