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

#include "logger_funcs.h"
#include "server_conf.h"

//Funzioni per gestire il file di log (se e' usato al momento  o no)

void setLogsFileUsed(){
	logFileUsed = 1;
}

void freeLogsFileUsed(){
	logFileUsed = 0;
}

int getLogsFileUsed(){
	return logFileUsed;
}

/*
*/
void printLoggerFileName(){
	printf("Nome file log: %s\n", conf.file_log_name);
}

/*
Apre il file di log e verifica la sua esistenza
*/
void openFileLogs(){
	logFileUsed = 0;
	FILE *logs_file;
	char *line = malloc(sizeof(char)*CONFIG_FILE_LEN);
	if((logs_file = fopen(conf.file_log_name,"r")) == NULL){
		printf("Errore nell apertura del file di confgigurazione\n");
	}else{
		printf("Accesso al file di log\n");
	}
	fclose(logs_file);
	free(line);
}

/*
Genera la linea di scrivere nel file di log
*/
char *makeLineToWrite(char *args[], int n){
	int i;
	char *newLine = malloc(sizeof(char)*MAX_SIZE_LOGS_LINE);
	strcpy(newLine,"");
	for(i = 0; i < n; i++){
		if(i != (n-1)){
			strcat(newLine,args[i]);
			strcat(newLine,",");
		}else{
			strcat(newLine,args[i]);
		}
	}
	return newLine;
}

/*
Scrive la linea passata in input nel file di log
*/
void writeToLogs(char *line){

	pthread_mutex_lock(&mtxl);

	while(getLogsFileUsed() == 1){
		pthread_cond_wait(&condl, &mtxl);
	}

	setLogsFileUsed();

	FILE *fd;
	fd=fopen(conf.file_log_name, "a");
	if( fd==NULL ) {
		perror("Errore in apertura del file");
		freeLogsFileUsed();
		pthread_mutex_unlock(&mtxl);
		return;
	}
	fprintf(fd, "%s\n", line);
	fclose(fd);

	freeLogsFileUsed();
	pthread_mutex_unlock(&mtxl);
}

//----- Funzioni di scrittura specifiche per ogni operazione -----//

void makeNewConnectionLog(int fd){
	char *line = malloc(sizeof(char)*MAX_SIZE_LOGS_LINE);
	strcpy(line,"");
	strcat(line,"newconnection,");
	char fdcs[6];
	sprintf(fdcs, "%d", fd);
	strcat(line,fdcs);
	writeToLogs(line);
	free(line);
}

void makeInsertLog(char *filename, int fd){
	char *line = malloc(sizeof(char)*MAX_SIZE_LOGS_LINE);
	strcpy(line,"");
	strcat(line,"insert,");
	strcat(line,filename);
	strcat(line,",");
	char fdcs[6];
	sprintf(fdcs, "%d", fd);
	strcat(line,fdcs);
	writeToLogs(line);
	free(line);
}

void makeOpenFileInStorageLog(char *filename, int fd){
	char *line = malloc(sizeof(char)*MAX_SIZE_LOGS_LINE);
	strcpy(line,"");
	strcat(line,"openFile,");
	strcat(line,filename);
	strcat(line,",");
	char fdcs[6];
	sprintf(fdcs, "%d", fd);
	strcat(line,fdcs);
	writeToLogs(line);
	free(line);
}

void makeCloseFileInStorageLog(char *filename, int fd){
	char *line = malloc(sizeof(char)*MAX_SIZE_LOGS_LINE);
	strcpy(line,"");
	strcat(line,"closeFile,");
	strcat(line,filename);
	strcat(line,",");
	char fdcs[6];
	sprintf(fdcs, "%d", fd);
	strcat(line,fdcs);
	writeToLogs(line);
	free(line);
}

void makeWriteFileLog(char *filename, int sizeBytes){
	char *line = malloc(sizeof(char)*MAX_SIZE_LOGS_LINE);
	strcpy(line,"");
	strcat(line,"writeFile,");
	char size[6];
	sprintf(size, "%d", sizeBytes);
	strcat(line,size);
	writeToLogs(line);
	free(line);
}

