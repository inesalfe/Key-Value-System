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
#include <errno.h>
#include "hash.h"

#define BUF_SIZE 100
#define SIZE_IN 50000
#define BACKLOG 5

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

// Table with the group-secret pairs
struct HashTable * table;

struct LocalSvrData {
	int file_desc;
	struct LocalSvrData * next;
};

// Linked list of the file descriptor of the connected local servers
struct LocalSvrData * connected_servers = NULL;

// Port to be used in the connection to the local servers
int initial_port = 58032;

// File descriptor for the datagram socket that are receiving the connections
int sfd;

// Adds a server to the linked list of connected servers
void AddServerToList(int fd) {

	struct LocalSvrData * new_node = (struct LocalSvrData *) calloc(1, sizeof(struct LocalSvrData));
	struct LocalSvrData * last = connected_servers;

	new_node->file_desc = fd;
	new_node->next = NULL;

	if (connected_servers == NULL)
	{
		connected_servers = new_node;
		return;
	}

	while (last->next != NULL)
		last = last->next;

	last->next = new_node;

	return;
}

// Removes a server to the linked list of connected servers
void RemoveServerFromList(int fd) {

	struct LocalSvrData * temp = connected_servers;
	struct LocalSvrData * prev = NULL;

	if (temp != NULL && (temp->file_desc == fd)) {
		if (close(temp->file_desc) == -1) {
			printf("Authentification Server: Error in closing socket\n");
			printf("The error message is: %s\n", strerror(errno));
		}
		connected_servers = temp->next;
		free(temp);
		return;
	}

	while (temp != NULL && (temp->file_desc != fd)) {
		prev = temp;
		temp = temp->next;
	}

	if (temp == NULL)
		return;

	if (close(temp->file_desc) == -1) {
		printf("Authentification Server: Error in closing socket\n");
		printf("The error message is: %s\n", strerror(errno));
	}

	prev->next = temp->next;

	free(temp);

	return;
}

