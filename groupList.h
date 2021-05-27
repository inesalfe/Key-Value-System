#include "hash.h"

#define BUF_SIZE 100

// For now, we have the group_name, the secret, a table of keys and values, and array of apps and a pointer to the next element
// After we finish this layer, the secret will be fetched from the autentification server
struct Group {
	char group_name[BUF_SIZE];
	char secret[BUF_SIZE];
	struct HashTable * table;
	struct App * apps;
	struct Group * next;
};

// There are three function that receive a Group ** and I don't know why
// Maybe try to tchnage this to Group *

// Given the name of the group and a key, it returns the corresponding value or NULL if the key / group does not exist
// This function is called when the function "get_value" of the app is called
char * getKeyValue(struct Group * head, char * name, char * key);

// Given the name of the group and a key, this function checks if a certain key is present in the table of the group
// It return true is the key is found and false otherwise
// This function is called when the function "delete_value" of the app is called
bool findKeyValue(struct Group * head, char * name, char * key);

// Given the name of the group and a key, this function deletes the entry in the Hashtable corresponding to a certain key
// It return true is the key is correctly deleted and false otherwise
// This function is called when the function "delete_value" of the app is called
void deleteKeyValue(struct Group * head, char * name, char * key);

// Function that prints information about all groups
// This is still not used
void ShowAllGroupsInfo(struct Group * group);

// Function that prints information for a given group
// Called when the user uses the "Show Group Info" command
// Returns false if the group name was not found, true otherwise
bool ShowGroupInfo(struct Group * head, char * name);

// Function that return whether a group with a certain name has been created or not
// This is used when an application is trying to connect to a certain group
// Returns false if the group name was not found, true otherwise
bool FindGroup(struct Group * head, char * name);

// Funtion that prints information for all connected apps
// Function to be called when the user uses the "Show application status" command
void ShowAppStatus(struct Group * head);

// Function to be called when the user uses the "Create Group" command
// Return the secret for the created group
char * CreateGroup(struct Group ** head_ref, char * name);

// Function to be called when the user uses the "Delete Group" command
bool deleteGroup(struct Group ** head_ref, char * name);

// Given the name of the group and the file descriptor of the app
// This function closes the corresponding app of the corresponding group
// It returns true in case of success and false otherwise
// This function is called when the function "close_connection" of the app is called
bool close_GroupApp(struct Group ** head_ref, char * name, int cl_fd);

// Given the name and secrect of the group and file descriptor and pid of the app
// This functions appens the app to a certain group
// It returns true in case of sucess and false otherwise
// This function is used when an app is trying to connect to a client
bool addApp_toGroup(struct Group * head, char * name, char * secret, int cl_fd, int pid_in);

// Given the name of the group, the file descriptor of the app and the key-values pair to be inserted
// It returns true in case of success and false otherwise
// This function is called when the function "put_value" of the app is called
bool addKeyValue_toGroup(struct Group * head, char * name, int cl_fd, char * key, char * value);

void deleteGroupList(struct Group**  head_ref);