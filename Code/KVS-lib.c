#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#define SV_SOCK_PATH "/tmp/server_sock"
#define SV_SOCK_PATH_CB "/tmp/server_sock_cb"

#define BUF_SIZE 100

// File descriptor for the UNIX socket
extern int cfd;
// App adress
extern struct sockaddr_un cl_addr;
// Local Server adress
extern struct sockaddr_un sv_addr;
// Thread id for the callback thread
extern pthread_t callback_tid;
// File descript for the UNIX socket used for int the callback thread
extern int cfd_cb;
// App adress for the callback thread
extern struct sockaddr_un cl_addr_cb;
// Local Server adress for the callback thread
extern struct sockaddr_un sv_addr_cb;
// Exit flag used to close the sockets properly
extern int exit_flag;
// Thread ID of the thread that handles the incoming requests from the keyboard
extern pthread_t tid;

// Shortcut to define the structure of the callback function
typedef void (*callback)(char*);

// Node to be used in the linked list - contains the key and the callback function
struct cb_info {
	char key[BUF_SIZE];
	callback cb;
	struct cb_info * next;
};

// Head node
struct cb_info * cb_list = NULL;

int close_connection() {

	// Function code definition so that the local server knows which set of instructions to execute
	int func_code = 4;

	// Free the linked list
	struct cb_info * current = cb_list;
	struct cb_info * next;

	while (current != NULL)
	{
		next = current->next;
		free(current);
		current = next;
	}

	cb_list = NULL;

	if (cfd == -1) {
		printf("App: Error in socket creation / Socket not created\n");
		return -2;
	}

	// Send the function name to the server
	if (send(cfd, &func_code, sizeof(int), 0) != sizeof(int)) {
		printf("App: Error in sending function code\n");
		printf("The error message is: %s\n", strerror(errno));
		return -3;
	}

	// Close file descriptor
	if (close(cfd) == -1) {
		printf("App: Error in closing socket\n");
		printf("The error message is: %s\n", strerror(errno));
		return -1;
	}

	// Close callback thread file descriptor
	if (close(cfd_cb) == -1) {
		printf("App: Error in closing socket\n");
		printf("The error message is: %s\n", strerror(errno));
		return -1;
	}	

	// Remove sockets paths to avoid binding problems in the next execution
	remove(cl_addr.sun_path);
	remove(cl_addr_cb.sun_path);

	return 1;

}

void * callback_thread(void * arg) {

	// To use the pthread cancel in the main function we need to use detach first, to that the memory can be correctly released
	if (pthread_detach(pthread_self()) != 0) {
		printf("App: Error in 'pthread_detach'\n");
		printf("The error message is: %s\n", strerror(errno));
	}

	// Number of received bytes - will be used to check for errors
	size_t numBytes;
	// Flag to check if we are to execute a callback function or to close the app
	int flag = 0;
	// Received changed key
	char changed_key[BUF_SIZE];
	while(1) {
		numBytes = recv(cfd_cb, &flag, sizeof(int), 0);
		if (numBytes == -1) {
			printf("App: Error in receiving flag\n");
			printf("The error message is: %s\n", strerror(errno));
			break;
		}
		if (flag == 1) {
			int ready_flag = 1;
			printf("App: Received flag for changed key\n");
			// Send ready flag saying that the app is ready to receive the changed key
			if (send(cfd_cb, &ready_flag, sizeof(int), 0) != sizeof(int)) {
				printf("App: Error in sending ready flag\n");
				printf("The error message is: %s\n", strerror(errno));
				break;
			}
			// Receive the changed key
			numBytes = recv(cfd_cb, changed_key, sizeof(changed_key), 0);
			if (numBytes == -1) {
				printf("App: Error in receiving changed flag\n");
				printf("The error message is: %s\n", strerror(errno));
				break;
			}
			// Search for the changed key in the callback linked list so that we know which function to execute
			struct cb_info * current = cb_list;
			while (current != NULL)
			{
				if (strcmp(current->key, changed_key) == 0) {
					// Call the callback function for the changed key
					current->cb(changed_key);
				}
				current = current->next;
			}
			// Clear changed key
			memset(changed_key, 0, sizeof(changed_key));
		}
		else if (flag == -1) {
			// Closing the connection
			pthread_cancel(tid);
			int sucess = close_connection();
			if (sucess == 1) {
				printf("App: Closing connection due to deletion of group or closed server\n");
			}
			else {
				printf("App: Error in closing connection due to deletion of group or closed server\n");
			}
			exit_flag = 1;
			break;
		}
		// Reset flag
		flag = 0;
	}

	pthread_exit(NULL);
}

