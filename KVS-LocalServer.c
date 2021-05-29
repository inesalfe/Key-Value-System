#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include "groupList.h"

#define SV_SOCK_PATH "/tmp/server_sock"
// Maximum size for the secret and group name
#define BUF_SIZE 100
#define BACKLOG 5

struct cl_info {
	int file_descriptor;
	long cl_pid;
};

// Linked List of groups
struct Group * groups;
// Server of Authentification Server
struct sockaddr_in sv_addr_auth;
// File descriptor for the connection to the AuthServer
int sfd_auth;

int put_value (char * group_name, int * app_fd) {

	char temp_key[BUF_SIZE];
	char temp_value[BUF_SIZE];
	int ready = 1;
	ssize_t numBytes;
	if (send(*app_fd, &ready, sizeof(int), 0) != sizeof(int)) {
		printf("Local Server: Error in sending ready flag\n");
		return -1;
	}
	numBytes = recv(*app_fd, temp_key, sizeof(temp_key), 0);
	if (numBytes == -1) {
		printf("Local Server: Error in reading key\n");
		return -1;
	}
	if (send(*app_fd, &ready, sizeof(int), 0) != sizeof(int)) {
		printf("Local Server: Error in sending ready flag\n");
		return -1;
	}
	numBytes = recv(*app_fd, temp_value, sizeof(temp_value), 0);
	if (numBytes == -1) {
		printf("Local Server: Error in reading value\n");
		return -1;
	}
	if (addKeyValue_toGroup(groups, group_name, *app_fd, temp_key, temp_value))
		return 1;
	else
		return 0;
}

int get_value (char * group_name, int * app_fd) {

	char temp_key[BUF_SIZE];
	char temp_value[BUF_SIZE];
	int ready = 1;
	int length = -1;
	ssize_t numBytes;
	if (send(*app_fd, &ready, sizeof(int), 0) != sizeof(int)) {
		printf("Local Server: Error in sending ready flag\n");
		return -1;
	}
	numBytes = recv(*app_fd, temp_key, sizeof(temp_key), 0);
	if (numBytes == -1) {
		printf("Local Server: Error in reading key\n");
		return -1;
	}
	char * temp_value = getKeyValue(groups, group_name, temp_key);
	if (temp_value != NULL) {
		length = strlen(temp_value);
	}
	if (send(*app_fd, &length, sizeof(int), 0) != sizeof(int)) {
		printf("Local Server: Error in sending length\n");
		return -1;
	}
	if (length == -1)
		return -1;
	if (send(*app_fd, temp_value, (length+1)*sizeof(char), 0) != (length+1)*sizeof(char)) {
		printf("Local Server: Error in sending value pointer\n");
		return -1;
	}
	return 1;
}

int delete_value (char * group_name, int * app_fd) {

	char temp_key[BUF_SIZE];
	char temp_value[BUF_SIZE];
	int ready = 1;
	int check_key = -1;
	ssize_t numBytes;
	if (send(*app_fd, &ready, sizeof(int), 0) != sizeof(int)) {
		printf("Local Server: Error in sending ready flag\n");
		return -1;
	}
	numBytes = recv(*app_fd, temp_key, sizeof(temp_key), 0);
	if (numBytes == -1) {
		printf("Local Server: Error in reading key\n");
		return -1;
	}
	if (findKeyValue(groups, group_name, temp_key))
		check_key = 1;
	if (send(*app_fd, &check_key, sizeof(int), 0) != sizeof(int)) {
		printf("Local Server: Error in sending check_key\n");
		return -1;
	}
	if (check_key == -1)
		return -1;
	deleteKeyValue(groups, group_name, temp_key);
	return 1;
}

int register_callback (int * app_fd) {
	return 0;
}

