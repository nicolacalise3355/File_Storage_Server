#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <dirent.h>
#include <pthread.h>
#include <assert.h>
#include <signal.h>
#include <getopt.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include "clientApi.h"

#define DEFAULT_SOCKET_FILE "./cs_sock"

#define CONFIG_FILE_LEN 30
#define LEN_FILENAME 100
#define MAX_FILE_SIZE 5000
#define MAX_LINE_FILE_SIZE 1000
#define LEN_CONF 50
#define SOCK_LEN 50
#define N 300
#define TIME_UTC 10
#define N_MAX_ARGS 10
#define MAX_FILE_TO_SEND 50

#define NO_FLAGS 0x00
#define O_CREATE 0x01
#define O_LOCK 0x02

//--- Variabili globali ---///

//Struttura che contiene i valori delle operazioni da effettuare
typedef struct valori_operazione{

	int codice;
	char *arg;

} valoriOp;

valoriOp valori; 

char *SOCKET_FILE = NULL;
char *DIRECTORY_EXP = NULL;
char *DIRECTORY_READ = NULL;

int flagOutput = 0;
int globalMsec = 0; 

int timeToWait = 0;


int resconn;


//--- Funzione per la ricerca ricorsiva delle cartelle ---//

/*
@params:
@effects:
@return:
@extra:
*/
int isdot(const char dir[]) {
  int l = strlen(dir);
  
  if ( (l>0 && dir[l-1] == '.') ) return 1;
  return 0;
}

/*
@params:
@effects:
@extra:
*/
void lsR(const char nomedir[],char **listOfArgs,int *count,int maxLen) {
    // controllo che il parametro sia una directory
    struct stat statbuf;
    int r;

    int conteggioDaSalvare = maxLen-1;

    DIR * dir;
    //fprintf(stdout, "-----------------------\n");
    //fprintf(stdout, "Directory %s:\n",nomedir);
    
    if ((dir=opendir(nomedir)) == NULL) {
	perror("opendir");
	printf("Errore aprendo la directory %s\n", nomedir);
	return;
    } else {
	struct dirent *file;
    
	while((errno=0, file =readdir(dir)) != NULL) {
	    struct stat statbuf;
	    char filename[LEN_FILENAME]; 
	    int len1 = strlen(nomedir);
	    int len2 = strlen(file->d_name);
	    if ((len1+len2+2)>LEN_FILENAME) {
			fprintf(stderr, "ERRORE: MAXFILENAME troppo piccolo\n");
			exit(0);
	    }	    
	    strncpy(filename,nomedir,      LEN_FILENAME-1);
	    strncat(filename,"/",          LEN_FILENAME-1);
	    strncat(filename,file->d_name, LEN_FILENAME-1);
	    
	    if (stat(filename, &statbuf)==-1) {
		perror("eseguendo la stat");
		printf("Errore nel file %s\n", filename);
		return;
	    }
	    if(S_ISDIR(statbuf.st_mode)) {
	    	conteggioDaSalvare = conteggioDaSalvare - 1;
	    	//printf("conteggio attuale = %d\n", conteggioDaSalvare);
			if ( !isdot(filename) ) lsR(filename,listOfArgs,count,conteggioDaSalvare);
		    } else {
		    if(conteggioDaSalvare >= 0){
		    	strcpy(listOfArgs[conteggioDaSalvare],filename);
		    	//printf("%s\n", filename);
		    }
		//fprintf(stdout, "%20s: %10ld  \n", file->d_name, statbuf.st_size);		
	    }
	}
	if (errno != 0) perror("readdir");
	closedir(dir);
	//fprintf(stdout, "-----------------------\n");
    }
}


//--- Funzioni di gestione ---//

//sleep per msec microsecondi
void msleep(int msec){
	long msts = msec / 1000;
	sleep(msts);
}

//setta il tempo di attesa fra richieste
void setTimeToWaitToSend(const char *stringTime){
	int newTime = atoi(stringTime);
	timeToWait = newTime;
	if(flagOutput == 1){
		printf("Abbiamo Modificato il Tempo di attesa fra invio di richieste -> %d sec\n", timeToWait);
	}
}

