#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "appList.h"
#include "groupList.h"

#define SIZE_IN 1000

void ShowAllGroupsInfo(struct Group * group)
{
	while (group != NULL)
	{	
        printf("Name: %s\n", group->group_name);
		printf("Secret: %s\n", group->secret);
		printf("Number of key-value pairs: %d\n", group->table->count);
        group = group->next;
	}
}

bool ShowGroupInfo(struct Group * head, char * name)
{
    struct Group * current = head;
    while (current != NULL)
    {
        if (strcmp(current->group_name, name) == 0) {
            printf("Name: %s\n", current->group_name);
            printf("Secret: %s\n", current->secret);
            printf("Number of key-value pairs: %d\n", current->table->count);
            return true;
        }
        current = current->next;
    }
    return false;
}

bool ShowAppStatus(struct Group * head, char * name)
{
    struct Group * current = head;
    while (current != NULL)
    {
        if (strcmp(current->group_name, name) == 0) {
            print_AppList(current->apps);
            return true;
        }
        current = current->next;
    }
    return false;
}

char * CreateGroup(struct Group ** head_ref, char * name)
{
    struct Group * new_node = (struct Group *) malloc(sizeof(struct Group));
    struct Group * last = * head_ref;

    char * secret_temp = "123";
    strcpy(new_node->group_name, name);
    strcpy(new_node->secret, secret_temp);
    new_node->table = create_table(SIZE_IN);
    new_node->apps = NULL;
    new_node->next = NULL;

    if (* head_ref == NULL)
    {
       *head_ref = new_node;
       return secret_temp;
    }

    while (last->next != NULL)
        last = last->next;

    last->next = new_node;
    return secret_temp;

}

bool addApp_toGroup(struct Group * head, char * name, struct sockaddr_un cl_addr_in) {
    
    struct Group * current = head;
    while (current != NULL)
    {
        if (strcmp(current->group_name, name) == 0) {
            append_App(&current->apps, cl_addr_in);
            return true;
        }
        current = current->next;
    }
    return false;
}


void deleteNode(struct Group ** head_ref, char * name)
{
    struct Group * temp = * head_ref, * prev;
 
    if (temp != NULL && (strcmp(temp->group_name, name)==0)) {
        *head_ref = temp->next;
        free(temp);
        return;
    }
 
    while (temp != NULL && (strcmp(temp->group_name, name)!=0)) {
        prev = temp;
        temp = temp->next;
    }
 
    if (temp == NULL)
        return;
 
    prev->next = temp->next;
 
    free(temp);
}