void makeStoragePopLog(char *filename, int sizeBytes){
	char *line = malloc(sizeof(char)*MAX_SIZE_LOGS_LINE);
	strcpy(line,"");
	strcat(line,"storagePop,");
	strcat(line,filename);
	strcat(line,",");
	char size[6];
	sprintf(size, "%d", sizeBytes);
	strcat(line,size);
	writeToLogs(line);
	free(line);
}

void makeReadFileLog(char *filename, int nBytes){
	char *line = malloc(sizeof(char)*MAX_SIZE_LOGS_LINE);
	strcpy(line,"");
	strcat(line,"readFile,");
	char size[6];
	sprintf(size, "%d", nBytes);
	strcat(line,size);
	strcat(line,",");
	strcat(line,filename);
	writeToLogs(line);
	free(line);
}

void makeReadNfileLog(int n){
	char *line = malloc(sizeof(char)*MAX_SIZE_LOGS_LINE);
	strcpy(line,"");
	strcat(line,"readNfile,");
	char number[6];
	sprintf(number, "%d", n);
	strcat(line,number);
	writeToLogs(line);
	free(line);

}

void makeOpenLockFileLog(char *filename){
	char *line = malloc(sizeof(char)*MAX_SIZE_LOGS_LINE);
	strcpy(line,"");
	strcat(line,"openLock,");
	strcat(line,filename);
	writeToLogs(line);
	free(line);
}
void makeLockFileLog(char *filename){
	char *line = malloc(sizeof(char)*MAX_SIZE_LOGS_LINE);
	strcpy(line,"");
	strcat(line,"lock,");
	strcat(line,filename);
	writeToLogs(line);
	free(line);
}
void makeUnlockFileLog(char *filename){
	char *line = malloc(sizeof(char)*MAX_SIZE_LOGS_LINE);
	strcpy(line,"");
	strcat(line,"unlock,");
	strcat(line,filename);
	writeToLogs(line);
	free(line);
}
void makeCloseFileLog(char *filename){
	char *line = malloc(sizeof(char)*MAX_SIZE_LOGS_LINE);
	strcpy(line,"");
	strcat(line,"close,");
	strcat(line,filename);
	writeToLogs(line);
	free(line);
}


void makeFinalLog(int maxNumber, int maxSize, int rimpiazzi, int maxConnessCont, int **nRichiestePerWorker, int nThreads){
	char *line1 = malloc(sizeof(char)*MAX_SIZE_LOGS_LINE);
	char number1[6];
	sprintf(number1, "%d", maxNumber);
	char *line2 = malloc(sizeof(char)*MAX_SIZE_LOGS_LINE);
	char number2[16];
	sprintf(number2, "%d", maxSize);
	char *line3 = malloc(sizeof(char)*MAX_SIZE_LOGS_LINE);
	char number3[6];
	sprintf(number3, "%d", rimpiazzi);
	char *line4 = malloc(sizeof(char)*MAX_SIZE_LOGS_LINE);
	char number4[6];
	sprintf(number4, "%d", maxConnessCont);
	strcpy(line1,"");
	strcat(line1,"NumeroMassimoDiFile,");
	strcat(line1,number1);
	strcpy(line2,"");
	strcat(line2,"BytesMassimiRaggiunti,");
	strcat(line2,number2);
	strcpy(line3,"");
	strcat(line3,"NumeroRimpiazzi,");
	strcat(line3,number3);
	strcpy(line4,"");
	strcat(line4,"NumeroMaxConnessioniContemporanee,");
	strcat(line4,number4);
	writeToLogs(line1);
	writeToLogs(line2);
	writeToLogs(line3);
	writeToLogs(line4);
	int i;
	for(i = 0; i < nThreads; i++){
		char *lineTmp = malloc(sizeof(char)*MAX_SIZE_LOGS_LINE);
		if(nRichiestePerWorker[i][1] > 0){
			char codicethread[7];
			char numeroRichieste[7];
			sprintf(codicethread, "%d", nRichiestePerWorker[i][0]);
			sprintf(numeroRichieste, "%d", nRichiestePerWorker[i][1]);
			strcpy(lineTmp,"");
			strcat(lineTmp,"thread");
			strcat(lineTmp,codicethread);
			strcat(lineTmp,",");
			strcat(lineTmp,numeroRichieste);
			writeToLogs(lineTmp);
		}
		free(lineTmp);
	}
	free(line1);
	free(line2);
	free(line3);
	free(line4);
}


