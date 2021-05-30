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
#include "hash.h"

#define BUF_SIZE 100
#define SIZE_IN 1000
#define BACKLOG 5

struct HashTable * table;

struct LocalSvrData {
	int file_desc;
	struct LocalSvrData * next;
};

struct LocalSvrData * connected_servers = NULL;

int initial_port = 58032;

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

void RemoveServerFromList(int fd) {

	struct LocalSvrData * temp = connected_servers, * prev;
 
	if (temp != NULL && (temp->file_desc == fd)) {
		if (close(temp->file_desc) == -1) {
			printf("Authentification Server: Error in closing socket\n");
		}
		connected_servers = temp->next;
		free(temp);
		return;
	}
 
	while (temp != NULL && (temp->file_desc == fd)) {
		if (close(temp->file_desc) == -1) {
			printf("Authentification Server: Error in closing socket\n");
		}
		prev = temp;
		temp = temp->next;
	}
 
	if (temp == NULL)
		return;
 
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
	int sfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sfd == -1) {
		printf("Local Server: Error in socket creation\n");
		pthread_exit(NULL);
	}
	// Clean auth server address
	memset(&sv_addr, 0, sizeof(sv_addr));
	sv_addr.sin_family = AF_INET;
	sv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	sv_addr.sin_port = htons(initial_port);

	// Bind address
	if (bind(sfd, (struct sockaddr *) &sv_addr, sizeof(struct sockaddr_in)) == -1) {
		printf("Authentification Server: Error in binding\n");
		pthread_exit(NULL);
	}

	AddServerToList(sfd);

	char buf[BUF_SIZE];
	char addrStr[INET_ADDRSTRLEN];
	len = sizeof(struct sockaddr_in);

	int ready_flag = -1;
	char g_name[BUF_SIZE];
	char secret[BUF_SIZE];

	printf("New Local Server Connected\n");

	// Creation of threads that will handle the apps
	for (;;) {
		if (recvfrom(sfd, buf, BUF_SIZE, 0, (struct sockaddr *) &addr, &len) == -1) {
			printf("Authentification Server: Error in recvfrom\n");
			pthread_exit(NULL);
		}
		else {
			// printf("Received %s in thread\n", buf);
			if (inet_ntop(AF_INET, &addr.sin_addr, addrStr, INET_ADDRSTRLEN) == NULL) {
				printf("Authentification Server: Couldn't convert client address to string\n");
				pthread_exit(NULL);
			}
			else {
				if (strcmp(buf, "NewGroup") == 0) {
					// printf("Authentification server accepted connection from (%s, %u)\n", addrStr, ntohs(addr.sin_port));
					ready_flag = 1;
					// Send flag saying that AuthServer is ready to receive group
					if (sendto(sfd, &ready_flag, sizeof(int), 0, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) != sizeof(int)) {
						printf("Authentification Server: Error in sendto\n");
						pthread_exit(NULL);
					}
					// Receive group name
					if (recvfrom(sfd, g_name, sizeof(g_name), 0, (struct sockaddr *) &addr, &len) == -1) {
						printf("Authentification Server: Error in recvfrom\n");
						pthread_exit(NULL);
					}
					// Check if group name already exists
					// If exists ready_flag = -1, else do nothing.
					if (ht_search(table, g_name) != NULL) {
						ready_flag = -1;
					}
					// Send flag saying that AuthServer is ready to receive (or not) secret
					if (sendto(sfd, &ready_flag, sizeof(int), 0, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) != sizeof(int)) {
						printf("Authentification Server: Error in sendto\n");
						pthread_exit(NULL);
					}
					// If ready_flag = -1, exit or continue cicle.
					if (ready_flag == -1) {
						printf("Group name already exists\n");
						continue;
					}
					// Receive secret
					if (recvfrom(sfd, secret, sizeof(secret), 0, (struct sockaddr *) &addr, &len) == -1) {
						printf("Authentification Server: Error in recvfrom\n");
						pthread_exit(NULL);
					}
					// Store in hash table
					ht_insert(table, g_name, secret);
					// Send success flag (reuse ready flag)
					if (sendto(sfd, &ready_flag, sizeof(int), 0, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) != sizeof(int)) {
						printf("Authentification Server: Error in sendto\n");
						pthread_exit(NULL);
					}
					if (ready_flag == 1) {
						printf("Group created!\n");
					}
				}
				else if (strcmp(buf, "GetSecret") == 0) {
					// printf("Authentification server accepted connection from (%s, %u)\n", addrStr, ntohs(addr.sin_port));
					ready_flag = 1;
					// Send flag saying that AuthServer is ready to receive group
					if (sendto(sfd, &ready_flag, sizeof(int), 0, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) != sizeof(int)) {
						printf("Authentification Server: Error in sendto\n");
						pthread_exit(NULL);
					}
					// Receive group name
					if (recvfrom(sfd, g_name, sizeof(g_name), 0, (struct sockaddr *) &addr, &len) == -1) {
						printf("Authentification Server: Error in recvfrom\n");
						pthread_exit(NULL);
					}
					// Check if group name exists
					// If it doesn't ready_flag = -1, else do nothing.
					if (ht_search(table, g_name) == NULL)
						ready_flag = -1;
					// Send flag saying that AuthServer is ready to receive secret
					if (sendto(sfd, &ready_flag, sizeof(int), 0, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) != sizeof(int)) {
						printf("Authentification Server: Error in sendto\n");
						pthread_exit(NULL);
					}
					// If ready_flag = -1, exit or continue cicle.
					if (ready_flag == -1) {
						printf("That group id doesn't exist!\n");
						continue;
					}
					// Receive flag saying that the server is ready to receive the secret
					if (recvfrom(sfd, &ready_flag, sizeof(ready_flag), 0, (struct sockaddr *) &addr, &len) == -1) {
						printf("Authentification Server: Error in recvfrom\n");
						pthread_exit(NULL);
					}
					char temp_secret[BUF_SIZE];
					strcpy(temp_secret, ht_search(table, g_name));
					// Send secret
					if (sendto(sfd, temp_secret, sizeof(temp_secret), 0, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) != sizeof(temp_secret)) {
						printf("Authentification Server: Error in sendto\n");
						pthread_exit(NULL);
					}
				}
				else if (strcmp(buf, "DeleteGroup") == 0) {
					// printf("Authentification server accepted connection from (%s, %u)\n", addrStr, ntohs(addr.sin_port));
					ready_flag = 1;
					// Send flag saying that AuthServer is ready to receive group
					if (sendto(sfd, &ready_flag, sizeof(int), 0, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) != sizeof(int)) {
						printf("Authentification Server: Error in sendto\n");
						pthread_exit(NULL);
					}
					// Receive group name
					if (recvfrom(sfd, g_name, sizeof(g_name), 0, (struct sockaddr *) &addr, &len) == -1) {
						printf("Authentification Server: Error in recvfrom\n");
						pthread_exit(NULL);
					}
					// Check if group name exists
					// If it doesn't ready_flag = -1, else do nothing.
					// printf("%s\n", ht_search(table, g_name));
					if (ht_search(table, g_name) == NULL) {
						ready_flag = -1;
						printf("Group name doesn't exist\n");
					}
					// Delete group. If group doesn't exist, success flag = -1, else success flag = 1.
					if (ready_flag == 1)
						ht_delete(table, g_name);
					// Send success flag (reuse ready flag)
					if (sendto(sfd, &ready_flag, sizeof(int), 0, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) != sizeof(int)) {
						printf("Authentification Server: Error in sendto\n");
						pthread_exit(NULL);
					}
					if (ready_flag == 1) {
						printf("Group deleted!\n");
					}
				}
				else if (strcmp(buf, "FindGroup") == 0) {
					// printf("Authentification server accepted connection from (%s, %u)\n", addrStr, ntohs(addr.sin_port));
					ready_flag = 1;
					// Send flag saying that AuthServer is ready to receive group
					if (sendto(sfd, &ready_flag, sizeof(int), 0, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) != sizeof(int)) {
						printf("Authentification Server: Error in sendto\n");
						pthread_exit(NULL);
					}
					// Receive group name
					if (recvfrom(sfd, g_name, sizeof(g_name), 0, (struct sockaddr *) &addr, &len) == -1) {
						printf("Authentification Server: Error in recvfrom\n");
						pthread_exit(NULL);
					}
					// Check if group name exists
					// If it doesn't ready_flag = -1, else do nothing.
					if (ht_search(table, g_name) == NULL)
						ready_flag = -1;
					// Send success flag (reuse ready flag)
					if (sendto(sfd, &ready_flag, sizeof(int), 0, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) != sizeof(int)) {
						printf("Authentification Server: Error in sendto\n");
						pthread_exit(NULL);
					}
				}
				else if (strcmp(buf, "CloseConnection") == 0) {
					RemoveServerFromList(sfd);
					printf("Connection closed\n");
					break;
				}
			}
		}
	}

	pthread_exit(NULL);
}

