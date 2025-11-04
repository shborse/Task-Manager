// task_manager_api.c
// Compile (MinGW-w64 64-bit):
// gcc -shared -o task_manager_api.dll task_manager_api.c -Wl,--add-stdcall-alias -O2 -std=c11 -m64

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

#ifdef __cplusplus
extern "C" {
#endif

// ----- Constants -----
#define MAX_USERS 20
#define MAX_USERNAME 50
#define MAX_NOTIF 200
#define MAX_NOTIF_MSG 256
#define JSON_BUF 32000

// ----- Task node for global BST -----
typedef struct TaskNode {
    int id;
    char title[128];
    int priority;
    char dueDate[20];
    char status[32];
    char timestamp[32];
    struct TaskNode *left, *right;
} TaskNode;

// ----- Doubly-linked list node to hold pointers to TaskNode (per-user assigned tasks) -----
typedef struct TaskDLL {
    TaskNode *task;
    struct TaskDLL *prev, *next;
} TaskDLL;

// ----- Per-user structure -----
typedef struct {
    char username[MAX_USERNAME];
    char password[64]; // optional (can be empty)
    TaskDLL *head, *tail; // assigned tasks list
    // undo/redo stacks (store task IDs)
    int undoStack[128];
    int undoTop;
    int redoStack[128];
    int redoTop;
} User;

// ----- Global storage -----
static TaskNode *taskRoot = NULL;      // BST of all tasks (global pool)
static int nextTaskID = 1;
static User users[MAX_USERS];
static int userCount = 0;

// Notification queue (circular)
static char notifQ[MAX_NOTIF][MAX_NOTIF_MSG];
static int notifFront = 0, notifRear = -1, notifCount = 0;

// ---------- Utility ----------
static void currentTimeStr(char *buf, int n) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    snprintf(buf, n, "%04d-%02d-%02d %02d:%02d:%02d",
             tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
             tm.tm_hour, tm.tm_min, tm.tm_sec);
}

static void enqueueNotif(const char *msg){
    if(notifCount >= MAX_NOTIF) {
        // drop oldest
        notifFront = (notifFront + 1) % MAX_NOTIF;
        notifCount--;
    }
    notifRear = (notifRear + 1) % MAX_NOTIF;
    strncpy(notifQ[notifRear], msg, MAX_NOTIF_MSG-1);
    notifQ[notifRear][MAX_NOTIF_MSG-1] = 0;
    notifCount++;
}

static const char* dequeueAllNotifsJSON(){
    static char buf[JSON_BUF];
    buf[0]=0;
    strcat(buf,"[");
    int first = 1;
    for(int i=0, idx=notifFront; i<notifCount; i++, idx=(idx+1)%MAX_NOTIF){
        if(!first) strcat(buf,",");
        first = 0;
        char tmp[512];
        snprintf(tmp, sizeof(tmp), "\"%s\"", notifQ[idx]);
        strcat(buf, tmp);
    }
    strcat(buf,"]");
    return buf;
}

// ---------- User management ----------
static User* findUser(const char* username){
    if(!username) return NULL;
    for(int i=0;i<userCount;i++){
        if(strcmp(users[i].username, username)==0) return &users[i];
    }
    return NULL;
}

static User* createOrGetUser(const char* username){
    User *u = findUser(username);
    if(u) return u;
    if(userCount >= MAX_USERS) return NULL;
    strncpy(users[userCount].username, username, MAX_USERNAME-1);
    users[userCount].username[MAX_USERNAME-1]=0;
    users[userCount].password[0]=0;
    users[userCount].head = users[userCount].tail = NULL;
    users[userCount].undoTop = users[userCount].redoTop = 0;
    return &users[userCount++];
}

// ---------- BST operations (global tasks) ----------
static TaskNode* createTaskNode(int id, const char* title, int priority, const char* due, const char* status){
    TaskNode *n = (TaskNode*)malloc(sizeof(TaskNode));
    if(!n) return NULL;
    n->id = id;
    strncpy(n->title, title?title:"", sizeof(n->title)-1);
    n->title[sizeof(n->title)-1]=0;
    n->priority = priority;
    strncpy(n->dueDate, due?due:"", sizeof(n->dueDate)-1);
    n->dueDate[sizeof(n->dueDate)-1]=0;
    strncpy(n->status, status?status:"", sizeof(n->status)-1);
    n->status[sizeof(n->status)-1]=0;
    currentTimeStr(n->timestamp, sizeof(n->timestamp));
    n->left = n->right = NULL;
    return n;
}

