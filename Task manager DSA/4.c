#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TITLE 50
#define MAX_NOTIF 50

// ---------------- TASK (BST) ----------------
typedef struct Task {
    int id;
    char title[MAX_TITLE];
    int priority;        // smaller number = higher priority
    char dueDate[11];    // format: YYYY-MM-DD
    char status[20];     // e.g., "Pending", "Completed"
    struct Task *left;
    struct Task *right;
} Task;

// ---------------- DLL (User’s Tasks) ----------------
typedef struct DLLNode {
    Task *task;
    struct DLLNode *prev;
    struct DLLNode *next;
} DLLNode;

// ---------------- USER ----------------
typedef struct User {
    char username[20];
    DLLNode *taskHead;
    DLLNode *taskTail;
} User;

// ---------------- STACK (Undo/Redo) ----------------
typedef struct Operation {
    char type[20]; // "assign" or "remove"
    int taskID;
} Operation;

typedef struct StackNode {
    Operation op;
    struct StackNode *next;
} StackNode;

StackNode *undoTop = NULL, *redoTop = NULL;

// ---------------- QUEUE (Notifications) ----------------
typedef struct Queue {
    char messages[MAX_NOTIF][100];
    int front, rear, count;
} Queue;

Queue notifQ = {.front = 0, .rear = -1, .count = 0};

// ---------------- Function Prototypes ----------------
// BST
Task* createTask(int id, char *title, int priority, char *dueDate, char *status);
Task* insertTaskBST(Task *root, Task *newtask);
Task* searchTaskBST(Task *root, int id);
void inorderBST(Task *root);

// DLL
void addTaskToUser(User *user, Task *task);
void removeTaskFromUser(User *user, int taskID);
void displayUserTasks(User *user);

// Stack
void pushUndo(Operation op);
Operation popUndo();
void pushRedo(Operation op);
Operation popRedo();
void undoOperation(User *user, Task *root);
void redoOperation(User *user, Task *root);

// Queue
void enqueueNotification(const char *msg);
void dequeueNotification();
void displayNotifications();

// ---------------- BST FUNCTIONS ----------------
Task* createTask(int id, char *title, int priority, char *dueDate, char *status) {
    Task* newtask = (Task*)malloc(sizeof(Task));
    newtask->id = id;
    strcpy(newtask->title, title);
    newtask->priority = priority;
    strcpy(newtask->dueDate, dueDate);
    strcpy(newtask->status, status);
    newtask->left = newtask->right = NULL;
    return newtask;
}

Task* insertTaskBST(Task *root, Task *newtask) {
    if (root == NULL) return newtask;
    if (newtask->priority < root->priority)
        root->left = insertTaskBST(root->left, newtask);
    else
        root->right = insertTaskBST(root->right, newtask);
    return root;
}

Task* searchTaskBST(Task *root, int id) {
    if (root == NULL || root->id == id) return root;
    if (id < root->id)
        return searchTaskBST(root->left, id);
    else
        return searchTaskBST(root->right, id);
}

void inorderBST(Task *root) {
    if (root != NULL) {
        inorderBST(root->left);
        printf("ID:%d | %s | Priority:%d | Due:%s | Status:%s\n",
               root->id, root->title, root->priority, root->dueDate, root->status);
        inorderBST(root->right);
    }
}

// ---------------- DLL FUNCTIONS ----------------
void addTaskToUser(User *user, Task *task) {
    DLLNode *newNode = (DLLNode*)malloc(sizeof(DLLNode));
    newNode->task = task;
    newNode->prev = NULL;
    newNode->next = NULL;

    if (user->taskHead == NULL) {
        user->taskHead = user->taskTail = newNode;
    } else {
        user->taskTail->next = newNode;
        newNode->prev = user->taskTail;
        user->taskTail = newNode;
    }
}

void removeTaskFromUser(User *user, int taskID) {
    DLLNode *temp = user->taskHead;
    while (temp != NULL && temp->task->id != taskID) {
        temp = temp->next;
    }

    if (temp == NULL) {
        printf("Task with ID %d not found for user %s!\n", taskID, user->username);
        return;
    }

    if (temp->prev) temp->prev->next = temp->next;
    else user->taskHead = temp->next;

    if (temp->next) temp->next->prev = temp->prev;
    else user->taskTail = temp->prev;

    free(temp);
    printf("Task with ID %d removed successfully from user %s!\n", taskID, user->username);
}

void displayUserTasks(User *user) {
    DLLNode *temp = user->taskHead;
    if (temp == NULL) {
        printf("No tasks for user %s!\n", user->username);
        return;
    }

    printf("Tasks for User %s:\n", user->username);
    while (temp != NULL) {
        printf("ID:%d | %s | Priority:%d | Due:%s | Status:%s\n",
                temp->task->id, temp->task->title, temp->task->priority,
                temp->task->dueDate, temp->task->status);
        temp = temp->next;
    }
}

// ---------------- STACK (Undo/Redo Functions) ----------------
void pushUndo(Operation op) {
    StackNode *newNode = (StackNode*)malloc(sizeof(StackNode));
    newNode->op = op;
    newNode->next = undoTop;
    undoTop = newNode;
}

Operation popUndo() {
    Operation dummy = {"none", -1};
    if (undoTop == NULL) return dummy;
    StackNode *temp = undoTop;
    Operation op = temp->op;
    undoTop = undoTop->next;
    free(temp);
    return op;
}

void pushRedo(Operation op) {
    StackNode *newNode = (StackNode*)malloc(sizeof(StackNode));
    newNode->op = op;
    newNode->next = redoTop;
    redoTop = newNode;
}

