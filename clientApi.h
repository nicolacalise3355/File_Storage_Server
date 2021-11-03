#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <dirent.h>
#include <assert.h>
#include <signal.h>
#include <sys/stat.h>
#include <errno.h>

//Variabili Globali

int skt; //SKT per la connessione 
struct sockaddr_un sa;
int codeLastRequest;

//directory di export dei rimpiazzi e di salvataggio dopo le read
char *DIRECTORY_EXP_C; 
char *DIRECTORY_READ_C;

int countGenName; 

char *pathTmp; 

void setCodeLastRequest(int code);

int closeConnection(const char *sockname);
int openConnection(const char *sockname, int msec, const struct timespec abstime);
char *sendRequest(char *req);

int writeFile(const char *pathname, const char *dirname);
int openFile(const char *pathname, int flags);
int readFile(const char *pathname, void **buff, size_t *size);
int readNfile(int enne, const char *dirname);
int lockFile(const char *dirname);
int unlockFile(const char *dirname);
int removeFile(const char *pathname);
int closeFile(const char *pathname);
int appendToFile(const char *pathname, void *buff, size_t size, const char *dirname);

char *generateRequest(char *codeToSend, const char *request, char *arg);
char *readInternalFile(const char *filename);
