#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "KVS-lib.h"

#define SV_SOCK_PATH "/tmp/server_sock"
#define BUF_SIZE 100

int cfd = -1;

int main(int argc, char *argv[]) {

    char group_id1[5] = "g1";
    char group_id2[5] = "g2";
    char secret[4] = "123";
    char key1[4] = "oke";
    char key2[4] = "def";
    char key3[4] = "def";
    char value1[10] = "oleee";
    char value4[10] = "oioioi";
    char * value2;
    char * value3;

    int flag = establish_connection(group_id1, secret);
    printf("%d\n", flag);

    flag = put_value(key3, value4);
    printf("%d\n", flag);

    flag = put_value(key1, value1);
    printf("%d\n", flag);

    printf("Before calling function\n");

    flag = get_value(key2, &value3);
    printf("%d\n", flag);
    printf("value: %s\n", value3);

    flag = close_connection();
    printf("%d\n", flag);

    return 0;

}