// File descriptor fot the datagram socket
int sfd;

void * handle_local_servers(void * arg) {

	// Length of the client address
	socklen_t len;
	// Addresses for auth server and local server
	struct sockaddr_in sv_addr_auth, sv_addr_local;
	char localSrvStr[INET_ADDRSTRLEN];
	char buf[BUF_SIZE];

	// Socket Creation
	sfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sfd == -1) {
		printf("Authentification Server: Error in socket creation\n");
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
		pthread_exit(NULL);
	}

	len = sizeof(struct sockaddr_in);

	// Creation of threads that will handle the apps
	for (;;) {
		if (recvfrom(sfd, buf, BUF_SIZE, 0, (struct sockaddr *) &sv_addr_local, &len) == -1) {
			printf("Authentification Server: Error in recvfrom\n");
			pthread_exit(NULL);
		}
		else {
			// printf("Received %s in main\n", buf);
			if (strcmp(buf, "Connect") == 0) {
				if (inet_ntop(AF_INET, &sv_addr_local.sin_addr, localSrvStr, INET_ADDRSTRLEN) == NULL) {
					printf("Authentification Server: Couldn't convert client address to string\n");
					pthread_exit(NULL);
				}
				printf("Authentification server received connection request from (%s, %u)\n", localSrvStr, ntohs(sv_addr_local.sin_port));
				initial_port++;
				if (sendto(sfd, &initial_port, sizeof(int), 0, (struct sockaddr *) &sv_addr_local, sizeof(struct sockaddr_in)) != sizeof(int)) {
					printf("Authentification Server: Error in sendto\n");
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

	table = create_table(SIZE_IN);

	pthread_t t_id;
	pthread_create(&t_id, NULL, handle_local_servers, NULL);

	char str[BUF_SIZE];
	while (1) {
		fgets(str, sizeof(str), stdin);
		if (strcmp(str, "Quit\n") == 0) {
			break;
		}
		else
			printf("Unknown Command\n");
	}

	struct LocalSvrData * current = connected_servers;
	struct LocalSvrData * next = NULL;
 
	while (current != NULL)
	{
		next = current->next;
		if (close(current->file_desc) == -1) {
			printf("Authentification Server: Error in closing socket\n");
		}
		free(current);
		current = next;
	}
 
	connected_servers = NULL;

	if (close(sfd) == -1) {
		printf("Authentification Server: Error in closing socket\n");
	}

	free_table(table);

	printf("Exiting Authentification Server...\n");

	return 0;

}