#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/select.h>
#include <pthread.h>
#include <assert.h>
#include <signal.h>

#include "server_conf.h"
#include "server_utils.h"
#include "server_files.h"
#include "logger_funcs.h"
#include "server_request_list.h"

//--- Define ---// 

#define CONFIG_FILE_LEN 30
#define LEN_FILENAME 50
#define N_SEMAFORI 5 
#define MAX_SIZE_STORAGE 500
#define N 300
#define N_MAX_ARGS 50
#define MAX_SIZE_LOGS_LINE 200

#define QUIT_CODE 1
#define PROGRESS 0

#define KILLER_CODE -2

//int countSignalExit = 0;

//--- Strutture dati e tipi di files ---// 

pthread_t manager_thread_pid;
pthread_t *workers;

int master_quit;
int workload_quitter;

//--- Variabili globali ---// 

int storagepc = 0; //counter dello storage per capire a che punto abbiamo saturato la memoria


static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cond = PTHREAD_COND_INITIALIZER;

static pthread_mutex_t mtx0 = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cond0 = PTHREAD_COND_INITIALIZER;

static pthread_mutex_t mtx1 = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cond1  = PTHREAD_COND_INITIALIZER;

static pthread_mutex_t mtx2 = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cond2  = PTHREAD_COND_INITIALIZER;

static pthread_mutex_t mtx3 = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cond3  = PTHREAD_COND_INITIALIZER;

static pthread_mutex_t mtx4 = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cond4  = PTHREAD_COND_INITIALIZER;


/*
posizioni:
0 -> conteggioRimpiazzi
1 -> numeroFileMaxNelServer
2 -> numeroByMaxNelServer
3 -> storagepc
4 -> storage
*/
int semaforo[N_SEMAFORI]; //indici rappresentao delle strutture dati usate dal server

int conteggioRimpiazzi = 0; //indica il numero dei rimpiazzi che sono stati eseguiti
int numeroFileMaxNelServer = 0; //indica il numero di file massimo che ci sono stati nel server
long int numeroKbMaxNelServer = 0; //indica il numero di mb max raggiunti nel server
long int numeroByMaxNelServer = 0;


struct sigaction sgac;
struct sigaction sgac2;
struct sigaction sgac3;

int **conteggioRichiestePerThread;


//--- Dichiarazione di funzioni ---// 

void modifyStorageCount(int flag);
int openFileInStorage(char *filename, int fd);
void finalLogger();
void stampaStorage();
void stampaListaFile();

void *workload(void *p);
void *manager(void *p);


//--- Funzioni controllo Thread e Connessioni ---//

//Funzioni per la gestione dei conteggi delle richieste x Thread

void setUpConteggioRichiestePerThread(){
	int nThread = conf.thread_workers;
	int i;
	
	conteggioRichiestePerThread = (int**)malloc(sizeof(int*)*nThread);
	for(i = 0; i < nThread; i++){
		conteggioRichiestePerThread[i] = malloc(sizeof(int)*2);
		conteggioRichiestePerThread[i][0] = i;
		conteggioRichiestePerThread[i][1] = 0;
	}
}

void increConteggioRichiestePerThread(long int codice){
	conteggioRichiestePerThread[codice][1] = conteggioRichiestePerThread[codice][1] + 1;
}


void freeConteggioRichiestePerThread(){
	int nThread = conf.thread_workers;
	int i;
	for(i = 0; i < nThread; i++){
		free(conteggioRichiestePerThread[i]);
	}
	free(conteggioRichiestePerThread);
}



//--- Funzioni di semaforo ---//


void setUpSemafori(){
	int i;
	for(i = 0; i < N_SEMAFORI; i++){
		semaforo[i] = 0;
	}
}

void setSemaforo(int i){
	semaforo[i] = 1;
}

void freeSemaforo(int i){
	semaforo[i] = 0;
}

/*
@params: i rappresenta un indice della lista dei semafori, uno rappresenta un semaforo con utilizzo diverso
@effects: controlla lo stato del semaforo
@return: ritorna il valore del semaforo
*/
int controlloSemaforo(int i){
	if(semaforo[i] == 1){
		//siamo occupati
		return 1;
	}else if(semaforo[i] == 0){
		//siamo liberi
		return 0;
	}
	return -1;
}

void controllerMultiploSem(){
	int i;
	for(i = 0; i < N_SEMAFORI; i++){
		printf("Sem %d = res %d\n",i, semaforo[i]);
	}
}

void timeOutSem(int i){
	sleep(5);
	freeSemaforo(i);
}



//----- Funzioni di configurazione -----// 


/*
@effects: setta i segnali che dobbiamo gestire
*/ 
void setSegnali(){
	sigaction(SIGINT, &sgac,NULL);
	sigaction(SIGQUIT, &sgac2,NULL); 
	sigaction(SIGHUP, &sgac3,NULL);
}