Operation popRedo() {
    Operation dummy = {"none", -1};
    if (redoTop == NULL) return dummy;
    StackNode *temp = redoTop;
    Operation op = temp->op;
    redoTop = redoTop->next;
    free(temp);
    return op;
}

void undoOperation(User *user, Task *root) {
    if (undoTop == NULL) {
        printf("Nothing to undo!\n");
        return;
    }
    Operation op = popUndo();
    if (strcmp(op.type, "assign") == 0) {
        removeTaskFromUser(user, op.taskID);
        pushRedo(op);
        enqueueNotification("Undo: Task unassigned.");
    } else if (strcmp(op.type, "remove") == 0) {
        Task *task = searchTaskBST(root, op.taskID);
        if (task) {
            addTaskToUser(user, task);
            pushRedo(op);
            enqueueNotification("Undo: Task re-assigned.");
        }
    }
}

void redoOperation(User *user, Task *root) {
    if (redoTop == NULL) {
        printf("Nothing to redo!\n");
        return;
    }
    Operation op = popRedo();
    if (strcmp(op.type, "assign") == 0) {
        Task *task = searchTaskBST(root, op.taskID);
        if (task) addTaskToUser(user, task);
        pushUndo(op);
        enqueueNotification("Redo: Task assigned again.");
    } else if (strcmp(op.type, "remove") == 0) {
        removeTaskFromUser(user, op.taskID);
        pushUndo(op);
        enqueueNotification("Redo: Task removed again.");
    }
}

// ---------------- QUEUE (Notifications) ----------------
void enqueueNotification(const char *msg) {
    if (notifQ.count == MAX_NOTIF) {
        printf("Notification Queue Full!\n");
        return;
    }
    notifQ.rear = (notifQ.rear + 1) % MAX_NOTIF;
    strcpy(notifQ.messages[notifQ.rear], msg);
    notifQ.count++;
}

void dequeueNotification() {
    if (notifQ.count == 0) {
        printf("No notifications.\n");
        return;
    }
    printf("Notification: %s\n", notifQ.messages[notifQ.front]);
    notifQ.front = (notifQ.front + 1) % MAX_NOTIF;
    notifQ.count--;
}

void displayNotifications() {
    if (notifQ.count == 0) {
        printf("No notifications to display.\n");
        return;
    }
    printf("Notifications:\n");
    int i, idx = notifQ.front;
    for (i = 0; i < notifQ.count; i++) {
        printf("- %s\n", notifQ.messages[idx]);
        idx = (idx + 1) % MAX_NOTIF;
    }
}

// ---------------- MAIN ----------------
int main() {
    Task *root = NULL;
    User user1;
    strcpy(user1.username, "Alice");
    user1.taskHead = user1.taskTail = NULL;

    int choice, priority;
    char title[MAX_TITLE], dueDate[11], status[20];
    int nextTaskID = 1;

    while (1) {
        printf("\n===== TASK MANAGEMENT MENU =====\n");
        printf("1. Add Task (to BST)\n");
        printf("2. Display All Tasks (BST Inorder)\n");
        printf("3. Assign Task to User\n");
        printf("4. Remove Task from User\n");
        printf("5. Display User's Tasks\n");
        printf("6. Undo Last Operation\n");
        printf("7. Redo Last Operation\n");
        printf("8. Show Notifications\n");
        printf("9. Exit\n");
        printf("Enter choice: ");
        scanf("%d", &choice);
        getchar();

        switch (choice) {
            case 1:
                printf("Enter Title: ");
                fgets(title, MAX_TITLE, stdin);
                title[strcspn(title, "\n")] = '\0';
                printf("Enter Priority (1=High, 2=Med, 3=Low): ");
                scanf("%d", &priority);
                getchar();
                printf("Enter Due Date (YYYY-MM-DD): ");
                fgets(dueDate, 11, stdin);
                dueDate[strcspn(dueDate, "\n")] = '\0';
                printf("Enter Status: ");
                fgets(status, 20, stdin);
                status[strcspn(status, "\n")] = '\0';

                root = insertTaskBST(root, createTask(nextTaskID++, title, priority, dueDate, status));
                printf("Task added successfully with ID %d!\n", nextTaskID - 1);
                enqueueNotification("New task added.");
                break;

            case 2:
                printf("\nAll Tasks in BST (sorted by priority):\n");
                inorderBST(root);
                break;

            case 3: {
                printf("Enter Task ID to assign: ");
                int assignID;
                scanf("%d", &assignID);
                Task *task = searchTaskBST(root, assignID);
                if (task) {
                    addTaskToUser(&user1, task);
                    printf("Task assigned to %s.\n", user1.username);
                    Operation op = {"assign", assignID};
                    pushUndo(op);
                    enqueueNotification("Task assigned to user.");
                } else {
                    printf("Task ID %d not found!\n", assignID);
                }
                break;
            }

            case 4: {
                printf("Enter Task ID to remove from user: ");
                int removeID;
                scanf("%d", &removeID);
                removeTaskFromUser(&user1, removeID);
                Operation op = {"remove", removeID};
                pushUndo(op);
                enqueueNotification("Task removed from user.");
                break;
            }

            case 5:
                displayUserTasks(&user1);
                break;

            case 6:
                undoOperation(&user1, root);
                break;

            case 7:
                redoOperation(&user1, root);
                break;

            case 8:
                displayNotifications();
                break;

            case 9:
                printf("Exiting...\n");
                exit(0);

            default:
                printf("Invalid choice!\n");
        }
    }
}