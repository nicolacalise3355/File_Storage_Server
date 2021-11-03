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

#include "server_conf.h"
#include "server_utils.h"
#include "server_files.h"

#define CONFIG_FILE_LEN 30
#define LEN_FILENAME 50
#define MAX_FILE_SIZE 1000
#define N_SEMAFORI 5 
#define MAX_SIZE_STORAGE 500
#define N 300
#define N_MAX_ARGS 50

/**
* al contrario di cosa potrebbe far capire il nome non fa uno switch
ma copia gli elementi del file in pos j nel file di pos i, simile ad una strcpy
*/
void switchPos(int i, int j){
	int maxS = conf.max_size;
	storage[i].f = (char*) realloc(storage[i].f,sizeof(char)*maxS);
	strcpy(storage[i].f,storage[j].f);
	strcpy(storage[i].filename,storage[j].filename);
	//printf("POST SWIOTCH %s\n", storage[i].f);
	storage[i].whoOpen = storage[j].whoOpen;
	storage[i].size = storage[j].size;
	storage[i].clientCodeLocker = storage[j].clientCodeLocker;
}


/*
@effects: setta tutte le posizioni dello storage a NULL
*/
void setStorageNull(){
	int nf = conf.max_file_n;
	int maxS = conf.max_size;
	if(!nf){
		exit(0);
		return;
	}
	if(!maxS){
		exit(0);
	}
	printf("%d %d\n", nf, maxS);
	int i;
	for(i = 0; i < nf; i++){
		storage[i].f = malloc(sizeof(char)*MAX_FILE_SIZE);
		strcpy(storage[i].f,"");
		strcpy(storage[i].filename,"");
		storage[i].whoOpen = NULL;
		storage[i].size = 0;
		storage[i].clientCodeLocker = -1;
	}
}

/*
@effects: alloca una lista chiamata storage che contiene un numero di file disponibili
*/
void makeStorage(){
	int nf = conf.max_file_n;
	if(!nf){
		printf("Errore grave\n");
		exit(0);
	}
	storage = calloc(nf,sizeof(file));
	setStorageNull();
}

/*
@effects: de alloca lo storage
*/
void freeStorage(){
	free(storage);
}


/*
@effects: libera la memoria dallo storage
*/
void freeAll(){
	int i;
	for(i = 0; i < conf.max_file_n; i++){
		if(storage[i].whoOpen != NULL){
			free(storage[i].whoOpen);
		}
		if(storage[i].f != NULL){
			free(storage[i].f);
		}
	}
	free(storage);
}

/*
* @params: l lista dove aggiungere il client
* @params: x file descriptor del client
* @effects: aggiunge un nuovo nodo alla lista 
* @return: ritorna la lista aggiornata 
*/
struct clientList *addToClientList(struct clientList *l, int x){
	struct clientList *new; 
	new = malloc(sizeof(struct clientList));
	new->fd = x;
	new->next = l;
	return new;
}

/*
*@params: l lista da dove rimuovere il client
*@params: x file descriptor del client
*@effects: rimuove il fd dalla lista dei client
*@return: lista aggiornata 
*/

struct clientList *removeFromClientList(struct clientList *l, int x){
	struct clientList *pre;
	struct clientList *post;

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
		free(post);
		return l;
}

/*
* @params: fdt -> file descriptor del client
* @params: pos -> posizione dello storage da controllare
* @effects: controlla la lista di chi ha aperto il file
* @return: 
*/
int findIfAlreadyOpen(int fdt, int pos){
	struct clientList *l = storage[pos].whoOpen;
	for(l; l != NULL; l = l->next){
		if(l->fd == fdt){
			return 1;
		}
	}
	return 0;
}


/*
*@params: fd client
*@params: posizione del file
*@effects: aggiunge un client alla lista dei client che hanno aperto quel file
*/
void addOpenerInFile(int fd, int pos){
	//printf("addfd %d\n", fd);
	storage[pos].whoOpen = addToClientList(storage[pos].whoOpen,fd);
}

/*
*@params: fd client
*@params: posizione del file
*@effects: rimuove un client alla lista dei client che hanno aperto quel file
*/
void closeOpenerInFile(int fd, int pos){
	storage[pos].whoOpen= removeFromClientList(storage[pos].whoOpen,fd);
}

/*
*@params: fd client
*@params: posizione del file
*@effects: setta il locker di un file con un client fd
*/
int lockFile(int fd, int pos){

	if(pos == -1){ return -1; }

	if(storage[pos].clientCodeLocker == -1){
		//printf("Lock ok\n");
		storage[pos].clientCodeLocker = fd;
		return 1;
	}else{
		//printf("Problemi con la lock\n");
		//si mette in lista 
		return 0;
	}
}

/*
*@params: fd client
*@params: posizione file
*@effects: sblocca il locker di un file
*/
int unlockFile(int fd, int pos){
	if(storage[pos].clientCodeLocker == fd){
		storage[pos].clientCodeLocker = -1;
		return 1;
	}else{
		return 0;
	}
}

/*
Controlla se un file e' lockato da un client tramite il suo fd
*/
int controlIfLocked(int fd, int pos){
	if(storage[pos].clientCodeLocker == fd){
		return 1;
	}else if(storage[pos].clientCodeLocker != fd && storage[pos].clientCodeLocker != -1){
		return -1;
	}else if(storage[pos].clientCodeLocker == -1){
		return 0;
	}
}

/*
Stampa la lista dei client aperti per ogni file
*/
void stampaListaClientApertiPerFile(int pos){
	struct clientList *tmp = storage[pos].whoOpen;
	struct clientList *ciclo;
	if(tmp == NULL){
		printf("Nessun Client ha aperto il file\n");
	}else{
		for(ciclo = tmp; ciclo != NULL; ciclo = ciclo->next){
			printf("Client fd = %d\n", tmp->fd);
		}
	}

}

/*
Genera la stringa che viene restituita al client per l espulsione di un file
*/
char *makeFileStringForPopping(file tmp){
	int size = tmp.size + LEN_FILENAME + 2;
	char *content = malloc(sizeof(char)*size);
	strcpy(content,tmp.filename);
	strcat(content,"#");
	strcat(content,tmp.f);
	return content;
}