/*
@params: int segnale arrivato
@effects: setta cosa succede quando arriva uno specifico segnale
nel caso 1 si aspetta che tutti i thread siano finiti
*/
static void segnalatore(int signum){
	printf("Ricevuto segnale: %d\n", signum);
	if(signum == 1){
		//IMPLEMENTARE ATTESA PER LA FINE DEI LAVORI
		workload_quitter = QUIT_CODE;
		master_quit = QUIT_CODE;
		insertKillerRequest(KILLER_CODE,conf.thread_workers);
	}else{
		workload_quitter = QUIT_CODE;
		master_quit = QUIT_CODE;
		insertKillerRequest(KILLER_CODE,conf.thread_workers);
	}

}

//----- Funzioni di utlita' sui conteggi -----//



// incrementa numero dei rimpiazzi 
void incrCR(){ 
	pthread_mutex_lock(&mtx0);
	while(controlloSemaforo(0) == 1){
			pthread_cond_wait(&cond0, &mtx0);
	}
	setSemaforo(0);
	conteggioRimpiazzi = conteggioRimpiazzi + 1;
	freeSemaforo(0);
	pthread_mutex_unlock(&mtx0);
	
}
//incrementa numero dei file totali mai contenuti nel server
void incrNfM(){
	pthread_mutex_lock(&mtx1);
	while(controlloSemaforo(1) == 1){
			pthread_cond_wait(&cond1, &mtx1);
	}
	setSemaforo(1);
	numeroFileMaxNelServer = numeroFileMaxNelServer + 1;
	freeSemaforo(1);
	pthread_mutex_unlock(&mtx1); 
	
}
//incremente numero dei bytes contenuti nel server
void increNkb(char *tmp){


	pthread_mutex_lock(&mtx2);
	while(controlloSemaforo(2) == 1){
			pthread_cond_wait(&cond2, &mtx2);
	}
	setSemaforo(2);
	
	long int tmpBy = strlen(tmp);
	long tmpKb = tmpBy / 1024;
	numeroByMaxNelServer = numeroByMaxNelServer + tmpBy;
	numeroKbMaxNelServer = numeroKbMaxNelServer + tmpKb;

	freeSemaforo(2);
	pthread_mutex_unlock(&mtx2);

}






//----- Funzioni di utilita' sullo storage -----//


/*
@params: fn -> filename
@effects: cerca un file nello storgae
@return: se lo trova ritorna la posizione o -1 altrimenti
*/
int findFileInStorage(char *fn){ //X 
	int len = conf.max_file_n;
	int i;
	for(i = 0; i < len; i++){
		if(strcmp(fn,storage[i].filename) == 0){
			return i;
		}
	}
	printf("File non trovato\n");
	return -1;
}

/*
@params: file appena generato
@effects: controlla che la grandezza in MB dei file non superi un certo limite impostato da config
@return: 1 se non superiamo il limite, -1 se superiamo il limite
*/
long int controlloGrandezzaStorage(){

	// da sommare grandezza del nuovo file con quello dello storage

	long int fileSizeInBytes = 0;
	int i;
	for(i = 0; i < conf.max_file_n; i++){
		if(strcmp(storage[i].f,"") != 0){
			//FILE *tmpFile = storage[i].f;
			long int tmp = (long int)storage[i].size;
			fileSizeInBytes = fileSizeInBytes + tmp;
			//fclose(tmpFile);
		}
	}

	// Convert the bytes to Kilobytes (1 KB = 1024 Bytes)
	long fileSizeInKB = fileSizeInBytes / 1024;
	// Convert the KB to MegaBytes (1 MB = 1024 KBytes)
	long fileSizeInMB = fileSizeInKB / 1024;

	return fileSizeInBytes;
}

/*
@params: tmpS stringa contente i dati del file da aggiungere
@effects: controlla se la grandezza attuale dello storage + quella della nuova stringa supera la Q. MAX
@return: risultato del controllo 1 o -1 a seconda se supera o no il valore max
*/
int controlloGrandezzaFinale(char *tmpS){ 
	
	long int valoreTmp = controlloGrandezzaStorage();
	long int valoreToAdd = sizeof(tmpS);
	long int newValore = valoreTmp + valoreToAdd;

	if(newValore <= conf.max_size){
		return 1;
	}else{
		return -1;
	} 
	return -1;
}



//----- Funzioni di lavoro sullo storage -----//


/*
@params: FILE di cui vogliamo conoscere la grandezza
@effects: calcola la grandezza del file
@return: long contenente grandezza del file
@link: 
*/
long int findSizeThisFile(char *fn){
	int pos = findFileInStorage(fn);
	if (pos == -1) {
        //printf("File Not Found!\n");
        return 0;
    }
	long int tmpRes = 0;
	if(strcmp(storage[pos].f,"") != 0){
		tmpRes = strlen(storage[pos].f);
	}
    return 0;
}
//recupera la size del file
int getFileSize(char *fn){
	int pos = findFileInStorage(fn);
	if(pos == -1){ return 0; }
	return storage[pos].size;
}

//Read senza controllare l fd se e' nella lista o se file e' bloccato
char *readWcControl(char *fn){
	int pos = findFileInStorage(fn);
	if(pos == -1){ return "NULL"; }
	if(pos != -1){
		if(strcmp(storage[pos].f,"") != 0){
			return storage[pos].f;
		}
	}
}

