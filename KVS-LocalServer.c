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
#include "appList.h"

// Path for the Local Server adress
#define SV_SOCK_PATH "/tmp/server_sock"
// Path for the Local Server adress for the callback thread
#define SV_SOCK_PATH_CB "/tmp/server_sock_cb"
// Maximum length for strings (key / value / secret / group id)
#define BUF_SIZE 100
// Backlog for the listen
#define BACKLOG 20

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

// Application info to the sent to the thread handling the application
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
// Local Server's adress for the callback thread
struct sockaddr_un sv_addr_cb;
// Local Server's socket address
struct sockaddr_un sv_addr;
// File descriptor - This is used to create the connections to the apps
int sfd_main;

// Port to be used in the connection to the Authentification Server
int initial_port = 58032;

int put_value (char * group_name, int * app_fd, int * pid) {

	// Received key
	char temp_key[BUF_SIZE] = {0};
	// Received value
	char temp_value[BUF_SIZE] = {0};
	// Ready flag
	int ready = 1;
	ssize_t numBytes;
	// Send flag saying that the server is ready to receive the key
	if (send(*app_fd, &ready, sizeof(int), 0) != sizeof(int)) {
		printf("Local Server: Error in sending ready flag\n");
		printf("The error message is: %s\n", strerror(errno));
		return -2;
	}
	// Receive the key
	numBytes = recv(*app_fd, temp_key, sizeof(temp_key), 0);
	if (numBytes == -1) {
		printf("Local Server: Error in reading key\n");
		printf("The error message is: %s\n", strerror(errno));
		return -2;
	}
	if (send(*app_fd, &ready, sizeof(int), 0) != sizeof(int)) {
		printf("Local Server: Error in sending ready flag\n");
		printf("The error message is: %s\n", strerror(errno));
		return -2;
	}
	numBytes = recv(*app_fd, temp_value, sizeof(temp_value), 0);
	if (numBytes == -1) {
		printf("Local Server: Error in reading value\n");
		printf("The error message is: %s\n", strerror(errno));
		return -2;
	}
	// If the key already exists and if the key is in the watchlist of an application belonging to the group, we will call the callback function(s)
	if (FindKeyValueLocalServer(groups, group_name, temp_key) && IsWatchListOfGroup(groups, group_name, temp_key)) {
		struct Group * current = groups;
		// Find group with the given name
		while (current != NULL)
		{
			if (strcmp(current->group_name, group_name) == 0) {
				// Search all apps for the ones with the key in their watchlist
				struct App * curr = current->apps;
				struct App * next;
				while (curr != NULL)
				{
					next = curr->next;
					if(IsWatchList(curr->wlist, temp_key)) {
						// Send flag saying that a key was changed
						int flag = 1;
						if (send(curr->fd_cb, &flag, sizeof(int), 0) != sizeof(int)) {
							printf("Local Server: Error in sending flag\n");
							printf("The error message is: %s\n", strerror(errno));
							return -2;
						}
						flag = -1;
						// Recv flag saying that the app is ready to receive the key
						numBytes = recv(curr->fd_cb, &flag, sizeof(int), 0);
						if (numBytes == -1) {
							printf("Local Server: Error in reading flag\n");
							printf("The error message is: %s\n", strerror(errno));
							return -2;
						}
						if (flag == -1) {
							printf("Local Server: Error in receiving ready flag\n");
							return -2;
						}
						if (send(curr->fd_cb, temp_key, sizeof(temp_key), 0) != sizeof(temp_key)) {
							printf("Local Server: Error in sending changed key\n");
							printf("The error message is: %s\n", strerror(errno));
							return -2;
						}
					}
					curr = next;
				}
			}
			current = current->next;
		}
	}
	// Add key value to the Hashtable of the group
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
	// Send flag saying that the server is ready to receive the key
	if (send(*app_fd, &ready, sizeof(int), 0) != sizeof(int)) {
		printf("Local Server: Error in sending ready flag\n");
		printf("The error message is: %s\n", strerror(errno));
		return -2;
	}
	// Receive the key
	numBytes = recv(*app_fd, temp_key, sizeof(temp_key), 0);
	if (numBytes == -1) {
		printf("Local Server: Error in reading key\n");
		return -2;
	}
	// Get value
	char * temp_value = GetKeyValueLocalServer(groups, group_name, temp_key);
	if (temp_value != NULL) {
		length = strlen(temp_value);
	}
	// Send value length to app
	if (send(*app_fd, &length, sizeof(int), 0) != sizeof(int)) {
		printf("Local Server: Error in sending length\n");
		printf("The error message is: %s\n", strerror(errno));
		return -2;
	}
	// Key doesn't exist
	if (length == -1)
		return -1;
	// Send value to app
	if (send(*app_fd, temp_value, (length+1)*sizeof(char), 0) != (length+1)*sizeof(char)) {
		printf("Local Server: Error in sending value pointer\n");
		printf("The error message is: %s\n", strerror(errno));
		return -2;
	}
	return 1;
}