//Resetta il socket name
void resetSocketName(const char *name){
	time_t timer = time(0);
	const struct timespec abst = {
		.tv_nsec = 0,
		.tv_sec = timer + 1
	};
	SOCKET_FILE = malloc(sizeof(char)*strlen(name));
	strcpy(SOCKET_FILE,name);
	if(flagOutput == 1){
		printf("Nuovo Nome Socket = %s\n", name);
	}
	resconn = openConnection(name,globalMsec,abst);
}
//recupera il socket name
char *getSocket(){
	if(SOCKET_FILE == NULL){
		return DEFAULT_SOCKET_FILE;
	}else{
		return SOCKET_FILE;
	}
}


void setDirectoryForRead(const char *name){
	DIRECTORY_READ = malloc(sizeof(char)*strlen(name));
	strcpy(DIRECTORY_READ,name);
	if(flagOutput == 1){
		printf("Nuovo dir Read = %s\n", name);
	}
}

void setDirectoryForExp(const char *name){
	DIRECTORY_EXP = malloc(sizeof(char)*strlen(name));
	strcpy(DIRECTORY_EXP,name);
	if(flagOutput == 1){
		printf("Nuovo dir exp = %s\n", name);
	}
}


//--- Funzione Pre-Api ---//

/*
Funzione di preprazione per effettuare una richieste di write
*/
void preW(char *str){
	if(flagOutput == 1){
		printf("Andiamo a scrivere su un file\n");
	}

	int res;
	char **listOfDir;
	char *dirname;
	int count = 0;

	listOfDir = malloc(sizeof(char)*N_MAX_ARGS);
	int i,j;
	for(i = 0; i < N_MAX_ARGS; i++){
		listOfDir[i] = malloc(sizeof(char)*LEN_FILENAME);
	}

	char *token;
	token = strtok(str, ",");
	while(token != NULL){
		strcpy(listOfDir[count],token);		
		int res1 = openFile(token,O_LOCK|O_CREATE);
		int res3 = writeFile(token,DIRECTORY_EXP_C);
		int tmpRes3 = unlockFile(token);
		int tmpRes4 = closeFile(token);
	    token = strtok(NULL, ",");
	    count = count + 1;
	    msleep(timeToWait);
	}

}

//Funzione che ci sposta in una cartella
DIR * goToDir(char * path){
	if(chdir(path) == -1) return NULL; 
	DIR *newdir = opendir(".");
	return newdir;
} 

//Funzione che ci crea la stringa del path completo di un file
char *getPath(char *file, char *path){
	int newSize = strlen(file) + strlen(path) + 2;
	char *newPath = malloc(sizeof(char)*newSize);
	strcpy(newPath,path);
	strcat(newPath,"/");
	strcat(newPath,file);
}

/**
 * Funzione che ci permette di visitare ricorsivamente una cartella
 * Utilizzata quando dobbiamo scrivere vari file nel server
*/
int visitDirAndWrite(char *path, DIR *dir, int n_remaining_files){

	if(dir == NULL){
		//set errno
		return -1;
	}
	errno = 0;
	struct dirent* ptr = readdir(dir);
	if(ptr == NULL && errno != 0){
		//set errno
		perror("- ");
		return -1;
	}else if(ptr != NULL){
		//printf("%s\n", ptr->d_name);
		if(strcmp(ptr->d_name,".") != 0 && strcmp(ptr->d_name,"..") != 0){
			struct stat info;
			stat(ptr->d_name, &info);
			if (S_ISDIR(info.st_mode)){
				DIR *newdir = goToDir(ptr->d_name);
				if( n_remaining_files == -2 || n_remaining_files > 0 ){
					printf( "SubDirectory: %s\n", ptr->d_name );
					char *newPath = getPath(ptr->d_name,path);
					printf("newpath %s\n", newPath);
					if( visitDirAndWrite(newPath, newdir, n_remaining_files) == -1 ) return -1;
					free(newPath);
				}

				closedir(newdir);
				chdir("..");
				
			}else if(S_ISREG(info.st_mode)){
				if( n_remaining_files != -2 && (n_remaining_files)-- < 0 ) return 0;

				char *tmpFileName = getPath(ptr->d_name, path);
				if(flagOutput == 1){
					printf("File che proviamo a mandare -> %s\n", tmpFileName);
				}

				setPathTmp(tmpFileName);

				int tmpRes = openFile(tmpFileName,O_LOCK|O_CREATE);
				if(tmpRes != -1){
					printf("ptr %s  tmp %s\n",ptr->d_name,tmpFileName);
					int tmpRes2 = writeFile(ptr->d_name,DIRECTORY_EXP_C);
					int tmpRes3 = unlockFile(tmpFileName);
					int tmpRes4 = closeFile(tmpFileName);
				}

				free(tmpFileName);
				msleep(timeToWait);
			}
		}
		if( n_remaining_files == -2 || n_remaining_files > 0 ){
			if( visitDirAndWrite(path, dir,n_remaining_files) == -1 ) return -1;
		}
	}
	return 0;
}