void * thread_func(void * arg) {

	struct sockaddr_in addr = *(struct sockaddr_in *)arg;
	// Length of the client address
	socklen_t len;
	// Addresses for auth server and local server
	struct sockaddr_in sv_addr;
	// Socket Creation
	int sfd_ls = socket(AF_INET, SOCK_DGRAM, 0);
	if (sfd_ls == -1) {
		printf("Local Server: Error in socket creation\n");
		printf("The error message is: %s\n", strerror(errno));
		pthread_exit(NULL);
	}
	// Clean auth server address
	memset(&sv_addr, 0, sizeof(sv_addr));
	sv_addr.sin_family = AF_INET;
	sv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	sv_addr.sin_port = htons(initial_port);

	// Bind address
	if (bind(sfd_ls, (struct sockaddr *) &sv_addr, sizeof(struct sockaddr_in)) == -1) {
		printf("Authentification Server: Error in binding\n");
		printf("The error message is: %s\n", strerror(errno));
		pthread_exit(NULL);
	}

	// Add server to list
	pthread_mutex_lock(&mtx);
	AddServerToList(sfd_ls);
	pthread_mutex_unlock(&mtx);

	char buf[BUF_SIZE] = {0};
	char addrStr[INET_ADDRSTRLEN];
	len = sizeof(struct sockaddr_in);

	int ready_flag = -1;
	char g_name[BUF_SIZE] = {0};
	char secret[BUF_SIZE] = {0};

	printf("New Local Server Connected\n");

	for (;;) {
		if (recvfrom(sfd_ls, buf, BUF_SIZE, 0, (struct sockaddr *) &addr, &len) == -1) {
			printf("Authentification Server: Error in recvfrom\n");
			printf("The error message is: %s\n", strerror(errno));
			pthread_exit(NULL);
		}
		else {
			if (inet_ntop(AF_INET, &addr.sin_addr, addrStr, INET_ADDRSTRLEN) == NULL) {
				printf("Authentification Server: Couldn't convert client address to string\n");
				printf("The error message is: %s\n", strerror(errno));
				pthread_exit(NULL);
			}
			else {
				if (strcmp(buf, "NewGroup") == 0) {
					pthread_mutex_lock(&mtx);
					ready_flag = 1;
					// Send flag saying that AuthServer is ready to receive group
					if (sendto(sfd_ls, &ready_flag, sizeof(int), 0, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) != sizeof(int)) {
						printf("Authentification Server: Error in sendto\n");
						printf("The error message is: %s\n", strerror(errno));
						pthread_exit(NULL);
					}
					// Receive group name
					if (recvfrom(sfd_ls, g_name, sizeof(g_name), 0, (struct sockaddr *) &addr, &len) == -1) {
						printf("Authentification Server: Error in recvfrom\n");
						printf("The error message is: %s\n", strerror(errno));
						pthread_exit(NULL);
					}
					// Check if group name already exists
					// If exists ready_flag = -1, else do nothing.
					if (ht_search(table, g_name) != NULL) {
						ready_flag = -1;
					}
					// Send flag saying that AuthServer is ready to receive (or not) secret
					if (sendto(sfd_ls, &ready_flag, sizeof(int), 0, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) != sizeof(int)) {
						printf("Authentification Server: Error in sendto\n");
						printf("The error message is: %s\n", strerror(errno));
						pthread_exit(NULL);
					}
					// If ready_flag = -1, exit or continue cicle.
					if (ready_flag == -1) {
						printf("Group name already exists\n");
						continue;
					}
					// Receive secret
					if (recvfrom(sfd_ls, secret, sizeof(secret), 0, (struct sockaddr *) &addr, &len) == -1) {
						printf("Authentification Server: Error in recvfrom\n");
						printf("The error message is: %s\n", strerror(errno));
						pthread_exit(NULL);
					}
					// Store in hash table
					ht_insert(table, g_name, secret);
					// Send success flag (reuse ready flag)
					if (sendto(sfd_ls, &ready_flag, sizeof(int), 0, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) != sizeof(int)) {
						printf("Authentification Server: Error in sendto\n");
						printf("The error message is: %s\n", strerror(errno));
						pthread_exit(NULL);
					}
					if (ready_flag == 1) {
						printf("Group created!\n");
					}
					pthread_mutex_unlock(&mtx);
				}
				else if (strcmp(buf, "GetSecret") == 0) {
					pthread_mutex_lock(&mtx);
					ready_flag = 1;
					// Send flag saying that AuthServer is ready to receive group
					if (sendto(sfd_ls, &ready_flag, sizeof(int), 0, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) != sizeof(int)) {
						printf("Authentification Server: Error in sendto\n");
						printf("The error message is: %s\n", strerror(errno));
						pthread_exit(NULL);
					}
					// Receive group name
					if (recvfrom(sfd_ls, g_name, sizeof(g_name), 0, (struct sockaddr *) &addr, &len) == -1) {
						printf("Authentification Server: Error in recvfrom\n");
						printf("The error message is: %s\n", strerror(errno));
						pthread_exit(NULL);
					}
					// Check if group name exists
					// If it doesn't ready_flag = -1, else do nothing.
					if (ht_search(table, g_name) == NULL)
						ready_flag = -1;
					// Send flag saying that AuthServer is ready to receive secret
					if (sendto(sfd_ls, &ready_flag, sizeof(int), 0, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) != sizeof(int)) {
						printf("Authentification Server: Error in sendto\n");
						printf("The error message is: %s\n", strerror(errno));
						pthread_exit(NULL);
					}
					// If ready_flag = -1, exit or continue cicle.
					if (ready_flag == -1) {
						printf("Group name doesn't exist\n");
						continue;
					}
					// Receive flag saying that the server is ready to receive the secret
					if (recvfrom(sfd_ls, &ready_flag, sizeof(ready_flag), 0, (struct sockaddr *) &addr, &len) == -1) {
						printf("Authentification Server: Error in recvfrom\n");
						printf("The error message is: %s\n", strerror(errno));
						pthread_exit(NULL);
					}
					char temp_secret[BUF_SIZE] = {0};
					strcpy(temp_secret, ht_search(table, g_name));
					// Send secret
					if (sendto(sfd_ls, temp_secret, strlen(temp_secret), 0, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) != strlen(temp_secret)) {
						printf("Authentification Server: Error in sendto\n");
						printf("The error message is: %s\n", strerror(errno));
						pthread_exit(NULL);
					}
					pthread_mutex_unlock(&mtx);
				}
				else if (strcmp(buf, "DeleteGroup") == 0) {
					pthread_mutex_lock(&mtx);
					ready_flag = 1;
					// Send flag saying that AuthServer is ready to receive group
					if (sendto(sfd_ls, &ready_flag, sizeof(int), 0, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) != sizeof(int)) {
						printf("Authentification Server: Error in sendto\n");
						printf("The error message is: %s\n", strerror(errno));
						pthread_exit(NULL);
					}
					// Receive group name
					if (recvfrom(sfd_ls, g_name, sizeof(g_name), 0, (struct sockaddr *) &addr, &len) == -1) {
						printf("Authentification Server: Error in recvfrom\n");
						printf("The error message is: %s\n", strerror(errno));
						pthread_exit(NULL);
					}
					// Check if group name exists
					// If it doesn't ready_flag = -1, else do nothing.
					if (ht_search(table, g_name) == NULL) {
						ready_flag = -1;
						printf("Group name doesn't exist\n");
					}
					// Delete group. If group doesn't exist, success flag = -1, else success flag = 1.
					if (ready_flag == 1)
						ht_delete(table, g_name);
					// Send success flag (reuse ready flag)
					if (sendto(sfd_ls, &ready_flag, sizeof(int), 0, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) != sizeof(int)) {
						printf("Authentification Server: Error in sendto\n");
						printf("The error message is: %s\n", strerror(errno));
						pthread_exit(NULL);
					}
					if (ready_flag == 1) {
						printf("Group deleted!\n");
					}
					pthread_mutex_unlock(&mtx);
				}
				else if (strcmp(buf, "FindGroup") == 0) {
					pthread_mutex_lock(&mtx);
					ready_flag = 1;
					// Send flag saying that AuthServer is ready to receive group
					if (sendto(sfd_ls, &ready_flag, sizeof(int), 0, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) != sizeof(int)) {
						printf("Authentification Server: Error in sendto\n");
						printf("The error message is: %s\n", strerror(errno));
						pthread_exit(NULL);
					}
					// Receive group name
					if (recvfrom(sfd_ls, g_name, sizeof(g_name), 0, (struct sockaddr *) &addr, &len) == -1) {
						printf("Authentification Server: Error in recvfrom\n");
						printf("The error message is: %s\n", strerror(errno));
						pthread_exit(NULL);
					}
					// Check if group name exists
					// If it doesn't ready_flag = -1, else do nothing.
					if (ht_search(table, g_name) == NULL)
						ready_flag = -1;
					// Send success flag (reuse ready flag)
					if (sendto(sfd_ls, &ready_flag, sizeof(int), 0, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) != sizeof(int)) {
						printf("Authentification Server: Error in sendto\n");
						printf("The error message is: %s\n", strerror(errno));
						pthread_exit(NULL);
					}
					pthread_mutex_unlock(&mtx);
				}
				else if (strcmp(buf, "CloseConnection") == 0) {
					pthread_mutex_lock(&mtx);
					RemoveServerFromList(sfd_ls);
					pthread_mutex_unlock(&mtx);
					printf("Connection closed\n");
					break;
				}
				memset(buf, 0, sizeof(buf));
			}
		}
	}

	pthread_exit(NULL);
}

