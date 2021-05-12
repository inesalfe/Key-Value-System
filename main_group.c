#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <pthread.h>
#include "appList.h"
#include "groupList.h"

int main()
{
    struct sockaddr_un cl_addr;
    cl_addr.sun_family = AF_UNIX;
    snprintf(cl_addr.sun_path, sizeof(cl_addr.sun_path), "/tmp/app_socket_%ld", (long) pthread_self());
    struct App * head = NULL;
    append_App(&head, cl_addr);
    print_AppList(head);
    struct sockaddr_un cl_addr_2;
    cl_addr_2.sun_family = AF_UNIX;
    snprintf(cl_addr_2.sun_path, sizeof(cl_addr_2.sun_path), "/tmp/app_socket_%ld", (long) (pthread_self()+1));
    append_App(&head, cl_addr_2);
    print_AppList(head);
    close_App(head, cl_addr);
    print_AppList(head);
    close_App(head, cl_addr_2);
    print_AppList(head);

    char n1[10] = "grupo1";
    char n2[10] = "grupo2";
    char n3[10] = "grupo3";
    struct Group * group_head = NULL;
    CreateGroup(&group_head, n1);
    ShowAllGroupsInfo(group_head);
    ShowGroupInfo(group_head, n1);
    CreateGroup(&group_head, n2);
    ShowAllGroupsInfo(group_head);
    ShowGroupInfo(group_head, n2);
    addApp_toGroup(group_head, n1, cl_addr);
    addApp_toGroup(group_head, n1, cl_addr_2);
    ShowAllGroupsInfo(group_head);
    
    return 0;
}