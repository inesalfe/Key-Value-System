#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/un.h>
#include <string.h>
#include <pthread.h>
#include "KVS-lib.h"

// Path for the Local Server adress
#define SV_SOCK_PATH "/tmp/server_sock"
// Path for the Local Server adress for the callback thread
#define SV_SOCK_PATH_CB "/tmp/server_sock_cb"
// Maximum length for strings (key / value / secret / group id)
#define BUF_SIZE 100

// File descriptor for the UNIX socket
int cfd = -1;
// App adress
struct sockaddr_un cl_addr;
// Local Server adress
struct sockaddr_un sv_addr;
// Thread id for the callback thread
pthread_t callback_tid;
// File descript for the UNIX socket used for int the callback thread
int cfd_cb = -1;
// App adress for the callback thread
struct sockaddr_un cl_addr_cb;
// Local Server adress for the callback thread
struct sockaddr_un sv_addr_cb;
// Exit flag used to close the sockets properly
int exit_flag = 0;

// Example of a callback function
void callback_function(char * changed_key) {
	printf("The key %s was changed\n", changed_key);
}

void * thread_f(void * arg) {

	// Detaching the thread so that the resources can be released in case of use of pthread_cancel
	if (pthread_detach(pthread_self()) != 0) {
		printf("App: Error in 'pthread_detach'\n");
	}

	// String from the keyboard
	char str[BUF_SIZE] = {0};
	// Group id
	char g_name[BUF_SIZE] = {0};
	// Secret
	char secret[BUF_SIZE] = {0};
	// Key
	char key[BUF_SIZE] = {0};
	// Value
	char value[BUF_SIZE] = {0};
	// Received Value
	char * value_in;

	size_t len;
	while (1) {
		// Getting string from the keyboard
		fgets(str, sizeof(str), stdin);
		if (strcmp(str, "establish_connection\n") == 0) {
			// Getting group id from the keyboard
			printf("Insert group id:\n");
			fgets(g_name, sizeof(g_name), stdin);
			// Eliminating the new line character
			len = strlen(g_name);
			if (len > 0 && g_name[len-1] == '\n') {
			  g_name[--len] = '\0';
			}
			// Getting the secret from the keyboard
			printf("Insert secret:\n");
			fgets(secret, sizeof(secret), stdin);
			// Eliminating the new line character
			len = strlen(secret);
			if (len > 0 && secret[len-1] == '\n') {
			  secret[--len] = '\0';
			}
			// Calling function from the KVS-lib library
			if (establish_connection(g_name, secret) == 0)
				printf("Successful 'establish_connection'\n");
			else {
				printf("Error in 'establish_connection'\n");	
			}
		}
		else if (strcmp(str, "put_value\n") == 0) {
			// Getting the key from the keyboard
			printf("Insert key:\n");
			fgets(key, sizeof(key), stdin);
			// Eliminating the new line character
			len = strlen(key);
			if (len > 0 && key[len-1] == '\n') {
			  key[--len] = '\0';
			}
			// Getting the value from the keyboard
			printf("Insert value:\n");
			fgets(value, sizeof(value), stdin);
			// Eliminating the new line character
			len = strlen(value);
			if (len > 0 && value[len-1] == '\n') {
			  value[--len] = '\0';
			}
			// Calling function from the KVS-lib library
			if (put_value(key, value) == 1)
				printf("Successful 'put_value'\n");
			else
				printf("Error in 'put_value'\n");
		}
		else if (strcmp(str, "get_value\n") == 0) {
			// Getting the key from the keyboard
			printf("Insert key:\n");
			fgets(key, sizeof(key), stdin);
			// Eliminating the new line character
			len = strlen(key);
			if (len > 0 && key[len-1] == '\n') {
			  key[--len] = '\0';
			}
			// Calling function from the KVS-lib library
			if (get_value(key, &value_in) == 1) {
				printf("Successful 'get_value'\n");
				printf("Value: %s\n", value_in);
				free(value_in);
			}
			else
				printf("Error in 'get_value'\n");
		}
		else if (strcmp(str, "delete_value\n") == 0) {
			// Getting the key from the keyboard
			printf("Insert key:\n");
			fgets(key, sizeof(key), stdin);
			// Eliminating the new line character
			len = strlen(key);
			if (len > 0 && key[len-1] == '\n') {
			  key[--len] = '\0';
			}
			// Calling function from the KVS-lib library
			if (delete_value(key) == 1) {
				printf("Successful 'delete_value'\n");
			}
			else
				printf("Error in 'delete_value'\n");
		}
		else if (strcmp(str, "register_callback\n") == 0) {
			// Getting the key from the keyboard
			printf("Insert key:\n");
			fgets(key, sizeof(key), stdin);
			// Eliminating the new line character
			len = strlen(key);
			if (len > 0 && key[len-1] == '\n') {
			  key[--len] = '\0';
			}
			// Calling function from the KVS-lib library
			if (register_callback(key, &callback_function) == 1) {
				printf("Successful 'register_callback'\n");
			}
			else
				printf("Error in 'register_callback'\n");
		}
		else if (strcmp(str, "close_connection\n") == 0) {
			// Canceling the callback thread
			pthread_cancel(callback_tid);
			// Calling function from the KVS-lib library
			if (close_connection() == 1) {
				printf("Successful 'close_connection'\n");
			}
			else
				printf("Error in 'close_connection'\n");
			// Setting the exit flag so that the cicle in the main function can be breaked
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
	// Create thread to handle the incoming requests from the keyboard
	pthread_create(&pid, NULL, thread_f, NULL);

	// Waiting for either the user inputs close_connection, or the the local server deletes a group, ou the local server disconnects
	while(exit_flag == 0);

	// If the we are exiting due to a received flag in the callback thread, we need to cancel the thread that is still reading inputs from the keyboard
	if (exit_flag == 1) {
		pthread_cancel(pid);
	}

	printf("Exiting the app...\n");

	return 0;

}
