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

extern int cfd;
extern struct sockaddr_un cl_addr;
extern struct sockaddr_un sv_addr;
extern pthread_t callback_pid;
extern int cfd_cb;
extern struct sockaddr_un cl_addr_cb;
extern struct sockaddr_un sv_addr_cb;
extern int exit_flag;

typedef void (*callback)(char*);

struct thread_args {
	char key[BUF_SIZE];
	callback cb;
	struct thread_args * next;
};

struct thread_args * cb_info = NULL;

int close_connection() {

	int func_code = 4;
	ssize_t numBytes;

	struct thread_args * current = cb_info;
	struct thread_args * next;

	while (current != NULL)
	{
		next = current->next;
		free(current);
		current = next;
	}

	cb_info = NULL;

	if (cfd == -1) {
		printf("App: Error in socket creation / Socket not created\n");
		return -2;
	}

	if (send(cfd, &func_code, sizeof(int), 0) != sizeof(int)) {
		printf("App: Error in sending function code\n");
		return -3;
	}

	if (close(cfd) == -1) {
		printf("App: Error in closing socket\n");
		return -1;
	}

	if (close(cfd_cb) == -1) {
		printf("App: Error in closing socket\n");
		return -1;
	}

	remove(cl_addr.sun_path);
	remove(cl_addr_cb.sun_path);

	return 1;

}

void * callback_thread(void * arg) {

	if (pthread_detach(pthread_self()) != 0) {
		printf("App: Error in 'pthread_detach'\n");
	}

	size_t numBytes;
	int flag = 0;
	char changed_key[BUF_SIZE];
	while(1) {
		numBytes = recv(cfd_cb, &flag, sizeof(int), 0);
		if (numBytes == -1) {
			printf("App: Error in receiving flag\n");
			break;
		}
		if (flag == 1) {
			int ready_flag = 1;
			printf("App: Received flag for changed key\n");
			if (send(cfd_cb, &ready_flag, sizeof(int), 0) != sizeof(int)) {
				printf("App: Error in sending ready flag\n");
				break;
			}
			numBytes = recv(cfd_cb, changed_key, sizeof(changed_key), 0);
			if (numBytes == -1) {
				printf("App: Error in receiving changed flag\n");
				break;
			}
			// printf("App: The changed key was %s\n", changed_key);
			struct thread_args * current = cb_info;
			while (current != NULL)
			{
				if (strcmp(current->key, changed_key) == 0) {
					current->cb(changed_key);
				}
				current = current->next;
			}
			memset(changed_key, 0, sizeof(changed_key));
		}
		else if (flag == -1) {
			int sucess = close_connection();
			if (sucess == 1) {
				printf("App: Closing connection due to deletion of group in server\n");
			}
			else {
				printf("App: Error in closing connection due to deletion of group in server\n");
			}
			exit_flag = 1;
			break;
		}
		flag = 0;
	}
	pthread_exit(NULL);
}