//Funzione di preparazione ad una write con visita ricorsiva
void pre_w(char *str){
	if(flagOutput == 1){
		printf("Andiamo a controllare quanti file dobbiamo scrivere\n");
	}
	char *dirName;
	int numeroFile = 0;

	char **listOfArgs;

	int count = 0;
	char *token;
	token = strtok(str,",");
	while(token != NULL){
		if(count == 0){
			//dirname
			dirName = malloc(sizeof(char)*strlen(token));
			strcpy(dirName,token);
			count = 1;
		}else{
			numeroFile = atoi(token);
		}
		token = strtok(NULL, ",");
	}
	printf("dir: %s, n = %d\n", dirName,numeroFile);

	if(numeroFile == 0){
		numeroFile = MAX_FILE_TO_SEND;
	}

	DIR *d = goToDir(dirName);
	int r = visitDirAndWrite(dirName,d,numeroFile);

	int i;
	listOfArgs = malloc(sizeof(char)*numeroFile);
	for(i = 0; i < numeroFile; i++){
		listOfArgs[i] = malloc(sizeof(char)*N);
	}

}

//Funzione di preparazione ad una richiesta di read
void pre_r(char *str){

	void *buff;
	//size_t size;

	int res;
	char **listOfDir;
	char *dirname;
	int count = 0;

	listOfDir = malloc(sizeof(char)*N_MAX_ARGS);
	int i,j;
	for(i = 0; i < N_MAX_ARGS; i++){
		listOfDir[i] = malloc(sizeof(char)*LEN_FILENAME);
	}
	char *token;
	token = strtok(str, ",");
	while(token != NULL){
		strcpy(listOfDir[count],token);		

		int res1 = openFile(token,NO_FLAGS);
		int res2 = readFile(token,&buff,0);
		int res3 = closeFile(token);
	    token = strtok(NULL, ",");
	    count = count + 1;
	    msleep(timeToWait);
	}

}

//Funzione di preparazione per una lock
void pre_lock(char *str){
	if(flagOutput == 1){
		printf("Andiamo a bloccare un file\n");
	}
	int count = 0;
	char *token;
	token = strtok(str, ",");
	while(token != NULL){
		int res = lockFile(token);
	    token = strtok(NULL, ",");
	    count = count + 1;
	    msleep(timeToWait);
	}
}
//Funzione di preparazione per una unlock
void pre_unlock(char *str){
	if(flagOutput == 1){
		printf("Andiamo a sbloccare un file\n");
	}
	int count = 0;
	char *token;
	token = strtok(str, ",");
	while(token != NULL){
		int res = unlockFile(token);
	    token = strtok(NULL, ",");
	    count = count + 1;
	    msleep(timeToWait);
	}
}

//Funzioen di preparazione per un ReadNfile
void pre_R(char *str){
	if(flagOutput == 1){
		if(DIRECTORY_READ != NULL){
			printf("I files verrano salvati in: %s\n", DIRECTORY_READ);
		}else{
			printf("I files non verranno salvati! \n");
		}
	}
	int n;
	if(strcmp(str,"NULL") == 0){
		n = 0;
	}else{
		n = atoi(str);
	}
	int res = readNfile(n,DIRECTORY_READ);
	msleep(timeToWait);
}

