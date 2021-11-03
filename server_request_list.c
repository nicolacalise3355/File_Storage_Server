
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
#include <errno.h>

#include "server_request_list.h"
#include "logger_funcs.h"

//Funzioni per la gestione del numero di connessioni contemporanee massime

void verificaMaxConnessioniCont(){
	if(numeroConnessioniAttuali > conteggioMaxConnessioni){
		conteggioMaxConnessioni = numeroConnessioniAttuali;
	}
}

void incrConnessioniAttuali(){
	numeroConnessioniAttuali = numeroConnessioniAttuali + 1;
}

void decrConnessioniAttuali(){
	numeroConnessioniAttuali = numeroConnessioniAttuali - 1;
}

int getConnessioniMassimeContemporanee(){
	return conteggioMaxConnessioni;
}

//Inizializza la lista delle richieste
void initListReq(){
	numeroClientConnessi = 0;
	listaRichieste = NULL;
	listaOccupata = 0;
	conteggioMaxConnessioni = 0;
    numeroConnessioniAttuali = 0;
}

/**/
//libera la lista delle richieste
void freeListaRequest(){
	for(listaRichieste; listaRichieste != NULL; listaRichieste = listaRichieste->next){
		free(listaRichieste->request);
	}
	free(listaRichieste);
}

//gestione della lista delle richieste (occupato o no)

void setListReqOcc(){
	listaOccupata = 1;
}
void freeListReqOcc(){
	listaOccupata = 0;
}

// 1 occupato, 0 no
int getListReqOcc(){
	return listaOccupata;
}

// --------- // 

//Stampa le richieste di clients
void stampaRichiesteClients(){
	struct client_request *tmp;
	printf("Richieste pendenti: \n");
	if(listaRichieste == NULL){
		printf("Lista richieste pendendi vuota\n");
		return;
	}
	for(tmp = listaRichieste; tmp != NULL; tmp = tmp->next){
		printf("Richiesta di %d -> %s\n", tmp->fd,tmp->request);
	}
}

void printReqList(){
	printf("--\n");
	stampaRichiesteClients(listaRichieste);
}

/*
Aggiunge una richiesta alla lista delle richieste
*/
struct client_request *addToClientRequestList(struct client_request *l, int x, char *newRequest, int stat){
	struct client_request *new; 
	new = calloc(1,sizeof(struct client_request));
	new->fd = x;
	new->status = stat;
	//int newLen = strlen(newRequest);
	new->request = calloc((N+2),sizeof(char));
	strncpy(new->request,newRequest,N);
	new->next = l;
	//printf("new->status = %d\n", new->status);
	
	return new;
}

/*
Rimuove una richiesta dalla lista
*/
struct client_request *removeFromClientRequestList(struct client_request *l, int x){
	struct client_request *pre;
	struct client_request *post;

	for(post = l, pre = NULL;
		post != NULL, post->fd != x;
		pre = post, post = post->next
	);
		if(post == NULL){
			return l;
		}else if(pre == NULL){
			l = l->next; 
		}else{
			pre->next = post->next;
		}
		free(post->request);
		free(post);
		return l;
}

/*
Recupera l ultima richiesta dalla lista
*/
char *getLastReq(struct client_request *l){
	struct client_request *tmp = l;
	while(tmp->next != NULL){
		tmp = tmp->next;
	}
	char *tmpResp = malloc(sizeof(char)*(N+1));
	strncpy(tmpResp, tmp->request,(N+1));
	return tmpResp;
}
//Recupera l fd dell ultima richiesta
int getLastFd(struct client_request *l){
	struct client_request *tmp = l;
	while(tmp->next != NULL){
		tmp = tmp->next;
	}
	int x = tmp->fd;
	return x;
}
//Recupera lo status dell ultima richiesta
int getLastStatus(struct client_request *l){
	struct client_request *tmp = l;
	while(tmp->next != NULL){
		tmp = tmp->next;
	}
	int x = tmp->status;
	return x;
}

/*
Restituisce l ultima richiesta della lista
*/
struct client_request *getLastRequest(struct client_request *l){
	struct client_request *tmp = l;
	while(tmp->next != NULL){
		tmp = tmp->next;
	}
	return tmp;
}

/*
Rimuove l ultima richiesta dalla lista e la restituisce
*/
struct client_request *popClientRequest(struct client_request *l){
	struct client_request *tmp = l;
	while(tmp->next != NULL){
		tmp = tmp->next;
	}
	return removeFromClientRequestList(l,tmp->fd);	

}

