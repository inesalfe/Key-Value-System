#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <pthread.h>

#define SV_SOCK_PATH "/tmp/server_sock"

// Mudar valores dos erros no caso do erro ser igual

// Resolver isto
int cfd = -1;

// Needs testing for different errors and use of sterror
int establish_connection (char * group_id, char * secret) {

    // Definition of server address
	struct sockaddr_un sv_addr, cl_addr;

    // Assignment of server address
    cfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (cfd == -1) {
        printf("App: Error in socket creation\n");
        return -1;
    }

    // Server address assignment
    sv_addr.sun_family = AF_UNIX;
    strncpy(sv_addr.sun_path, SV_SOCK_PATH, sizeof(sv_addr.sun_path) - 1);

    // App address assignment with PID
    cl_addr.sun_family = AF_UNIX;
    snprintf(cl_addr.sun_path, sizeof(cl_addr.sun_path), "/tmp/app_socket_%ld", (long) pthread_self());

    // Bind app
    if (bind(cfd, (struct sockaddr *) &cl_addr, sizeof(struct sockaddr_un)) == -1) {
        printf("App: Error in binding\n");
        return -2;
    }

    // Connect to server
    if (connect(cfd, (struct sockaddr *) &sv_addr, sizeof(struct sockaddr_un)) == -1) {
        printf("App: Error in connect\n");
        return -3;
    }

    ssize_t numBytes;
    int check_connection = -1;
    int check_group = -1;
    int check_secret = -1;

    // Wait for flag saying that the connectiong was established
    numBytes = recv(cfd, &check_connection, sizeof(int), 0);
    if (numBytes == -1) {
        printf("App: Error in receaving response of the server for the established connection\n");
        return -4;
    }

    // If the connection flag is not 1, the connection was not established
    if (check_connection != 1) {
        printf("App: Error in establishing connection to the server\n");
        return -5;
    }

    // Sending the group_id to the server
	if (send(cfd, group_id, sizeof(group_id), 0) != sizeof(group_id)) {
        if (close(cfd) == -1) {
        	printf("App: Error in closing socket after error in sending group_id\n");
        	return -6;
        }
        printf("App: Error in sending group_id\n");
        return -7;
	}

    // Receaving flag saying if the group_id is correct
	numBytes = recv(cfd, &check_group, sizeof(int), 0);
    if (numBytes == -1) {
        printf("App: Error in receaving response of the server for the sent group_id\n");
        return -8;
    }

    // Wrong group
    if (check_group != 1) {
    	if (close(cfd) == -1) {
        	printf("App: Error in closing socket after inexisting group_id\n");
        	return -9;
        }
        printf("App: Inexisting group_id\n");
        return -10; 	
    }

    // Sending secret
    if (send(cfd, secret, sizeof(secret), 0) != sizeof(secret)) {
        if (close(cfd) == -1) {
            printf("App: Error in closing socket after error in sending secret\n");
            return -11;
        }
        printf("App: Error in sending secret\n");
        return -12;
    }

    // Receaving flag for the secret
    numBytes = recv(cfd, &check_secret, sizeof(int), 0);
    if (numBytes == -1) {
        printf("App: Error in receaving response of the server for the sent secret\n");
        return -13;
    }
    if (check_secret != 1) {
        if (close(cfd) == -1) {
            printf("App: Error in closing socket after sending incorrect secret\n");
            return -14;
        }
        printf("App: Incorrect secret\n");
        return -15;
    }

    if (check_secret == 1)
        return 0;

    if (close(cfd) == -1) {
        printf("App: Error in closing socket after failure\n");
        return -16;
    }

    printf("App: Other error\n");
    return -17;

}

// Needs testing for different errors and use of sterror
int put_value (char * key, char * value) {

	if (cfd == -1) {
		printf("App: Socket is not created\n");
        return -1;
	}

    int func_code = 0;

    // Send the function name to the server
	if (send(cfd, &func_code, sizeof(int), 0) != sizeof(int)) {
        printf("App: Error in sending function name\n");
        return -2;
	}

    int ready = -1;
    ssize_t numBytes;

    // See if server is ready to send the key
    numBytes = recv(cfd, &ready, sizeof(int), 0);
    if (numBytes == -1) {
        printf("App: Error in receaving ready\n");
        return -3;
    }

    if (ready != 1) {
        printf("App: Server not ready\n");
        return -4;
    }

    // Send the key
    if (send(cfd, key, sizeof(key), 0) != sizeof(key)) {
        printf("App: Error in sending key\n");
        return -5;
    }

    // Reuse of the ready flag to see if the server is ready for the sending of the value
    ready = -1;

    // See if server is ready to send the key
    numBytes = recv(cfd, &ready, sizeof(int), 0);
    if (numBytes == -1) {
        printf("App: Error in receaving ready\n");
        return -6;
    }

    if (ready != 1) {
        printf("App: Server not ready\n");
        return -7;
    }

    // Sending the value
    if (send(cfd, value, sizeof(value), 0) != sizeof(value)) {
        printf("App: Error in sending value\n");
        return -8;
    }

    return 1;

}

