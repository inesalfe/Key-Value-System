#include "hash.h"

#define BUF_SIZE 100

struct Group {
    char group_name[BUF_SIZE];
    char secret[BUF_SIZE];
    struct HashTable * table;
    struct App * apps;
    struct Group * next;
};

void ShowAllGroupsInfo(struct Group * group);

bool ShowGroupInfo(struct Group * head, char * name);

bool FindGroup(struct Group * head, char * name);

void ShowAppStatus(struct Group * head);

char * CreateGroup(struct Group ** head_ref, char * name);

void deleteGroup(struct Group ** head_ref, char * name);

bool close_GroupApp(struct Group ** head_ref, char * name, int cl_fd);

bool addApp_toGroup(struct Group * head, char * name, char * secret, int cl_fd, int pid_in);