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

char * getSecret(char * g_name) {

	char func_str[BUF_SIZE] = "GetSecret";
	int ready_flag = -1;

	if (sendto(sfd_auth, func_str, sizeof(func_str), 0, (struct sockaddr *) &sv_addr_auth, sizeof(struct sockaddr_in)) != sizeof(func_str)) {
		printf("Server: Error in sendto\n");
		exit(-1);
	}

	if (recvfrom(sfd_auth, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
		printf("Error in recvfrom\n");
		exit(-1);
	}

	if (ready_flag == -1) {
		printf("Server not ready\n");
		return NULL;
	}

	if (sendto(sfd_auth, g_name, sizeof(g_name), 0, (struct sockaddr *) &sv_addr_auth, sizeof(struct sockaddr_in)) != sizeof(g_name)) {
		printf("Server: Error in sendto\n");
		exit(-1);
	}

	if (recvfrom(sfd_auth, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
		printf("Error in recvfrom\n");
		exit(-1);
	}

	if (ready_flag == -1) {
		printf("Group name doesn't exist\n");
		return NULL;
	}

	if (sendto(sfd_auth, &ready_flag, sizeof(ready_flag), 0, (struct sockaddr *) &sv_addr_auth, sizeof(struct sockaddr_in)) != sizeof(ready_flag)) {
		printf("Server: Error in sendto\n");
		exit(-1);
	}

	char * secret = (char *)malloc(BUF_SIZE*sizeof(char));

	if (recvfrom(sfd_auth, secret, sizeof(secret), 0, NULL, NULL) == -1) {
		printf("Error in recvfrom\n");
		exit(-1);
	}

	return secret;
}

bool findGroupAuthServer(char * g_name) {

	char func_str[BUF_SIZE] = "FindGroup";
	int ready_flag = -1;

	if (sendto(sfd_auth, func_str, sizeof(func_str), 0, (struct sockaddr *) &sv_addr_auth, sizeof(struct sockaddr_in)) != sizeof(func_str)) {
		printf("Server: Error in sendto\n");
		exit(-1);
	}

	if (recvfrom(sfd_auth, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
		printf("Error in recvfrom\n");
		exit(-1);
	}

	if (ready_flag == -1) {
		printf("Server not ready\n");
		return false;
	}

	if (sendto(sfd_auth, g_name, sizeof(g_name), 0, (struct sockaddr *) &sv_addr_auth, sizeof(struct sockaddr_in)) != sizeof(g_name)) {
		printf("Server: Error in sendto\n");
		exit(-1);
	}

	if (recvfrom(sfd_auth, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
		printf("Error in recvfrom\n");
		exit(-1);
	}

	if (ready_flag == -1) {
		printf("Group name doesn't exist\n");
		return false;
	}
	else {
		printf("Group found\n");
		return true;
	}
}

void ShowAllGroupsInfo(struct Group * group)
{
	while (group != NULL)
	{	
		printf("Name: %s\n", group->group_name);
		char * secret_recv = (char *)malloc(BUF_SIZE*sizeof(char));
		if (getSecret(group->group_name) != NULL) {
			strncpy(secret_recv, getSecret(group->group_name), sizeof(getSecret(group->group_name)));
			printf("Secret: %s\n", secret_recv);
		}
		printf("Number of key-value pairs: %d\n", group->table->count);
		group = group->next;
	}
}

bool ShowGroupInfo(struct Group * head, char * name)
{
	struct Group * current = head;
	while (current != NULL)
	{
		if (strcmp(current->group_name, name) == 0) {
			printf("Name: %s\n", current->group_name);
			char * secret_recv = (char *)malloc(BUF_SIZE*sizeof(char));
			if (getSecret(current->group_name) != NULL) {
				strncpy(secret_recv, getSecret(current->group_name), sizeof(getSecret(current->group_name)));
				printf("Secret: %s\n", secret_recv);
			}
			printf("Number of key-value pairs: %d\n", current->table->count);
			return true;
		}
		current = current->next;
	}
	return false;
}

void ShowAppStatus(struct Group * group)
{
	while (group != NULL)
	{   
		printf("%s:\n", group->group_name);
		print_AppList(group->apps);
		group = group->next;
	}
}

void createGroupAuthServer(char * g_name, char * secret) {

	char func_str[BUF_SIZE] = "NewGroup";
	int ready_flag = -1;

	if (sendto(sfd_auth, func_str, sizeof(func_str), 0, (struct sockaddr *) &sv_addr_auth, sizeof(struct sockaddr_in)) != sizeof(func_str)) {
		printf("Server: Error in sendto\n");
		exit(-1);
	}

	if (recvfrom(sfd_auth, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
		printf("Error in recvfrom\n");
		exit(-1);
	}

	if (ready_flag == -1) {
		printf("Server not ready\n");
		return;
	}

	if (sendto(sfd_auth, g_name, sizeof(g_name), 0, (struct sockaddr *) &sv_addr_auth, sizeof(struct sockaddr_in)) != sizeof(g_name)) {
		printf("Server: Error in sendto\n");
		exit(-1);
	}

	if (recvfrom(sfd_auth, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
		printf("Error in recvfrom\n");
		exit(-1);
	}

	if (ready_flag == -1) {
		printf("Group name already exists\n");
		return;
	}

	if (sendto(sfd_auth, secret, sizeof(secret), 0, (struct sockaddr *) &sv_addr_auth, sizeof(struct sockaddr_in)) != sizeof(secret)) {
		printf("Server: Error in sendto\n");
		exit(-1);
	}

	if (recvfrom(sfd_auth, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
		printf("Error in recvfrom\n");
		exit(-1);
	}

	if (ready_flag == -1) {
		printf("Error in create new group\n");
		return;
	}
	else {
		printf("Group created\n");
	}

	return;
}

char * CreateGroup(struct Group ** head_ref, char * name) {

	if(FindGroup(*head_ref, name) || findGroupAuthServer(name)) {
		return NULL;
	}

	struct Group * new_node = (struct Group *) malloc(sizeof(struct Group));
	struct Group * last = * head_ref;

	char * secret_temp = "123";
	strcpy(new_node->group_name, name);

	createGroupAuthServer(new_node->group_name, secret_temp);

	new_node->table = create_table(SIZE_IN);
	new_node->apps = NULL;
	new_node->next = NULL;

	if (* head_ref == NULL)
	{
	   *head_ref = new_node;
	   return secret_temp;
	}

	while (last->next != NULL)
		last = last->next;

	last->next = new_node;
	return secret_temp;

}

char * getKeyValue(struct Group * head, char * name, char * key) {

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

bool findKeyValue(struct Group * head, char * name, char * key) {

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

void deleteKeyValue(struct Group * head, char * name, char * key) {

	struct Group * current = head;
	while (current != NULL)
	{
		if (strcmp(current->group_name, name) == 0) {
			ht_delete(current->table, key);
			return;
		}
		current = current->next;
	}

	return;
}


bool addKeyValue_toGroup(struct Group * head, char * name, int cl_fd, char * key, char * value) {

	struct Group * current = head;
	while (current != NULL)
	{
		if (strcmp(current->group_name, name) == 0) {
			if (FindApp(current->apps, cl_fd)) {
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

bool FindGroup(struct Group * head, char * name) {

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

bool close_GroupApp(struct Group ** head_ref, char * name, int cl_fd) {

	struct Group * current = * head_ref;
	while (current != NULL)
	{
		if (strcmp(current->group_name, name) == 0) {
			return close_App(current->apps, cl_fd);
		}
		current = current->next;
	}
	return false;

}

bool addApp_toGroup(struct Group * head, char * name, char * secret, int cl_fd, int pid_in) {
	
	struct Group * current = head;
	while (current != NULL)
	{
		if (strcmp(current->group_name, name) == 0) {
			if (strcmp(getSecret(current->group_name), secret) == 0) {
				append_App(&current->apps, cl_fd, pid_in);
				return true;                
			}
			else
				return false;
		}
		current = current->next;
	}
	return false;
}

void deleteGroupAuthServer(char * g_name) {

	char func_str[BUF_SIZE] = "DeleteGroup";
	int ready_flag = -1;

	if (sendto(sfd_auth, func_str, sizeof(func_str), 0, (struct sockaddr *) &sv_addr_auth, sizeof(struct sockaddr_in)) != sizeof(func_str)) {
		printf("Server: Error in sendto\n");
		exit(-1);
	}

	if (recvfrom(sfd_auth, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
		printf("Error in recvfrom\n");
		exit(-1);
	}

	if (ready_flag == -1) {
		printf("Server not ready\n");
		return;
	}

	if (sendto(sfd_auth, g_name, sizeof(g_name), 0, (struct sockaddr *) &sv_addr_auth, sizeof(struct sockaddr_in)) != sizeof(g_name)) {
		printf("Server: Error in sendto\n");
		exit(-1);
	}

	if (recvfrom(sfd_auth, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
		printf("Error in recvfrom\n");
		exit(-1);
	}

	if (ready_flag == -1) {
		printf("Group name doesn't exist\n");
	}
	else {
		printf("Group deleted\n");
	}

	return;

}

bool deleteGroup(struct Group ** head_ref, char * name)
{   
	if(FindGroup(*head_ref, name) == false) {
		return false;
	}

	struct Group * temp = * head_ref, * prev;
 
	if (temp != NULL && (strcmp(temp->group_name, name)==0)) {
		deleteGroupAuthServer(temp->group_name);
		* head_ref = temp->next;
		free(temp);
		return true;
	}
 
	while (temp != NULL && (strcmp(temp->group_name, name)!=0)) {
		deleteGroupAuthServer(temp->group_name);
		prev = temp;
		temp = temp->next;
	}
 
	if (temp == NULL)
		return true;
 
	prev->next = temp->next;
 
	free(temp);

	return true;
}

void deleteGroupList(struct Group ** head_ref) {

   struct Group * current = * head_ref;
   struct Group * next;
 
   while (current != NULL)
   {
	   next = current->next;
	   deleteGroupAuthServer(current->group_name);
	   deleteAppList(&current->apps);
	   free_table(current->table);
	   free(current);
	   current = next;
   }
   
   * head_ref = NULL;
}



