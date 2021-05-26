#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "appList.h"
#include "groupList.h"

// Maximum number of key-value pairs in each table
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

void ShowAppStatus(struct Group * group)
{
    while (group != NULL)
    {   
        printf("%s:\n", group->group_name);
        print_AppList(group->apps);
        group = group->next;
    }
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

char * getKeyValue(struct Group * head, char * name, char * key) {

    struct Group * current = head;
    while (current != NULL)
    {
        if (strcmp(current->group_name, name) == 0) {
            return ht_search(current->table, key);
        }
        current = current->next;
    }
    return NULL;
}

bool findKeyValue(struct Group * head, char * name, char * key) {

    struct Group * current = head;
    while (current != NULL)
    {
        if (strcmp(current->group_name, name) == 0) {
            if (ht_search(current->table, key) != NULL)
                return true;
            else
                return false;
        }
        current = current->next;
    }

    return false;

}

void deleteKeyValue(struct Group * head, char * name, char * key) {

    struct Group * current = head;
    while (current != NULL)
    {
        if (strcmp(current->group_name, name) == 0) {
            ht_delete(current->table, key);
            return;
        }
        current = current->next;
    }

    return;
}


bool addKeyValue_toGroup(struct Group * head, char * name, int cl_fd, char * key, char * value) {

    struct Group * current = head;
    while (current != NULL)
    {
        if (strcmp(current->group_name, name) == 0) {
            if (FindApp(current->apps, cl_fd)) {
                ht_insert(current->table, key, value);
                return true;
            }
            else {
                printf("App doesn't belong to this group\n");
                return false;
            }
        }
        current = current->next;
    }
    return false;

}

bool FindGroup(struct Group * head, char * name) {

    struct Group * current = head;
    while (current != NULL)
    {
        // printf("current->group_name: %s\n", current->group_name);
        // printf("name: %s\n", name);
        if (strcmp(current->group_name, name) == 0) {
           return true;
        }
        current = current->next;
    }
    return false;

}

bool close_GroupApp(struct Group ** head_ref, char * name, int cl_fd) {

    struct Group * current = *head_ref;
    while (current != NULL)
    {
        if (strcmp(current->group_name, name) == 0) {
            return close_App(current->apps, cl_fd);
        }
        current = current->next;
    }
    return false;

}

bool addApp_toGroup(struct Group * head, char * name, char * secret, int cl_fd, int pid_in) {
    
    struct Group * current = head;
    while (current != NULL)
    {
        if (strcmp(current->group_name, name) == 0) {
            if (strcmp(current->secret, secret) == 0) {
                append_App(&current->apps, cl_fd, pid_in);
                return true;                
            }
            else
                return false;
        }
        current = current->next;
    }
    return false;
}

void deleteGroup(struct Group ** head_ref, char * name)
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

void deleteGroupList(struct Group** head_ref) {
   struct Group* current = *head_ref;
   struct Group* next;
 
   while (current != NULL)
   {
       next = current->next;
       deleteAppList(&current->apps);
       free_table(current->table);
       free(current);
       current = next;
   }
   
   *head_ref = NULL;
}