int delete_value (char * group_name, int * app_fd) {

	char temp_key[BUF_SIZE] = {0};
	int ready = 1;
	int check_key = -1;
	ssize_t numBytes;
	// Send flag saying that the server is ready to receive the key
	if (send(*app_fd, &ready, sizeof(int), 0) != sizeof(int)) {
		printf("Local Server: Error in sending ready flag\n");
		printf("The error message is: %s\n", strerror(errno));
		return -2;
	}
	// Receive the key
	numBytes = recv(*app_fd, temp_key, sizeof(temp_key), 0);
	if (numBytes == -1) {
		printf("Local Server: Error in reading key\n");
		printf("The error message is: %s\n", strerror(errno));
		return -2;
	}
	// Check if received key exists
	if (FindKeyValueLocalServer(groups, group_name, temp_key))
		check_key = 1;
	// Send flag to app
	if (send(*app_fd, &check_key, sizeof(int), 0) != sizeof(int)) {
		printf("Local Server: Error in sending check_key\n");
		printf("The error message is: %s\n", strerror(errno));
		return -2;
	}
	// Key doesn't exist
	if (check_key == -1)
		return -1;
	// Delete pair if key exists
	if (DeleteKeyValue(groups, group_name, temp_key) == false)
		return -1;
	return 1;
}

int register_callback (char * group_name, int * pid, int * app_fd) {

	char temp_key[BUF_SIZE] = {0};
	int ready = 1;
	int check_key = -1;
	ssize_t numBytes;
	// Send flag saying that the server is ready to receive the key
	if (send(*app_fd, &ready, sizeof(int), 0) != sizeof(int)) {
		printf("Local Server: Error in sending ready flag\n");
		printf("The error message is: %s\n", strerror(errno));
		return -2;
	}
	// Receive the key
	numBytes = recv(*app_fd, temp_key, sizeof(temp_key), 0);
	if (numBytes == -1) {
		printf("Local Server: Error in reading key\n");
		printf("The error message is: %s\n", strerror(errno));
		return -2;
	}
	// Check if received key exists
	if (FindKeyValueLocalServer(groups, group_name, temp_key))
		check_key = 1;
	// Send flag to app
	if (send(*app_fd, &check_key, sizeof(int), 0) != sizeof(int)) {
		printf("Local Server: Error in sending check_key\n");
		printf("The error message is: %s\n", strerror(errno));
		return -2;
	}
	// Key doesn't exist
	if (check_key == -1)
		return -1;
	// Delete pair if key exists
	if (AddKeyToWatchList(groups, group_name, *pid, temp_key) == false)
		return -1;
	return 1;
}

