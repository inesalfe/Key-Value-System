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

bool ShowAppStatus(struct Group * head, char * name);

char * CreateGroup(struct Group ** head_ref, char * name);

void deleteNode(struct Group ** head_ref, char * name);

bool addApp_toGroup(struct Group * head, char * name, struct sockaddr_un cl_addr_in);