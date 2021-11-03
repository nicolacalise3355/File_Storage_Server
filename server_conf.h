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

//--- Define ---// 

#define CONFIG_FILE_LEN 30
#define LEN_FILENAME 50
#define N_SEMAFORI 5 
#define MAX_SIZE_STORAGE 500
#define N 300
#define N_MAX_ARGS 50

typedef struct configuration{

	int max_file_n;
	int max_size;
	int max_thread;
	char socket_name[30];
	int thread_workers;
	char file_log_name[30];

} config;

config conf; //file di configurazione

int pipefd[2];

void setConfigurazione(int mtn, int ms, char *sn, int mt, int tw, char *lfn);
void leggiConfigurazione(const char *fileconfig);
void printConf();
void cleanup();
void removeSocketFile();