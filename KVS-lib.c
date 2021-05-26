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
    snprintf(cl_addr.sun_path, sizeof(cl_addr.sun_path), "/tmp/client_socket_%ld", (long) pthread_self());
    // printf("%s\n", cl_addr.sun_path);

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
    numBytes = read(cfd, &check_connection, sizeof(int));
    if (numBytes == -1) {
        printf("App: Error in reading response of the server for the established connection\n");
        return -4;
    }

    // If the connection flag is not 1, the connection was not established
    if (check_connection != 1) {
        printf("App: Error in establishing connection to the server\n");
        return -5;
    }

    // Sending the group_id to the server
	if (write(cfd, group_id, sizeof(group_id)) != sizeof(group_id)) {
        if (close(cfd) == -1) {
        	printf("App: Error in closing socket after error in sending group_id\n");
        	return -6;
        }
        printf("App: Error in sending group_id\n");
        return -7;
	}

    // Reading flag saying if the group_id is correct
	numBytes = read(cfd, &check_group, sizeof(int));
    if (numBytes == -1) {
        printf("App: Error in reading response of the server for the sent group_id\n");
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
    if (write(cfd, secret, sizeof(secret)) != sizeof(secret)) {
        if (close(cfd) == -1) {
            printf("App: Error in closing socket after error in sending secret\n");
            return -11;
        }
        printf("App: Error in sending secret\n");
        return -12;
    }

    // Reading flag for the secret
    numBytes = read(cfd, &check_secret, sizeof(int));
    if (numBytes == -1) {
        printf("App: Error in reading response of the server for the sent secret\n");
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
	if (write(cfd, &func_code, sizeof(func_code)) != sizeof(func_code)) {
        printf("App: Error in sending function name\n");
        return -2;
	}

    int ready = -1;
    int operation_sucess = -1;
    ssize_t numBytes;

    // See if server is ready to send the key
    numBytes = read(cfd, &ready, sizeof(int));
    if (numBytes == -1) {
        printf("App: Error in reading ready\n");
        return -3;
    }

    if (ready != 1) {
        printf("App: Server not ready\n");
        return -4;
    }

    // Send the key
    if (write(cfd, key, sizeof(key)) != sizeof(key)) {
        printf("App: Error in sending key\n");
        return -5;
    }

    // Reuse of the ready flag to see if the server is ready for the sending of the value
    ready = -1;

    // See if server is ready to send the key
    numBytes = read(cfd, &ready, sizeof(int));
    if (numBytes == -1) {
        printf("App: Error in reading ready\n");
        return -6;
    }

    if (ready != 1) {
        printf("App: Server not ready\n");
        return -7;
    }

    // Sending the value
    if (write(cfd, value, sizeof(value)) != sizeof(value)) {
        printf("App: Error in sending value\n");
        return -8;
    }

    numBytes = read(cfd, &operation_sucess, sizeof(int));
    if (numBytes == -1) {
        printf("App: Error in reading operation_sucess\n");
        return -9;
    }

    if (operation_sucess == 1)
        return 1;

    printf("App: Other error\n");
    return -10;

}

// Como é que se sabe se a operação foi um sucesso?
int get_value (char * key, char ** value) {

    if (cfd == -1) {
        printf("App: Socket is not created\n");
        return -1;
    }

    int func_code = 1;

    // Send the function name to the server
    if (write(cfd, &func_code, sizeof(func_code)) != sizeof(func_code)) {
        printf("App: Error in sending function name\n");
        return -2;
    }

    int ready = -1;
    int length = -1;
    int operation_sucess = -1;
    ssize_t numBytes;

    // See if server is ready to send the key
    numBytes = read(cfd, &ready, sizeof(int));
    if (numBytes == -1) {
        printf("App: Error in reading ready\n");
        return -3;
    }

    if (ready != 1) {
        printf("App: Server not ready\n");
        return -4;
    }

    // Send the key
    if (write(cfd, key, sizeof(key)) != sizeof(key)) {
        printf("App: Error in sending key\n");
        return -5;
    }

    // See if server is ready to send the key
    numBytes = read(cfd, &length, sizeof(int));
    if (numBytes == -1) {
        printf("App: Error in reading ready\n");
        return -6;
    }

    if (length == -1) {
        printf("App: Key not existing\n");
        return -7;
    }

    *value = (char *) malloc((length+1)*sizeof(char));

    numBytes = read(cfd, *value, sizeof(*value));
    if (numBytes == -1) {
        printf("App: Error in reading value\n");
        return -8;
    }

    numBytes = read(cfd, &operation_sucess, sizeof(operation_sucess));
    if (numBytes == -1) {
        printf("App: Error in reading operation_sucess\n");
        return -9;
    }

    if (operation_sucess == 1)
        return 1;

    printf("App: Other error\n");
    return -10;

}

int delete_value (char * key) {

    if (cfd == -1) {
        printf("App: Socket is not created\n");
        return -1;
    }

    int func_code = 2;

    // Send the function code to the server
    if (write(cfd, &func_code, sizeof(func_code)) != sizeof(func_code)) {
        printf("App: Error in sending function code\n");
        return -2;
    }

    int ready = -1;
    int check_key = -1;
    int operation_sucess = -1;
    ssize_t numBytes;

    // See if server is ready to receive the key
    numBytes = read(cfd, &ready, sizeof(int));
    if (numBytes == -1) {
        printf("App: Error in reading ready\n");
        return -3;
    }

    if (ready != 1) {
        printf("App: Server not ready\n");
        return -4;
    }

    // Send the key
    if (write(cfd, key, sizeof(key)) != sizeof(key)) {
        printf("App: Error in sending key\n");
        return -5;
    }

    // Receive flag saying if the key is an existing one or not
    numBytes = read(cfd, &check_key, sizeof(int));
    if (numBytes == -1) {
        printf("App: Error in reading operation_sucess\n");
        return -6;
    }

    // Error for non existing key
    if (check_key != 1) {
        printf("App: Non existing key\n");
        return -7;
    }

    // Receive operation sucess flag
    numBytes = read(cfd, &operation_sucess, sizeof(int));
    if (numBytes == -1) {
        printf("App: Error in reading operation_sucess\n");
        return -8;
    }

    if (operation_sucess == 1)
        return 1;

    printf("App: Other error\n");
    return -9;

}

// Isto ainda não foi testado, o mais provável é não funcionar
int register_callback (char * key, void (*callback_function)(char *)) {

    if (cfd == -1) {
        printf("App: Socket is not created\n");
        return -1;
    }

    int func_code = 3;

    // Send the function name to the server
    if (write(cfd, &func_code, sizeof(func_code)) != sizeof(func_code)) {
        printf("App: Error in sending function name\n");
        return -2;
    }

    int ready = -1;
    int check_key = -1;
    int operation_success = -1;
    ssize_t numBytes;

    // See if server is ready to send the key
    numBytes = read(cfd, &ready, sizeof(int));
    if (numBytes == -1) {
        printf("App: Error in reading ready\n");
        return -3;
    }

    if (ready != 1) {
        printf("App: Server not ready\n");
        return -4;
    }

    // Send the key
    if (write(cfd, key, sizeof(key)) != sizeof(key)) {
        printf("App: Error in sending key\n");
        return -5;
    }

    // See if server is ready to send the key
    numBytes = read(cfd, &check_key, sizeof(int));
    if (numBytes == -1) {
        printf("App: Error in reading ready\n");
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
    if (write(cfd, callback_function, sizeof(callback_function)) != sizeof(callback_function)) {
        printf("App: Error in sending the function pointer\n");
        return -9;
    }

    numBytes = read(cfd, &operation_success, sizeof(&operation_success));
    if (numBytes == -1) {
        printf("App: Error in reading value\n");
        return -10;
    }

    if (operation_success == 1)
        return 1;

    printf("App: Other error\n");
    return -11;

}

// Isto tem de ser mudado mas para isso é preciso fazer uma lista ligada de app's no servidor
int close_connection() {

    int operation_success = -1;
    ssize_t numBytes;

    if (cfd == -1) {
        printf("App: Socket is not created\n");
        return -1;
    }

    numBytes = read(cfd, &operation_success, sizeof(&operation_success));
    if (numBytes == -1) {
        printf("App: Error in reading value\n");
        return -2;
    }

    if (close(cfd) == -1) {
        printf("App: Error in closing socket\n");
        return -3;
    }

    if (operation_success == 1)
        return 1;

    printf("App: Other error\n");
    return -4;
}