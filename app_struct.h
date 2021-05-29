#ifndef APP_STRUCT
#define APP_STRUCT

struct App {
    long pid;
    struct sockaddr_un cl_addr;
    struct timespec start;
    struct timespec stop;
    struct App * next;
};

#endif