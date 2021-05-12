#include <stdio.h>
#include <unistd.h>
#include <sys/un.h>
#include <stdbool.h>

struct App {
    long pid;
    int fd;
    struct timespec start;
    struct timespec stop;
    struct App *next;
};

void print_AppList(struct App * app);

void append_App(struct App ** head_ref, int cl_fd, int pid_in);

bool close_App(struct App * head, int cl_fd);