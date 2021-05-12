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
// #include "hash.h"
// #include "appList.h"
#include "groupList.h"

#define SV_SOCK_PATH "/tmp/server_sock"
#define BUF_SIZE 100
#define BACKLOG 5

struct Group * groups;

char group_id[3] = "g1";
char secret[4] = "123";
char key[BUF_SIZE];
char value[BUF_SIZE];
char key2[BUF_SIZE];
char value2[BUF_SIZE];

int put_value (int * app_fd) {

    int ready = 1;
    ssize_t numBytes;
    if (write(*app_fd, &ready, sizeof(ready)) != sizeof(ready)) {
        printf("Server: Error in sending ready flag\n");
        return -1;
    }
    numBytes = read(*app_fd, key, sizeof(key));
    if (numBytes == -1) {
        printf("Server: Error in reading key\n");
        return -1;
    }
    if (write(*app_fd, &ready, sizeof(ready)) != sizeof(ready)) {
        printf("Server: Error in sending ready flag\n");
        return -1;
    }
    numBytes = read(*app_fd, value, sizeof(value));
    if (numBytes == -1) {
        printf("Server: Error in reading value\n");
        return -1;
    }
    return 1;
}

int get_value (int * app_fd) {

    int ready = 1;
    int length = -1;
    ssize_t numBytes;
    if (write(*app_fd, &ready, sizeof(ready)) != sizeof(ready)) {
        printf("Server: Error in sending ready flag\n");
        return -1;
    }
    numBytes = read(*app_fd, key2, sizeof(key2));
    if (numBytes == -1) {
        printf("Server: Error in reading key\n");
        return -1;
    }
    if (strcmp(key2, key) == 0)
        length = strlen(value);
    if (write(*app_fd, &length, sizeof(length)) != sizeof(length)) {
        printf("Server: Error in sending length\n");
        return -1;
    }
    if (length == -1)
        return -1;
    if (write(*app_fd, &value, sizeof(&value)) != sizeof(&value)) {
        printf("Server: Error in sending value pointer\n");
        return -1;
    }
    return 1;
}

int delete_value (int * app_fd) {

    int ready = 1;
    int check_key = -1;
    ssize_t numBytes;
    if (write(*app_fd, &ready, sizeof(ready)) != sizeof(ready)) {
        printf("Server: Error in sending ready flag\n");
        return -1;
    }
    numBytes = read(*app_fd, key2, sizeof(key2));
    if (numBytes == -1) {
        printf("Server: Error in reading key\n");
        return -1;
    }
    if (strcmp(key2, key) == 0)
        check_key = 1;
    if (write(*app_fd, &check_key, sizeof(check_key)) != sizeof(check_key)) {
        printf("Server: Error in sending check_key\n");
        return -1;
    }
    if (check_key == -1)
        return -1;
    memset(value,0,strlen(value));
    return 1;
}

int register_callback (int * app_fd) {
}

