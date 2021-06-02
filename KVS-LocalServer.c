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
#include <signal.h>
#include <errno.h>
#include "groupList.h"

#define SV_SOCK_PATH "/tmp/server_sock"
#define SV_SOCK_PATH_CB "/tmp/server_sock_cb"
// Maximum size for the secret and group name
#define BUF_SIZE 100
#define BACKLOG 20

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

struct cl_info {
	int file_descriptor;
	long cl_pid;
};

// Linked List of groups
struct Group * groups = NULL;
// Server of Authentification Server
struct sockaddr_in sv_addr_auth;
// File descriptor for the connection to the AuthServer
int sfd_auth;

int put_value (char * group_name, int * app_fd, int * fd_callback, int * pid) {

	char temp_key[BUF_SIZE] = {0};
	char temp_value[BUF_SIZE] = {0};
	int ready = 1;
	ssize_t numBytes;
	if (send(*app_fd, &ready, sizeof(int), 0) != sizeof(int)) {
		printf("Local Server: Error in sending ready flag\n");
		return -2;
	}
	numBytes = recv(*app_fd, temp_key, sizeof(temp_key), 0);
	if (numBytes == -1) {
		printf("Local Server: Error in reading key\n");
		return -2;
	}
	if (send(*app_fd, &ready, sizeof(int), 0) != sizeof(int)) {
		printf("Local Server: Error in sending ready flag\n");
		return -2;
	}
	numBytes = recv(*app_fd, temp_value, sizeof(temp_value), 0);
	if (numBytes == -1) {
		printf("Local Server: Error in reading value\n");
		return -2;
	}
	if (FindKeyValueLocalServer(groups, group_name, temp_key) && IsWatchListOfGroup(groups, group_name, temp_key)) {
		int flag = 1;
		if (send(*fd_callback, &flag, sizeof(int), 0) != sizeof(int)) {
			printf("Local Server: Error in sending flag\n");
			return -2;
		}
		flag = -1;
		numBytes = recv(*fd_callback, &flag, sizeof(int), 0);
		if (numBytes == -1) {
			printf("Local Server: Error in reading flag\n");
			return -2;
		}
		if (flag == -1) {
			printf("Local Server: Error in receiving ready flag");
			return -2;
		}
		if (send(*fd_callback, temp_key, sizeof(temp_key), 0) != sizeof(temp_key)) {
			printf("Local Server: Error in sending changed key\n");
			return -2;
		}
	}
	if (AddKeyValueToGroup(groups, group_name, *pid, temp_key, temp_value))
		return 1;
	else
		return 0;
}

int get_value (char * group_name, int * app_fd) {

	char temp_key[BUF_SIZE] = {0};
	int ready = 1;
	int length = -1;
	ssize_t numBytes;
	if (send(*app_fd, &ready, sizeof(int), 0) != sizeof(int)) {
		printf("Local Server: Error in sending ready flag\n");
		return -2;
	}
	numBytes = recv(*app_fd, temp_key, sizeof(temp_key), 0);
	if (numBytes == -1) {
		printf("Local Server: Error in reading key\n");
		return -2;
	}
	char * temp_value = GetKeyValueLocalServer(groups, group_name, temp_key);
	if (temp_value != NULL) {
		length = strlen(temp_value);
	}
	if (send(*app_fd, &length, sizeof(int), 0) != sizeof(int)) {
		printf("Local Server: Error in sending length\n");
		return -2;
	}
	if (length == -1)
		return -1;
	if (send(*app_fd, temp_value, (length+1)*sizeof(char), 0) != (length+1)*sizeof(char)) {
		printf("Local Server: Error in sending value pointer\n");
		return -2;
	}
	return 1;
}

int delete_value (char * group_name, int * app_fd) {

	char temp_key[BUF_SIZE] = {0};
	int ready = 1;
	int check_key = -1;
	ssize_t numBytes;
	if (send(*app_fd, &ready, sizeof(int), 0) != sizeof(int)) {
		printf("Local Server: Error in sending ready flag\n");
		return -2;
	}
	numBytes = recv(*app_fd, temp_key, sizeof(temp_key), 0);
	if (numBytes == -1) {
		printf("Local Server: Error in reading key\n");
		return -2;
	}
	if (FindKeyValueLocalServer(groups, group_name, temp_key))
		check_key = 1;
	if (send(*app_fd, &check_key, sizeof(int), 0) != sizeof(int)) {
		printf("Local Server: Error in sending check_key\n");
		return -2;
	}
	if (check_key == -1)
		return -1;
	if (DeleteKeyValue(groups, group_name, temp_key) == false)
		return -1;
	return 1;
}

