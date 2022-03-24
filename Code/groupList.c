#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <errno.h>
#include "appList.h"
#include "groupList.h"

// Maximum number of key-value pairs in each table
#define SIZE_IN 1000
#define SECRET_LEN 6

extern struct sockaddr_in sv_addr_auth;
extern int sfd_auth;

bool CreateGroupAuthServer(char * g_name, char * secret) {

	char func_str[BUF_SIZE] = "NewGroup";
	int ready_flag = -1;

	if (sendto(sfd_auth, func_str, sizeof(func_str), 0, (struct sockaddr *) &sv_addr_auth, sizeof(struct sockaddr_in)) != sizeof(func_str)) {
		printf("Server: Fatal error in sendto\n");
		printf("The error message is: %s\n", strerror(errno));
		exit(-1);
	}

	if (recvfrom(sfd_auth, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
		printf("Server: Fatal error in recvfrom\n");
		printf("The error message is: %s\n", strerror(errno));
		exit(-1);
	}

	if (ready_flag == -1) {
		printf("Server: Server not ready\n");
		exit(-1);
	}

	if (sendto(sfd_auth, g_name, strlen(g_name), 0, (struct sockaddr *) &sv_addr_auth, sizeof(struct sockaddr_in)) != strlen(g_name)) {
		printf("Server: Fatal error in sendto\n");
		printf("The error message is: %s\n", strerror(errno));
		exit(-1);
	}

	if (recvfrom(sfd_auth, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
		printf("Server: Fatal error in recvfrom\n");
		printf("The error message is: %s\n", strerror(errno));
		exit(-1);
	}

	if (ready_flag == -1) {
		printf("Group name already exists\n");
		return false;
	}

	if (sendto(sfd_auth, secret, sizeof(secret), 0, (struct sockaddr *) &sv_addr_auth, sizeof(struct sockaddr_in)) != sizeof(secret)) {
		printf("Server: Fatal error in sendto\n");
		printf("The error message is: %s\n", strerror(errno));
		exit(-1);
	}

	if (recvfrom(sfd_auth, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
		printf("Server: Fatal error in recvfrom\n");
		printf("The error message is: %s\n", strerror(errno));
		exit(-1);
	}

	if (ready_flag == -1)
		return false;
	else
		return true;
}

char * CreateGroupLocalServer(struct Group ** head_ref, char * name) {

	if(FindGroupLocalServer(* head_ref, name) || FindGroupAuthServer(name)) {
		printf("Group id already exists\n");
		return NULL;
	}

	// Create new node

	struct Group * new_node = (struct Group *) calloc(1, sizeof(struct Group));
	struct Group * last = * head_ref;

	int len = 5;
	char * secret_temp = (char *) calloc (len+1, sizeof(char));
	char charset[] = "0123456789"
										 "abcdefghijklmnopqrstuvwxyz"
										 "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	srand(time(NULL));
	int it = 0;
	while (it < len) {
			size_t index = (double) rand() / RAND_MAX * (sizeof charset - 1);
			secret_temp[it] = charset[index];
			it++;
	}
	secret_temp[len] = '\0';

	strcpy(new_node->group_name, name);

	char secret[BUF_SIZE] = {0};
	strcpy(secret, secret_temp);

	// Create group in Authentification Server

	if(CreateGroupAuthServer(new_node->group_name, secret) == false) {
		return NULL;
	}

	new_node->table = CreateTable(SIZE_IN);
	new_node->apps = NULL;
	new_node->next = NULL;

	// If the head in null, we set the head to the new node

	if (* head_ref == NULL)
	{
		* head_ref = new_node;
		return secret_temp;
	}

	// Advance until the last node

	while (last->next != NULL)
		last = last->next;

	// Set the last as the new node

	last->next = new_node;

	// Return the generated secret

	return secret_temp;
}

bool FindKeyValueLocalServer(struct Group * head, char * name, char * key) {

	struct Group * current = head;
	while (current != NULL)
	{
		// Find group with given name
		if (strcmp(current->group_name, name) == 0) {
			// Check if key belong to the group table
			if (SearchInTable(current->table, key) != NULL)
				return true;
			else
				return false;
		}
		current = current->next;
	}

	return false;
}

bool FindGroupAuthServer(char * g_name) {

	char func_str[BUF_SIZE] = "FindGroup";
	int ready_flag = -1;

	if (sendto(sfd_auth, func_str, sizeof(func_str), 0, (struct sockaddr *) &sv_addr_auth, sizeof(struct sockaddr_in)) != sizeof(func_str)) {
		printf("Server: Fatal error in sendto\n");
		printf("The error message is: %s\n", strerror(errno));
		exit(-1);
	}

	if (recvfrom(sfd_auth, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
		printf("Server: Fatal error in recvfrom\n");
		printf("The error message is: %s\n", strerror(errno));
		exit(-1);
	}

	if (ready_flag == -1) {
		printf("Server: Server not ready\n");
		printf("The error message is: %s\n", strerror(errno));
		exit(-1);
	}

	if (sendto(sfd_auth, g_name, strlen(g_name), 0, (struct sockaddr *) &sv_addr_auth, sizeof(struct sockaddr_in)) != strlen(g_name)) {
		printf("Server: Fatal error in sendto\n");
		printf("The error message is: %s\n", strerror(errno));
		exit(-1);
	}

	if (recvfrom(sfd_auth, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
		printf("Server: Fatal error in recvfrom\n");
		printf("The error message is: %s\n", strerror(errno));
		exit(-1);
	}

	if (ready_flag == -1) {
		return false;
	}
	else {
		return true;
	}
}

bool FindGroupLocalServer(struct Group * head, char * name) {

	struct Group * current = head;
	while (current != NULL)
	{
		if (strcmp(current->group_name, name) == 0) {
			return true;
		}
		current = current->next;
	}
	return false;
}

char * GetSecretFromAuthServer(char * g_name) {

	char func_str[BUF_SIZE] = "GetSecret";
	int ready_flag = -1;

	if (sendto(sfd_auth, func_str, sizeof(func_str), 0, (struct sockaddr *) &sv_addr_auth, sizeof(struct sockaddr_in)) != sizeof(func_str)) {
		printf("Server: Fatal error in sendto\n");
		printf("The error message is: %s\n", strerror(errno));
		exit(-1);
	}

	if (recvfrom(sfd_auth, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
		printf("Server: Fatal error in recvfrom\n");
		printf("The error message is: %s\n", strerror(errno));
		exit(-1);
	}

	if (ready_flag == -1) {
		printf("Server: Server not ready\n");
		exit(-1);
	}

	if (sendto(sfd_auth, g_name, strlen(g_name), 0, (struct sockaddr *) &sv_addr_auth, sizeof(struct sockaddr_in)) != strlen(g_name)) {
		printf("Server: Fatal error in sendto\n");
		printf("The error message is: %s\n", strerror(errno));
		exit(-1);
	}

	if (recvfrom(sfd_auth, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
		printf("Server: Fatal error in recvfrom\n");
		printf("The error message is: %s\n", strerror(errno));
		exit(-1);
	}

	if (ready_flag == -1) {
		printf("Group id doesn't exist\n");
		return NULL;
	}

	if (sendto(sfd_auth, &ready_flag, sizeof(ready_flag), 0, (struct sockaddr *) &sv_addr_auth, sizeof(struct sockaddr_in)) != sizeof(ready_flag)) {
		printf("Server: Fatal error in sendto\n");
		printf("The error message is: %s\n", strerror(errno));
		exit(-1);
	}

	char * secret = (char *)malloc(BUF_SIZE*sizeof(char));

	if (recvfrom(sfd_auth, secret, sizeof(secret), 0, NULL, NULL) == -1) {
		printf("Server: Fatal error in recvfrom\n");
		printf("The error message is: %s\n", strerror(errno));
		exit(-1);
	}

	return secret;
}

char * GetKeyValueLocalServer(struct Group * head, char * name, char * key) {

	struct Group * current = head;
	while (current != NULL)
	{
		if (strcmp(current->group_name, name) == 0) {
			return SearchInTable(current->table, key);
		}
		current = current->next;
	}
	return NULL;
}

bool AddAppToGroup(struct Group * head, char * name, char * secret, int cl_fd, int fd_cb, int pid_in) {

	struct Group * current = head;
	while (current != NULL)
	{
		if (strcmp(current->group_name, name) == 0) {
			char * secret_recv = GetSecretFromAuthServer(current->group_name);
			// Check if the input key is the group key
			if (strcmp(secret_recv, secret) == 0) {
				AppendApp(&current->apps, cl_fd, fd_cb, pid_in);
				free(secret_recv);
				return true;
			}
			else {
				printf("Incorrect secret\n");
				free(secret_recv);
				return false;
			}
		}
		current = current->next;
	}
	return false;
}


bool AddKeyToWatchList(struct Group * head, char * name, int pid_in, char * key) {

	struct Group * current = head;
	while (current != NULL)
	{
		if (strcmp(current->group_name, name) == 0) {
			struct App * curr = current->apps;
			while (curr != NULL) {
				// find app with given pid
				if (curr->pid == pid_in) {
					AddKeyToList(&curr->wlist, key);
					return true;
				}
				curr = curr->next;
			}
			return false;
		}
		current = current->next;
	}
	return false;
}

bool AddKeyValueToGroup(struct Group * head, char * name, int pid, char * key, char * value) {

	struct Group * current = head;
	while (current != NULL)
	{
		if (strcmp(current->group_name, name) == 0) {
			// Check if the app belongs to this group
			if (FindApp(current->apps, pid)) {
				InsertToTable(current->table, key, value);
				return true;
			}
			else {
				printf("App doesn't belong to this group\n");
				return false;
			}
		}
		current = current->next;
	}
	return false;
}

bool CloseApp(struct Group ** head_ref, char * name, int pid) {

	struct Group * current = * head_ref;
	while (current != NULL)
	{
		if (strcmp(current->group_name, name) == 0) {
			return CloseConnection(current->apps, pid);
		}
		current = current->next;
	}
	return false;
}

bool IsWatchListOfGroup(struct Group * head, char * name, char * key) {

	struct Group * current = head;
	while (current != NULL)
	{
		if (strcmp(current->group_name, name) == 0) {
			struct App * curr = current->apps;
			struct App * next;
			while (curr != NULL)
			{
				next = curr->next;
				if(IsWatchList(curr->wlist, key))
					return true;
				curr = next;
			}
			return false;
		}
		current = current->next;
	}

	return false;

}

bool DeleteKeyValue(struct Group * head, char * name, char * key) {

	struct Group * current = head;
	while (current != NULL)
	{
		if (strcmp(current->group_name, name) == 0) {
			struct App * curr = current->apps;
			struct App * next;
			while (curr != NULL)
			{
				next = curr->next;
				if(IsWatchList(curr->wlist, key))
					DeleteFromWatchList(&curr->wlist, key);
				curr = next;
			}
			DeleteFromTable(current->table, key);
			return true;
		}
		current = current->next;
	}

	return false;
}

int DeleteGroupAuthServer(char * g_name) {

	char func_str[BUF_SIZE] = "DeleteGroup";
	int ready_flag = -1;

	if (sendto(sfd_auth, func_str, sizeof(func_str), 0, (struct sockaddr *) &sv_addr_auth, sizeof(struct sockaddr_in)) != sizeof(func_str)) {
		printf("Server: Fatal error in sendto\n");
		printf("The error message is: %s\n", strerror(errno));
		exit(-1);
	}

	if (recvfrom(sfd_auth, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
		printf("Server: Fatal error in recvfrom\n");
		printf("The error message is: %s\n", strerror(errno));
		exit(-1);
	}

	if (ready_flag == -1) {
		printf("Server: Server not ready\n");
		exit(-1);
	}

	if (sendto(sfd_auth, g_name, strlen(g_name), 0, (struct sockaddr *) &sv_addr_auth, sizeof(struct sockaddr_in)) != strlen(g_name)) {
		printf("Server: Fatal error in sendto\n");
		printf("The error message is: %s\n", strerror(errno));
		exit(-1);
	}

	if (recvfrom(sfd_auth, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
		printf("Server: Fatal error in recvfrom\n");
		printf("The error message is: %s\n", strerror(errno));
		exit(-1);
	}

	if (ready_flag == -1) {
		printf("Group id doesn't exist\n");
		return 0;
	}

	return 1;
}

void SendDeleteGroupFlags(struct Group ** head_ref, char * name) {

	int flag = -1;

	struct Group * current = * head_ref;

	while (current != NULL)
	{
		if (strcmp(current->group_name, name) == 0) {
			struct App * curr = current->apps;
			while (curr != NULL)
			{
				if (curr->isClosed == false) {
					if (send(curr->fd_cb, &flag, sizeof(int), 0) != sizeof(int)) {
						printf("Local Server: Error in sending flag\n");
						printf("The error message is: %s\n", strerror(errno));
					}
				}
				curr = curr->next;
			}
			return;
		}
		current = current->next;
	}
	return;

}

bool DeleteGroupLocalServer(struct Group ** head_ref, char * name) {

	if(FindGroupLocalServer(*head_ref, name) == false) {
		printf("Group id doesn't exist\n");
		return false;
	}

	struct Group * temp = * head_ref, * prev;

	// If the group is in the head node, delete it and set the head to next

	if (temp != NULL && (strcmp(temp->group_name, name)==0)) {
		DeleteGroupAuthServer(temp->group_name);
		DeleteAppList(&temp->apps);
		FreeTable(temp->table);
		* head_ref = temp->next;
		free(temp);
		return true;
	}

	// Advance until the find the group

	while (temp != NULL && (strcmp(temp->group_name, name)!=0)) {
		prev = temp;
		temp = temp->next;
	}

	// If the node is null, the group was not found

	if (temp == NULL)
		return false;

	prev->next = temp->next;

	DeleteGroupAuthServer(temp->group_name);
	DeleteAppList(&temp->apps);
	FreeTable(temp->table);
	free(temp);

	return true;
}

void SendDeleteAllGroupsFlags(struct Group ** head_ref) {

	struct Group * current = * head_ref;
	struct Group * next;

	int flag = -1;

	while (current != NULL)
	{
		next = current->next;
		struct App * curr = current->apps;
		while (curr != NULL)
		{
			if (curr->isClosed == false) {
				if (send(curr->fd_cb, &flag, sizeof(int), 0) != sizeof(int)) {
					printf("Local Server: Error in sending flag\n");
					printf("The error message is: %s\n", strerror(errno));
				}
			}
			curr = curr->next;
		}
		current = next;
	}

	return;

}

bool AllAppsClosed(struct Group ** head_ref) {

	struct Group * current = * head_ref;
	struct Group * next;

	while (current != NULL)
	{
		next = current->next;
		struct App * curr = current->apps;
		while (curr != NULL)
		{
			if (curr->isClosed == false) {
				return false;
			}
			curr = curr->next;
		}
		current = next;
	}

	return true;

}

bool AllAppsFromGroupClosed(struct Group ** head_ref, char * name) {

	struct Group * current = * head_ref;

	while (current != NULL)
	{
		if (strcmp(current->group_name, name) == 0) {
			struct App * curr = current->apps;
			while (curr != NULL)
			{
				if (curr->isClosed == false) {
					return false;
				}
				curr = curr->next;
			}
		}
		current = current->next;
	}
	return true;

}

int DeleteGroupList(struct Group ** head_ref) {

	struct Group * current = * head_ref;
	struct Group * next;

	int success_flag = 1;

	while (current != NULL)
	{
		next = current->next;
		if(DeleteGroupAuthServer(current->group_name) == -1)
			success_flag = -1;
		DeleteAppList(&current->apps);
		FreeTable(current->table);
		free(current);
		current = next;
	}

	* head_ref = NULL;

	return success_flag;
}

void ShowAllGroupsInfo(struct Group * head) {

	struct Group * current = head;
	while (current != NULL)
	{
		printf("Name: %s\n", current->group_name);
		char * secret_recv = GetSecretFromAuthServer(current->group_name);
		if (secret_recv != NULL) {
			printf("Secret: %s\n", secret_recv);
		}
		printf("Number of key-value pairs: %d\n", current->table->count);
		PrintTable(current->table);
		current = current->next;
		free(secret_recv);
	}
	return;
}

bool ShowGroupInfo(struct Group * head, char * name) {

	struct Group * current = head;
	while (current != NULL)
	{
		if (strcmp(current->group_name, name) == 0) {
			printf("Name: %s\n", current->group_name);
			char * secret_recv = GetSecretFromAuthServer(current->group_name);
			if (secret_recv != NULL) {
				printf("Secret: %s\n", secret_recv);
			}
			printf("Number of key-value pairs: %d\n", current->table->count);
			free(secret_recv);
			return true;
		}
		current = current->next;
	}
	return false;
}

void ShowAppStatus(struct Group * head) {
	
	struct Group * current = head;
	while (current != NULL)
	{
		printf("%s:\n", current->group_name);
		PrintAppList(current->apps);
		current = current->next;
	}
	return;
}
