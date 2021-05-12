#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/un.h>
#include <stdbool.h>
#include "appList.h"

void print_AppList(struct App *app)
{
    int counter = 0;
	while (app != NULL)
	{	
        counter++;
        printf("App %d\n", counter);
		printf("PID: %ld\n", app->pid);
		printf("Connection time: %f\n", app->start.tv_sec + (double) app->start.tv_nsec / 1.0e9);
		printf("Close Connection time: %f\n", app->stop.tv_sec + (double) app->stop.tv_nsec / 1.0e9);
		app = app->next;
	}
}

void append_App(struct App** head_ref, struct sockaddr_un cl_addr_in)
{
    struct App* new_node = (struct App*) malloc(sizeof(struct App));
    struct App *last = *head_ref;
    new_node->cl_addr = cl_addr_in;
    clock_gettime(CLOCK_REALTIME, &new_node->start);
	new_node->pid = atol(new_node->cl_addr.sun_path + strlen("/tmp/app_socket_"));
    new_node->next = NULL;
    if (*head_ref == NULL)
    {
       *head_ref = new_node;
       return;
    }
    while (last->next != NULL)
        last = last->next;
    last->next = new_node;
    return;
}

bool equals(struct sockaddr_un s1, struct sockaddr_un s2) {
    if (strcmp(s1.sun_path, s2.sun_path) == 0)
        if (s1.sun_family == s2.sun_family)
            return true;
        else
            return false;
    else
        return false;
}

bool close_App(struct App* head, struct sockaddr_un cl_addr_in)
{
    struct App* current = head;
    while (current != NULL)
    {
        if (equals(current->cl_addr, cl_addr_in) == true) {
    		clock_gettime(CLOCK_REALTIME, &current->stop);
        	return true;
        }
        current = current->next;
    }
    return false;
}





