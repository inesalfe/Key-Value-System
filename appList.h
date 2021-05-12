#include <stdio.h>
#include <unistd.h>
#include <sys/un.h>
#include <stdbool.h>

struct App {
    long pid;
    struct sockaddr_un cl_addr;
    struct timespec start;
    struct timespec stop;
    struct App *next;
};

void print_AppList(struct App * app);

void append_App(struct App ** head_ref, struct sockaddr_un cl_addr_in);

bool close_App(struct App * head, struct sockaddr_un cl_addr_in);