/*
@params: fn -> filename
@effects: recupera il contenuto del file attraverso una funzione ausiliria
@return: restituisce il contenuto del file
*/
char *readFile(char *fn, int fd, int flag){

	int pos = findFileInStorage(fn);
	if(pos == -1){ return "NULL"; }
	if(flag == 0){
		int lfd = findIfAlreadyOpen(fd,pos);
		if(lfd == 0){
			printf("Il Client non ha aperto il file\n");
			return NULL;
		}
		int lg = controlIfLocked(fd,pos);
		if(lg == -1){
			printf("Il file e' Lockato da un altro client\n");
			return "NULL";
		}
	}


	if(pos != -1){
		if(strcmp(storage[pos].f,"") != 0){
			makeReadFileLog(storage[pos].filename, storage[pos].size);
			return storage[pos].f;
		}
	}else{
		return "";
	}
}


/*
@params: s -> numero di file da leggere
@effects: andiamo a formattare una stringa filename1:data1#filename2:data2#.... che contiene
nome e dati dei file recuperati, a seconda di N sappiamo quanti file inviare.
@return: stringa formattata che contiene nome e dati dei primi N/tutti files
*/
char *readNfile(char *s, int fd){
	int n = atoi(s);
	long int size = controlloGrandezzaStorage();
	long int max_size = size * 2; //da considerare la formattazione della stringa	
	char *finalString = malloc(sizeof(char)*max_size);
	strcpy(finalString,"");
	int i;
	int range = conf.max_file_n;
	if(n <= 0){
		//tutto
		for(i = 0; i < range; i++){
			if(strcmp(storage[i].f,"") != 0){
				char *tmpString1 = malloc(sizeof(char)*size);
				strcpy(tmpString1,storage[i].filename);
				strcat(tmpString1,"¬");
				char *tmpString2 = readFile(storage[i].filename,fd,1);
				strcat(finalString,tmpString1);
				strcat(finalString,tmpString2);
				strcat(finalString,"#");
				free(tmpString1);
				free(tmpString2);
			}
		}
	makeReadNfileLog(range);
	}else if(n > range){
		//tutto
		for(i = 0; i < range; i++){
			if(strcmp(storage[i].f,"") != 0){
				char *tmpString1 = malloc(sizeof(char)*size);
				strcpy(tmpString1,storage[i].filename);
				strcat(tmpString1,"¬");
				char *tmpString2 = readFile(storage[i].filename,fd,1);
				strcat(finalString,tmpString1);
				strcat(finalString,tmpString2);
				strcat(finalString,"#");
				free(tmpString1);
				free(tmpString2);
			}
		}
	makeReadNfileLog(range);
	}else{
		//primi N
		for(i = 0; i < n; i++){
			if(strcmp(storage[i].f,"") != 0){
				//char *tmpString1 = malloc(sizeof(char)*size);
				char *tmpString2 = malloc(sizeof(char)*size);
				//strcpy(tmpString1,storage[i].filename);
				//strcat(tmpString1,"¬");
				if(strcmp(storage[i].f,"") != 0){
					strncpy(tmpString2,storage[i].f,storage[i].size);
					//strcat(finalString,tmpString1);
					strcat(finalString,"#");
					strncat(finalString,tmpString2,storage[i].size);
				}else{
					n = n + 1;
					if(n == range){
						return finalString;
					}
				}
				//free(tmpString1);
				free(tmpString2);
			}
		}
	makeReadNfileLog(n);
	}
	return finalString;
}

/*
@effects: effettua la pop del file in posizione 0 e poi sposta tutti i file avanti di una pos.
@return: file poppato
*/
file storagePop(){ 

	file tmp;
	//fai una riallocazione su tmp

	strcpy(tmp.filename,storage[0].filename);
	tmp.f = malloc(sizeof(char)*conf.max_size);
	tmp.whoOpen = NULL;
	strcpy(tmp.f,storage[0].f);

	int flag = 0;
	int i;
	int j;
	int final = conf.max_file_n-1;
	int maxS = conf.max_size;
	for(i = 0; i < (conf.max_file_n-1); i++){
		j = i + 1;
		//storage[i] = storage[j];
		switchPos(i,j);
	}
	free(storage[final].f);
	storage[final].f = malloc(sizeof(char)*maxS);
	strcpy(storage[final].f,"");
	strcpy(storage[final].filename,"");
	storage[final].whoOpen = NULL;
	storage[final].size = 0;
	storage[final].clientCodeLocker = -1;
	//storage[conf.max_file_n-1].f = NULL;
	modifyStorageCount(flag);
	makeStoragePopLog(tmp.filename, tmp.size);

	return tmp;
}

/*
@effects: ricerca una posizione libera nello storage
@return: posizione libera trovata o -1
*/
int findFreeStoragePos(){ 
	int range = conf.max_file_n;
	int i;
	for(i = 0; i < range; i++){
		if(strcmp(storage[i].filename,"") == 0){
			return i;
		}
	}
	return -1;
}


