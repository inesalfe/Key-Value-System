#include <stdio.h>
#include <stdlib.h>
#include "KVS-lib.h"

#define SV_SOCK_PATH "/tmp/server_sock"
#define BUF_SIZE 100

int main(int argc, char *argv[]) {

    char group_id[3] = "g1\n";
    char secret[4] = "123";
    char key1[4] = "abc";
    char key2[4] = "def";
    char value1[4] = "999";
    char * value2;

    int flag = establish_connection (group_id, secret);
    printf("%d\n", flag);

    flag = put_value(key1, value1);
    printf("%d\n", flag);

    // Neste exemplo a key está errada e é suposto dar erro
    // flag = get_value(key2, &value2);
    // printf("%d\n", flag);
    // printf("value: %s\n", value2);

    flag = get_value(key1, &value2);
    printf("%d\n", flag);
    printf("value: %s\n", value2);

    // Neste exemplo a key está errada e é suposto dar erro
    // flag = delete_value(key2);
    // printf("%d\n", flag);

    flag = delete_value(key1);
    printf("%d\n", flag);

    exit(0);

    flag = close_connection();
    printf("%d\n", flag);

    return 0;

}