// Needs testing for different errors and use of sterror
int establish_connection (char * group_id, char * secret) {

	if (cfd != -1) {
		printf("App: Error - Connection already established\n");
		return -16;
	}

	// Assignment of server address
	cfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (cfd == -1) {
		printf("App: Error in socket creation / Socket not created\n");
		return -2;
	}

	// Clean socket path
	// remove(SV_SOCK_PATH);

	// Server address assignment
	memset(&sv_addr, 0, sizeof(struct sockaddr_un));
	sv_addr.sun_family = AF_UNIX;
	strncpy(sv_addr.sun_path, SV_SOCK_PATH, sizeof(sv_addr.sun_path) - 1);

	// App address assignment with PID
	memset(&cl_addr, 0, sizeof(struct sockaddr_un));
	cl_addr.sun_family = AF_UNIX;
	snprintf(cl_addr.sun_path, sizeof(cl_addr.sun_path), "/tmp/app_socket_%ld", (long) pthread_self());

	remove(cl_addr.sun_path);
	unlink(cl_addr.sun_path);

	// Bind app
	if (bind(cfd, (struct sockaddr *) &cl_addr, sizeof(struct sockaddr_un)) == -1) {
		printf("App: Error in binding\n");
		if (close(cfd) == -1) {
			printf("App: Error in closing socket\n");
			return -1;
		}
		cfd = -1;
		return -11;
	}

	// Connect to server
	if (connect(cfd, (struct sockaddr *) &sv_addr, sizeof(struct sockaddr_un)) == -1) {
		printf("App: Error in connect\n");
		if (close(cfd) == -1) {
			printf("App: Error in closing socket\n");
			return -1;
		}
		cfd = -1;
		return -12;
	}

	ssize_t numBytes;
	int check_connection = -1;
	int check_group = -1;
	int check_secret = -1;

	// Wait for flag saying that the connectiong was established
	numBytes = recv(cfd, &check_connection, sizeof(int), 0);
	if (numBytes == -1) {
		printf("App: Error in receiving response for the established connection\n");
		if (close(cfd) == -1) {
			printf("App: Error in closing socket\n");
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
			return -1;
		}
		cfd = -1;
		return -14;
	}

	// Sending the group_id to the server
	if (send(cfd, group_id, sizeof(group_id), 0) != sizeof(group_id)) {
		if (close(cfd) == -1) {
			printf("App: Error in closing socket\n");
			return -1;
		}
		cfd = -1;
		printf("App: Error in sending key / group_id / value / secret\n");
		return -6;
	}

	// receiving flag saying if the group_id is correct
	numBytes = recv(cfd, &check_group, sizeof(int), 0);
	if (numBytes == -1) {
		printf("App: Error in receiving response for the sent key / group_id / secret\n");
		if (close(cfd) == -1) {
			printf("App: Error in closing socket\n");
			return -1;
		}
		cfd = -1;
		return -8;
	}

	// Wrong group
	if (check_group != 1) {
		if (close(cfd) == -1) {
			printf("App: Error in closing socket\n");
			return -1;
		}
		cfd = -1;
		printf("App: Key / group_id not existing\n");
		return -7;
	}

	// Sending secret
	if (send(cfd, secret, sizeof(secret), 0) != sizeof(secret)) {
		if (close(cfd) == -1) {
			printf("App: Error in closing socket\n");
			return -1;
		}
		cfd = -1;
		printf("App: Error in sending key / group_id / value / secret\n");
		return -6;
	}

	int enable = 1;
	if (setsockopt(cfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
	    printf("setsockopt(SO_REUSEADDR) failed");
	    if (close(cfd) == -1) {
			printf("App: Error in closing socket\n");
			return -1;
		}
		cfd = -1;
	    return -15;
	}

	// printf("First connection established\n");

	// Assignment of server address
	cfd_cb = socket(AF_UNIX, SOCK_STREAM, 0);
	if (cfd_cb == -1) {
		printf("App: Error in socket creation / Socket not created\n");
		return -2;
	}

	printf("Callback socket: %d\n", cfd_cb);

	// App address assignment with PID
	memset(&cl_addr_cb, 0, sizeof(struct sockaddr_un));
	cl_addr_cb.sun_family = AF_UNIX;
	snprintf(cl_addr_cb.sun_path, sizeof(cl_addr_cb.sun_path), "/tmp/app_socket_cb_%ld", (long) pthread_self());
	// printf("Client address: %s\n", cl_addr_cb.sun_path);

	remove(cl_addr_cb.sun_path);
	unlink(cl_addr_cb.sun_path);

	// Bind app
	if (bind(cfd_cb, (struct sockaddr *) &cl_addr_cb, sizeof(struct sockaddr_un)) == -1) {
		printf("App: Error in binding\n");
		if (close(cfd_cb) == -1) {
			printf("App: Error in closing socket\n");
			return -1;
		}
		cfd_cb = -1;
		return -11;
	}

	// remove(SV_SOCK_PATH_CB);

	// Server address assignment
	memset(&sv_addr_cb, 0, sizeof(struct sockaddr_un));
	sv_addr_cb.sun_family = AF_UNIX;
	strncpy(sv_addr_cb.sun_path, SV_SOCK_PATH_CB, sizeof(sv_addr_cb.sun_path) - 1);
	// printf("Server address: %s\n", sv_addr_cb.sun_path);
	
	// usleep(50);

	// Connect to server
	int counter = 0;
	int sucess_flag = connect(cfd_cb, (struct sockaddr *) &sv_addr_cb, sizeof(struct sockaddr_un));
	while(sucess_flag == -1) {
		if (counter > 5) {
			printf("App: Error in connect\n");
			printf("Value of errno: %d\n", errno);
			printf("The error message is: %s\n", strerror(errno));
			perror("Message from perror");
			if (close(cfd_cb) == -1) {
				printf("App: Error in closing socket\n");
				exit(-1);
			}
			exit(-1);
		}
		counter++;
		usleep(50);
		sucess_flag = connect(cfd_cb, (struct sockaddr *) &sv_addr_cb, sizeof(struct sockaddr_un));sucess_flag = connect(cfd_cb, (struct sockaddr *) &sv_addr_cb, sizeof(struct sockaddr_un));
	}

	// printf("Connection accepted\n");

	check_connection = -1;

	// Wait for flag saying that the connectiong was established
	numBytes = recv(cfd_cb, &check_connection, sizeof(int), 0);
	if (numBytes == -1) {
		printf("App: Error in receiving response for the established connection\n");
		if (close(cfd_cb) == -1) {
			printf("App: Error in closing socket\n");
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
			return -1;
		}
		cfd_cb = -1;
		return -14;
	}

	// receiving flag for the secret
	numBytes = recv(cfd, &check_secret, sizeof(int), 0);
	if (numBytes == -1) {
		printf("App: Error in receiving response for the sent key / group_id / secret\n");
		if (close(cfd_cb) == -1) {
			printf("App: Error in closing socket\n");
			return -1;
		}
		cfd_cb = -1;		
		return -8;
	}
	if (check_secret != 1) {
		if (close(cfd) == -1 || close(cfd_cb) == -1) {
			printf("App: Error in closing socket\n");
			return -1;
		}
		cfd = -1;
		printf("App: Incorrect secret\n");
		return -9;
	}

	pthread_create(&callback_pid, NULL, callback_thread, NULL);
	return 0;

}

// Needs testing for different errors and use of sterror
int put_value (char * key, char * value) {

	if (cfd == -1) {
		printf("App: Error in socket creation / Socket not created\n");
		return -2;
	}

	int func_code = 0;

	// Send the function name to the server
	if (send(cfd, &func_code, sizeof(int), 0) != sizeof(int)) {
		printf("App: Error in sending function code\n");
		return -3;
	}

	int ready = -1;
	ssize_t numBytes;

	// See if server is ready to send the key
	numBytes = recv(cfd, &ready, sizeof(int), 0);
	if (numBytes == -1) {
		printf("App: Error in receiving ready flag\n");
		return -4;
	}

	if (ready != 1) {
		printf("App: Server not ready\n");
		return -4;
	}

	// Send the key
	if (send(cfd, key, sizeof(key), 0) != sizeof(key)) {
		printf("App: Error in sending key / group_id / value / secret\n");
		return -6;
	}

	// Reuse of the ready flag to see if the server is ready for the sending of the value
	ready = -1;

	// See if server is ready to send the key
	numBytes = recv(cfd, &ready, sizeof(int), 0);
	if (numBytes == -1) {
		printf("App: Error in receiving ready flag\n");
		return -4;
	}

	if (ready != 1) {
		printf("App: Server not ready\n");
		return -5;
	}

	// Sending the value
	if (send(cfd, value, sizeof(value), 0) != sizeof(value)) {
		printf("App: Error in sending key / group_id / value / secret\n");
		return -6;
	}

	return 1;

}

int get_value (char * key, char ** value) {

	if (cfd == -1) {
		printf("App: Error in socket creation / Socket not created\n");
		return -2;
	}

	int func_code = 1;

	// Send the function name to the server
	if (send(cfd, &func_code, sizeof(int), 0) != sizeof(int)) {
		printf("App: Error in sending function code\n");
		return -3;
	}

	int ready = -1;
	int length = -1;
	ssize_t numBytes;

	// See if server is ready to send the key
	numBytes = recv(cfd, &ready, sizeof(int), 0);
	if (numBytes == -1) {
		printf("App: Error in receiving ready flag\n");
		return -4;
	}

	if (ready != 1) {
		printf("App: Server not ready\n");
		return -5;
	}

	// Send the key
	if (send(cfd, key, sizeof(key), 0) != sizeof(key)) {
		printf("App: Error in sending key / group_id / value / secret\n");
		return -6;
	}

	// See if server is ready to send the key
	numBytes = recv(cfd, &length, sizeof(int), 0);
	if (numBytes == -1) {
		printf("App: Error in receiving ready flag\n");
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
		return -10;
	}

	return 1;

}

int delete_value (char * key) {

	if (cfd == -1) {
		printf("App: Error in socket creation / Socket not created\n");
		return -2;
	}

	int func_code = 2;

	// Send the function code to the server
	if (send(cfd, &func_code, sizeof(int), 0) != sizeof(int)) {
		printf("App: Error in sending function code\n");
		return -3;
	}

	int ready = -1;
	int check_key = -1;
	ssize_t numBytes;

	// See if server is ready to receive the key
	numBytes = recv(cfd, &ready, sizeof(int), 0);
	if (numBytes == -1) {
		printf("App: Error in receiving ready flag\n");
		return -4;
	}

	if (ready != 1) {
		printf("App: Server not ready\n");
		return -5;
	}

	// Send the key
	if (send(cfd, key, sizeof(key), 0) != sizeof(key)) {
		printf("App: Error in sending key / group_id / value / secret\n");
		return -6;
	}

	// Receive flag saying if the key is an existing one or not
	numBytes = recv(cfd, &check_key, sizeof(int), 0);
	if (numBytes == -1) {
		printf("App: Error in receiving response for the sent key / group_id / secret\n");
		return -8;
	}

	// Error for non existing key
	if (check_key != 1) {
		printf("App: Key / group_id not existing\n");
		return -7;
	}

	struct thread_args * temp = cb_info, * prev;

	if (temp != NULL && (strcmp(temp->key, key)==0)) {
		cb_info = temp->next;
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

	int func_code = 3;

	// Send the function code to the server
	if (send(cfd, &func_code, sizeof(int), 0) != sizeof(int)) {
		printf("App: Error in sending function code\n");
		return -3;
	}

	int ready = -1;
	int check_key = -1;
	ssize_t numBytes;

	// See if server is ready to receive the key
	numBytes = recv(cfd, &ready, sizeof(int), 0);
	if (numBytes == -1) {
		printf("App: Error in receiving ready flag\n");
		return -4;
	}

	if (ready != 1) {
		printf("App: Server not ready\n");
		return -5;
	}

	// Send the key
	if (send(cfd, key, sizeof(key), 0) != sizeof(key)) {
		printf("App: Error in sending key / group_id / value / secret\n");
		return -6;
	}

	// Receive flag saying if the key is an existing one or not
	numBytes = recv(cfd, &check_key, sizeof(int), 0);
	if (numBytes == -1) {
		printf("App: Error in receiving response for the sent key / group_id / secret\n");
		return -8;
	}

	// Error for non existing key
	if (check_key != 1) {
		printf("App: Key / group_id not existing\n");
		return -7;
	}

	struct thread_args * current = cb_info;
	while (current != NULL)
	{
		if (strcmp(current->key, key) == 0) {
			current->cb = callback_function;
			return 1;
		}
		current = current->next;
	}

	struct thread_args * new_node = (struct thread_args *) calloc(1, sizeof(struct thread_args));
	struct thread_args * last = cb_info;

	strcpy(new_node->key, key);
	new_node->cb = callback_function;
	new_node->next = NULL;

	if (cb_info == NULL)
	{
		cb_info = new_node;
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