/*
@params: flag -> in base a quello sappiamo se scendere o salire con il count;
@effects: se il flaf e' a 0 allora proviamo a ridurre il count in quanto abbiamo eliminato un file
se il file e' a 1 proviamo ad aumentarlo, se siamo gia' a al numero max di file non aumenta
ma rimane uguale
*/
void modifyStorageCount(int flag){

	pthread_mutex_lock(&mtx3);
	while(controlloSemaforo(3) == 1){
			pthread_cond_wait(&cond3, &mtx3);
	}
	setSemaforo(3);
	
	switch(flag){
		case 0: //cancellazione
			//printf("0\n");
			if(storagepc > 0){
				storagepc = storagepc - 1;
			}
			break;
		case 1: //inserimento
			//printf("1\n");
			if(storagepc < conf.max_file_n){
				storagepc = storagepc + 1;
			}
			break;
	}

	freeSemaforo(3);
	pthread_mutex_unlock(&mtx3);

}

/*
@params: input_filename -> nome del file da inserire
@effects: acquisice ME sullo storage , cerca un pos libera, se non c'e fa una pop e chiama ricorsivamente
se stessa per effettuare un nuovo tentativo. alla fine inserisce il file (vuoto) nello storage
@return: 1 successo -1 se troviamo doppione
*/
char *insertFile(char *input_filename, char *espulsione, int fd){ 
	char *espTmp = malloc(sizeof(char)*(N+1));
	strcpy(espTmp,espulsione);
	char *res;

	int flag = 1;
	pthread_mutex_lock(&mtx4);
	while(controlloSemaforo(4) == 1){
			pthread_cond_wait(&cond4, &mtx4);
	}
	setSemaforo(4);
	int doppione = findFileInStorage(input_filename);
	if(doppione != -1){
		//doppione trovato
		freeSemaforo(4);
		pthread_mutex_unlock(&mtx4);
		free(espTmp);
		return "-1";
	}
	int tmpPos = findFreeStoragePos();
	if(tmpPos != -1){
		printf("POS LIBERA %d \n", tmpPos);
		strcpy(storage[tmpPos].filename,input_filename);
		storage[tmpPos].size = 0;
		incrNfM();
		modifyStorageCount(flag);
		makeInsertLog(input_filename,fd);
		//---
		freeSemaforo(4);
		pthread_mutex_unlock(&mtx4);
		printf("espTmp pre fine %s\n", espTmp);
		if(strcmp(espTmp,"") == 0){
			free(espTmp);
			return "1";
		}
		return espTmp;
	}else{
		printf("Posizione libera non trovata dobbiamo fare una pop\n");
		file tmp = storagePop();
		printf("File poppato %s\n", tmp.filename);
		incrCR();
		char *string = makeFileStringForPopping(tmp);
		strncat(espTmp,string,N);
		freeSemaforo(4);
		pthread_mutex_unlock(&mtx4);
		printf("espTmp %s\n", espTmp);
		res = insertFile(input_filename, espTmp,fd);
		//free(espTmp);
		free(tmp.f);
		free(string);
		if(tmp.whoOpen != NULL){ free(tmp.whoOpen); }
		return res;
	}

	freeSemaforo(4);
	pthread_mutex_unlock(&mtx4);
	
}




/*
@params: flaga -> serve per capire se dobbiamo aumentare il numero dei KB del server o no
		input_filename -> nome del file da scrivere
		input_s -> dati da scrivere		
@effects: cerchaimo un file in storage, se lo troviamo abbiamo la sua pos, andiamo quindi ad effettuare
un controllo sulla grandezza dello storage e se va tutto bene creiamo un file con input di dati input_s
e lo immagazzianiamo. altrimento dovremo effetturare una pop per liberare spazio, tutto ovviamente gestito
con la ME ed il semaforo specifico
@return: 1 successo -1 errore	
*/ 
char *writeFile(char *input_filename,char *input_s, int flaga, int fd, char *espulsione){ 

	char *espTmp = malloc(sizeof(char)*(strlen(espulsione)+1));
	strcpy(espTmp,espulsione);
	printf("-\n");
	char *res;
	int pos = findFileInStorage(input_filename);
	if(pos == -1){ printf("File non trovato"); free(espTmp); return "-1"; }
	int lfd = findIfAlreadyOpen(fd,pos);
	if(lfd == 0){
		printf("Il Client non ha aperto il file\n");
		free(espTmp);
		return "-1";
	};
	int lg = controlIfLocked(fd,pos);
	if(lg == -1){
		printf("Il file e' Lockato da un altro client\n");
		free(espTmp);
		return "-1";
	}
	size_t tmpSize = strlen(input_s);
	pthread_mutex_lock(&mtx4);
	while(controlloSemaforo(4) == 1){
		printf("Waiting\n");
		pthread_cond_wait(&cond4, &mtx4);
	}
	setSemaforo(4);


	if(1){
		int cgs = controlloGrandezzaFinale(input_s); 
		if(cgs == -1){
			incrCR();
			file tmp = storagePop();
			strcat(espTmp,makeFileStringForPopping(tmp));
			freeSemaforo(4);
		    pthread_mutex_unlock(&mtx4);
			res = writeFile(input_filename,input_s,flaga,fd,espTmp);
			free(espTmp);
			free(tmp.f);
			if(tmp.whoOpen != NULL){ free(tmp.whoOpen); }
			return res;
		}else if(cgs == 1){
			strncpy(storage[pos].f,input_s,tmpSize);
			storage[pos].size = strlen(input_s);
			makeWriteFileLog(input_filename,storage[pos].size);
			if(flaga == 1){
				increNkb(input_s);
			}
			freeSemaforo(4);
		    pthread_mutex_unlock(&mtx4);
			//res = malloc(sizeof(char)*3);
			//strcpy(res,"1");
			if(strcmp(espTmp,"") == 0){
				free(espTmp);
				return "1";
			}
			free(espTmp);
			return espTmp;
		}

		freeSemaforo(4);
		pthread_mutex_unlock(&mtx4);

		return res;
	}

	freeSemaforo(4);
	pthread_mutex_unlock(&mtx4);

	printf("Errore generico\n");
	return "-1";
}

