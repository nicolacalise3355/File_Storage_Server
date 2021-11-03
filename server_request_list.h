#if !defined(SRL)
#define SRL

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

#define N 300

struct client_request{
	int fd;
	char *request;
	int status;
	struct client_request *next;
};
/*
int numeroClientConnessi = 0;
struct client_request *listaRichieste = NULL;
int listaOccupata = 0;
*/
int numeroClientConnessi;
struct client_request *listaRichieste;
int listaOccupata;

int conteggioMaxConnessioni;
int numeroConnessioniAttuali;

//struct client_request *currentClientReq = NULL;

static pthread_mutex_t mtxRL = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  condRL = PTHREAD_COND_INITIALIZER;

void setListReqOcc();
void freeListReqOcc();

void initListReq();

int getListReqOcc();

void stampaRichiesteClients();
struct client_request *addToClientRequestList(struct client_request *l, int x, char *newRequest, int stat);
struct client_request *removeFromClientRequestList(struct client_request *l, int x);
struct client_request *popClientRequest(struct client_request *l);
//void setCurrentRequest(struct client_request *curr);

void importClientReq(int x, char *newReq);
char *exportClientReq();
void printReqList();
char *getLastReq(struct client_request *l);
struct client_request exportClientRequest();

void incrCurrentClientConn();
void dectCurrentClientConn();
int getCurrentClientConn();

char *parseRequest(char *request);

#endif