int establish_connection (char * group_id, char * secret) {

	if (cfd != -1) {
		printf("App: Error - Connection already established\n");
		return -16;
	}

	// Assignment of server address
	cfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (cfd == -1) {
		printf("App: Error in socket creation / Socket not created\n");
		printf("The error message is: %s\n", strerror(errno));
		return -2;
	}

	// Server address assignment
	memset(&sv_addr, 0, sizeof(struct sockaddr_un));
	sv_addr.sun_family = AF_UNIX;
	strncpy(sv_addr.sun_path, SV_SOCK_PATH, sizeof(sv_addr.sun_path) - 1);

	// App address assignment with PID
	memset(&cl_addr, 0, sizeof(struct sockaddr_un));
	cl_addr.sun_family = AF_UNIX;
	snprintf(cl_addr.sun_path, sizeof(cl_addr.sun_path), "/tmp/app_socket_%ld", (long) getpid());

	// Unlink the app path before binding
	remove(cl_addr.sun_path);
	unlink(cl_addr.sun_path);

	// Bind app
	if (bind(cfd, (struct sockaddr *) &cl_addr, sizeof(struct sockaddr_un)) == -1) {
		printf("App: Error in binding\n");
		printf("The error message is: %s\n", strerror(errno));
		if (close(cfd) == -1) {
			printf("App: Error in closing socket\n");
			printf("The error message is: %s\n", strerror(errno));
			return -1;
		}
		cfd = -1;
		return -11;
	}

	// Connect to server
	if (connect(cfd, (struct sockaddr *) &sv_addr, sizeof(struct sockaddr_un)) == -1) {
		printf("App: Error in connect\n");
		printf("The error message is: %s\n", strerror(errno));
		if (close(cfd) == -1) {
			printf("App: Error in closing socket\n");
			printf("The error message is: %s\n", strerror(errno));
			return -1;
		}
		cfd = -1;
		return -12;
	}

	// Number of received bytes - will be used to check for errors
	ssize_t numBytes;
	int check_connection = -1;
	int check_group = -1;
	int check_secret = -1;

	// Wait for flag saying that the connectiong was established
	numBytes = recv(cfd, &check_connection, sizeof(int), 0);
	if (numBytes == -1) {
		printf("App: Error in receiving response for the established connection\n");
		printf("The error message is: %s\n", strerror(errno));
		if (close(cfd) == -1) {
			printf("App: Error in closing socket\n");
			printf("The error message is: %s\n", strerror(errno));
			return -1;
		}
		cfd = -1;
		return -13;
	}

	// If the connection flag is not 1, the connection was not established
	if (check_connection != 1) {
		printf("App: Error in establishing connection to the server\n");
		if (close(cfd) == -1) {
			printf("App: Error in closing socket\n");
			printf("The error message is: %s\n", strerror(errno));
			return -1;
		}
		cfd = -1;
		return -14;
	}

	// Sending the group_id to the server
	if (send(cfd, group_id, strlen(group_id), 0) != strlen(group_id)) {
		if (close(cfd) == -1) {
			printf("App: Error in closing socket\n");
			printf("The error message is: %s\n", strerror(errno));
			return -1;
		}
		cfd = -1;
		printf("App: Error in sending key / group_id / value / secret\n");
		printf("The error message is: %s\n", strerror(errno));
		return -6;
	}

	// receiving flag saying if the group_id is correct
	numBytes = recv(cfd, &check_group, sizeof(int), 0);
	if (numBytes == -1) {
		printf("App: Error in receiving response for the sent key / group_id / secret\n");
		printf("The error message is: %s\n", strerror(errno));
		if (close(cfd) == -1) {
			printf("App: Error in closing socket\n");
			printf("The error message is: %s\n", strerror(errno));
			return -1;
		}
		cfd = -1;
		return -8;
	}

	// Wrong group
	if (check_group != 1) {
		if (close(cfd) == -1) {
			printf("App: Error in closing socket\n");
			printf("The error message is: %s\n", strerror(errno));
			return -1;
		}
		cfd = -1;
		printf("App: Key / group_id not existing\n");
		return -7;
	}

	// Sending secret
	if (send(cfd, secret, strlen(secret), 0) != strlen(secret)) {
		if (close(cfd) == -1) {
			printf("App: Error in closing socket\n");
			printf("The error message is: %s\n", strerror(errno));
			return -1;
		}
		cfd = -1;
		printf("App: Error in sending key / group_id / value / secret\n");
		printf("The error message is: %s\n", strerror(errno));
		return -6;
	}

	// Avois using the same socket
	int enable = 1;
	if (setsockopt(cfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
	    printf("setsockopt(SO_REUSEADDR) failed");
	    printf("The error message is: %s\n", strerror(errno));
	    if (close(cfd) == -1) {
			printf("App: Error in closing socket\n");
			printf("The error message is: %s\n", strerror(errno));
			return -1;
		}
		cfd = -1;
	    return -15;
	}

	// Assignment of socket for the callback thread
	cfd_cb = socket(AF_UNIX, SOCK_STREAM, 0);
	if (cfd_cb == -1) {
		printf("App: Error in socket creation / Socket not created\n");
		printf("The error message is: %s\n", strerror(errno));
		return -2;
	}

	// App address assignment with PID for the callback thread
	memset(&cl_addr_cb, 0, sizeof(struct sockaddr_un));
	cl_addr_cb.sun_family = AF_UNIX;
	snprintf(cl_addr_cb.sun_path, sizeof(cl_addr_cb.sun_path), "/tmp/app_socket_cb_%ld", (long) getpid());

	// Unlink app address before bind
	remove(cl_addr_cb.sun_path);
	unlink(cl_addr_cb.sun_path);

	// Bind app
	if (bind(cfd_cb, (struct sockaddr *) &cl_addr_cb, sizeof(struct sockaddr_un)) == -1) {
		printf("App: Error in binding\n");
		printf("The error message is: %s\n", strerror(errno));
		if (close(cfd_cb) == -1) {
			printf("App: Error in closing socket\n");
			printf("The error message is: %s\n", strerror(errno));
			return -1;
		}
		cfd_cb = -1;
		return -11;
	}

	// Server address assignment
	memset(&sv_addr_cb, 0, sizeof(struct sockaddr_un));
	sv_addr_cb.sun_family = AF_UNIX;
	strncpy(sv_addr_cb.sun_path, SV_SOCK_PATH_CB, sizeof(sv_addr_cb.sun_path) - 1);

	// Connect to server
	// After 5 tries we output the error
	int counter = 0;
	int sucess_flag = connect(cfd_cb, (struct sockaddr *) &sv_addr_cb, sizeof(struct sockaddr_un));
	while(sucess_flag == -1) {
		if (counter > 5) {
			printf("App: Error in connect\n");
			// printf("Value of errno: %d\n", errno);
			printf("The error message is: %s\n", strerror(errno));
			// perror("Message from perror");
			if (close(cfd_cb) == -1 || close(cfd) == -1) {
				printf("App: Error in closing socket\n");
				printf("The error message is: %s\n", strerror(errno));
				exit(-1);
			}
			exit(-1);
		}
		counter++;
		// Small wait before another try
		usleep(50);
		sucess_flag = connect(cfd_cb, (struct sockaddr *) &sv_addr_cb, sizeof(struct sockaddr_un));sucess_flag = connect(cfd_cb, (struct sockaddr *) &sv_addr_cb, sizeof(struct sockaddr_un));
	}

	// Reuse of the check_connection flag
	check_connection = -1;

	// Wait for flag saying that the connectiong was established
	numBytes = recv(cfd_cb, &check_connection, sizeof(int), 0);
	if (numBytes == -1) {
		printf("App: Error in receiving response for the established connection\n");
		printf("The error message is: %s\n", strerror(errno));
		if (close(cfd) == -1 || close(cfd_cb) == -1) {
			printf("App: Error in closing socket\n");
			printf("The error message is: %s\n", strerror(errno));
			return -1;
		}
		cfd_cb = -1;
		return -13;
	}

	// If the connection flag is not 1, the connection was not established
	if (check_connection != 1) {
		printf("App: Error in establishing connection to the server\n");
		if (close(cfd_cb) == -1 || close(cfd) == -1) {
			printf("App: Error in closing socket\n");
			printf("The error message is: %s\n", strerror(errno));
			return -1;
		}
		cfd_cb = -1;
		return -14;
	}

	// Receiving flag for the secret
	numBytes = recv(cfd, &check_secret, sizeof(int), 0);
	if (numBytes == -1) {
		printf("App: Error in receiving response for the sent key / group_id / secret\n");
		printf("The error message is: %s\n", strerror(errno));
		if (close(cfd) == -1 || close(cfd_cb) == -1) {
			printf("App: Error in closing socket\n");
			printf("The error message is: %s\n", strerror(errno));
			return -1;
		}
		cfd_cb = -1;
		return -8;
	}
	// Incorrect Secret
	if (check_secret != 1) {
		if (close(cfd) == -1 || close(cfd_cb) == -1) {
			printf("App: Error in closing socket\n");
			printf("The error message is: %s\n", strerror(errno));
			return -1;
		}
		cfd = -1;
		printf("App: Incorrect secret\n");
		return -9;
	}

	// Create second thread for the callback function
	pthread_create(&callback_tid, NULL, callback_thread, NULL);

	// Print app PID
	printf("App PID: %ld\n", (long) getpid());

	return 0;

}

int put_value (char * key, char * value) {

	if (cfd == -1) {
		printf("App: Error in socket creation / Socket not created\n");
		return -2;
	}

	// Function code definition so that the local server knows which set of instructions to execute
	int func_code = 0;

	// Send the function name to the server
	if (send(cfd, &func_code, sizeof(int), 0) != sizeof(int)) {
		printf("App: Error in sending function code\n");
		printf("The error message is: %s\n", strerror(errno));
		return -3;
	}

	// Ready flag
	int ready = -1;
	// Number of received bytes - will be used to check for errors
	ssize_t numBytes;

	// See if server is ready to send the key
	numBytes = recv(cfd, &ready, sizeof(int), 0);
	if (numBytes == -1) {
		printf("App: Error in receiving ready flag\n");
		printf("The error message is: %s\n", strerror(errno));
		return -4;
	}

	if (ready != 1) {
		printf("App: Server not ready\n");
		return -4;
	}

	// Send the key
	if (send(cfd, key, strlen(key), 0) != strlen(key)) {
		printf("App: Error in sending key / group_id / value / secret\n");
		printf("The error message is: %s\n", strerror(errno));
		return -6;
	}

	// Reuse of the ready flag to see if the server is ready for the sending of the value
	ready = -1;

	// See if server is ready to send the key
	numBytes = recv(cfd, &ready, sizeof(int), 0);
	if (numBytes == -1) {
		printf("App: Error in receiving ready flag\n");
		printf("The error message is: %s\n", strerror(errno));
		return -4;
	}

	if (ready != 1) {
		printf("App: Server not ready\n");
		return -5;
	}

	// Sending the value
	if (send(cfd, value, strlen(value), 0) != strlen(value)) {
		printf("App: Error in sending key / group_id / value / secret\n");
		printf("The error message is: %s\n", strerror(errno));
		return -6;
	}

	return 1;

}

int get_value (char * key, char ** value) {

	if (cfd == -1) {
		printf("App: Error in socket creation / Socket not created\n");
		return -2;
	}

	// Function code definition so that the local server knows which set of instructions to execute
	int func_code = 1;

	// Send the function name to the server
	if (send(cfd, &func_code, sizeof(int), 0) != sizeof(int)) {
		printf("App: Error in sending function code\n");
		printf("The error message is: %s\n", strerror(errno));
		return -3;
	}

	// Ready flag
	int ready = -1;
	// Length of the value so that we can allocate memory
	int length = -1;
	// Number of received bytes - will be used to check for errors
	ssize_t numBytes;

	// See if server is ready to send the key
	numBytes = recv(cfd, &ready, sizeof(int), 0);
	if (numBytes == -1) {
		printf("App: Error in receiving ready flag\n");
		printf("The error message is: %s\n", strerror(errno));
		return -4;
	}

	if (ready != 1) {
		printf("App: Server not ready\n");
		return -5;
	}

	// Send the key
	if (send(cfd, key, strlen(key), 0) != strlen(key)) {
		printf("App: Error in sending key / group_id / value / secret\n");
		printf("The error message is: %s\n", strerror(errno));
		return -6;
	}

	// See if server is ready to send the key
	numBytes = recv(cfd, &length, sizeof(int), 0);
	if (numBytes == -1) {
		printf("App: Error in receiving ready flag\n");
		printf("The error message is: %s\n", strerror(errno));
		return -4;
	}

	if (length == -1) {
		printf("App: Key / group_id not existing\n");
		return -7;
	}

	*value = (char *) malloc((length+1)*sizeof(char));

	numBytes = recv(cfd, *value, (length+1)*sizeof(char), 0);
	if (numBytes == -1) {
		printf("App: Error in receiving values\n");
		printf("The error message is: %s\n", strerror(errno));
		return -10;
	}

	return 1;

}

int delete_value (char * key) {

	if (cfd == -1) {
		printf("App: Error in socket creation / Socket not created\n");
		return -2;
	}

	// Function code definition so that the local server knows which set of instructions to execute
	int func_code = 2;

	// Send the function code to the server
	if (send(cfd, &func_code, sizeof(int), 0) != sizeof(int)) {
		printf("App: Error in sending function code\n");
		printf("The error message is: %s\n", strerror(errno));
		return -3;
	}

	// Ready flag
	int ready = -1;
	// Check key flag
	int check_key = -1;
	// Number of received bytes - will be used to check for errors
	ssize_t numBytes;

	// See if server is ready to receive the key
	numBytes = recv(cfd, &ready, sizeof(int), 0);
	if (numBytes == -1) {
		printf("App: Error in receiving ready flag\n");
		printf("The error message is: %s\n", strerror(errno));
		return -4;
	}

	if (ready != 1) {
		printf("App: Server not ready\n");
		printf("The error message is: %s\n", strerror(errno));
		return -5;
	}

	// Send the key
	if (send(cfd, key, strlen(key), 0) != strlen(key)) {
		printf("App: Error in sending key / group_id / value / secret\n");
		printf("The error message is: %s\n", strerror(errno));
		return -6;
	}

	// Receive flag saying if the key is an existing one or not
	numBytes = recv(cfd, &check_key, sizeof(int), 0);
	if (numBytes == -1) {
		printf("App: Error in receiving response for the sent key / group_id / secret\n");
		printf("The error message is: %s\n", strerror(errno));
		return -8;
	}

	// Error for non existing key
	if (check_key != 1) {
		printf("App: Key / group_id not existing\n");
		return -7;
	}

	// If the deleted key was in the callback list we need to delete it

	struct cb_info * temp = cb_list, * prev;

	if (temp != NULL && (strcmp(temp->key, key)==0)) {
		cb_list = temp->next;
		free(temp);
		return 1;
	}

	while (temp != NULL && (strcmp(temp->key, key)!=0)) {
		prev = temp;
		temp = temp->next;
	}

	if (temp == NULL)
		return 1;

	prev->next = temp->next;

	free(temp);

	return 1;

}

int register_callback (char * key, void (*callback_function)(char *)) {

	if (cfd == -1) {
		printf("App: Error in socket creation / Socket not created\n");
		return -2;
	}

	// Function code definition so that the local server knows which set of instructions to execute
	int func_code = 3;

	// Send the function code to the server
	if (send(cfd, &func_code, sizeof(int), 0) != sizeof(int)) {
		printf("App: Error in sending function code\n");
		printf("The error message is: %s\n", strerror(errno));
		return -3;
	}

	// Ready flag
	int ready = -1;
	// Check key flag
	int check_key = -1;
	// Number of received bytes - will be used to check for errors
	ssize_t numBytes;

	// See if server is ready to receive the key
	numBytes = recv(cfd, &ready, sizeof(int), 0);
	if (numBytes == -1) {
		printf("App: Error in receiving ready flag\n");
		printf("The error message is: %s\n", strerror(errno));
		return -4;
	}

	if (ready != 1) {
		printf("App: Server not ready\n");
		return -5;
	}

	// Send the key
	if (send(cfd, key, strlen(key), 0) != strlen(key)) {
		printf("App: Error in sending key / group_id / value / secret\n");
		printf("The error message is: %s\n", strerror(errno));
		return -6;
	}

	// Receive flag saying if the key is an existing one or not
	numBytes = recv(cfd, &check_key, sizeof(int), 0);
	if (numBytes == -1) {
		printf("App: Error in receiving response for the sent key / group_id / secret\n");
		printf("The error message is: %s\n", strerror(errno));
		return -8;
	}

	// Error for non existing key
	if (check_key != 1) {
		printf("App: Key / group_id not existing\n");
		return -7;
	}

	// The key may already existe in the callback list
	// We only have to change the callback function
	struct cb_info * current = cb_list;
	while (current != NULL)
	{
		if (strcmp(current->key, key) == 0) {
			current->cb = callback_function;
			return 1;
		}
		current = current->next;
	}

	// If it is not in the list, we need to add it

	struct cb_info * new_node = (struct cb_info *) calloc(1, sizeof(struct cb_info));
	struct cb_info * last = cb_list;

	strcpy(new_node->key, key);
	new_node->cb = callback_function;
	new_node->next = NULL;

	if (cb_list == NULL)
	{
		cb_list = new_node;
		return 1;
	}

	while (last->next != NULL)
		last = last->next;

	last->next = new_node;

	return 1;

}

/* Códigos e erros (-1 a -16)

printf("App: Error in closing socket\n");
printf("App: Error in socket creation / Socket not created\n");
printf("App: Error in sending function code\n");
printf("App: Error in receiving ready flag\n");
printf("App: Server not ready\n");
printf("App: Error in sending key / group_id / value / secret\n");
printf("App: Key / group_id not existing\n");
printf("App: Error in receiving response for the sent key / group_id / secret\n");
printf("App: Incorrect secret\n");
printf("App: Error in receiving values\n");
printf("App: Error in binding\n");
printf("App: Error in connect\n");
printf("App: Error in receiving response for the established connection\n");
printf("App: Error in establishing connection to the server\n");
printf("App: Other error\n");
printf("App: Error - Connection already established\n");

*/