int register_callback (char * group_name, int * pid, int * app_fd) {

	char temp_key[BUF_SIZE] = {0};
	int ready = 1;
	int check_key = -1;
	ssize_t numBytes;
	if (send(*app_fd, &ready, sizeof(int), 0) != sizeof(int)) {
		printf("Local Server: Error in sending ready flag\n");
		return -2;
	}
	numBytes = recv(*app_fd, temp_key, sizeof(temp_key), 0);
	if (numBytes == -1) {
		printf("Local Server: Error in reading key\n");
		return -2;
	}
	if (FindKeyValueLocalServer(groups, group_name, temp_key))
		check_key = 1;
	if (send(*app_fd, &check_key, sizeof(int), 0) != sizeof(int)) {
		printf("Local Server: Error in sending check_key\n");
		return -2;
	}
	if (check_key == -1)
		return -1;
	if (AddKeyToWatchList(groups, group_name, *pid, temp_key) == false)
		return -1;
	return 1;
}

void * thread_func(void * arg) {

	struct cl_info * info = arg;
	int cfd = info->file_descriptor;
	int pid = info->cl_pid;

	// Group_id received by the app
	char group_id_app[BUF_SIZE] = {0};
	// Secret received by the app
	char secret_app[BUF_SIZE] = {0};

	// Definition of app file descriptor
	ssize_t numBytes;
	int error_flag = 1;
	bool flag;

	if (pthread_detach(pthread_self()) != 0) {
			printf("Local Server: Error in 'pthread_detach'\n");
	}

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
	pthread_mutex_lock(&mtx);
	int flag_int = FindGroupAuthServer(group_id_app);
	if (flag_int == -1 || flag_int == 0)
		error_flag = 0;
	else
		error_flag = 1;
	pthread_mutex_unlock(&mtx);

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

	printf("First connection established\n");

	// File descriptor assignment
	int sfd_callback = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sfd_callback == -1) {
		printf("Local Server: Error in socket creation\n");
		pthread_exit(NULL);
	}

	struct sockaddr_un sv_addr_cb;
	memset(&sv_addr_cb, 0, sizeof(struct sockaddr_un));

	// Clean socket path
	remove(SV_SOCK_PATH_CB);

	// Bind
	sv_addr_cb.sun_family = AF_UNIX;
	strncpy(sv_addr_cb.sun_path, SV_SOCK_PATH_CB, sizeof(sv_addr_cb.sun_path) - 1);

	if (bind(sfd_callback, (struct sockaddr *) &sv_addr_cb, sizeof(struct sockaddr_un)) == -1) {
		printf("Local Server: Error in binding\n");
		pthread_exit(NULL);
	}

	printf("After bind\n");

	// Listen
	if (listen(sfd_callback, BACKLOG) == - 1) {
		printf("Local Server: Error in listening\n");
		pthread_exit(NULL);
	}

	printf("After listen\n");

	struct sockaddr_un app_addr;
	memset(&app_addr, 0, sizeof(struct sockaddr_un));
	socklen_t len = sizeof(struct sockaddr_un);

	int fd_cb = accept(sfd_callback, (struct sockaddr *) &app_addr, &len);
	if (fd_cb == -1) {
		printf("Local Server: Error in accepting\n");
		pthread_exit(NULL);
	}

	printf("Connection accepted\n");

	error_flag = 1;

	// Sending flag saying that connection was established
	if (send(fd_cb, &error_flag, sizeof(int), 0) != sizeof(int)) {
		printf("Local Server: Error in sending flag for established connection\n");
		if (close(cfd) == -1) {
			printf("Local Server: Error in closing socket file descriptor\n");
		}
		pthread_exit(NULL);
	}

	// Check if secret is correct
	pthread_mutex_lock(&mtx);
	flag = AddAppToGroup(groups, group_id_app, secret_app, cfd, fd_cb, pid);
	if (flag == true)
		error_flag = 1;
	else
		error_flag = 0;
	pthread_mutex_unlock(&mtx);

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
		numBytes = recv(cfd, &func_code, sizeof(int), 0);
		if (numBytes < 0)
			break;
		sucess_flag = -1;
		if (func_code == 0) {
			pthread_mutex_lock(&mtx);
			sucess_flag = put_value(group_id_app, &cfd, &fd_cb, &pid);
			if (sucess_flag == -2) {
				printf("Fatal communication error in 'put_value' operation\n");
				pthread_exit(NULL);
			}
			else if (sucess_flag == -1)
				printf("Error in 'put_value' operation\n");
			else
				printf("Successful 'put_value' operation\n");
			pthread_mutex_unlock(&mtx);
		}
		else if (func_code == 1) {
			pthread_mutex_lock(&mtx);
			sucess_flag = get_value(group_id_app, &cfd);
			if (sucess_flag == -2) {
				printf("Fatal communication error in 'get_value' operation\n");
				pthread_exit(NULL);
			}
			else if (sucess_flag == -1)
				printf("Error in 'get_value' operation\n");
			else
				printf("Successful 'get_value' operation\n");
			pthread_mutex_unlock(&mtx);
		}
		else if (func_code == 2) {
			pthread_mutex_lock(&mtx);
			sucess_flag = delete_value(group_id_app, &cfd);
			if (sucess_flag == -2) {
				printf("Fatal communication error in 'delete_value' operation\n");
				pthread_exit(NULL);
			}
			else if (sucess_flag == -1)
				printf("Error in 'delete_value' operation\n");
			else
				printf("Successful 'delete_value' operation\n");
			pthread_mutex_unlock(&mtx);
		}
		else if (func_code == 3) {
			pthread_mutex_lock(&mtx);
			sucess_flag = register_callback(group_id_app, &pid, &cfd);
			if (sucess_flag == -2) {
				printf("Fatal communication error in 'register_callback' operation\n");
				pthread_exit(NULL);
			}
			else if (sucess_flag == -1)
				printf("Error in 'register_callback' operation\n");
			else
				printf("Successful 'register_callback' operation\n");
			pthread_mutex_unlock(&mtx);
		}
		else if (func_code == 4) {
			pthread_mutex_lock(&mtx);
			if (CloseApp(&groups, group_id_app, pid))
				sucess_flag = 1;
			if (sucess_flag == -2) {
				printf("Fatal communication error in 'close_connection' operation\n");
				pthread_exit(NULL);
			}
			else if (sucess_flag == -1)
				printf("Error in 'close_connection' operation\n");
			else
				printf("Successful 'close_connection' operation\n");
			pthread_mutex_unlock(&mtx);
			break;
		}
		else
			printf("Unknown command\n");
	}

	pthread_exit(NULL);
}

