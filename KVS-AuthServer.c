#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <math.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include "hash.h"

#define BUF_SIZE 100
#define SIZE_IN 1000

int sfd;
struct HashTable * table;

int main(int argc, char *argv[]) {

	struct sockaddr_in sv_addr, claddr;
	ssize_t numBytes;
	socklen_t len;
	char buf[BUF_SIZE];
	char claddrStr[INET_ADDRSTRLEN];

	table = create_table(SIZE_IN);

	// Socket creation
	sfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sfd == -1) {
		printf("Server: Error in socket creation\n");
		exit(-1);
	}

	// Set address
	memset(&sv_addr, 0, sizeof(sv_addr));
	sv_addr.sin_family = AF_INET;
	sv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	sv_addr.sin_port = htons(58032);
	
	// Bind address
	if (bind(sfd, (struct sockaddr *) &sv_addr, sizeof(struct sockaddr_in)) == -1) {
		printf("Server: Error in binding\n");
		exit(-1);
	}

	len = sizeof(struct sockaddr_in);
	int ready_flag = -1;
	char g_name[BUF_SIZE];
	char secret[BUF_SIZE];

	// Creation of threads that will handle the apps
	for (;;) {
		if (recvfrom(sfd, buf, BUF_SIZE, 0, (struct sockaddr *) &claddr, &len) == -1) {
			printf("Error in recvfrom\n");
			exit(-1);
		}
		else {
			if (inet_ntop(AF_INET, &claddr.sin_addr, claddrStr, INET_ADDRSTRLEN) == NULL) {
				printf("Server: Couldn't convert client address to string\n");
				exit(-1);
			}
			else {
				if (strcmp(buf, "NewGroup") == 0) {
					printf("Authentication server accepted connection from (%s, %u)\n", claddrStr, ntohs(claddr.sin_port));
					ready_flag = 1;
					// Send flag saying that AuthServer is ready to receive group
					if (sendto(sfd, &ready_flag, sizeof(int), 0, (struct sockaddr *) &claddr, sizeof(struct sockaddr_in)) != sizeof(int)) {
						printf("Server: Error in sendto\n");
						exit(-1);
					}
					// Receive group name
					if (recvfrom(sfd, g_name, sizeof(g_name), 0, (struct sockaddr *) &claddr, &len) == -1) {
						printf("Error in recvfrom\n");
						exit(-1);
					}
					// Check if group name already exists
					// If exists ready_flag = -1, else do nothing.
					if (ht_search(table, g_name) != NULL)
						ready_flag = -1;
					// Send flag saying that AuthServer is ready to receive (or not) secret
					if (sendto(sfd, &ready_flag, sizeof(int), 0, (struct sockaddr *) &claddr, sizeof(struct sockaddr_in)) != sizeof(int)) {
						printf("Server: Error in sendto\n");
						exit(-1);
					}
					// If ready_flag = -1, exit or continue cicle.
					if (ready_flag == -1) {
						printf("That group id already exists!\n");
						continue;
					}
					// Receive secret
					if (recvfrom(sfd, secret, sizeof(secret), 0, (struct sockaddr *) &claddr, &len) == -1) {
						printf("Error in recvfrom\n");
						exit(-1);
					}
					// Store in hash table
					ht_insert(table, g_name, secret);
					// Send success flag (reuse ready flag)
					if (sendto(sfd, &ready_flag, sizeof(int), 0, (struct sockaddr *) &claddr, sizeof(struct sockaddr_in)) != sizeof(int)) {
						printf("Server: Error in sendto\n");
						exit(-1);
					}
				}
				else if (strcmp(buf, "CheckSecret") == 0) {
					printf("Authentication server accepted connection from (%s, %u)\n", claddrStr, ntohs(claddr.sin_port));
					ready_flag = 1;
					// Send flag saying that AuthServer is ready to receive group
					if (sendto(sfd, &ready_flag, sizeof(int), 0, (struct sockaddr *) &claddr, sizeof(struct sockaddr_in)) != sizeof(int)) {
						printf("Server: Error in sendto\n");
						exit(-1);
					}
					// Receive group name
					if (recvfrom(sfd, g_name, sizeof(g_name), 0, (struct sockaddr *) &claddr, &len) == -1) {
						printf("Error in recvfrom\n");
						exit(-1);
					}
					// Check if group name exists
					// If it doesn't ready_flag = -1, else do nothing.
					if (ht_search(table, g_name) == NULL)
						ready_flag = -1;
					// Send flag saying that AuthServer is ready to receive secret
					if (sendto(sfd, &ready_flag, sizeof(int), 0, (struct sockaddr *) &claddr, sizeof(struct sockaddr_in)) != sizeof(int)) {
						printf("Server: Error in sendto\n");
						exit(-1);
					}
					// If ready_flag = -1, exit or continue cicle.
					if (ready_flag == -1) {
						printf("That group id doesn't exist!\n");
						continue;
					}
					// Receive secret
					if (recvfrom(sfd, secret, sizeof(secret), 0, (struct sockaddr *) &claddr, &len) == -1) {
						printf("Error in recvfrom\n");
						exit(-1);
					}
					// Get secret from hash table and compare
					if (strcmp(ht_search(table, g_name), secret) != 0)
						ready_flag = -1;
					// Send success flag (reuse ready flag)
					if (sendto(sfd, &ready_flag, sizeof(int), 0, (struct sockaddr *) &claddr, sizeof(struct sockaddr_in)) != sizeof(int)) {
						printf("Server: Error in sendto\n");
						exit(-1);
					}
				}
				else if (strcmp(buf, "GetSecret") == 0) {
					printf("Authentication server accepted connection from (%s, %u)\n", claddrStr, ntohs(claddr.sin_port));
					ready_flag = 1;
					// Send flag saying that AuthServer is ready to receive group
					if (sendto(sfd, &ready_flag, sizeof(int), 0, (struct sockaddr *) &claddr, sizeof(struct sockaddr_in)) != sizeof(int)) {
						printf("Server: Error in sendto\n");
						exit(-1);
					}
					// Receive group name
					if (recvfrom(sfd, g_name, sizeof(g_name), 0, (struct sockaddr *) &claddr, &len) == -1) {
						printf("Error in recvfrom\n");
						exit(-1);
					}
					// Check if group name exists
					// If it doesn't ready_flag = -1, else do nothing.
					if (ht_search(table, g_name) == NULL)
						ready_flag = -1;
					// Send flag saying that AuthServer is ready to receive secret
					if (sendto(sfd, &ready_flag, sizeof(int), 0, (struct sockaddr *) &claddr, sizeof(struct sockaddr_in)) != sizeof(int)) {
						printf("Server: Error in sendto\n");
						exit(-1);
					}
					// If ready_flag = -1, exit or continue cicle.
					if (ready_flag == -1) {
						printf("That group id doesn't exist!\n");
						continue;
					}
					// Receive flag saying that the server is ready to receive the secret
					if (recvfrom(sfd, &ready_flag, sizeof(ready_flag), 0, (struct sockaddr *) &claddr, &len) == -1) {
						printf("Error in recvfrom\n");
						exit(-1);
					}
					char temp_secret[BUF_SIZE];
					strcpy(temp_secret, ht_search(table, g_name));
					// Send secret
					if (sendto(sfd, temp_secret, sizeof(temp_secret), 0, (struct sockaddr *) &claddr, sizeof(struct sockaddr_in)) != sizeof(temp_secret)) {
						printf("Server: Error in sendto\n");
						exit(-1);
					}
				}
				else if (strcmp(buf, "DeleteGroup") == 0) {
					printf("Authentication server accepted connection from (%s, %u)\n", claddrStr, ntohs(claddr.sin_port));
					ready_flag = 1;
					// Send flag saying that AuthServer is ready to receive group
					if (sendto(sfd, &ready_flag, sizeof(int), 0, (struct sockaddr *) &claddr, sizeof(struct sockaddr_in)) != sizeof(int)) {
						printf("Server: Error in sendto\n");
						exit(-1);
					}
					// Receive group name
					if (recvfrom(sfd, g_name, sizeof(g_name), 0, (struct sockaddr *) &claddr, &len) == -1) {
						printf("Error in recvfrom\n");
						exit(-1);
					}
					// Check if group name exists
					// If it doesn't ready_flag = -1, else do nothing.
					if (ht_search(table, g_name) == NULL)
						ready_flag = -1;
					// Delete group. If group doesn't exist, success flag = -1, else success flag = 1.
					if (ready_flag == 1)
						ht_delete(table, g_name);
					// Send success flag (reuse ready flag)
					if (sendto(sfd, &ready_flag, sizeof(int), 0, (struct sockaddr *) &claddr, sizeof(struct sockaddr_in)) != sizeof(int)) {
						printf("Server: Error in sendto\n");
						exit(-1);
					}
				}
                else if (strcmp(buf, "FindGroup") == 0) {
                    printf("Authentication server accepted connection from (%s, %u)\n", claddrStr, ntohs(claddr.sin_port));
                    ready_flag = 1;
                    // Send flag saying that AuthServer is ready to receive group
                    if (sendto(sfd, &ready_flag, sizeof(int), 0, (struct sockaddr *) &claddr, sizeof(struct sockaddr_in)) != sizeof(int)) {
                        printf("Server: Error in sendto\n");
                        exit(-1);
                    }
                    // Receive group name
                    if (recvfrom(sfd, g_name, sizeof(g_name), 0, (struct sockaddr *) &claddr, &len) == -1) {
                        printf("Error in recvfrom\n");
                        exit(-1);
                    }
                    // Check if group name exists
                    // If it doesn't ready_flag = -1, else do nothing.
                    if (ht_search(table, g_name) == NULL)
                        ready_flag = -1;
                    // Send success flag (reuse ready flag)
                    if (sendto(sfd, &ready_flag, sizeof(int), 0, (struct sockaddr *) &claddr, sizeof(struct sockaddr_in)) != sizeof(int)) {
                        printf("Server: Error in sendto\n");
                        exit(-1);
                    }
                }
			}
		}
		memset(&claddr, 0, sizeof(claddr));
	}

	free_table(table);

	return 0;

}