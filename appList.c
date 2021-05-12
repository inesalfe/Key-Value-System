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

void append_App(struct App** head_ref, int cl_fd, int pid_in)
{
    struct App* new_node = (struct App*) malloc(sizeof(struct App));
    struct App *last = *head_ref;
    new_node->fd = cl_fd;
    clock_gettime(CLOCK_REALTIME, &new_node->start);
	new_node->pid = pid_in;
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

bool close_App(struct App* head, int cl_fd)
{
    struct App* current = head;
    while (current != NULL)
    {
        if (current->fd == cl_fd) {
    		clock_gettime(CLOCK_REALTIME, &current->stop);
        	return true;
        }
        current = current->next;
    }
    return false;
}