void * handle_local_servers(void * arg) {

	// Detach the thread so we can release the memory with pthread_cancel
	if (pthread_detach(pthread_self()) != 0) {
		printf("Local Server: Error in 'pthread_detach'\n");
		printf("The error message is: %s\n", strerror(errno));
	}

	// Length of the client address
	socklen_t len;
	// Addresses for auth server and local server
	struct sockaddr_in sv_addr_auth, sv_addr_local;
	char localSrvStr[INET_ADDRSTRLEN];
	char buf[BUF_SIZE] = {0};

	// Socket Creation
	sfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sfd == -1) {
		printf("Authentification Server: Error in socket creation\n");
		printf("The error message is: %s\n", strerror(errno));
		pthread_exit(NULL);
	}

	// Clean auth server address
	memset(&sv_addr_auth, 0, sizeof(sv_addr_auth));
	sv_addr_auth.sin_family = AF_INET;
	sv_addr_auth.sin_addr.s_addr = htonl(INADDR_ANY);
	sv_addr_auth.sin_port = htons(initial_port);

	// Bind address
	if (bind(sfd, (struct sockaddr *) &sv_addr_auth, sizeof(struct sockaddr_in)) == -1) {
		printf("Authentification Server: Error in binding\n");
		printf("The error message is: %s\n", strerror(errno));
		pthread_exit(NULL);
	}

	len = sizeof(struct sockaddr_in);

	// Creation of threads that will handle the apps
	for (;;) {
		if (recvfrom(sfd, buf, BUF_SIZE, 0, (struct sockaddr *) &sv_addr_local, &len) == -1) {
			printf("Authentification Server: Error in recvfrom\n");
			printf("The error message is: %s\n", strerror(errno));
			pthread_exit(NULL);
		}
		else {
			// printf("Received %s in main\n", buf);
			if (strcmp(buf, "Connect") == 0) {
				if (inet_ntop(AF_INET, &sv_addr_local.sin_addr, localSrvStr, INET_ADDRSTRLEN) == NULL) {
					printf("Authentification Server: Couldn't convert client address to string\n");
					printf("The error message is: %s\n", strerror(errno));
					pthread_exit(NULL);
				}
				printf("Authentification server received connection request from (%s, %u)\n", localSrvStr, ntohs(sv_addr_local.sin_port));
				initial_port++;
				if (sendto(sfd, &initial_port, sizeof(int), 0, (struct sockaddr *) &sv_addr_local, sizeof(struct sockaddr_in)) != sizeof(int)) {
					printf("Authentification Server: Error in sendto\n");
					printf("The error message is: %s\n", strerror(errno));
					pthread_exit(NULL);
				}
				pthread_t t_id;
				pthread_create(&t_id, NULL, thread_func, &sv_addr_local);
			}
		}
	}

	pthread_exit(NULL);
}