//Funzione di preparazione per una cancellazione di un file
void pre_canc(char *str){
	if(flagOutput == 1){
		printf("Andiamo a cancellare un file\n");
	}
	int count = 0;
	char *token;
	token = strtok(str, ",");
	while(token != NULL){
		int res = removeFile(token);
	    token = strtok(NULL, ",");
	    count = count + 1;
	    msleep(timeToWait);
	}
}

//--- Funzione per gestione delle operazioni ---//

/*
@params: code -> codice che indica cosa fare, arg -> argomenti da inviare
@effects: uno switch legge il codice e capisce quale richiesta abbiamo richiesto.
*/
void operazione(int code, char *arg){

	if(resconn == -1){
  		printf("Errore nella connessione\n");
  		exit(0);
  	}

	if(flagOutput == 1){
		printf("codice %d\n", code);
		printf("Arg %s\n", arg);
	}

	switch(code){
		case 0:
			help();
			break;
		case 1:
			if(flagOutput == 1){
				printf("Abbiamo cambiato il timer\n");
			}
			break;
		case 2:
			if(flagOutput == 1){
				printf("Cambiamo il nome della socket\n");
			}
			resetSocketName(arg);
			break;
		case 3:
			if(flagOutput == 1){
				printf("Andiamo a scrivere uno o piu file\n");
			}
			preW(arg);
			break;
		case 4:
			if(flagOutput == 1){
				printf("Andiamo e leggere uno o piu file\n");
			}
			pre_r(arg);
			break;
		case 5:
			if(flagOutput == 1){
				printf("Andiamo a leggere N file\n");
			}
			pre_R(arg);
			break;
		case 6:
			if(flagOutput == 1){
				printf("Andiamo a cancellare un file\n");
			}
			pre_canc(arg);
			break;
		case 7:
			if(flagOutput == 1){
				printf("Andiamo a creare un file\n");
			}
			//preOpenFile(arg);
			break;
		case 8:
			if(flagOutput == 1){
				printf("Andiamo solo ad aprire un file\n");
			}
			break;
		case 9:
			if(flagOutput == 1){
				printf("Andiamo a creare o aprire un file\n");
			}
			pre_o(arg);
			break;
		case 10:
			if(flagOutput == 1){
				printf("Andiamo a lockare un file\n");
			}
			pre_lock(arg);
			break;
		case 11:
			if(flagOutput == 1){
				printf("Andiamo a sblocare un file\n");
			}
			pre_unlock(arg);
			break;
		case 12:
			if(flagOutput == 1){
				printf("Scrittura ricorsiva dei file\n");
			}
			pre_w(arg);
	}

}


//--- OPT ---//


int arg_h(const char* programname){
	valori.codice = 0;
	valori.arg = malloc(sizeof(char)*1);
	strcpy(valori.arg,"");
	operazione(valori.codice,valori.arg);
	return -1;
}

int arg_p(const char* programname){
	printf("Abbiamo abilitato l output\n");
	flagOutput = 1;
	return -1;
}

int arg_f(const char* arg){
	printf("Reset Socket Name\n");
	valori.codice = 2;
	size_t sizeArg = strlen(arg);
	valori.arg = malloc(sizeof(char)*sizeArg);
	strcpy(valori.arg,arg);
	resetSocketName(valori.arg);
	//operazione(valori.codice,valori.arg);
}

int arg_o(const char* arg){
	printf("o:\n");
	valori.codice = 9;
	size_t sizeArg = strlen(arg);
	valori.arg = malloc(sizeof(char)*sizeArg);
	strcpy(valori.arg,arg);
	operazione(valori.codice,valori.arg);
}

int arg_w(const char* arg){
	printf("w:\n");
	valori.codice = 12;
	size_t sizeArg = strlen(arg);
	valori.arg = malloc(sizeof(char)*sizeArg);
	strcpy(valori.arg,arg);
	operazione(valori.codice,valori.arg);
}