// Definition of the socket address and file descriptor
struct sockaddr_un sv_addr;
int sfd_main;

void * handle_apps(void * arg) {

	if (pthread_detach(pthread_self()) != 0) {
		printf("Local Server: Error in 'pthread_detach'\n");
		pthread_exit(NULL);
	}

	// File descriptor assignment
	sfd_main = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sfd_main == -1) {
		printf("Local Server: Error in socket creation\n");
		pthread_exit(NULL);
	}

	// Clean socket path
	remove(SV_SOCK_PATH);

	// Bind
	sv_addr.sun_family = AF_UNIX;
	strncpy(sv_addr.sun_path, SV_SOCK_PATH, sizeof(sv_addr.sun_path) - 1);

	if (bind(sfd_main, (struct sockaddr *) &sv_addr, sizeof(struct sockaddr_un)) == -1) {
		printf("Local Server: Error in binding\n");
		pthread_exit(NULL);
	}

	// Listen
	if (listen(sfd_main, BACKLOG) == -1) {
		printf("Local Server: Error in listening\n");
		pthread_exit(NULL);
	}

	struct sockaddr_un app_addr;
	socklen_t len = sizeof(struct sockaddr_un);

	// Creation of threads that will handle the apps
	for (;;) {
		struct cl_info temp_info;
		temp_info.file_descriptor = accept(sfd_main, (struct sockaddr *) &app_addr, &len);
		temp_info.cl_pid = atol(app_addr.sun_path + strlen("/tmp/app_socket_"));
		if (temp_info.file_descriptor == -1) {
			printf("Local Server: Error in accepting\n");
			pthread_exit(NULL);
		}
		pthread_t t_id;
		pthread_create(&t_id, NULL, thread_func, &temp_info);
	}

	pthread_exit(NULL);
}