static TaskNode* bst_insert(TaskNode* root, TaskNode* node){
    if(!root) return node;
    if(node->id < root->id) root->left = bst_insert(root->left, node);
    else root->right = bst_insert(root->right, node);
    return root;
}

static TaskNode* bst_search(TaskNode* root, int id){
    if(!root) return NULL;
    if(root->id == id) return root;
    if(id < root->id) return bst_search(root->left, id);
    return bst_search(root->right, id);
}

// in-order traversal that appends JSON of all tasks
static void bst_all_tasks_json(TaskNode* root, char *buf, int *first) {
    if(!root) return;
    bst_all_tasks_json(root->left, buf, first);
    char tmp[512];
    if(!(*first)) strcat(buf, ",");
    *first = 0;
    snprintf(tmp, sizeof(tmp),
        "{\"id\":%d,\"title\":\"%s\",\"priority\":%d,\"due\":\"%s\",\"status\":\"%s\",\"time\":\"%s\"}",
        root->id, root->title, root->priority, root->dueDate, root->status, root->timestamp);
    strcat(buf, tmp);
    bst_all_tasks_json(root->right, buf, first);
}

// ---------- Per-user DLL list helpers ----------
static TaskDLL* makeDLLNode(TaskNode *task){
    TaskDLL *n = (TaskDLL*)malloc(sizeof(TaskDLL));
    n->task = task;
    n->prev = n->next = NULL;
    return n;
}

static void user_add_taskdll(User *u, TaskNode *task){
    if(!u || !task) return;
    // check if already assigned
    TaskDLL *iter = u->head;
    while(iter){
        if(iter->task == task) return; // already assigned
        iter = iter->next;
    }
    TaskDLL *nd = makeDLLNode(task);
    if(!u->head){
        u->head = u->tail = nd;
    } else {
        u->tail->next = nd;
        nd->prev = u->tail;
        u->tail = nd;
    }
}

static int user_remove_taskdll_byid(User *u, int id){
    if(!u) return 0;
    TaskDLL *iter = u->head;
    while(iter){
        if(iter->task->id == id){
            if(iter->prev) iter->prev->next = iter->next;
            else u->head = iter->next;
            if(iter->next) iter->next->prev = iter->prev;
            else u->tail = iter->prev;
            free(iter);
            return 1;
        }
        iter = iter->next;
    }
    return 0;
}

// build JSON of a user's tasks (from their DLL)
static void user_tasks_to_json(User *u, char *buf) {
    buf[0]=0;
    strcat(buf,"[");
    TaskDLL *it = u->head;
    int first = 1;
    while(it){
        char tmp[512];
        if(!first) strcat(buf,",");
        first = 0;
        snprintf(tmp, sizeof(tmp),
            "{\"id\":%d,\"title\":\"%s\",\"priority\":%d,\"due\":\"%s\",\"status\":\"%s\",\"time\":\"%s\"}",
            it->task->id, it->task->title, it->task->priority, it->task->dueDate, it->task->status, it->task->timestamp);
        strcat(buf,tmp);
        it = it->next;
    }
    strcat(buf,"]");
}

// ---------- Undo/Redo simple helpers ----------
static void user_push_undo(User *u, int taskID){
    if(!u) return;
    if(u->undoTop < 128) u->undoStack[u->undoTop++] = taskID;
}

static int user_pop_undo(User *u){
    if(!u || u->undoTop==0) return -1;
    return u->undoStack[--u->undoTop];
}
static void user_push_redo(User *u, int taskID){
    if(!u) return;
    if(u->redoTop < 128) u->redoStack[u->redoTop++] = taskID;
}
static int user_pop_redo(User *u){
    if(!u || u->redoTop==0) return -1;
    return u->redoStack[--u->redoTop];
}

// ---------- Exported API functions ----------

// login_user_api: create user if not exists; simple "login" (no password required here)
EXPORT int STDCALL login_user_api(const char* username, const char* password) {
    if(!username) return 0;
    User *u = findUser(username);
    if(!u) {
        u = createOrGetUser(username);
        if(!u) return 0;
        if(password && strlen(password)>0) {
            strncpy(u->password, password, sizeof(u->password)-1);
            u->password[sizeof(u->password)-1]=0;
        }
        return 1; // created => considered logged in
    }
    // If user exists and password provided, check (if password set), otherwise allow
    if(password && strlen(u->password)>0) {
        return strcmp(u->password, password)==0 ? 1 : 0;
    }
    return 1;
}