void * thread_func(void * arg) {

	struct cl_info * info = arg;
	int cfd = info->file_descriptor;
	int pid = info->cl_pid;

	// Group_id received by the app
	char group_id_app[BUF_SIZE];
	// Secret received by the app
	char secret_app[BUF_SIZE];

	// Definition of app file descriptor
	ssize_t numBytes;
	int error_flag = 1;
	bool flag;

	// Sending flag saying that connection was established
	if (send(cfd, &error_flag, sizeof(int), 0) != sizeof(int)) {
		printf("Local Server: Error in sending flag for established connection\n");
		if (close(cfd) == -1) {
			printf("Local Server: Error in closing socket file descriptor\n");
		}
		pthread_exit(NULL);
	}

	// Reusing the error flag to check the group_id
	error_flag = -1;

	// Reading group_id
	numBytes = recv(cfd, group_id_app, sizeof(group_id_app), 0);

	// Error in reading group_id
	if (numBytes == -1) {
		printf("Local Server: Error in reading group_id\n");
		if (close(cfd) == -1) {
			printf("Local Server: Error in closing socket file descriptor\n");
		}
		pthread_exit(NULL);        
	}

	// Check if group_id is correct
	flag = FindGroup(groups, group_id_app);
	if (flag == true)
		error_flag = 1;
	else
		error_flag = 0;

	// Sending flag saying if group_id is correct or not
	if (send(cfd, &error_flag, sizeof(int), 0) != sizeof(int)) {
		printf("Local Server: Error in sending flag for correct/incorrect group_id\n");
		if (close(cfd) == -1) {
			printf("Local Server: Error in closing socket file descriptor\n");
		}
		pthread_exit(NULL);
	}

	// If the group_id is incorrect we exit the thread
	if (error_flag == 0)
		pthread_exit(NULL);

	// Reusing the error flag to check the secret
	error_flag = -1;

	// Reading secret
	numBytes = recv(cfd, secret_app, sizeof(secret_app), 0);

	// Errot in reading secret
	if (numBytes == -1) {
		printf("Local Server: Error in reading secret\n");
		if (close(cfd) == -1) {
			printf("Local Server: Error in closing socket file descriptor\n");
		}
		pthread_exit(NULL);        
	}

	// Check if secret is correct
	flag = addApp_toGroup(groups, group_id_app, secret_app, cfd, pid);
	if (flag == true)
		error_flag = 1;
	else
		error_flag = 0;

	// Sending flag saying if secret is correct or not
	if (send(cfd, &error_flag, sizeof(int), 0) != sizeof(int)) {
		printf("Local Server: Error in sending flag for correct/incorrect secret\n");
		if (close(cfd) == -1) {
			printf("Local Server: Error in closing socket file descriptor\n");
		}
		pthread_exit(NULL);
	}

	// End of establish connection

	// This flag is sent to the app after each function indicating sucess
	int sucess_flag;
	// Variable with a code saying which function the app wants to execute
	int func_code = -1;
	while(1) {
		// printf("Before receive func_code\n");
		numBytes = recv(cfd, &func_code, sizeof(int), 0);
		// printf("After receive func_code\n");
		if (numBytes < 0)
			break;
		sucess_flag = -1;
		if (func_code == 0) {
			sucess_flag = put_value(group_id_app, &cfd);
			if (sucess_flag == -1)
				printf("Error in 'put_value' operation\n");
			else
				printf("Successful 'put_value' operation\n");
		}
		else if (func_code == 1) {
			sucess_flag = get_value(group_id_app, &cfd);
			if (sucess_flag == -1)
				printf("Error in 'get_value' operation\n");
			else
				printf("Successful 'get_value' operation\n");
		}
		else if (func_code == 2) {
			sucess_flag = delete_value(group_id_app, &cfd);
			if (sucess_flag == -1)
				printf("Error in 'delete_value' operation\n");
			else
				printf("Successful 'delete_value' operation\n");
		}
		else if (func_code == 3)
			register_callback(&cfd);
		else {
			if (close_GroupApp(&groups, group_id_app, cfd))
				sucess_flag = 1;
			if (sucess_flag == -1)
				printf("Error in 'close_connection' operation\n");
			else
				printf("Successful 'close_connection' operation\n");
			break;
		}
	}

	pthread_exit(NULL);
}

