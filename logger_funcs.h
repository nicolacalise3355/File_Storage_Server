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
#define MAX_SIZE_LOGS_LINE 200

static pthread_mutex_t mtxl = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  condl = PTHREAD_COND_INITIALIZER;

int logFileUsed;

void printLoggerFileName();
void openFileLogs();
char *makeLineToWrite(char *args[], int n);
void writeToLogs(char *line);

void makeInsertLog(char *filename,int fd);
void makeOpenFileInStorageLog(char *filename, int fd);
void makeCloseFileInStorageLog(char *filename, int fd);
void makeWriteFileLog(char *filename, int sizeBytes);
void makeStoragePopLog(char *filename, int size);
void makeReadFileLog(char *filename, int nBytes);
void makeReadNfileLog(int n);
void makeOpenLockFileLog(char *filename);
void makeLockFileLog(char *filename);
void makeUnlockFileLog(char *filename);
void makeCloseFileLog(char *filename);

void makeFinalLog(int maxNumber, int maxSize, int rimpiazzi, int maxConnessCont, int **nRichiestePerWorker, int nThreads);

