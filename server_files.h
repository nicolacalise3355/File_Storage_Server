#if !defined(_SERVER_FILES)
#define _SERVER_FILES

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <pthread.h>
#include <assert.h>
#include <signal.h>

#define CONFIG_FILE_LEN 30
#define LEN_FILENAME 50
#define N_SEMAFORI 5 
#define MAX_SIZE_STORAGE 500
#define N 300
#define N_MAX_ARGS 50

struct clientList{

	int fd;
	struct clientList *next;

};

//typedef struct clientList cl;
//typedef cl *clientList;

typedef struct struct_file{

	char filename[LEN_FILENAME];
	char *f;
	int clientCodeLocker; //implementa lista tipo whoOpen
	struct clientList *whoOpen;
	int size;

} file;

file *storage; //lista di oggetti file 

void setStorageNull();
void makeStorage();
void freeStorage();
void freeAll();

struct clientList *addToClientList(struct clientList *l, int x);
struct clientList *removeFromClientList(struct clientList *l, int x);
int findIfAlreadyOpen(int fdt, int pos);
void addOpenerInFile(int fd, int pos);
void closeOpenerInFile(int fd, int pos);
int lockFile(int fd, int pos);
int unlockFile(int fd, int pos);
int controlIfLocked(int fd, int pos);

char *makeFileStringForPopping(file tmp);

void stampaListaClientApertiPerFile(int pos);

#endif