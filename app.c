#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/un.h>
#include <string.h>
#include <pthread.h>
#include "KVS-lib.h"

#define SV_SOCK_PATH "/tmp/server_sock"
#define SV_SOCK_PATH_CB "/tmp/server_sock_cb"
#define BUF_SIZE 100

int cfd = -1;
struct sockaddr_un cl_addr;
struct sockaddr_un sv_addr;
pthread_t callback_pid;
int cfd_cb = -1;
struct sockaddr_un cl_addr_cb;
struct sockaddr_un sv_addr_cb;
int exit_flag = 0;

void callback_function(char * changed_key) {
	printf("The key %s was changed\n", changed_key);
}

void * thread_f(void * arg) {

	if (pthread_detach(pthread_self()) != 0) {
		printf("App: Error in 'pthread_detach'\n");
	}

	char str[BUF_SIZE] = {0};
	char g_name[BUF_SIZE] = {0};
	char secret[BUF_SIZE] = {0};
	char key[BUF_SIZE] = {0};
	char value[BUF_SIZE] = {0};
	char * value_in;

	size_t len;
	while (1) {
		fgets(str, sizeof(str), stdin);
		if (strcmp(str, "establish_connection\n") == 0) {
			printf("Insert group id:\n");
			fgets(g_name, sizeof(g_name), stdin);
			len = strlen(g_name);
			if (len > 0 && g_name[len-1] == '\n') {
			  g_name[--len] = '\0';
			}
			printf("Insert secret:\n");
			fgets(secret, sizeof(secret), stdin);
			len = strlen(secret);
			if (len > 0 && secret[len-1] == '\n') {
			  secret[--len] = '\0';
			}
			if (establish_connection(g_name, secret) == 0)
				printf("Successful 'establish_connection'\n");
			else
				printf("Error in 'establish_connection'\n");
		}
		else if (strcmp(str, "put_value\n") == 0) {
			printf("Insert key:\n");
			fgets(key, sizeof(key), stdin);
			len = strlen(key);
			if (len > 0 && key[len-1] == '\n') {
			  key[--len] = '\0';
			}
			printf("Insert value:\n");
			fgets(value, sizeof(value), stdin);
			len = strlen(value);
			if (len > 0 && value[len-1] == '\n') {
			  value[--len] = '\0';
			}
			if (put_value(key, value) == 1)
				printf("Successful 'put_value'\n");
			else
				printf("Error in 'put_value'\n");
		}
		else if (strcmp(str, "get_value\n") == 0) {
			printf("Insert key:\n");
			fgets(key, sizeof(key), stdin);
			len = strlen(key);
			if (len > 0 && key[len-1] == '\n') {
			  key[--len] = '\0';
			}
			if (get_value(key, &value_in) == 1) {
				printf("Successful 'get_value'\n");
				printf("Value: %s\n", value_in);
				free(value_in);
			}
			else
				printf("Error in 'get_value'\n");
		}
		else if (strcmp(str, "delete_value\n") == 0) {
			printf("Insert key:\n");
			fgets(key, sizeof(key), stdin);
			len = strlen(key);
			if (len > 0 && key[len-1] == '\n') {
			  key[--len] = '\0';
			}
			if (delete_value(key) == 1) {
				printf("Successful 'delete_value'\n");
			}
			else
				printf("Error in 'delete_value'\n");
		}
		else if (strcmp(str, "register_callback\n") == 0) {
			printf("Insert key:\n");
			fgets(key, sizeof(key), stdin);
			len = strlen(key);
			if (len > 0 && key[len-1] == '\n') {
			  key[--len] = '\0';
			}
			if (register_callback(key, &callback_function) == 1) {
				printf("Successful 'register_callback'\n");
			}
			else
				printf("Error in 'register_callback'\n");
		}
		else if (strcmp(str, "close_connection\n") == 0) {
			if (close_connection() == 1) {
				printf("Successful 'close_connection'\n");
			}
			else
				printf("Error in 'close_connection'\n");
			pthread_cancel(callback_pid);
			exit_flag = 2;
			break;
		}
		else
			printf("Unknown Command\n");
	}

	pthread_exit(NULL);
}

int main(int argc, char *argv[]) {


	pthread_t pid;
	pthread_create(&pid, NULL, thread_f, NULL);

	while(exit_flag == 0);

	if (exit_flag == 1) {
		pthread_cancel(pid);
	}

	return 0;

}
