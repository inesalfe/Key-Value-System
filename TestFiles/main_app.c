#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/un.h>
#include <sys/socket.h>
#include "appList.h"

int main()
{
	long pid1 = (long) pthread_self();
	long pid2 = (long) pthread_self()+1;
	long pid3 = (long) pthread_self()+10;
	long pid4 = (long) pthread_self()+20;

	struct sockaddr_un cl_addr;
	cl_addr.sun_family = AF_UNIX;
	snprintf(cl_addr.sun_path, sizeof(cl_addr.sun_path), "/tmp/app_socket_%ld", (long) pthread_self());

	struct sockaddr_un cl_addr_2;
	cl_addr_2.sun_family = AF_UNIX;
	snprintf(cl_addr_2.sun_path, sizeof(cl_addr_2.sun_path), "/tmp/app_socket_%ld", (long) (pthread_self()+1));

	struct sockaddr_un cl_addr_3;
	cl_addr_3.sun_family = AF_UNIX;
	snprintf(cl_addr_3.sun_path, sizeof(cl_addr_3.sun_path), "/tmp/app_socket_%ld", (long) (pthread_self()+10));

	struct sockaddr_un cl_addr_4;
	cl_addr_4.sun_family = AF_UNIX;
	snprintf(cl_addr_4.sun_path, sizeof(cl_addr_4.sun_path), "/tmp/app_socket_%ld", (long) (pthread_self()+20));

	remove(cl_addr.sun_path);
	remove(cl_addr_2.sun_path);
	remove(cl_addr_3.sun_path);
	remove(cl_addr_4.sun_path);
	
	int cfd1 = socket(AF_UNIX, SOCK_STREAM, 0);
	if (cfd1 == -1) {
		printf("Error in socket creation\n");
		exit(0);
	}
	if (bind(cfd1, (struct sockaddr *) &cl_addr, sizeof(struct sockaddr_un)) == -1) {
		printf("Error in binding\n");
		exit(0);
	}

	int cfd2 = socket(AF_UNIX, SOCK_STREAM, 0);
	if (cfd2 == -1) {
		printf("Error in socket creation\n");
		exit(0);
	}
	if (bind(cfd2, (struct sockaddr *) &cl_addr_2, sizeof(struct sockaddr_un)) == -1) {
		printf("Error in binding\n");
		exit(0);
	}

	int cfd3 = socket(AF_UNIX, SOCK_STREAM, 0);
	if (cfd3 == -1) {
		printf("Error in socket creation\n");
		exit(0);
	}
	if (bind(cfd3, (struct sockaddr *) &cl_addr_3, sizeof(struct sockaddr_un)) == -1) {
		printf("Error in binding\n");
		exit(0);
	}

	int cfd4 = socket(AF_UNIX, SOCK_STREAM, 0);
	if (cfd4 == -1) {
		printf("Error in socket creation\n");
		exit(0);
	}
	if (bind(cfd4, (struct sockaddr *) &cl_addr_4, sizeof(struct sockaddr_un)) == -1) {
		printf("Error in binding\n");
		exit(0);
	}

	struct App * head = NULL;
 
	AppendApp(&head, cfd1, pid1);

	printf("App one appended!\n");
	printf("Printing app list...\n");

	PrintAppList(head);

	AppendApp(&head, cfd2, pid2);

	printf("App two appended!\n");
	printf("Printing app list...\n");

	PrintAppList(head);

	AppendApp(&head, cfd3, pid3);

	printf("App three appended!\n");
	printf("Closing first app...\n");

	CloseConnection(head, cfd1);

	printf("Printing app list...\n");

	PrintAppList(head);

	printf("Closing second and third app...\n");

	CloseConnection(head, cfd2);
	CloseConnection(head, cfd3);

	printf("Printing app list...\n");

	PrintAppList(head);

	printf("Was the file descriptor %d found? %d\n", cfd1, FindApp(head, cfd1));

	printf("Was the file descriptor %d found? %d\n", cfd4, FindApp(head, cfd4));

	if (close(cfd1) == -1) {
		printf("Error in closing socket\n");
		exit(0);
	}
	if (close(cfd2) == -1) {
		printf("Error in closing socket\n");
		exit(0);
	}
	if (close(cfd3) == -1) {
		printf("Error in closing socket\n");
		exit(0);
	}
	if (close(cfd4) == -1) {
		printf("Error in closing socket\n");
		exit(0);
	}

	remove(cl_addr.sun_path);
	remove(cl_addr_2.sun_path);
	remove(cl_addr_3.sun_path);
	remove(cl_addr_4.sun_path);

	DeleteAppList(&head);

	return 0;
}