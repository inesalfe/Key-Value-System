#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/un.h>
#include <stdbool.h>
#include "appList.h"

void PrintAppList(struct App * app)
{
	int counter = 0;
	while (app != NULL)
	{	
		counter++;
		printf("App %d\n", counter);
		printf("PID: %ld\n", app->pid);
		if (app->isClosed)
			printf("Status: Closed\n");
		else
			printf("Status: Connected\n");
		printf("Connection time: %f\n", app->start.tv_sec + (double) app->start.tv_nsec / 1.0e9);
		printf("Close Connection time: %f\n", app->stop.tv_sec + (double) app->stop.tv_nsec / 1.0e9);
		if (app->isClosed)
			printf("Connected time interval: %f\n", app->delta_t);
		else {
			clock_gettime(CLOCK_REALTIME, &app->stop);
			double time = (app->stop.tv_sec - app->start.tv_sec) + (double) (app->stop.tv_nsec - app->start.tv_nsec) / 1.0e9;
			printf("Connected time interval: %f\n", time);
		}
		app = app->next;
	}
}

void AppendApp(struct App ** head_ref, int cl_fd, int pid_in)
{
	struct App * new_node = (struct App *) calloc(1, sizeof(struct App));
	struct App * last = * head_ref;
	new_node->fd = cl_fd;
	new_node->isClosed = false;
	clock_gettime(CLOCK_REALTIME, &new_node->start);
	new_node->pid = pid_in;
	new_node->delta_t = 0;
	new_node->next = NULL;
	if (* head_ref == NULL) {
	   * head_ref = new_node;
	   return;
	}
	while (last->next != NULL)
		last = last->next;
	last->next = new_node;
	return;
}

bool FindApp(struct App * head, int pid) {

	struct App * current = head;
	while (current != NULL)
	{
		if (current->pid == pid) {
			return true;
		}
		current = current->next;
	}
	return false;    
}

bool CloseConnection(struct App * head, int pid)
{
	struct App * current = head;
	while (current != NULL)
	{
		if (current->pid == pid) {
			clock_gettime(CLOCK_REALTIME, &current->stop);
			current->delta_t = (current->stop.tv_sec - current->start.tv_sec) + (double) (current->stop.tv_nsec - current->start.tv_nsec) / 1.0e9;
			if (close(current->fd) == -1) {
				printf("Local Server: Error in closing socket\n");
			}
			current->isClosed = true;
			return true;
		}
		current = current->next;
	}
	return false;
}

void CloseFileDesc(struct App** head_ref) {

	struct App * current = * head_ref;
	struct App * next;

	while (current != NULL)
	{
		next = current->next;
		if (current->isClosed == false) {
			clock_gettime(CLOCK_REALTIME, &current->stop);
			current->delta_t = (current->stop.tv_sec - current->start.tv_sec) + (double) (current->stop.tv_nsec - current->start.tv_nsec) / 1.0e9;
			if (close(current->fd) == -1) {
				printf("Local Server: Error in closing socket\n");
			}
			current->isClosed = true;
		}
		current = next;
	}

	return;
}

void DeleteAppList(struct App ** head_ref) {

	struct App * current = * head_ref;
	struct App * next;

	while (current != NULL)
	{
		next = current->next;
		free(current);
		current = next;
	}

	* head_ref = NULL;
}