void * thread_func(void * arg) {

	// Get app information from arguments
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

	// File descriptor assignment for the callback
	int sfd_callback = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sfd_callback == -1) {
		printf("Local Server: Error in socket creation\n");
		printf("The error message is: %s\n", strerror(errno));
		pthread_exit(NULL);
	}

	// Clean server callback adress
	memset(&sv_addr_cb, 0, sizeof(struct sockaddr_un));

	// Clean socket path
	remove(SV_SOCK_PATH_CB);
	unlink(SV_SOCK_PATH_CB);

	// Bind
	sv_addr_cb.sun_family = AF_UNIX;
	strncpy(sv_addr_cb.sun_path, SV_SOCK_PATH_CB, sizeof(sv_addr_cb.sun_path) - 1);

	if (bind(sfd_callback, (struct sockaddr *) &sv_addr_cb, sizeof(struct sockaddr_un)) == -1) {
		printf("Local Server: Error in binding\n");
		printf("The error message is: %s\n", strerror(errno));
		pthread_exit(NULL);
	}

	// Sending flag saying that connection was established
	if (send(cfd, &error_flag, sizeof(int), 0) != sizeof(int)) {
		printf("Local Server: Error in sending flag for established connection\n");
		printf("The error message is: %s\n", strerror(errno));
		if (close (sfd_callback) == -1 || close(cfd) == -1) {
			printf("Local Server: Error in closing socket file descriptor\n");
			printf("The error message is: %s\n", strerror(errno));
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
		printf("The error message is: %s\n", strerror(errno));
		if (close (sfd_callback) == -1 || close(cfd) == -1) {
			printf("Local Server: Error in closing socket file descriptor\n");
			printf("The error message is: %s\n", strerror(errno));
		}
		pthread_exit(NULL);
	}

	// Check if group_id is correct
	pthread_mutex_lock(&mtx);
	if(FindGroupLocalServer(groups, group_id_app))
		error_flag = 1;
	else
		error_flag = 0;
	pthread_mutex_unlock(&mtx);

	// Sending flag saying if group_id is correct or not
	if (send(cfd, &error_flag, sizeof(int), 0) != sizeof(int)) {
		printf("Local Server: Error in sending flag for correct/incorrect group_id\n");
		printf("The error message is: %s\n", strerror(errno));
		if (close (sfd_callback) == -1 || close(cfd) == -1) {
			printf("Local Server: Error in closing socket file descriptor\n");
			printf("The error message is: %s\n", strerror(errno));
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
		printf("The error message is: %s\n", strerror(errno));
		if (close (sfd_callback) == -1 || close(cfd) == -1) {
			printf("Local Server: Error in closing socket file descriptor\n");
			printf("The error message is: %s\n", strerror(errno));
		}
		pthread_exit(NULL);
	}

	// Listen
	if (listen(sfd_callback, BACKLOG) == - 1) {
		printf("Local Server: Error in listening\n");
		printf("The error message is: %s\n", strerror(errno));
		pthread_exit(NULL);
	}

	struct sockaddr_un app_addr;
	memset(&app_addr, 0, sizeof(struct sockaddr_un));
	socklen_t len = sizeof(struct sockaddr_un);

	// Connect to the callback fie descriptor
	int fd_cb = accept(sfd_callback, (struct sockaddr *) &app_addr, &len);
	if (fd_cb == -1) {
		printf("Local Server: Error in accepting\n");
		printf("The error message is: %s\n", strerror(errno));
		if (close (sfd_callback) == -1 || close(cfd) == -1) {
			printf("Local Server: Error in closing socket file descriptor\n");
			printf("The error message is: %s\n", strerror(errno));
		}
		pthread_exit(NULL);
	}

	// Reuse of error flag for the established connection
	error_flag = 1;

	// Sending flag saying that connection was established
	if (send(fd_cb, &error_flag, sizeof(int), 0) != sizeof(int)) {
		printf("Local Server: Error in sending flag for established connection\n");
		printf("The error message is: %s\n", strerror(errno));
		if (close (sfd_callback) == -1 || close(cfd) == -1) {
			printf("Local Server: Error in closing socket file descriptor\n");
			printf("The error message is: %s\n", strerror(errno));
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
		printf("The error message is: %s\n", strerror(errno));
		if (close (sfd_callback) == -1 || close(cfd) == -1) {
			printf("Local Server: Error in closing socket file descriptor\n");
			printf("The error message is: %s\n", strerror(errno));
		}
		pthread_exit(NULL);
	}

	if (error_flag == 0) {
		printf("Local Server: Incorrect secret\n");
		if (close (sfd_callback) == -1 || close(cfd) == -1) {
			printf("Local Server: Error in closing socket file descriptor\n");
			printf("The error message is: %s\n", strerror(errno));
		}
		pthread_exit(NULL);
	}

	// Print information for the connected app
	printf("Connection established with app with PID %ld\n", pid);

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
			sucess_flag = put_value(group_id_app, &cfd, &pid);
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

void * handle_apps(void * arg) {

	// Detach thread because we will cancel it when exiting the serverr
	if (pthread_detach(pthread_self()) != 0) {
		printf("Local Server: Error in 'pthread_detach'\n");
		printf("The error message is: %s\n", strerror(errno));
		pthread_exit(NULL);
	}

	// File descriptor assignment
	sfd_main = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sfd_main == -1) {
		printf("Local Server: Error in socket creation\n");
		printf("The error message is: %s\n", strerror(errno));
		pthread_exit(NULL);
	}

	// Clean socket path
	remove(SV_SOCK_PATH);

	// Bind
	sv_addr.sun_family = AF_UNIX;
	strncpy(sv_addr.sun_path, SV_SOCK_PATH, sizeof(sv_addr.sun_path) - 1);

	if (bind(sfd_main, (struct sockaddr *) &sv_addr, sizeof(struct sockaddr_un)) == -1) {
		printf("Local Server: Error in binding\n");
		printf("The error message is: %s\n", strerror(errno));
		pthread_exit(NULL);
	}

	// Listen
	if (listen(sfd_main, BACKLOG) == -1) {
		printf("Local Server: Error in listening\n");
		printf("The error message is: %s\n", strerror(errno));
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
			printf("The error message is: %s\n", strerror(errno));
			pthread_exit(NULL);
		}
		pthread_t t_id;
		pthread_create(&t_id, NULL, thread_func, &temp_info);
	}

	pthread_exit(NULL);
}

int main(int argc, char *argv[]) {

	signal(SIGPIPE, SIG_IGN);

	// Create socket
	sfd_auth = socket(AF_INET, SOCK_DGRAM, 0);
	if (sfd_auth == -1) {
		printf("Local Server: Error in socket creation\n");
		printf("The error message is: %s\n", strerror(errno));
		exit(-1);
	}

	// Address of the authentification server
	sv_addr_auth.sin_family = AF_INET;
	sv_addr_auth.sin_port = htons(initial_port);

	// Sending a string to the Auth Server to connect
	char str_connect[BUF_SIZE] = "Connect";
	if (sendto(sfd_auth, str_connect, sizeof(str_connect), 0, (struct sockaddr *) &sv_addr_auth, sizeof(struct sockaddr_in)) != sizeof(str_connect)) {
		printf("Local Server: Error in sendto\n");
		printf("The error message is: %s\n", strerror(errno));
		exit(-1);
	}
	int port_number = -1;
	// Receive the new port number
	if (recvfrom(sfd_auth, &port_number, sizeof(port_number), 0, NULL, NULL) == -1) {
		printf("Local Server: Error in recvfrom\n");
		printf("The error message is: %s\n", strerror(errno));
		exit(-1);
	}
	printf("Port Number: %d\n", port_number);
	sv_addr_auth.sin_port = htons(port_number);

	// Creation of thread that will execute the commands
	pthread_t t_id_apps;
	pthread_create(&t_id_apps, NULL, handle_apps, NULL);

	// We only try to recvfrom / sendto for one second
	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	if (setsockopt(sfd_auth, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
	    printf("Local Server: Error in setting socket options\n");
	    printf("The error message is: %s\n", strerror(errno));
	    exit(-1);
	}
	
	char str[BUF_SIZE] = {0};
	char g_name[BUF_SIZE] = {0};
	size_t len;
	// Getting the commands from the keyboard
	while (1) {
		fgets(str, sizeof(str), stdin);
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
			SendDeleteGroupFlags(&groups, g_name);
			pthread_mutex_unlock(&mtx);
			while(AllAppsFromGroupClosed(&groups, g_name) == false);
			pthread_mutex_lock(&mtx);
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
				printf("Group id not found!\n");
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

	// Cancel thread that is receiving connections from all the apps
	pthread_cancel(t_id_apps);

	// Close file descriptor of the socket that was receiving connections from the apps
	if (close(sfd_main) == -1) {
		printf("Local Server: Error in closing socket\n");
		printf("The error message is: %s\n", strerror(errno));
	}

	pthread_mutex_lock(&mtx);
	// Force apps to close connections
	SendDeleteAllGroupsFlags(&groups);
	pthread_mutex_unlock(&mtx);
	// Wait until all connections are closed
	while(AllAppsClosed(&groups) == false);
	pthread_mutex_lock(&mtx);
	// Delete all groups
	if (DeleteGroupList(&groups) == -1)
		printf("Error in deleting the group list\n");
	pthread_mutex_unlock(&mtx);

	// Send string to the Authentification server saying that the connection is beeing closed
	char cmd[BUF_SIZE] = "CloseConnection";
	if (sendto(sfd_auth, cmd, sizeof(cmd), 0, (struct sockaddr *) &sv_addr_auth, sizeof(struct sockaddr_in)) != sizeof(cmd)) {
		printf("Server: Error in sendto\n");
		printf("The error message is: %s\n", strerror(errno));
		exit(-1);
	}

	// Close file descriptor of the connection to the Authentification server
	if (close(sfd_auth) == -1) {
		printf("Local Server: Error in closing socket\n");
		printf("The error message is: %s\n", strerror(errno));
		exit(-1);
	}

	// Remove paths to avoid problem with future binds
	remove(sv_addr.sun_path);
	remove(sv_addr_cb.sun_path);

	printf("Exiting Local Server...\n");

	return 0;

}
