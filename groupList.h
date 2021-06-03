#include "hash.h"

#define BUF_SIZE 100

// For now, we have the group_name, the secret, a table of keys and values, and array of apps and a pointer to the next element
// After we finish this layer, the secret will be fetched from the autentification server
struct Group {
	char group_name[BUF_SIZE];
	struct HashTable * table;
	struct App * apps;
	struct Group * next;
};

// Function that creates a new entry in the Hashtable in the Authentification Server with the groups and secrets
// It's called by the "CreateGroupLocalServer" function
// Returns 1 in case of sucess and -1 in case of error
bool CreateGroupAuthServer(char * g_name, char * secret);

// Function to be called when the user uses the "Create Group" command
// Returns the secret for the created group
// It also adds the group in the authentification server
char * CreateGroupLocalServer(struct Group ** head_ref, char * name);

// Given the name of the group and a key, this function checks if a certain key is present in the table of the group
// It returns true is the key is found and false otherwise
// This function is called when the function "delete_value" of the app is called
// It is also called int the "put_value" function to determine if we might need to signal the callback function
// And in the function "register_callback" to check is a given key is in the Hashtable of the group
bool FindKeyValueLocalServer(struct Group * head, char * name, char * key);

// Function that checks if a group is present in the Authentification Server
// Returns false in case of group not found and true in case of group found
// Called during the "establish_connection" phase of the app
bool FindGroupAuthServer(char * g_name);

// Function that return whether a group with a certain name has been created or not
// This is used when an application is trying to connect to a certain group
// Returns false if the group name was not found, true otherwise
bool FindGroupLocalServer(struct Group * head, char * name);

// Function that returns the secret for a given group
// Returns NULL if the group is not found or in case of error
char * GetSecretFromAuthServer(char * g_name);

// Given the name of the group and a key, it returns the corresponding value or NULL if the key / group does not exist
// This function is called when the function "get_value" of the app is called
char * GetKeyValueLocalServer(struct Group * head, char * name, char * key);

// Given the name and secrect of the group and file descriptor, pid and file descriptor for the callback of the app
// This functions appens the app to a certain group
// It returns true in case of sucess and false otherwise
// This function is used when an app is trying to connect to a client
bool AddAppToGroup(struct Group * head, char * name, char * secret, int cl_fd, int fd_cb, int pid_in);

// Adds a key to the watchlist of an app with the given pid
// Return true in case of sucess and false otherwise
// Called in the "register_callback" function in the local server
bool AddKeyToWatchList(struct Group * head, char * name, int pid_in, char * key);

// Given the name of the group, the pid of the app and the key-values pair to be inserted
// It returns true in case of success and false otherwise
// This function is called when the function "put_value" of the app is called
bool AddKeyValueToGroup(struct Group * head, char * name, int pid, char * key, char * value);

// Given the name of the group and the pid of the app
// This function closes the corresponding app of the corresponding group
// It returns true in case of success and false otherwise
// This function is called when the function "close_connection" of the app is called
bool CloseApp(struct Group ** head_ref, char * name, int pid);

// Determines if a key in the watchlist of some app in a given group
// It is also called int the "put_value" function to determine if we might need to signal the callback function
bool IsWatchListOfGroup(struct Group * head, char * name, char * key);

// Send a flag (-1) telling all the connected apps in thr group to close connection
// Called when we delete a group
void SendDeleteGroupFlags(struct Group ** head_ref, char * name);

// Given the name of the group and a key, this function deletes the entry in the Hashtable corresponding to a certain key
// It returns true is the key is correctly deleted and false otherwise
// This function is called when the function "delete_value" of the app is called
bool DeleteKeyValue(struct Group * head, char * name, char * key);

// Deletes an entry of group / secret in the Hashtable in the Authentification Server
int DeleteGroupAuthServer(char * g_name);

// Function to be called when the user uses the "Delete Group" command
// Return true if group was deleted or false if the group was not found
bool DeleteGroupLocalServer(struct Group ** head_ref, char * name);

// Same as the "SendDeleteGroupFlags" function but for all the groups
// Used when exiting the server
void SendDeleteAllGroupsFlags(struct Group ** head_ref);

// Same as the "AllAppsFromGroupClosed" function but for all the groups
// Used when exiting the server
bool AllAppsClosed(struct Group ** head_ref);

// Retuns if all the apps from the group are closed
// This is used to wait until all the apps are closed so that we can delete the groups and app lists
// Called when deleting a group
bool AllAppsFromGroupClosed(struct Group ** head_ref, char * name);

// Delete all groups in the Local Server in both the Local and Authentification Server
// Used when exiting the server
int DeleteGroupList(struct Group **  head_ref);

// Function that prints information about all groups
// Used for debugging purposes
void ShowAllGroupsInfo(struct Group * head);

// Function that prints information for a given group
// Called when the user uses the "Show Group Info" command
// Returns false if the group name was not found, true otherwise
bool ShowGroupInfo(struct Group * head, char * name);

// Funtion that prints information for all connected apps
// Function to be called when the user uses the "Show application status" command
void ShowAppStatus(struct Group * head);