// add_task_api: create a global task and automatically assign to username
EXPORT int STDCALL add_task_api(const char* username, const char* title, int priority, const char* dueDate, const char* status) {
    if(!username || !title) return -1;
    User *u = createOrGetUser(username);
    if(!u) return -1;
    TaskNode *n = createTaskNode(nextTaskID++, title, priority, dueDate, status);
    if(!n) return -1;
    taskRoot = bst_insert(taskRoot, n);
    // assign to user (make link in user's DLL)
    user_add_taskdll(u, n);
    // push undo (for removal)
    user_push_undo(u, n->id);
    // notify
    char nm[128];
    snprintf(nm, sizeof(nm), "Task #%d created by %s: %s", n->id, username, title);
    enqueueNotif(nm);
    return n->id;
}

// edit_task_api: modify global task fields (must be global)
EXPORT int STDCALL edit_task_api(const char* username, int id, const char* title, int priority, const char* dueDate, const char* status) {
    TaskNode *t = bst_search(taskRoot, id);
    if(!t) return -1;
    if(title && strlen(title)>0) strncpy(t->title, title, sizeof(t->title)-1);
    t->priority = priority;
    if(dueDate && strlen(dueDate)>0) strncpy(t->dueDate, dueDate, sizeof(t->dueDate)-1);
    if(status && strlen(status)>0) strncpy(t->status, status, sizeof(t->status)-1);
    currentTimeStr(t->timestamp, sizeof(t->timestamp));
    char nm[128];
    snprintf(nm, sizeof(nm), "Task #%d edited by %s", id, username?username:"unknown");
    enqueueNotif(nm);
    return 0;
}

// remove_task_api: unassign from user and remove from global BST (for simplicity we only unassign)
EXPORT int STDCALL remove_task_api(const char* username, int id) {
    User *u = findUser(username);
    if(!u) return 0;
    // remove from user's DLL
    int removed = user_remove_taskdll_byid(u, id);
    if(removed){
        user_push_undo(u, id);
        char nm[128];
        snprintf(nm,sizeof(nm), "Task #%d removed by %s", id, username);
        enqueueNotif(nm);
        return 1;
    }
    return 0;
}

// assign_task_api: assign existing global task to another user
EXPORT int STDCALL assign_task_api(const char* fromUser, const char* toUser, int id) {
    TaskNode *t = bst_search(taskRoot, id);
    if(!t) return 0;
    User *to = createOrGetUser(toUser);
    if(!to) return 0;
    user_add_taskdll(to, t);
    // push undo on target
    user_push_undo(to, id);
    char nm[128];
    snprintf(nm,sizeof(nm),"Task #%d assigned to %s by %s", id, toUser?toUser:"", fromUser?fromUser:"");
    enqueueNotif(nm);
    return 1;
}

// undo_api: simple undo pop (reverses last assign/remove for that user)
EXPORT int STDCALL undo_api(const char* username) {
    User *u = findUser(username);
    if(!u) return 0;
    int id = user_pop_undo(u);
    if(id < 0) return 0;
    // if task assigned currently => remove (undo assign), else if not assigned => re-add (undo remove)
    TaskDLL *it = u->head;
    int found = 0;
    while(it){ if(it->task->id==id){ found = 1; break; } it = it->next; }
    if(found){
        user_remove_taskdll_byid(u, id);
        user_push_redo(u, id);
        enqueueNotif("Undo performed: unassigned task");
    } else {
        TaskNode *tn = bst_search(taskRoot, id);
        if(tn){
            user_add_taskdll(u, tn);
            user_push_redo(u, id);
            enqueueNotif("Undo performed: re-assigned task");
        }
    }
    return 1;
}

// redo_api: reverse undo
EXPORT int STDCALL redo_api(const char* username) {
    User *u = findUser(username);
    if(!u) return 0;
    int id = user_pop_redo(u);
    if(id < 0) return 0;
    // perform redo logic similar to above
    TaskDLL *it = u->head;
    int found = 0;
    while(it){ if(it->task->id==id){ found = 1; break; } it = it->next; }
    if(found){
        // if present, redo might remove -> remove
        user_remove_taskdll_byid(u, id);
        user_push_undo(u, id);
    } else {
        TaskNode *tn = bst_search(taskRoot, id);
        if(tn){
            user_add_taskdll(u, tn);
            user_push_undo(u, id);
        }
    }
    enqueueNotif("Redo performed");
    return 1;
}

