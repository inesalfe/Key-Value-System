#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "appList.h"
#include "groupList.h"

// Maximum number of key-value pairs in each table
#define SIZE_IN 1000

extern struct sockaddr_in sv_addr_auth;
extern int sfd_auth;

int CreateGroupAuthServer(char * g_name, char * secret) {

	char func_str[BUF_SIZE] = "NewGroup";
	int ready_flag = -1;

	if (sendto(sfd_auth, func_str, sizeof(func_str), 0, (struct sockaddr *) &sv_addr_auth, sizeof(struct sockaddr_in)) != sizeof(func_str)) {
		printf("Server: Error in sendto\n");
		return -1;
	}

	if (recvfrom(sfd_auth, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
		printf("Error in recvfrom\n");
		return -1;
	}

	if (ready_flag == -1) {
		printf("Server not ready\n");
		return -1;
	}

	if (sendto(sfd_auth, g_name, sizeof(g_name), 0, (struct sockaddr *) &sv_addr_auth, sizeof(struct sockaddr_in)) != sizeof(g_name)) {
		printf("Server: Error in sendto\n");
		return -1;
	}

	if (recvfrom(sfd_auth, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
		printf("Error in recvfrom\n");
		return -1;
	}

	if (ready_flag == -1) {
		printf("Group name already exists\n");
		return -1;
	}

	if (sendto(sfd_auth, secret, sizeof(secret), 0, (struct sockaddr *) &sv_addr_auth, sizeof(struct sockaddr_in)) != sizeof(secret)) {
		printf("Server: Error in sendto\n");
		return -1;
	}

	if (recvfrom(sfd_auth, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
		printf("Error in recvfrom\n");
		return -1;
	}

	return ready_flag;
}

char * CreateGroupLocalServer(struct Group ** head_ref, char * name) {

	if(FindGroupLocalServer(* head_ref, name) || FindGroupAuthServer(name)) {
		printf("Group id already exists\n");
		return NULL;
	}

	struct Group * new_node = (struct Group *) calloc(1, sizeof(struct Group));
	struct Group * last = * head_ref;

	char * secret_temp = "123";
	strcpy(new_node->group_name, name);

	if(CreateGroupAuthServer(new_node->group_name, secret_temp) == -1) {
		return NULL;
	}

	new_node->table = create_table(SIZE_IN);
	new_node->apps = NULL;
	new_node->next = NULL;

	if (* head_ref == NULL)
	{
		* head_ref = new_node;
		return secret_temp;
	}

	while (last->next != NULL)
		last = last->next;

	last->next = new_node;
	return secret_temp;
}

bool FindKeyValueLocalServer(struct Group * head, char * name, char * key) {

	struct Group * current = head;
	while (current != NULL)
	{
		if (strcmp(current->group_name, name) == 0) {
			if (ht_search(current->table, key) != NULL)
				return true;
			else
				return false;
		}
		current = current->next;
	}

	return false;
}

int FindGroupAuthServer(char * g_name) {

	char func_str[BUF_SIZE] = "FindGroup";
	int ready_flag = -1;

	if (sendto(sfd_auth, func_str, sizeof(func_str), 0, (struct sockaddr *) &sv_addr_auth, sizeof(struct sockaddr_in)) != sizeof(func_str)) {
		printf("Server: Error in sendto\n");
		return -1;
	}

	if (recvfrom(sfd_auth, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
		printf("Error in recvfrom\n");
		return -1;
	}

	if (ready_flag == -1) {
		printf("Server not ready\n");
		return -1;
	}

	if (sendto(sfd_auth, g_name, sizeof(g_name), 0, (struct sockaddr *) &sv_addr_auth, sizeof(struct sockaddr_in)) != sizeof(g_name)) {
		printf("Server: Error in sendto\n");
		return -1;
	}

	if (recvfrom(sfd_auth, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
		printf("Error in recvfrom\n");
		return -1;
	}

	if (ready_flag == -1) {
		return 0;
	}
	else {
		return 1;
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
		printf("Server: Error in sendto\n");
		return NULL;
	}

	if (recvfrom(sfd_auth, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
		printf("Error in recvfrom\n");
		return NULL;
	}

	if (ready_flag == -1) {
		printf("Server not ready\n");
		return NULL;
	}

	if (sendto(sfd_auth, g_name, sizeof(g_name), 0, (struct sockaddr *) &sv_addr_auth, sizeof(struct sockaddr_in)) != sizeof(g_name)) {
		printf("Server: Error in sendto\n");
		return NULL;
	}

	if (recvfrom(sfd_auth, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
		printf("Error in recvfrom\n");
		return NULL;
	}

	if (ready_flag == -1) {
		printf("Group name doesn't exist\n");
		return NULL;
	}

	if (sendto(sfd_auth, &ready_flag, sizeof(ready_flag), 0, (struct sockaddr *) &sv_addr_auth, sizeof(struct sockaddr_in)) != sizeof(ready_flag)) {
		printf("Server: Error in sendto\n");
		return NULL;
	}

	char * secret = (char *)malloc(BUF_SIZE*sizeof(char));

	if (recvfrom(sfd_auth, secret, sizeof(secret), 0, NULL, NULL) == -1) {
		printf("Error in recvfrom\n");
		return NULL;
	}

	return secret;
}

char * GetKeyValueLocalServer(struct Group * head, char * name, char * key) {

	struct Group * current = head;
	while (current != NULL)
	{
		if (strcmp(current->group_name, name) == 0) {
			return ht_search(current->table, key);
		}
		current = current->next;
	}
	return NULL;
}

bool AddAppToGroup(struct Group * head, char * name, char * secret, int cl_fd, int pid_in) {
	
	struct Group * current = head;
	while (current != NULL)
	{
		if (strcmp(current->group_name, name) == 0) {
			char * secret_recv = GetSecretFromAuthServer(current->group_name);
			if (strcmp(secret_recv, secret) == 0) {
				AppendApp(&current->apps, cl_fd, pid_in);
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

bool AddKeyValueToGroup(struct Group * head, char * name, int pid, char * key, char * value) {

	struct Group * current = head;
	while (current != NULL)
	{
		if (strcmp(current->group_name, name) == 0) {
			if (FindApp(current->apps, pid)) {
				ht_insert(current->table, key, value);
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

bool DeleteKeyValue(struct Group * head, char * name, char * key) {

	struct Group * current = head;
	while (current != NULL)
	{
		if (strcmp(current->group_name, name) == 0) {
			ht_delete(current->table, key);
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
		printf("Server: Error in sendto\n");
		return -1;
	}

	if (recvfrom(sfd_auth, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
		printf("Error in recvfrom\n");
		return -1;
	}

	if (ready_flag == -1) {
		printf("Server not ready\n");
		return -1;
	}

	if (sendto(sfd_auth, g_name, sizeof(g_name), 0, (struct sockaddr *) &sv_addr_auth, sizeof(struct sockaddr_in)) != sizeof(g_name)) {
		printf("Server: Error in sendto\n");
		return -1;
	}

	if (recvfrom(sfd_auth, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
		printf("Error in recvfrom\n");
		return -1;
	}

	if (ready_flag == -1) {
		printf("Group name doesn't exist\n");
		return 0;
	}

	return 1;
}

bool DeleteGroupLocalServer(struct Group ** head_ref, char * name) {   
	
	if(FindGroupLocalServer(*head_ref, name) == false) {
		printf("Group name doesn't exist\n");
		return false;
	}

	struct Group * temp = * head_ref, * prev;
 
	if (temp != NULL && (strcmp(temp->group_name, name)==0)) {
		DeleteGroupAuthServer(temp->group_name);
		DeleteAppList(&temp->apps);
		free_table(temp->table);
		* head_ref = temp->next;
		free(temp);
		return true;
	}
 
	while (temp != NULL && (strcmp(temp->group_name, name)!=0)) {
		DeleteGroupAuthServer(temp->group_name);
		DeleteAppList(&temp->apps);
		free_table(temp->table);
		prev = temp;
		temp = temp->next;
	}
 
	if (temp == NULL)
		return true;
 
	prev->next = temp->next;
 
	free(temp);

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
		free_table(current->table);
		free(current);
		current = next;
	}
	
	* head_ref = NULL;

	return success_flag;
}

void CloseAllFileDesc(struct Group**  head_ref) {

	struct Group * current = * head_ref;
	struct Group * next;
 
	int success_flag = 1;

	while (current != NULL)
	{
		next = current->next;
		CloseFileDesc(&current->apps);
		current = next;
	}

	return;

}

void ShowAllGroupsInfo(struct Group * group) {
	while (group != NULL)
	{	
		printf("Name: %s\n", group->group_name);
		char * secret_recv = GetSecretFromAuthServer(group->group_name);
		if (secret_recv != NULL) {
			printf("Secret: %s\n", secret_recv);
		}
		printf("Number of key-value pairs: %d\n", group->table->count);
		group = group->next;
		free(secret_recv);
	}
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

void ShowAppStatus(struct Group * group) {
	while (group != NULL)
	{   
		printf("%s:\n", group->group_name);
		PrintAppList(group->apps);
		group = group->next;
	}
}