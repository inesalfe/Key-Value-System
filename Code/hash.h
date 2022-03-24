typedef struct Ht_item Ht_item;
 
struct Ht_item {
    char * key;
    char * value;
};

typedef struct LinkedList LinkedList;
 
struct LinkedList {
    Ht_item * item; 
    LinkedList * next;
};
 
typedef struct HashTable HashTable;

struct HashTable {
    Ht_item ** items;
    LinkedList ** overflow_buckets;
    // Size of the table
    int size;
    // Number of elements in the table
    int count;
};

// Returns the code for a given key
unsigned long HashFunction(char * str);
// Allocates memory for a Linkedlist pointer
static LinkedList * AllocateList();
// Inserts the item onto the Linked List
static LinkedList * InsertInLinkedList(LinkedList * list, Ht_item * item);
// Frees a single Linked List
static void FreeLinkedList(LinkedList * list);
// Create the overflow buckets - an array of linkedlists
static LinkedList ** CreateOverflowBuckets(HashTable * table);
// Free all the overflow bucket lists
static void FreeOverflowBuckets(HashTable * table);
// Returns a pointer to a new hash table item
Ht_item * CreateItem(char * key, char * value);
// Creates a new table and returns a pointer for it
HashTable * CreateTable(int size);
// Frees an item
void FreeItem(Ht_item * item);
// Frees the table
void FreeTable(HashTable * table);
// When two keys have the same hash function code, 
void HandleCollision(HashTable * table, unsigned long index, Ht_item * item);
// Insert item into the table
void InsertToTable(HashTable * table, char * key, char * value);
// Searches the table for a given key and returns it's value (or NULL if the key doesn't exist)
char * SearchInTable(HashTable * table, char * key);
// Deletes item with a given key from the table
void DeleteFromTable(HashTable * table, char * key);
// Prints the table
void PrintTable(HashTable * table);