int arg_W(const char* arg){
	printf("W: \n");
	valori.codice = 3;
	size_t sizeArg = strlen(arg);
	valori.arg = malloc(sizeof(char)*sizeArg);
	strcpy(valori.arg,arg);
	operazione(valori.codice,valori.arg);
}

int arg_r(const char* arg){
	printf("r: \n");
	valori.codice = 4;
	size_t sizeArg = strlen(arg);
	valori.arg = malloc(sizeof(char)*sizeArg);
	strcpy(valori.arg,arg);
	operazione(valori.codice,valori.arg);
}

int arg_R(const char* arg){
	printf("R:\n");
	valori.codice = 5;
	if(arg != NULL){
		size_t sizeArg = strlen(arg);
		valori.arg = malloc(sizeof(char)*sizeArg);
		strcpy(valori.arg,arg);
	}else{
		size_t sizeArg = strlen("NULL");
		valori.arg = malloc(sizeof(char)*sizeArg);
		strcpy(valori.arg,"NULL");
	}
	operazione(valori.codice,valori.arg);
}

int arg_d(const char* arg){
	printf("d:\n");
	setDirectoryForRead(arg);
	setDirRead(arg);
	return -1;
}

int arg_t(const char* arg){
	printf("t:\n");
	setTimeToWaitToSend(arg);
	return -1;
}

int arg_l(const char* arg){
	printf("l:\n");
	valori.codice = 10;
	size_t sizeArg = strlen(arg);
	valori.arg = malloc(sizeof(char)*sizeArg);
	strcpy(valori.arg,arg);
	operazione(valori.codice,valori.arg);
}

int arg_c(const char* arg){
	printf("c:\n");
	valori.codice = 6;
	size_t sizeArg = strlen(arg);
	valori.arg = malloc(sizeof(char)*sizeArg);
	strcpy(valori.arg,arg);
	operazione(valori.codice,valori.arg);
}

int arg_u(const char* arg){
	printf("u:\n");
	valori.codice = 11;
	size_t sizeArg = strlen(arg);
	valori.arg = malloc(sizeof(char)*sizeArg);
	strcpy(valori.arg,arg);
	operazione(valori.codice,valori.arg);
}

int arg_D(const char* arg){
	printf("D:\n");
	setDirectoryForExp(arg);
	setDirExp(arg);
	return -1;
}




//--- Main ---//		

int main(int argc,char const *argv[]){
	//printf("Avvio Client\n");

	if (argc <= 1) {
	    printf("almeno una opzione deve essere passata\n");
	    //arg_h(argv[0]);
	    return -1;
  	}
  	int opt;


  	time_t timer = time(0);
	const struct timespec abst = {
		.tv_nsec = 0,
		.tv_sec = timer + 1
	};

  	countGenName = 0;

	char *socketToConnect = getSocket();
	resconn = openConnection(socketToConnect,globalMsec,abst);


  	while ((opt = getopt(argc,argv, ":hf:w:W:r:o:R:d:D:t:l:u:c:p")) != -1) {
        switch(opt) {
        	case 'p': arg_p(argv[0]); break;
        	case 'd': arg_d(optarg); break;
	        case 'h': arg_h(argv[0]);  break;
	        case 'f': arg_f(optarg);  break;
	        case 'w': arg_w(optarg);  break;
	        case 'W': arg_W(optarg); break;
	        case 'r': arg_r(optarg); break;
	        case 'o': arg_o(optarg); break;
	        case 'R': arg_R(optarg); break; //non obbligatorio
	        case 't': arg_t(optarg); break;
	        case 'l': arg_l(optarg); break;
	        case 'u': arg_u(optarg); break;
	        case 'c': arg_c(optarg); break;
	        case 'D': arg_D(optarg); break;
	        case ':': {
	        	if(optopt == 'R'){ arg_R(NULL); break; }
	        	printf("l'opzione '-%c' richiede un argomento\n", optopt);
	        } break;
	        case '?': {  
	          	printf("l'opzione '-%c' non e' gestita\n", optopt);
	        } break;
	        default:;
        }
    }

    closeConnection(DEFAULT_SOCKET_FILE);
	return 0;

}