int main(int argc, char *argv[]) {

	signal(SIGPIPE, SIG_IGN);

	sfd_auth = socket(AF_INET, SOCK_DGRAM, 0);
	if (sfd_auth == -1) {
		printf("Local Server: Error in socket creation\n");
		exit(-1);
	}

	sv_addr_auth.sin_family = AF_INET;
	sv_addr_auth.sin_port = htons(58032);

	char str_connect[BUF_SIZE] = "Connect";
	if (sendto(sfd_auth, str_connect, sizeof(str_connect), 0, (struct sockaddr *) &sv_addr_auth, sizeof(struct sockaddr_in)) != sizeof(str_connect)) {
		printf("Local Server: Error in sendto\n");
		exit(-1);
	}
	int port_number = -1;
	if (recvfrom(sfd_auth, &port_number, sizeof(port_number), 0, NULL, NULL) == -1) {
		printf("Local Server: Error in recvfrom\n");
		exit(-1);
	}
	printf("Port Number: %d\n", port_number);
	sv_addr_auth.sin_port = htons(port_number);

	// Creation of thread that will execute the commands
	pthread_t t_id_apps;
	pthread_create(&t_id_apps, NULL, handle_apps, NULL);

	char str[BUF_SIZE] = {0};
	char g_name[BUF_SIZE] = {0};
	size_t len;
	while (1) {
		fgets(str, sizeof(str), stdin);
		// ShowAllGroupsInfo(groups);
		if (strcmp(str, "Create group\n") == 0) {
			pthread_mutex_lock(&mtx);
			printf("Insert group id:\n");
			fgets(g_name, sizeof(g_name), stdin);
			len = strlen(g_name);
			if (len > 0 && g_name[len-1] == '\n') {
			  g_name[--len] = '\0';
			}
			char * secret_recv = CreateGroupLocalServer(&groups, g_name);
			if (secret_recv != NULL) {
				printf("Group created with secret %s!\n", secret_recv);
			}
			free(secret_recv);
			pthread_mutex_unlock(&mtx);
		}
		else if (strcmp(str, "Delete group\n") == 0) {
			pthread_mutex_lock(&mtx);
			printf("Insert group id:\n");
			fgets(g_name, sizeof(g_name), stdin);
			len = strlen(g_name);
			if (len > 0 && g_name[len-1] == '\n') {
			  g_name[--len] = '\0';
			}
			printf("Group to delete: %s\n", g_name);
			if (DeleteGroupLocalServer(&groups, g_name))
				printf("Group deleted!\n");
			pthread_mutex_unlock(&mtx);
		}
		else if (strcmp(str, "Show group info\n") == 0) {
			pthread_mutex_lock(&mtx);
			printf("Insert group id:\n");
			fgets(g_name, sizeof(g_name), stdin);
			len = strlen(g_name);
			if (len > 0 && g_name[len-1] == '\n') {
			  g_name[--len] = '\0';
			}
			if (ShowGroupInfo(groups, g_name) == false)
				printf("Group not found!\n");
			pthread_mutex_unlock(&mtx);
		}
		else if (strcmp(str, "Show application status\n") == 0) {
			pthread_mutex_lock(&mtx);
			ShowAppStatus(groups);
			pthread_mutex_unlock(&mtx);
		}
		else if (strcmp(str, "Quit\n") == 0) {
			break;
		}
		else
			printf("Unknown Command\n");
		memset(str, 0, sizeof(str));
		memset(g_name, 0, sizeof(g_name));
	}

	pthread_cancel(t_id_apps);

	if (close(sfd_main) == -1) {
		printf("Local Server: Error in closing socket\n");
	}

	pthread_mutex_lock(&mtx);
	if (DeleteGroupList(&groups) == -1)
		printf("Error in deleting the group list\n");
	pthread_mutex_unlock(&mtx);

	// CloseAllFileDesc(&groups);

	char cmd[BUF_SIZE] = "CloseConnection";
	if (sendto(sfd_auth, cmd, sizeof(cmd), 0, (struct sockaddr *) &sv_addr_auth, sizeof(struct sockaddr_in)) != sizeof(cmd)) {
		printf("Server: Error in sendto\n");
		exit(-1);
	}

	if (close(sfd_auth) == -1) {
		printf("Local Server: Error in closing socket\n");
		exit(-1);
	}

	remove(sv_addr.sun_path);

	printf("Exiting Local Server...\n");

	return 0;

}