// Como é que se sabe se a operação foi um sucesso?
int get_value (char * key, char ** value) {

    if (cfd == -1) {
        printf("App: Socket is not created\n");
        return -1;
    }

    int func_code = 1;

    // Send the function name to the server
    if (send(cfd, &func_code, sizeof(int), 0) != sizeof(int)) {
        printf("App: Error in sending function name\n");
        return -2;
    }

    int ready = -1;
    int length = -1;
    ssize_t numBytes;

    // See if server is ready to send the key
    numBytes = recv(cfd, &ready, sizeof(int), 0);
    if (numBytes == -1) {
        printf("App: Error in receaving ready\n");
        return -3;
    }

    if (ready != 1) {
        printf("App: Server not ready\n");
        return -4;
    }

    // Send the key
    if (send(cfd, key, sizeof(key), 0) != sizeof(key)) {
        printf("App: Error in sending key\n");
        return -5;
    }

    // See if server is ready to send the key
    numBytes = recv(cfd, &length, sizeof(int), 0);
    if (numBytes == -1) {
        printf("App: Error in receaving ready\n");
        return -6;
    }

    printf("length: %d\n", length);

    if (length == -1) {
        printf("App: Key not existing\n");
        return -7;
    }

    *value = (char *) malloc((length+1)*sizeof(char));

    numBytes = recv(cfd, *value, (length+1)*sizeof(char), 0);
    if (numBytes == -1) {
        printf("App: Error in receaving value\n");
        return -8;
    }

    return 1;

}

int delete_value (char * key) {

    if (cfd == -1) {
        printf("App: Socket is not created\n");
        return -1;
    }

    int func_code = 2;

    // Send the function code to the server
    if (send(cfd, &func_code, sizeof(int), 0) != sizeof(int)) {
        printf("App: Error in sending function code\n");
        return -2;
    }

    int ready = -1;
    int check_key = -1;
    ssize_t numBytes;

    // See if server is ready to receive the key
    numBytes = recv(cfd, &ready, sizeof(int), 0);
    if (numBytes == -1) {
        printf("App: Error in receaving ready\n");
        return -3;
    }

    if (ready != 1) {
        printf("App: Server not ready\n");
        return -4;
    }

    // Send the key
    if (send(cfd, key, sizeof(key), 0) != sizeof(key)) {
        printf("App: Error in sending key\n");
        return -5;
    }

    // Receive flag saying if the key is an existing one or not
    numBytes = recv(cfd, &check_key, sizeof(int), 0);
    if (numBytes == -1) {
        printf("App: Error in receaving check_key\n");
        return -6;
    }

    // Error for non existing key
    if (check_key != 1) {
        printf("App: Non existing key\n");
        return -7;
    }

    return 1;

}

// Isto ainda não foi testado, o mais provável é não funcionar
int register_callback (char * key, void (*callback_function)(char *)) {

    if (cfd == -1) {
        printf("App: Socket is not created\n");
        return -1;
    }

    int func_code = 3;

    // Send the function name to the server
    if (send(cfd, &func_code, sizeof(func_code), 0) != sizeof(func_code)) {
        printf("App: Error in sending function name\n");
        return -2;
    }

    int ready = -1;
    int check_key = -1;
    ssize_t numBytes;

    // See if server is ready to send the key
    numBytes = recv(cfd, &ready, sizeof(int), 0);
    if (numBytes == -1) {
        printf("App: Error in receaving ready\n");
        return -3;
    }

    if (ready != 1) {
        printf("App: Server not ready\n");
        return -4;
    }

    // Send the key
    if (send(cfd, key, sizeof(key), 0) != sizeof(key)) {
        printf("App: Error in sending key\n");
        return -5;
    }

    // See if server is ready to send the key
    numBytes = recv(cfd, &check_key, sizeof(int), 0);
    if (numBytes == -1) {
        printf("App: Error in receaving ready\n");
        return -6;
    }

    if (check_key == 0) {
        printf("App: Key not existing\n");
        return -7;
    }
    else if (check_key != 1) {
        printf("App: Server not ready\n");
        return -8;
    }

    // Send the pointer to the function
    if (send(cfd, callback_function, sizeof(callback_function), 0) != sizeof(callback_function)) {
        printf("App: Error in sending the function pointer\n");
        return -9;
    }

    return 1;

}

int close_connection() {

    int func_code = 4;
    ssize_t numBytes;

    if (cfd == -1) {
        printf("App: Socket is not created\n");
        return -1;
    }

    if (send(cfd, &func_code, sizeof(int), 0) != sizeof(int)) {
        printf("App: Error in sending function name\n");
        return -2;
    }

    if (close(cfd) == -1) {
        printf("App: Error in closing socket\n");
        return -3;
    }

    return 1;

}