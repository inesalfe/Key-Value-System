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
#include <stdbool.h>
#include <arpa/inet.h>

// 194.210.156.96
// 127.0.0.1

#define BUF_SIZE 100

struct sockaddr_in sv_addr;
int sfd;

bool findGroup(char * g_name) {

	char func_str[BUF_SIZE] = "FindGroup";
	int ready_flag = -1;

	if (sendto(sfd, func_str, sizeof(func_str), 0, (struct sockaddr *) &sv_addr, sizeof(struct sockaddr_in)) != sizeof(func_str)) {
		printf("Server: Error in sendto\n");
		exit(-1);
	}

	if (recvfrom(sfd, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
		printf("Error in recvfrom\n");
		exit(-1);
	}

	if (ready_flag == -1) {
		printf("Server not ready\n");
		return false;
	}

	if (sendto(sfd, g_name, sizeof(g_name), 0, (struct sockaddr *) &sv_addr, sizeof(struct sockaddr_in)) != sizeof(g_name)) {
		printf("Server: Error in sendto\n");
		exit(-1);
	}

	if (recvfrom(sfd, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
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

void createGroup(char * g_name, char * secret) {

	char func_str[BUF_SIZE] = "NewGroup";
	int ready_flag = -1;

	if (sendto(sfd, func_str, sizeof(func_str), 0, (struct sockaddr *) &sv_addr, sizeof(struct sockaddr_in)) != sizeof(func_str)) {
		printf("Server: Error in sendto\n");
		exit(-1);
	}

	if (recvfrom(sfd, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
		printf("Error in recvfrom\n");
		exit(-1);
	}

	if (ready_flag == -1) {
		printf("Server not ready\n");
		return;
	}

	if (sendto(sfd, g_name, sizeof(g_name), 0, (struct sockaddr *) &sv_addr, sizeof(struct sockaddr_in)) != sizeof(g_name)) {
		printf("Server: Error in sendto\n");
		exit(-1);
	}

	if (recvfrom(sfd, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
		printf("Error in recvfrom\n");
		exit(-1);
	}

	if (ready_flag == -1) {
		printf("Group name already exists\n");
		return;
	}

	if (sendto(sfd, secret, sizeof(secret), 0, (struct sockaddr *) &sv_addr, sizeof(struct sockaddr_in)) != sizeof(secret)) {
		printf("Server: Error in sendto\n");
		exit(-1);
	}

	if (recvfrom(sfd, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
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

void checkSecret(char * g_name, char * secret) {

	char func_str[BUF_SIZE] = "CheckSecret";
	int ready_flag = -1;

	if (sendto(sfd, func_str, sizeof(func_str), 0, (struct sockaddr *) &sv_addr, sizeof(struct sockaddr_in)) != sizeof(func_str)) {
		printf("Server: Error in sendto\n");
		exit(-1);
	}

	if (recvfrom(sfd, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
		printf("Error in recvfrom\n");
		exit(-1);
	}

	if (ready_flag == -1) {
		printf("Server not ready\n");
		return;
	}

	if (sendto(sfd, g_name, sizeof(g_name), 0, (struct sockaddr *) &sv_addr, sizeof(struct sockaddr_in)) != sizeof(g_name)) {
		printf("Server: Error in sendto\n");
		exit(-1);
	}

	if (recvfrom(sfd, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
		printf("Error in recvfrom\n");
		exit(-1);
	}

	if (ready_flag == -1) {
		printf("Group name doesn't exist\n");
		return;
	}

	if (sendto(sfd, secret, sizeof(secret), 0, (struct sockaddr *) &sv_addr, sizeof(struct sockaddr_in)) != sizeof(secret)) {
		printf("Server: Error in sendto\n");
		exit(-1);
	}

	if (recvfrom(sfd, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
		printf("Error in recvfrom\n");
		exit(-1);
	}

	if (ready_flag == -1) {
		printf("Wrong secret\n");
	}
	else {
		printf("Correct secret\n");
	}

	return;

}

void deleteGroup(char * g_name) {

	char func_str[BUF_SIZE] = "DeleteGroup";
	int ready_flag = -1;

	if (sendto(sfd, func_str, sizeof(func_str), 0, (struct sockaddr *) &sv_addr, sizeof(struct sockaddr_in)) != sizeof(func_str)) {
		printf("Server: Error in sendto\n");
		exit(-1);
	}

	if (recvfrom(sfd, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
		printf("Error in recvfrom\n");
		exit(-1);
	}

	if (ready_flag == -1) {
		printf("Server not ready\n");
		return;
	}

	if (sendto(sfd, g_name, sizeof(g_name), 0, (struct sockaddr *) &sv_addr, sizeof(struct sockaddr_in)) != sizeof(g_name)) {
		printf("Server: Error in sendto\n");
		exit(-1);
	}

	if (recvfrom(sfd, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
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

char * getSecret(char * g_name) {

	char func_str[BUF_SIZE] = "GetSecret";
	int ready_flag = -1;

	if (sendto(sfd, func_str, sizeof(func_str), 0, (struct sockaddr *) &sv_addr, sizeof(struct sockaddr_in)) != sizeof(func_str)) {
		printf("Server: Error in sendto\n");
		exit(-1);
	}

	if (recvfrom(sfd, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
		printf("Error in recvfrom\n");
		exit(-1);
	}

	if (ready_flag == -1) {
		printf("Server not ready\n");
		return NULL;
	}

	if (sendto(sfd, g_name, sizeof(g_name), 0, (struct sockaddr *) &sv_addr, sizeof(struct sockaddr_in)) != sizeof(g_name)) {
		printf("Server: Error in sendto\n");
		exit(-1);
	}

	if (recvfrom(sfd, &ready_flag, sizeof(ready_flag), 0, NULL, NULL) == -1) {
		printf("Error in recvfrom\n");
		exit(-1);
	}

	if (ready_flag == -1) {
		printf("Group name doesn't exist\n");
		return NULL;
	}

	if (sendto(sfd, &ready_flag, sizeof(ready_flag), 0, (struct sockaddr *) &sv_addr, sizeof(struct sockaddr_in)) != sizeof(ready_flag)) {
		printf("Server: Error in sendto\n");
		exit(-1);
	}

	char * secret = (char *)malloc(BUF_SIZE*sizeof(char));

	if (recvfrom(sfd, secret, sizeof(secret), 0, NULL, NULL) == -1) {
		printf("Error in recvfrom\n");
		exit(-1);
	}

	return secret;

}

int main(int argc, char *argv[]) {

	ssize_t numBytes;
	char s_send[BUF_SIZE] = "s1";
	char s_cmp[BUF_SIZE] = "s2";
	char g_name_send[BUF_SIZE] = "g1";
	char g_name_cmp[BUF_SIZE] = "g2";

	sfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sfd == -1) {
		printf("Client: Error in socket creation\n");
		exit(-1);
	}

	sv_addr.sin_family = AF_INET;
	sv_addr.sin_port = htons(58032);

	createGroup(g_name_send, s_send);
	createGroup(g_name_send, s_send);
	checkSecret(g_name_send, s_send);
	checkSecret(g_name_send, s_cmp);
	if (findGroup(g_name_send))
		printf("Group found!\n");
	else
		printf("Group not found!\n");
	if (findGroup(g_name_cmp))
		printf("Group found!\n");
	else
		printf("Group not found!\n");
	char * secret_recv = getSecret(g_name_send);
	if (secret_recv != NULL) {
		printf("Secret: %s\n", secret_recv);
	}
	free(secret_recv);
	secret_recv = getSecret(g_name_cmp);
	if (secret_recv != NULL) {
		printf("Secret: %s\n", secret_recv);
	}
	free(secret_recv);
	deleteGroup(g_name_cmp);
	deleteGroup(g_name_send);

	if (close(sfd) == -1) {
		printf("Client: Error in socket creation\n");
		exit(-1);
	}

	return 0;

}