/* 
@params: input_filename -> nome del file da eliminare
@effects: grazie alla strtok scorre la stringa recuperando tutti i nomi dei file da eliminare, poi recupera
la posizione del file e lo elimina, tutto ovviamente dopo aver acquisito la ME
@return: 1 successo , -1 errore, 0 eliminazione parziale
*/
int removeFile(char *input_filename){
	int flag = 0;
	pthread_mutex_lock(&mtx4);
	while(controlloSemaforo(4) == 1){
			pthread_cond_wait(&cond4, &mtx4);
	}
	setSemaforo(4);
	
	int pos = findFileInStorage(input_filename);
	if(pos < 0){
		freeSemaforo(4);
		pthread_mutex_unlock(&mtx4); 
		return -1; 
	}

	if(strcmp(storage[pos].filename,"") != 0){
		strcpy(storage[pos].filename,"");
		strcpy(storage[pos].f,"");
		free(storage[pos].whoOpen);
		storage[pos].whoOpen = NULL;
		storage[pos].size = 0;
		storage[pos].clientCodeLocker = -1;
		modifyStorageCount(flag);
		freeSemaforo(4);
		pthread_mutex_unlock(&mtx4);
		return 1;
	}else{
		freeSemaforo(4);
		pthread_mutex_unlock(&mtx4);
		return -1;
	}

	freeSemaforo(4);
	pthread_mutex_unlock(&mtx4);
	return 0;

}




/*
@params: nome del file -> filename , cosa ascrivere in append al file -> towrite, size
@effects: recupera i dati dal file che abbiamo scelto, dopo di che crea una stringa contentene tutti i dati
(vecchi + nuovi) e testa la grandezza dello storage, se e' tutto ok allora riscrive il file, altrimenti
effettua una pop e ri chiama ricorsivamente la funzione
@return: 1 successo, -1 errore
*/
int appendToFile(char *filename, char *towrite, int fd){ 

	char *tmpLine = malloc(sizeof(char)*conf.max_size);
	size_t size = strlen(towrite);
	tmpLine = readFile(filename,fd,0);

	if(strcmp("",tmpLine) == 0){ 
		return -1; 
	}

	pthread_mutex_lock(&mtx4);
	while(controlloSemaforo(4) == 1){
			pthread_cond_wait(&cond4, &mtx4);
	}
	setSemaforo(4);


	strncat(tmpLine,towrite,size);

	size_t newSize = strlen(tmpLine);

	int cgs = controlloGrandezzaFinale(tmpLine);
	//int cgs = 1;
	if(cgs == -1){
		//printf("Problema grandezza\n");
		incrCR();
		file tmp = storagePop();
		freeSemaforo(4);
		pthread_mutex_unlock(&mtx4);
		int res = appendToFile(filename,towrite,fd);
	}else if(cgs == 1){
		freeSemaforo(4);
		pthread_mutex_unlock(&mtx4);
		//int finalres = writeFile(filename,tmpLine,1);
		int finalres = 1;
		//free(tmpLine);
		return finalres;
	}
	freeSemaforo(4);
	pthread_mutex_unlock(&mtx4);

	return -1;
}

/*
@params: filename -> nome file
@effects: setta il flag open del file a 1
*/
int openFileInStorage(char *filename, int fd){
	int pos = findFileInStorage(filename);
	if(pos != -1){
		int tmp = findIfAlreadyOpen(fd,pos);
		if(tmp == 0){
			printf("Questo utente non ha aperto %d \n", fd);
			//--- Creazione della stringa di log
			makeOpenFileInStorageLog(filename,fd);
			//---
			addOpenerInFile(fd,pos);
		}else{
			printf("Questo utente ha aperto %d \n", fd);
		}
	}
}

/*
@params: filename -> nome file
@effects: setta il flag open del file a 0
*/
int closeFileInStorage(char *filename, int fd){

	int ret;

	pthread_mutex_lock(&mtx4);
	while(controlloSemaforo(4) == 1){
			pthread_cond_wait(&cond4, &mtx4);
	}
	setSemaforo(4);
	
	int pos = findFileInStorage(filename);
	if(pos != -1){
		int tmp = findIfAlreadyOpen(fd,pos);
		if(tmp == 0){
			printf("Questo utente non ha aperto %d \n", fd);
			ret = -1;
		}else{
			printf("Questo utente ha aperto %d \n", fd);
			closeOpenerInFile(fd,pos);
			makeCloseFileInStorageLog(filename,fd);
			ret = 1;
		}
	}

	freeSemaforo(4);
	pthread_mutex_unlock(&mtx4); 

	return ret;
}





