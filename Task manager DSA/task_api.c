#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#define STDCALL __stdcall
#else
#define EXPORT
#define STDCALL
#endif

#define MAX_TASKS 100
#define MAX_USERS 10
#define MAX_HISTORY 50

typedef struct {
    int id;
    char title[100];
    int priority;
    char dueDate[20];
    char status[20];
    char timestamp[30];
} Task;

typedef struct {
    char username[50];
    Task tasks[MAX_TASKS];
    int taskCount;

    // Undo/Redo stacks
    Task undoStack[MAX_HISTORY];
    int undoCount;
    Task redoStack[MAX_HISTORY];
    int redoCount;

} User;

static User users[MAX_USERS];
static int userCount = 0;
static int nextTaskID = 1;

// Utility to get current time as string
void currentTime(char* buffer, int size) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    snprintf(buffer, size, "%04d-%02d-%02d %02d:%02d:%02d",
            tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec);
}

// Get or create user
User* getUser(const char* username) {
    for(int i=0;i<userCount;i++)
        if(strcmp(users[i].username, username)==0)
            return &users[i];

    if(userCount>=MAX_USERS) return NULL;
    strcpy(users[userCount].username, username);
    users[userCount].taskCount = 0;
    users[userCount].undoCount = 0;
    users[userCount].redoCount = 0;
    return &users[userCount++];
}

// Add task
EXPORT int STDCALL add_task_api(const char* username, const char* title, int priority, const char* dueDate, const char* status) {
    User* u = getUser(username);
    if(!u) 
        return -1;
    if(u->taskCount>=MAX_TASKS) 
        return -1;

    Task* t = &u->tasks[u->taskCount++];
    t->id = nextTaskID++;
    strncpy(t->title, title, 99);
    t->priority = priority;
    strncpy(t->dueDate, dueDate, 19);
    strncpy(t->status, status, 19);
    currentTime(t->timestamp, 30);

    // Push to undo stack
    if(u->undoCount<MAX_HISTORY) u->undoStack[u->undoCount++] = *t;
    u->redoCount = 0; // Clear redo

    return t->id;
}

// Remove task
EXPORT int STDCALL remove_task_api(const char* username, int taskID) {
    User* u = getUser(username);
    if(!u) return 0;
    for(int i=0;i<u->taskCount;i++){
        if(u->tasks[i].id == taskID){
            // push to undo
            if(u->undoCount<MAX_HISTORY) u->undoStack[u->undoCount++] = u->tasks[i];
            u->redoCount = 0;

            // shift tasks
            for(int j=i;j<u->taskCount-1;j++)
                u->tasks[j] = u->tasks[j+1];
            u->taskCount--;
            return 1;
        }
    }
    return 0;
}

// Undo
EXPORT int STDCALL undo_api(const char* username) {
    User* u = getUser(username);
    if(!u || u->undoCount==0) return 0;

    Task t = u->undoStack[--u->undoCount];

    // push to redo
    if(u->redoCount<MAX_HISTORY) u->redoStack[u->redoCount++] = t;

    // remove from task list if exists
    for(int i=0;i<u->taskCount;i++)
        if(u->tasks[i].id==t.id){
            for(int j=i;j<u->taskCount-1;j++) u->tasks[j]=u->tasks[j+1];
            u->taskCount--;
            break;
        }

    return 1;
}

// Redo
EXPORT int STDCALL redo_api(const char* username) {
    User* u = getUser(username);
    if(!u || u->redoCount==0) return 0;

    Task t = u->redoStack[--u->redoCount];

    // push to undo
    if(u->undoCount<MAX_HISTORY) u->undoStack[u->undoCount++] = t;

    // add to tasks
    if(u->taskCount<MAX_TASKS) u->tasks[u->taskCount++] = t;
    return 1;
}

// List tasks as JSON
EXPORT const char* STDCALL list_tasks_api(const char* username) {
    static char buffer[10000];
    buffer[0]=0;
    User* u = getUser(username);
    if(!u) return "[]";

    strcat(buffer,"[");
    for(int i=0;i<u->taskCount;i++){
        char line[300];
        snprintf(line, 300,
                "{\"id\":%d,\"title\":\"%s\",\"priority\":%d,\"due\":\"%s\",\"status\":\"%s\",\"time\":\"%s\"}%s",
                u->tasks[i].id, u->tasks[i].title, u->tasks[i].priority,
                u->tasks[i].dueDate, u->tasks[i].status, u->tasks[i].timestamp,
                (i==u->taskCount-1)?"":",");
        strcat(buffer,line);
    }
    strcat(buffer,"]");
    return buffer;
}

// Notifications (all tasks for simplicity)
EXPORT const char* STDCALL notifications_api(const char* username) {
    return list_tasks_api(username);
}
