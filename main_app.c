#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/un.h>
#include <sys/socket.h>
#include "appList.h"

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

    return 0;
}