// list_tasks_api: returns JSON array for the user's assigned tasks
EXPORT const char* STDCALL list_tasks_api(const char* username) {
    static char buf[JSON_BUF];
    if(!username) return "[]";
    User *u = findUser(username);
    if(!u) return "[]";
    user_tasks_to_json(u, buf);
    return buf;
}

// notifications_api: returns notifications for a user (here we return all notifications for simplicity)
EXPORT const char* STDCALL notifications_api(const char* username) {
    // Could filter by user; for now return all notifications
    return dequeueAllNotifsJSON();
}

// manager_tasks_api: returns JSON array of all tasks in global BST (manager view)
EXPORT const char* STDCALL manager_tasks_api() {
    static char buf[JSON_BUF];
    buf[0]=0;
    strcat(buf,"[");
    int first = 1;
    bst_all_tasks_json(taskRoot, buf, &first);
    strcat(buf,"]");
    return buf;
}

// manager_notifications_api: aggregated notifications (same as notifications_api)
EXPORT const char* STDCALL manager_notifications_api() {
    return dequeueAllNotifsJSON();
}

// list_users_api
EXPORT const char* STDCALL list_users_api() {
    static char buf[1024];
    buf[0]=0;
    strcat(buf,"[");
    for(int i=0;i<userCount;i++){
        if(i) strcat(buf,",");
        char tmp[128];
        snprintf(tmp,sizeof(tmp),"\"%s\"", users[i].username);
        strcat(buf,tmp);
    }
    strcat(buf,"]");
    return buf;
}

// search_task_api (simple: find by substring in title across user's assigned tasks)
EXPORT const char* STDCALL search_task_api(const char* username, const char* q) {
    static char buf[JSON_BUF];
    buf[0]=0;
    strcat(buf,"[");
    User *u = findUser(username);
    if(!u || !q) { strcat(buf,"]"); return buf; }
    TaskDLL *it = u->head;
    int first=1;
    while(it){
        if(strstr(it->task->title, q)){
            char tmp[512];
            if(!first) strcat(buf,",");
            first=0;
            snprintf(tmp,sizeof(tmp), "{\"id\":%d,\"title\":\"%s\",\"priority\":%d,\"due\":\"%s\",\"status\":\"%s\"}",
                     it->task->id, it->task->title, it->task->priority, it->task->dueDate, it->task->status);
            strcat(buf,tmp);
        }
        it = it->next;
    }
    strcat(buf,"]");
    return buf;
}

// filter_task_api (filter by status or priority for a user)
EXPORT const char* STDCALL filter_task_api(const char* username, const char* status, int priority) {
    static char buf[JSON_BUF];
    buf[0]=0;
    strcat(buf,"[");
    User *u = findUser(username);
    if(!u) { strcat(buf,"]"); return buf; }
    TaskDLL *it = u->head;
    int first=1;
    while(it){
        int ok = 1;
        if(status && strlen(status)>0) ok &= (strcmp(it->task->status, status)==0);
        if(priority>0) ok &= (it->task->priority == priority);
        if(ok){
            char tmp[512];
            if(!first) strcat(buf,",");
            first=0;
            snprintf(tmp,sizeof(tmp), "{\"id\":%d,\"title\":\"%s\",\"priority\":%d,\"due\":\"%s\",\"status\":\"%s\"}",
                     it->task->id, it->task->title, it->task->priority, it->task->dueDate, it->task->status);
            strcat(buf,tmp);
        }
        it = it->next;
    }
    strcat(buf,"]");
    return buf;
}

// analytics_api (basic stats)
EXPORT const char* STDCALL analytics_api(const char* username) {
    static char buf[256];
    int total = 0;
    int comp = 0;
    for(int i=0;i<userCount;i++){
        total += users[i].undoTop; // meaningless small stat: number of undo ops per user (placeholder)
    }
    snprintf(buf,sizeof(buf), "{\"users\":%d,\"tasks_total_estimate\":%d}", userCount, nextTaskID-1);
    return buf;
}

// sort_tasks_api placeholder (returns success)
EXPORT int STDCALL sort_tasks_api(const char* username, const char* criterion) {
    (void)username; (void)criterion;
    return 1;
}

// save/load placeholders
EXPORT int STDCALL save_data_api(const char* filename) { (void)filename; return 1; }
EXPORT int STDCALL load_data_api(const char* filename) { (void)filename; return 1; }

// clear_notifications_api
EXPORT int STDCALL clear_notifications_api(const char* username) {
    (void)username;
    notifFront = 0; notifRear = -1; notifCount = 0;
    return 1;
}

// ----------------- End extern "C"
#ifdef __cplusplus
}
#endif
