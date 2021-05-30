#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/un.h>
#include "KVS-lib.h"

#define SV_SOCK_PATH "/tmp/server_sock"
#define BUF_SIZE 100

int cfd = -1;
struct sockaddr_un cl_addr;

int main(int argc, char *argv[]) {

    char group_id1[5] = "g1";
    char group_id2[5] = "g2";
    char secret[4] = "123";
    char key1[4] = "abc";
    char key2[4] = "def";
    char value1[4] = "999";
    char * value2;
    char * value3;

    int flag = establish_connection(group_id1, secret);
    printf("%d\n", flag);

    flag = put_value(key1, value1);
    printf("%d\n", flag);

    // printf("Before calling function\n");

    flag = get_value(key1, &value2);
    printf("%d\n", flag);
    printf("value: %s\n", value2);

    // flag = delete_value(key1);
    // printf("%d\n", flag);

    // flag = get_value(key1, &value3);
    // printf("%d\n", flag);
    // printf("value: %s\n", value3);

    flag = close_connection();
    printf("%d\n", flag);

    free(value2);

    return 0;

}