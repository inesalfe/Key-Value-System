#include <stdio.h>
#include <unistd.h>
#include <sys/un.h>
#include <stdbool.h>

#define BUF_SIZE 100

struct WatchList {
    char key[BUF_SIZE];
    struct WatchList * next;
};

// The app has a pid, a file descriptor, a start and stop time (of the connection) and a pointer to the next app
struct App {
    long pid;
    int fd;
    bool isClosed;
    int fd_cb;
    struct timespec start;
    struct timespec stop;
    double delta_t;
    struct WatchList * wlist;
    struct App * next;
};

// Prints all the information about all apps
// This function is called by the "ShowAppStatus" function, called by the "Show application status" command
void PrintAppList(struct App * head);

// Function that return whether a app with a certain file descriptor is in the list
// This is used in the function "addKeyValue_toGroup", called by the function "put_value"
// Returns false if the app was not found, true otherwise
bool FindApp(struct App * head, int pid);

// Appends a new app to the end of the app List
// This funciton is called by the "addApp_toGroup" function, called when an app is trying to connect to a group
void AppendApp(struct App ** head_ref, int cl_fd, int fd_cb, int pid_in);

// Closes the file descriptor of an app and stops the connection time
// This funciton is called by the "close_GroupApp" function, called when the function "close_connection" of the app is called
bool CloseConnection(struct App * head, int pid);

void CloseAllConnections(struct App * head);

// void CloseFileDesc(struct App ** head_ref);

void DeleteAppList(struct App ** head_ref);

void AddKeyToList(struct WatchList ** head_ref, char * key);

void DeleteWatchList(struct WatchList ** head_ref);

void DeleteFromWatchList(struct WatchList ** head_ref, char * key);

bool IsWatchList(struct WatchList * head, char * key);