void * get_cmd_func(void * arg) {

	char str[BUF_SIZE];
	char g_name[BUF_SIZE];
	bool flag;
	size_t len;
	while (1) {
		fgets(str, sizeof(str), stdin);
		if (strcmp(str, "Create group\n") == 0) {
			printf("Insert group id:\n");
			fgets(g_name, sizeof(g_name), stdin);
			len = strlen(g_name);
			if (len > 0 && g_name[len-1] == '\n') {
			  g_name[--len] = '\0';
			}
			if (CreateGroup(&groups, g_name) == NULL)
				printf("That group already exists!\n");
			else
				printf("Group created!\n");
		}
		else if (strcmp(str, "Delete group\n") == 0) {
			printf("Insert group id:\n");
			fgets(g_name, sizeof(g_name), stdin);
			len = strlen(g_name);
			if (len > 0 && g_name[len-1] == '\n') {
			  g_name[--len] = '\0';
			}
			if (deleteGroup(&groups, g_name))
				printf("Group deleted!\n");
			else
				printf("Group not found!\n");
		}
		else if (strcmp(str, "Show group info\n") == 0) {
			printf("Insert group id:\n");
			fgets(g_name, sizeof(g_name), stdin);
			len = strlen(g_name);
			if (len > 0 && g_name[len-1] == '\n') {
			  g_name[--len] = '\0';
			}
			flag = ShowGroupInfo(groups, g_name);
			if (flag == false)
				printf("Group not found!\n");
		}
		else if (strcmp(str, "Show application status\n") == 0) {
			ShowAppStatus(groups);
		}
		else if (strcmp(str, "q\n") == 0) {
			printf("Exiting server...\n");
			break;
		}
		else
			printf("Unknown command\n");
	}

	pthread_exit(NULL);
}

int main(int argc, char *argv[]) {

	// Definition of the socket address and file descriptor
	struct sockaddr_un sv_addr;
	int sfd;

	sfd_auth = socket(AF_INET, SOCK_DGRAM, 0);
	if (sfd_auth == -1) {
		printf("Local Server: Error in socket creation\n");
		exit(-1);
	}

	sv_addr_auth.sin_family = AF_INET;
	sv_addr_auth.sin_port = htons(58032);

	// File descriptor assignment
	sfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sfd == -1)
		printf("Local Server: Error in socket creation\n");

	// Clean socket path
	remove(SV_SOCK_PATH);

	// Bind
	sv_addr.sun_family = AF_UNIX;
	strncpy(sv_addr.sun_path, SV_SOCK_PATH, sizeof(sv_addr.sun_path) - 1);

	if (bind(sfd, (struct sockaddr *) &sv_addr, sizeof(struct sockaddr_un)) == -1)
		printf("Local Server: Error in binding\n");

	// Listen
	if (listen(sfd, BACKLOG) == 1)
		printf("Local Server: Error in listening\n");

	struct sockaddr_un app_addr;
	socklen_t len = sizeof(struct sockaddr_un);

	groups = NULL;

	// Creation of thread that will execute the commands
	pthread_t t_id_cmd;
	pthread_create(&t_id_cmd, NULL, get_cmd_func, NULL);

	// Creation of threads that will handle the apps
	for (;;) {
		struct cl_info temp_info;
		temp_info.file_descriptor = accept(sfd, (struct sockaddr *) &app_addr, &len);
		temp_info.cl_pid = atol(app_addr.sun_path + strlen("/tmp/app_socket_"));
		if (temp_info.file_descriptor == -1)
			printf("Local Server: Error in accepting\n");
		pthread_t t_id;
		pthread_create(&t_id, NULL, thread_func, &temp_info);
	}

	if (close(sfd_auth) == -1) {
		printf("Local Error in closing socket\n");
		exit(-1);
	}

	remove(sv_addr.sun_path);

	return 0;

}