//Readn per leggere l invio dal client
int readn(long fd, void *buf, size_t size) {
	size_t left = size;
	int r;
	char *bufptr = (char*)buf;
	errno = 0;
	while(left>0) {
		//printf("prereadinsidewhile\n");
		if ((r=read((int)fd ,bufptr,left)) == -1) {
			printf("bad\n");
			if (errno == EINTR) {printf("inserr\n"); continue;}
			return -1;
		}
		//printf("bufptr %s\n", bufptr);
		if (r == 0) {printf("ret0");return 0;} // EOF
		left -= r;
		//printf("-1\n");
		bufptr += r;
		//printf("-2\n");
	}
	//printf("pre return size\n");
	return size;
}

//Inserisce una richiesta killer per chiudere i Threads
void insertKillerRequest(int killerCode,int numberOfThreads){

	pthread_mutex_lock(&mtxRL);
	int i;
	for(i = 0; i < numberOfThreads; i++){
		listaRichieste = addToClientRequestList(listaRichieste,killerCode,"killerreq",0);
	}
	printf("Richieste inserite\n");
	pthread_cond_signal(&condRL);
	pthread_mutex_unlock(&mtxRL);
}

/*
* Importa una richiesta inviata da un client leggendo il contentuo della readn
*/
void importClientReq(int x, char *newReq){
	char *bufferRead = malloc(sizeof(char)*N);

	pthread_mutex_lock(&mtxRL);

	int res = readn(x,bufferRead,N); //n,file1

	if(res == 0){
		printf("Fine delle richieste di un client\n");
		decrConnessioniAttuali();
		free(bufferRead);
		close(x);
		pthread_cond_signal(&condRL);
		pthread_mutex_unlock(&mtxRL);
		return;
	}else if(res == -1){
		printf("Errore nella readn\n");
		free(bufferRead);
		close(x);
		pthread_cond_signal(&condRL);
		pthread_mutex_unlock(&mtxRL);
		return;
	}
	printf("RESULT READN = %d\n", res);

	listaRichieste = addToClientRequestList(listaRichieste,x,bufferRead,res);
	free(bufferRead);
	//signal condRL
	pthread_cond_signal(&condRL);
	pthread_mutex_unlock(&mtxRL);


}

/*
* Esporta e restituisce una richiesta dalla lista delle rich
*/
struct client_request exportClientRequest(){

	pthread_mutex_lock(&mtxRL);

	//while elem = 0 aspetta 
	while(controlListReqNull() == 1){
		pthread_cond_wait(&condRL, &mtxRL);
	}

	struct client_request tmpReq;
	//tmpReq = getLastRequest(listaRichieste);

	char *newReq = getLastReq(listaRichieste);
	int newFd = getLastFd(listaRichieste);
	int newStatus = getLastStatus(listaRichieste);

	tmpReq.request = malloc(sizeof(char)*N+1);
	strcpy(tmpReq.request,newReq);
	tmpReq.fd = newFd;
	tmpReq.status = newStatus;


	listaRichieste = popClientRequest(listaRichieste);

	pthread_cond_signal(&condRL);
	pthread_mutex_unlock(&mtxRL); 
	free(newReq);
	return tmpReq;
}


/*
Esporta e restituisce una richiesta dalla lista delle rich
*/
char *exportClientReq(){

	pthread_mutex_lock(&mtxRL);

	//while elem = 0 aspetta 
	while(controlListReqNull() == 1){
		pthread_cond_wait(&condRL, &mtxRL);
	}

	struct client_request tmpReq;

	char *tmpString = getLastReq(listaRichieste);
	listaRichieste = popClientRequest(listaRichieste);

	printf("RICHIESTA %s\n", tmpString);

	pthread_mutex_unlock(&mtxRL); 

	return tmpString;
}

/*
Controlla che la lista delle rich non sia nulla
*/
int controlListReqNull(){
	if(listaRichieste == NULL){
		return 1;
	}else{
		return 0;
	}
}

/*

*/
void incrCurrentClientConn(){
	numeroClientConnessi = numeroClientConnessi + 1;
}
void dectCurrentClientConn(){
	numeroClientConnessi = numeroClientConnessi - 1;
}
int getCurrentClientConn(){
	return numeroClientConnessi;
}
//legge la stringa di una richiesta e restituisce i caratteri validi 
//in quanto contiene X caratteri extra per garantire la dimensione fissa del mex
char *parseRequest(char *request){
	int ci2,ci3;
	int countChar = 0;

	//printf("reqInside = %s\n", request);

	for(ci2 = 0; ci2 < N; ci2++){
		if(request[ci2] != 126){
			countChar++;
		}
	}
	//printf("countChar = %d\n", countChar);
	char *finalInput = malloc(sizeof(char)*(countChar+1));

	for(ci3 = 0; ci3 < countChar; ci3++){
		finalInput[ci3] = request[ci3];
	}


	char *final = malloc(sizeof(char)*countChar+1);
	strcpy(final,"");
	strncat(final,finalInput,countChar);
	free(finalInput);
	printf("final %s + %d\n",final,(int)strlen(final));
	return final;
}