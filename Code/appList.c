#define _POSIX_C_SOURCE 199309L

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/un.h>
#include <stdbool.h>
#include "appList.h"

static const char default_format[] = "%a %d-%m-%Y %H:%M:%S";

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
		time_t t_c = (time_t) (app->start.tv_sec + (double) app->start.tv_nsec / 1.0e9);
		time_t t_d = (time_t) (app->stop.tv_sec + (double) app->stop.tv_nsec / 1.0e9);
		const char * format = default_format;
		struct tm lt_c;
		struct tm lt_d;
		char res[80];
		localtime_r(&t_c, &lt_c);
		localtime_r(&t_d, &lt_d);
		strftime(res, sizeof(res), format, &lt_c);
		printf("Connection time: %s\n", res);
		strftime(res, sizeof(res), format, &lt_d);
		if(app->isClosed)
			printf("Close Connection time: %s\n", res);
		else
			printf("Close Connection time: -\n");
		if (app->isClosed)
			printf("Connected time interval: %f\n", app->delta_t);
		else {
			clock_gettime(CLOCK_REALTIME, &app->stop);
			double time = (app->stop.tv_sec - app->start.tv_sec) + (double) (app->stop.tv_nsec - app->start.tv_nsec) / 1.0e9;
			printf("Connected time interval: %f\n", time);
		}
		printf("Keys in WatchList for callback function:\n");
		struct WatchList * curr = app->wlist;
		while (curr != NULL) {
			printf("%s\n", curr->key);
			curr = curr->next;
		}
		app = app->next;
	}
}

void AppendApp(struct App ** head_ref, int cl_fd, int fd_cb, int pid_in)
{
	struct App * new_node = (struct App *) calloc(1, sizeof(struct App));
	struct App * last = * head_ref;
	new_node->fd = cl_fd;
	new_node->fd_cb = fd_cb;
	new_node->isClosed = false;
	clock_gettime(CLOCK_REALTIME, &new_node->start);
	new_node->pid = pid_in;
	new_node->delta_t = 0;
	new_node->next = NULL;
	new_node->wlist = NULL;
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
			if (current->isClosed == false) {
				clock_gettime(CLOCK_REALTIME, &current->stop);
				current->delta_t = (current->stop.tv_sec - current->start.tv_sec) + (double) (current->stop.tv_nsec - current->start.tv_nsec) / 1.0e9;
				if (close(current->fd) == -1) {
					printf("Local Server: Error in closing socket\n");
				}
				if (close(current->fd_cb) == -1) {
					printf("Local Server: Error in closing socket\n");
				}
				current->isClosed = true;
			}
			return true;
		}
		current = current->next;
	}
	return false;
}

void DeleteAppList(struct App ** head_ref) {

	struct App * current = * head_ref;
	struct App * next;

	while (current != NULL)
	{
		next = current->next;
		DeleteWatchList(&current->wlist);
		free(current);
		current = next;
	}

	* head_ref = NULL;
}

void AddKeyToList(struct WatchList ** head_ref, char * key) {

	struct WatchList * current = * head_ref;
	while (current != NULL)
	{
		if (strcmp(current->key, key) == 0) {
			return;
		}
		current = current->next;
	}

	struct WatchList * new_node = (struct WatchList *) calloc(1, sizeof(struct WatchList));
	struct WatchList * last = * head_ref;
	strcpy(new_node->key, key);
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

void DeleteWatchList(struct WatchList** head_ref) {

	struct WatchList * current = * head_ref;
	struct WatchList * next;

	while (current != NULL)
	{
		next = current->next;
		free(current);
		current = next;
	}

	* head_ref = NULL;
}

void DeleteFromWatchList(struct WatchList ** head_ref, char * key) {

	struct WatchList * temp = * head_ref, * prev;

	if (temp != NULL && (strcmp(temp->key, key)==0)) {
		* head_ref = temp->next;
		free(temp);
		return;
	}

	while (temp != NULL && (strcmp(temp->key, key)!=0)) {
		prev = temp;
		temp = temp->next;
	}

	if (temp == NULL)
		return;

	prev->next = temp->next;

	free(temp);

	return;

}

bool IsWatchList(struct WatchList * head, char * key) {

	struct WatchList * current = head;
	while (current != NULL)
	{
		if (strcmp(current->key, key) == 0) {
			return true;
		}
		current = current->next;
	}
	return false;

}
