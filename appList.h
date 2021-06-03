#include <stdio.h>
#include <unistd.h>
#include <sys/un.h>
#include <stdbool.h>

#define BUF_SIZE 100

// Linked List with all the keys for which we will call the callback function
struct WatchList {
    char key[BUF_SIZE]; // Keys for the callback
    struct WatchList * next;
};

struct App {
    long pid; // PID
    int fd; // File descriptor
    int fd_cb; // File descriptor for the callback
    bool isClosed; // Flag to check if the connection is closed
    struct timespec start; // Start time
    struct timespec stop; // Finish time
    double delta_t; // Connected time
    struct WatchList * wlist; // WatchList with the keys for the callbacks
    struct App * next;
};

// Prints all the information about all apps
// This function is called by the "ShowAppStatus" function, called by the "Show application status" command
void PrintAppList(struct App * head);

// Function that return whether a app with a certain file descriptor is in the list
// This is used in the function "AddKeyValueToGroup", called by the function "put_value"
// Returns false if the app was not found, true otherwise
bool FindApp(struct App * head, int pid);

// Appends a new app to the end of the app List
// This funciton is called by the "AddAppToGroup" function, called when an app is trying to connect to a group
void AppendApp(struct App ** head_ref, int cl_fd, int fd_cb, int pid_in);

// Closes the file descriptor of an app and stops the connection time
// This funciton is called by the "CloseApp" function, called when the function "close_connection" of the app is called
bool CloseConnection(struct App * head, int pid);

// Deletes a linked app list
// Called by the "DeleteGroupList" and "DeleteGroupLocalServer" functions
void DeleteAppList(struct App ** head_ref);

// Adds keys to watchlist of an app
// Called by the "AddKeyToWatchList" function
void AddKeyToList(struct WatchList ** head_ref, char * key);

// Deleste an app watchlist
// Called by the "DeleteGroupList" and "DeleteGroupLocalServer" functions
void DeleteWatchList(struct WatchList ** head_ref);

// Deletes a certain key from a watchlist
// Called by the "DeleteKeyValue" function
void DeleteFromWatchList(struct WatchList ** head_ref, char * key);

// Determines if a key belong to the watchlist of a certain app
// Called in the "put_value" function the local server
bool IsWatchList(struct WatchList * head, char * key);