//----- Funzioni di stampa -----// 



//stampa la lista dei file nello storage, nome e size
void stampaListaFile(){
	int i;
	for(i = 0; i < conf.max_file_n; i++){
		//if(strcmp(storage[i].f,"") != 0){
			//long int size = findSizeThisFile(storage[i].filename);
			int size = storage[i].size;
			printf("________________\n");
			printf("Filename: %s\n", storage[i].filename);
			printf("Size: %d\n", size);
			if(storage[i].clientCodeLocker != -1){
				printf("locker fd %d\n", storage[i].clientCodeLocker);
			}
			//stampaListaClientApertiPerFile(i);
		//}
	}
}


/*
@effects: stampa tutti i dati relativi a tutti i file che ci sono / posizioni allocate nello storage
*/
void stampaStorage(){
	int i;
	int range = conf.max_file_n;
	for(i = 0; i < range; i++){
		if(strcmp(storage[i].f,"") == 0){
			printf("Pos: %d -> %s FILE NON SCRITTO \n", i,storage[i].filename);
		}else{
			printf("Pos: %d -> %s \n", i,storage[i].filename);
		}
	}
}

/*
@effects: logger che viene stampato alla chiusura del server che contiene alcuni dati importanti
*/
void finalLogger(){

	printf("***LOG FINALE***\n");
	printf("Numero di rimpiazzi: %d\n", conteggioRimpiazzi);
	printf("Numero di file Massimo raggiunto nel server: %d\n", numeroFileMaxNelServer);
	printf("Numero di KB totali immagazzinati: %li\n", numeroByMaxNelServer);//da cambiare
	printf("Numero di file contenuti alla chiusura: %d\n", storagepc);
	printf("Lista dei file contenuti alla chiusura:\n");
	//stampaListaFile();

}


//----- Funzioni Thread Workers -----//