int main(int argc, char *argv[]) {

	// Create table
	table = create_table(SIZE_IN);

	// Create thread that will handle the incoming connections
	pthread_t t_id;
	pthread_create(&t_id, NULL, handle_local_servers, NULL);

	// Main loop where we wait for the input Quit
	char str[BUF_SIZE] = {0};
	while (1) {
		fgets(str, sizeof(str), stdin);
		if (strcmp(str, "Quit\n") == 0) {
			break;
		}
		else
			printf("Unknown Command\n");
	}

	// When exiting, cancel the thread that is receiving the incoming connections
	pthread_cancel(t_id);

	// Clear the linked list while closing all the file descriptors
	struct LocalSvrData * current = connected_servers;
	struct LocalSvrData * next = NULL;

	while (current != NULL)
	{
		next = current->next;
		if (close(current->file_desc) == -1) {
			printf("Authentification Server: Error in closing socket\n");
			printf("The error message is: %s\n", strerror(errno));
		}
		free(current);
		current = next;
	}

	connected_servers = NULL;

	if (close(sfd) == -1) {
		printf("Authentification Server: Error in closing socket\n");
		printf("The error message is: %s\n", strerror(errno));
	}

	// Free table
	free_table(table);

	printf("Exiting Authentification Server...\n");

	return 0;

}