void * thread_func(void * arg) {

    // Group_id received by the app
    char group_id_app[BUF_SIZE];
    // Secret received by the app
    char secret_app[BUF_SIZE];

    // Definition of app file descriptor
    int cfd = *(int *)arg;
    ssize_t numBytes;
    int error_flag = 1;
    bool flag;

    // Sending flag saying that connection was established
    if (write(cfd, &error_flag, sizeof(int)) != sizeof(int)) {
        printf("Server: Error in sending flag for established connection\n");
        if (close(cfd) == -1) {
            printf("Server: Error in closing socket file descriptor\n");
        }
        pthread_exit(NULL);
    }

    // Reusing the error flag to check the group_id
    error_flag = -1;

    // Reading group_id
    numBytes = read(cfd, group_id_app, BUF_SIZE);

    // Error in reading group_id
    if (numBytes == -1) {
        printf("Server: Error in reading group_id\n");
        if (close(cfd) == -1) {
            printf("Server: Error in closing socket file descriptor\n");
        }
        pthread_exit(NULL);        
    }

    // Check if group_id is correct
    flag = FindGroup(groups, group_id_app);
    if (flag == true)
        error_flag = 1;
    else
        error_flag = 0;

    // Sending flag saying if group_id is correct or not
    if (write(cfd, &error_flag, sizeof(int)) != sizeof(int)) {
        printf("Server: Error in sending flag for correct/incorrect group_id\n");
        if (close(cfd) == -1) {
            printf("Server: Error in closing socket file descriptor\n");
        }
        pthread_exit(NULL);
    }

    // If the group_id is incorrect we exit the thread
    // Should we also close the connection here or should we let the app close the connection?
    if (error_flag == 0)
        pthread_exit(NULL);

    // Reusing the error flag to check the secret
    error_flag = -1;

    struct sockaddr_un cl_addr;
    socklen_t len;
    len = sizeof(struct sockaddr_un);

    // Reading secret
    numBytes = recvfrom(cfd, secret_app, sizeof(BUF_SIZE), 0, (struct sockaddr *) &cl_addr, &len);

    // Errot in reading secret
    if (numBytes == -1) {
        printf("Server: Error in reading secret\n");
        if (close(cfd) == -1) {
            printf("Server: Error in closing socket file descriptor\n");
        }
        pthread_exit(NULL);        
    }

    int pid = atol(cl_addr.sun_path + strlen("/tmp/app_socket_"));
    
    // Check if secret is correct
    flag = addApp_toGroup(groups, group_id_app, secret_app, cfd, pid);
    if (flag == true)
        error_flag = 1;
    else
        error_flag = 0;

    // Sending flag saying if secret is correct or not
    if (write(cfd, &error_flag, sizeof(int)) != sizeof(int)) {
        printf("Server: Error in sending flag for correct/incorrect secret\n");
        if (close(cfd) == -1) {
            printf("Server: Error in closing socket file descriptor\n");
        }
        pthread_exit(NULL);
    }

    // End of establish connection

    // This flag is sent to the app after each function indicating sucess
    int sucess_flag;
    // Variable with a code saying which function the app wants to execute
    int func_code;
    while((numBytes = read(cfd, &func_code, sizeof(int))) > 0) {
        sucess_flag = -1;
        if (func_code == 0) {
            sucess_flag = put_value(&cfd);
            printf("key: %s\n", key);
            printf("value: %s\n", value);
            if (write(cfd, &sucess_flag, sizeof(int)) != sizeof(int)) {
                printf("Server: Error in sending sucess flag\n");
            }
        }
        else if (func_code == 1) {
            sucess_flag = get_value(&cfd);
            if (write(cfd, &sucess_flag, sizeof(sucess_flag)) != sizeof(sucess_flag)) {
                printf("Server: Error in sending sucess flag\n");
            }
        }
        else if (func_code == 2) {
            sucess_flag = delete_value(&cfd);
            printf("value: %s\n", value);
            if (write(cfd, &sucess_flag, sizeof(sucess_flag)) != sizeof(sucess_flag)) {
                printf("Server: Error in sending sucess flag\n");
            }
        }
        else if (func_code == 3)
            register_callback(&cfd);
        else
            close_GroupApp(&groups, group_id_app, cfd);
    }

    printf("Server: Error in reading function name\n");
    if (close(cfd) == -1) {
        printf("Server: Error in closing socket file descriptor\n");
    }
    pthread_exit(NULL);

}

int main(int argc, char *argv[]) {

    // Definition of the socket address and file descriptor
    struct sockaddr_un sv_addr;
    int sfd;

    // File descriptor assignment
    sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sfd == -1)
        printf("Server: Error in socket creation\n");

    // Clean socket path
    remove(SV_SOCK_PATH);

    // Bind
    sv_addr.sun_family = AF_UNIX;
    strncpy(sv_addr.sun_path, SV_SOCK_PATH, sizeof(sv_addr.sun_path) - 1);

    if (bind(sfd, (struct sockaddr *) &sv_addr, sizeof(struct sockaddr_un)) == -1)
        printf("Server: Error in binding\n");

    // Listen
    if (listen(sfd, BACKLOG) == 1)
        printf("Server: Error in listening\n");

    struct sockaddr_un app_addr;
    socklen_t len = sizeof(struct sockaddr_un);

    groups = NULL;

    int pid = fork();

    if (pid == 0) {
        for (;;) {
            int cfd = accept(sfd, (struct sockaddr *) &app_addr, &len);
            if (cfd == -1)
                printf("Server: Error in acception\n");
            pthread_t t_id;
            pthread_create(&t_id, NULL, thread_func, &cfd);
        }
        exit(0);
    }
    else {
        char str[BUF_SIZE];
        char g_name[BUF_SIZE];
        bool flag;
        while (1) {
            fgets(str, sizeof(str), stdin);
            if (strcmp(str, "Create group\n") == 0) {
                printf("Here!\n");
                fgets(g_name, sizeof(g_name), stdin);
                CreateGroup(&groups, g_name);
                printf("Group created!\n");
            }
            else if (strcmp(str, "Delete group\n") == 0) {
                fgets(g_name, sizeof(g_name), stdin);
                deleteGroup(&groups, g_name);
                printf("Group deleted!\n");
            }
            else if (strcmp(str, "Show group info\n") == 0) {
                fgets(g_name, sizeof(g_name), stdin);
                flag = ShowGroupInfo(groups, g_name);
                if (flag == false)
                    printf("Group not found!\n");
            }
            else if (strcmp(str, "Show application status\n") == 0) {
                ShowAppStatus(groups);
            }
        }
        wait(NULL);
    }

    remove(sv_addr.sun_path);

    return 0;

}