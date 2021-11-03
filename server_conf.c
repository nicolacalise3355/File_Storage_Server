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

//--- Define ---// 

#define CONFIG_FILE_LEN 30
#define LEN_FILENAME 50
#define MAX_FILE_SIZE 1000
#define N_SEMAFORI 5 
#define MAX_SIZE_STORAGE 500
#define N 300
#define N_MAX_ARGS 50


/*
@params: sono i valori necessari per settare la varibaile di tipo config 
@effects: setta i valori della variabile globale conf
*/
void setConfigurazione(int mtn, int ms, char *sn, int mt, int tw, char *lfn){
	conf.max_file_n = mtn;
	conf.max_size = ms;
	strcpy(conf.socket_name,sn);
	conf.max_thread = mt;
	conf.thread_workers = tw;
	strcpy(conf.file_log_name, lfn);
}


/*
@effetcs: legge il file di configurazione e passa i valori trovati ad
una funzione che setta i valori sulla variabile globale definita per memorizzare i valori
*/
void leggiConfigurazione(const char *fileconfig){
	FILE *conf_file;
	char *line = malloc(sizeof(char)*CONFIG_FILE_LEN);
	if((conf_file = fopen(fileconfig,"r")) == NULL){
		printf("Errore nell apertura del file di confgigurazione\n");
	}
	int riga = 1;

	int param1;
	int param2;
	char param3[CONFIG_FILE_LEN];
	int param4;
	int param5;
	char param6[CONFIG_FILE_LEN];

	while(!feof(conf_file)){
		fscanf(conf_file,"%s",line);

		if(riga == 1){
			param1 = strtol(line,NULL,10);
		}else if(riga == 2){
			param2 = strtol(line,NULL,10);
		}else if(riga == 3){
			strcpy(param3,line);
		}else if(riga == 4){
			param4 = strtol(line,NULL,10);
		}else if(riga == 5){
			param5 = strtol(line,NULL,10);
		}else if(riga == 6){
			strcpy(param6,line);
		}

		riga = riga + 1;

	}

	setConfigurazione(param1,param2,param3,param4,param5,param6);
	free(line);
	fclose(conf_file);
}

/*
* @effects: stampa i dati della configurazione
*/
void printConf(){
	printf("***Configurazione***\n");
	printf("max file: %d\n", conf.max_file_n);
	printf("max size: %d\n", conf.max_size);
	printf("sock name: %s\n", conf.socket_name);
	printf("thread workers: %d\n", conf.thread_workers);
	printf("Logs file: %s\n", conf.file_log_name);
	printf("******\n");
}

/*
@effects: effettua la unlink del file di connessione della socket
*/
void cleanup() {
  unlink(conf.socket_name);
}

/*
@effects: rimuove il file della socket
*/
void removeSocketFile(){
		int res;
  		res=remove(conf.socket_name);
  		if( res!=0 ){
  			perror("Errore nella rimozione del file");
  		}

}