/*
@params: codice arrivato dal client, argomenti, e codice connessione
@effetcts: uno switch in base al codice arrivato avvia la funzione dedicata a quella richiesta
*/
void switcher(int code, char **args,long connfd){
	printf("code %d\n", code);
	if(code < 0){
		exit(EXIT_FAILURE);
	}

	char *contenutoFile;
	char *response;
	switch(code){
		case 0:
			printf("Help\n");
			write(connfd,"hfs",4);
			break;
		case 1:
			printf("Andiamo ad aprire un file, se non esiste ritorniamo errore\n");
			int res = openFileInStorage(args[1],connfd);
			if(res == -1){
				write(connfd, "-1", 2);
				printf("Errore, file non trovato\n");
			}else{
				write(connfd, "0", 2);
			}
			break;
		case 2:
			printf("Andiamo a creare un file, se esiste gia ritorniamo errore\n");
			int bufferEspulsiSize = N;
		    char *espulsi = malloc(sizeof(char)*bufferEspulsiSize);
		    strcpy(espulsi,"");
			char *res2 = insertFile(args[1],espulsi,connfd);
			printf("res2 after insert %s\n", res2);
			if(strcmp(res2,"-1") == 0){
				printf("Errore, file gia esistente\n");
				write(connfd, "-1", 2);
			}else{
				printf("esp -> %s\n", res2);
				write(connfd, res2, (int)(strlen(res2)+1));
			}
			free(espulsi);
			break;
		case 3:
			printf("Andiamo a leggere un file\n");
			//muore qua 
			printf("%lu %s \n",connfd,args[1]);
			contenutoFile = readFile(args[1],connfd,0);
			int scf = getFileSize(args[1]);
			if(strcmp(contenutoFile,"") == 0){
				printf("Errore file non trovato\n");
				write(connfd,"-1",2);
			}else{
				//int ts2 = conf.max_size * 2;
				int ncifre = findCifre(scf);
				char tmpStringSize[ncifre];
				int ts2 = 1 + ncifre + scf;
				response = malloc(sizeof(char)*ts2+1);
				strcpy(response,"");
				sprintf(tmpStringSize, "%d", scf);
				strcat(response,tmpStringSize);
				strcat(response,"#");
				strncat(response,contenutoFile,scf);
				//printf("%s\n", contenutoFile);
				write(connfd,response,ts2);
			}
			free(response);
			break;
		case 4:
			contenutoFile = readNfile(args[1],connfd);
			if(strcmp(contenutoFile,"") == 0){
				printf("Errore file non trovato\n");
				write(connfd,"-1",2);
			}else{
				int sizeTmp = strlen(contenutoFile);
				printf("Risposta: %s\n", contenutoFile);
				write(connfd,contenutoFile,sizeTmp);
			}
			free(contenutoFile);
			break;
		case 5:
		    printf("Andiamo a scrivere su un file\n");
		    int bufferEspulsiSize2 = N;
		    char *espulsi2 = malloc(sizeof(char)*bufferEspulsiSize2);
		    strcpy(espulsi2,"");
			char *res3 = writeFile(args[1],args[2],1,connfd,espulsi2);
			printf("res3 %s\n", res3);
			if(strcmp(res3,"-1") == 0){
				printf("Errore nella scrittura\n");
				write(connfd,"-1",2);
			}else{
				printf("Esp -> %s\n", res3);
				printf("File scritto correttamente\n");
				write(connfd,"0",2);
			}
			free(espulsi2);
			break;	
		case 6:
			printf("Scriviamo in append su un file\n");
			int res4 = appendToFile(args[1],args[2],connfd);
			if(res4 == -1){
				printf("Errore nella scrittura in append\n");
				write(connfd,"-1",2);
			}else{
				write(connfd,"0",2);
			}
			printf("fine di un append\n");
			break;
		case 7:
			printf("Chiudiamo un file\n");
			int res6 = closeFileInStorage(args[1],connfd);
			if(res6 == -1){
				write(connfd,"-1",2);
			}else{
				write(connfd,"0",2);
			}
			break;
		case 8:
			printf("Cancelliamo i/il file\n");
			int res5 = removeFile(args[1]); //da modificare codici di arrivo ? 
			if(res5 == -1){
				write(connfd,"-1",2);
			}else{
				makeCloseFileLog(args[1]);
				write(connfd,"0",2);
			}
			break;
		case 9:
			printf("Richiesta di lock del file\n");
			int pos9 = findFileInStorage(args[1]);
			int res9 = lockFile(connfd,pos9);
			if(res9 == 1){
				makeLockFileLog(args[1]);
				write(connfd,"0",2);
			}else{
				write(connfd,"-1",2);
			}
			break;
		case 10:
			printf("Richiesta di sbloccare un file\n");
			int pos10 = findFileInStorage(args[1]);
			int res10 = unlockFile(connfd,pos10);
			if(res10 == 1){
				makeUnlockFileLog(args[1]);
				printf("File Sbloccato\n");
				write(connfd, "0", 2);
			}else{
				printf("File non sbloccato\n");
				write(connfd, "-1", 2);
			}
			break;
		case 11:
			printf("Richiesta di OpenLock\n");
			int bufferEspulsiSize3 = N;
		    char *espulsi3 = malloc(sizeof(char)*bufferEspulsiSize3);
		    strcpy(espulsi3,"");
			char *res111 = insertFile(args[1],espulsi3,connfd);
			printf("res11 %s\n", res111);
			int sizeRes111 = (int)strlen(res111);
			int res11 = openFileInStorage(args[1],connfd);
			int pos11 = findFileInStorage(args[1]);
			int res12 = lockFile(connfd,pos11);
			if(res11 == -1 || res12 == -1){
				write(connfd, "-1", 2);
				printf("Errore, file non trovato\n");
			}else{
				makeOpenLockFileLog(args[1]);
				write(connfd, res111, sizeRes111);
			}
			free(espulsi3);
			break;
		default:
			printf("CASE DEFUALT\n");
			break;
	}

}


//Thread worker, recupera una richiesta con una export dalla lista delle rich
//e la inoltra alla funzione switcher
void *workload(void *p){
	printf("Thread Worker Avviato %lu\n", (long)p);
	workload_quitter = PROGRESS;
	while(workload_quitter != QUIT_CODE){
		//char *req = exportClientReq();
		struct client_request req = exportClientRequest();

		if(req.fd == KILLER_CODE){
			printf("Thread Terminato\n");
			if( write(pipefd[1], &(req.fd), sizeof(req.fd)) == -1 ){
	   			printf("Errore in WriteFD\n");
				//pthread_kill(sig_handler_tid, SIGINT);
				break;
			}
			free(req.request);
			return NULL;
		}
		//printf("NODIE\n");

		increConteggioRichiestePerThread((long)p);

		int stats = req.status; 
	   	if(stats > 0){
	   		if( write(pipefd[1], &(req.fd), sizeof(req.fd)) == -1 ){
	   			printf("Errore in WriteFD\n");
				//pthread_kill(sig_handler_tid, SIGINT);
				break;
			}
	   	}else{
	   		continue;
	   	}

		char *token;
		int code;
		int verify = 0;
		int count = 0;
		//char **saveptr;

		char *requestTmp = parseRequest(req.request);
		printf("Richiesta = %s\n", requestTmp);

		//printf("Richiesta: %s\n", requestTmp);
		//printf("Numero fd %d\n", req.fd);
		const char *delim = ",";
		token = strtok(requestTmp, delim);
		char **listOfArgs;
		listOfArgs = malloc(sizeof(char)*(N_MAX_ARGS));
		while( token != NULL ){
			if(verify == 0){
				code = atoi(token);
			}else{
				listOfArgs[count] = malloc(sizeof(char)*(strlen(token)+1));
				strcpy(listOfArgs[count],token);
			}
		    token = strtok(NULL, ",");
		    verify = verify + 1;
		    count = count + 1;
	   	}

	   	//printf("Codice %d - Params %s\n", code, params);
	   	switcher(code,listOfArgs,req.fd); 
	   	free(req.request);
	   	free(requestTmp);
	   	int free1;
	   	for(free1 = 1; free1 < count; free1++){
	   		free(listOfArgs[free1]);
	   	}
	   	free(token);
	   	free(listOfArgs);
	}
	printf("Thread Terminato\n");
	return NULL;
}

