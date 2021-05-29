#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "appList.h"
#include "groupList.h"

// Server of Authentification Server
struct sockaddr_in sv_addr_auth;
// File descriptor for the connection to the AuthServer
int sfd_auth;

int main() {

    sfd_auth = socket(AF_INET, SOCK_DGRAM, 0);
    if (sfd_auth == -1) {
        printf("Client: Error in socket creation\n");
        exit(-1);
    }

    sv_addr_auth.sin_family = AF_INET;
    sv_addr_auth.sin_port = htons(58032);

    long pid1 = (long) pthread_self();
    long pid2 = (long) pthread_self()+1;
    long pid3 = (long) pthread_self()+10;
    long pid4 = (long) pthread_self()+20;

    int cfd1 = socket(AF_UNIX, SOCK_STREAM, 0);
    if (cfd1 == -1) {
        printf("Error in socket creation\n");
        exit(0);
    }
    int cfd2 = socket(AF_UNIX, SOCK_STREAM, 0);
    if (cfd2 == -1) {
        printf("Error in socket creation\n");
        exit(0);
    }
    int cfd3 = socket(AF_UNIX, SOCK_STREAM, 0);
    if (cfd3 == -1) {
        printf("Error in socket creation\n");
        exit(0);
    }
    int cfd4 = socket(AF_UNIX, SOCK_STREAM, 0);
    if (cfd4 == -1) {
        printf("Error in socket creation\n");
        exit(0);
    }

    struct App * app1 = NULL;
    struct App * app2 = NULL;

    char n1[10] = "grupo1";
    char n2[10] = "grupo2";
    char n3[10] = "grupo3";

    char s1[10];
    char s2[10];
    char s3[10] = "456";

    struct Group * group_head = NULL;
    bool flag;

    strncpy(s1, CreateGroup(&group_head, n1), sizeof(s1));
    printf("Secret of %s: %s\n", n1, s1);
    printf("Printing information of all groups...\n");
    ShowAllGroupsInfo(group_head);
    printf("Printing information of of the first group...\n");
    flag = ShowGroupInfo(group_head, n1);
    if (flag == false)
        printf("Group not fround!\n");
    printf("Printing information of of the second group...\n");
    flag = ShowGroupInfo(group_head, n2);
    if (flag == false)
        printf("Group not fround!\n");
    strncpy(s2, CreateGroup(&group_head, n2), sizeof(s2));
    printf("Secret of %s: %s\n", n2, s2);
    printf("Printing information of all groups...\n");
    ShowAllGroupsInfo(group_head);
    printf("Printing information of of the second group...\n");
    flag = ShowGroupInfo(group_head, n2);
    if (flag == false)
        printf("Group not fround!\n");

    flag = addApp_toGroup(group_head, n1, s1, cfd1, pid1);
    if (flag == false)
        printf("Error in appending app to group!\n");
    else
        printf("App 1 appended to %s\n", n1);
    flag = addApp_toGroup(group_head, n1, s1, cfd2, pid2);
    if (flag == false)
        printf("Error in appending app to group!\n");
    else
        printf("App 2 appended to %s\n", n1);
    flag = addApp_toGroup(group_head, n2, s1, cfd3, pid3);
    if (flag == false)
        printf("Error in appending app to group!\n");
    else
        printf("App 3 appended to %s\n", n2);
    flag = addApp_toGroup(group_head, n2, s3, cfd3, pid3);
    if (flag == false)
        printf("Error in appending app to group!\n");
    else
        printf("App 3 appended to %s\n", n2);
    flag = addApp_toGroup(group_head, n2, s2, cfd4, pid4);
    if (flag == false)
        printf("Error in appending app to group!\n");
    else
        printf("App 4 appended to %s\n", n2);
    ShowAppStatus(group_head);
    ShowAllGroupsInfo(group_head);

    char k1[30] = "key1";
    char k2[30] = "key2";
    char k3[30] = "key3";

    char v1[30] = "oioioi";
    char v2[30] = "oléeee";
    char v3[30] = "chimichanga";

    flag = findKeyValue(group_head, n1, k1);
    if (flag == false)
        printf("Key not fround!\n");
    flag = FindGroup(group_head, n1);
    if (flag == false)
        printf("Group %s not fround!\n", n1);
    flag = FindGroup(group_head, n3);
    if (flag == false)
        printf("Group %s not fround!\n", n3);

    flag = addKeyValue_toGroup(group_head, n1, cfd3, k1, v1);
    if (flag == false)
        printf("Error in adding key-value pair to group %s\n", n1);
    flag = addKeyValue_toGroup(group_head, n1, cfd1, k1, v1);
    if (flag == false)
        printf("Error in adding key-value pair to group %s\n", n1);
    flag = addKeyValue_toGroup(group_head, n1, cfd2, k2, v2);
    if (flag == false)
        printf("Error in adding key-value pair to group %s\n", n1);
    flag = addKeyValue_toGroup(group_head, n2, cfd3, k3, v3);
    if (flag == false)
        printf("Error in adding key-value pair to group %s\n", n2);

    ShowAllGroupsInfo(group_head);

    printf("\n\n\n");

    char v1_recv[30];
    char v2_recv[30];
    char v3_recv[30];

    //v1_recv = (getKeyValue(group_head, n1, k3) == NULL) ? NULL : strncpy(v1_recv, getKeyValue(group_head, n1, k3), sizeof(v1_recv));
    //getKeyValue(group_head, n1, k3) == NULL ? v1_recv = NULL : strncpy(v1_recv, getKeyValue(group_head, n1, k3), sizeof(v1_recv));

    if (getKeyValue(group_head, n1, k3) == NULL)
        printf("Null string\n");
    else
        printf("Received key: %s\n", getKeyValue(group_head, n1, k3));

    if (getKeyValue(group_head, n1, k1) == NULL)
        printf("Null string\n");
    else
        printf("Received key: %s\n", getKeyValue(group_head, n1, k1));

    flag = close_GroupApp(&group_head, n1, cfd3);
    if (flag == false)
        printf("Error in closing app %d in group %s\n", cfd3, n1);
    flag = close_GroupApp(&group_head, n1, cfd1);
    if (flag == false)
        printf("Error in closing app %d in group %s\n", cfd1, n1);
    ShowAppStatus(group_head);

    flag = addKeyValue_toGroup(group_head, n1, cfd1, k1, v2);
    if (flag == false)
        printf("Error in adding key-value pair to group %s\n", n1);
    strncpy(v1_recv, getKeyValue(group_head, n1, k1), sizeof(v1_recv));
    printf("Received key: %s\n", v1_recv);

    deleteKeyValue(group_head, n1, k1);
    if (getKeyValue(group_head, n1, k1) == NULL)
        printf("Null string\n");
    else
        printf("Received key: %s\n", getKeyValue(group_head, n1, k1));
    ShowAllGroupsInfo(group_head);

    deleteGroup(&group_head, n1);
    ShowAllGroupsInfo(group_head);

    deleteGroupList(&group_head);
    
    return 0;
}