//Manager secondo lo schema MasterWorker , si occupa di accettare
//le richieste dei client e inserirle nella lista
//oltre che alla gestione di un isneiem degli fd dei client
void *manager(void *p){
	printf("Thread Manager Avviato\n");
	master_quit = 0;
	int socket_fd;
	socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);


	struct sockaddr_un serv_addr;
	memset(&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sun_family = AF_UNIX;
	strncpy(serv_addr.sun_path, conf.socket_name, strlen(conf.socket_name)+1);
	//errno = 0;
	bind(socket_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	listen(socket_fd, SOMAXCONN);

	fd_set set, tmpset;
	FD_ZERO(&set); //pulisco il set e il tmpset (inizializzo)
	FD_ZERO(&tmpset);

	FD_SET(socket_fd, &set); //aggiungo fd al set
	FD_SET(pipefd[0], &set);

	int fdmax = max(2, socket_fd, pipefd[0]);
	int i;
	int *fds_from_pipe = calloc(100, sizeof(int));
	int connfd;

	while(master_quit != QUIT_CODE){
		//start dei dati condivisi
		tmpset = set;
		//monitora file dexriptor aspetta finche  un fd e' pronto per un operazione 
		select(fdmax+1, &tmpset, NULL, NULL, NULL);
		int fd;
		//ciclo fd fino al max di fd
		for(fd=0; fd <= fdmax; fd++) {
			//controllo se fd appartiene al set
			if (FD_ISSET(fd, &tmpset)) {
				//se fd e' un socket fd e non dobbiamo uscire
				if (fd == socket_fd && master_quit == 0) {
					//accettiamo la connessione
					connfd = accept(socket_fd, (struct sockaddr*)NULL ,NULL);
					makeNewConnectionLog(connfd);
					incrConnessioniAttuali();
					verificaMaxConnessioniCont();
					FD_SET(connfd, &set);
					if(connfd > fdmax){ fdmax = connfd; }
					continue;
					//fd = primo fd nella pipe, quindi fd di lettura
				}else if(fd == pipefd[0] && master_quit != QUIT_CODE){
					read(pipefd[0], fds_from_pipe, 100*sizeof(int));
					for(i = 0;fds_from_pipe[i]!=0;++i){
						//printf("preset\n");
						FD_SET(fds_from_pipe[i], &set);
						if(fds_from_pipe[i]>fdmax){ fdmax = fds_from_pipe[i]; } 
						fds_from_pipe[i] = 0;
					}
					continue;
					//quit
				}else if(fd == pipefd[0] && master_quit == QUIT_CODE){
					printf("Server Terminato e non ci sono piu' rihcieste da importare\n");
					break;
				}
				connfd = fd;
				FD_CLR(connfd, &set); //rimuovo l fd dal set

				importClientReq(connfd,"");
			}
		}

	}

	free(fds_from_pipe);
	//printf("REQ %d\n", controlListReqNull());
	printf("Manager Terminato\n");
	return NULL;
}





//--- MAIN ---//

int main(int argc, char const *argv[])
{
	
	if(argc==1) {
	    printf("Deve essere passato come parametro il nome del file di config in formato .txt\n");
	    return -1;
	}

	sgac.sa_handler = segnalatore;
	sgac2.sa_handler = segnalatore;
	sgac3.sa_handler = segnalatore;

	setSegnali();

	cleanup();

	leggiConfigurazione(argv[1]); //inizializziamo il file di config all apertura
	// a questo punto abbiamo i dati relativi alla configurazione in file globale

	printConf();
	makeStorage();

	initListReq();
	setUpConteggioRichiestePerThread();
	printConf();
	openFileLogs();



	//qua dobbiamo andare a inviare la richiesta e in base alla richiesta smistare

	if (pthread_create(&manager_thread_pid, NULL, &manager, NULL) != 0) {
		fprintf(stderr, "Creazione del main Fallita");
		return;
    }
	int n_thread_workers = conf.thread_workers;

	pipe(pipefd);

	workers = malloc(sizeof(pthread_t)*n_thread_workers);
	long i;
	int ii;
	for(i = 0;i<n_thread_workers; i++){
		pthread_create(&(workers[i]), NULL, &workload, (void*)i);
	}

	pthread_join(manager_thread_pid, NULL);


	for(ii = 0;ii<n_thread_workers; ii++){
		pthread_join(workers[ii], NULL);
	}


	freeListaRequest();
	finalLogger();
	makeFinalLog(numeroFileMaxNelServer,numeroByMaxNelServer,conteggioRimpiazzi,getConnessioniMassimeContemporanee(),conteggioRichiestePerThread,conf.thread_workers);
	removeSocketFile();
	freeAll();
	freeConteggioRichiestePerThread();
	free(workers);

	return 0;
}


