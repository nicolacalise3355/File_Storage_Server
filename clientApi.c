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

#include "clientApi.h"

//andando a modifcare questi dati chiaramente modifichiamo la grandezza massima dei file
#define N 300
#define MAX_FILE_SIZE 200
#define MAX_LINE_FILE_SIZE 50

/*
Setta il path temporaneo 
*/
void setPathTmp(char *tmp){
	pathTmp = NULL;
	pathTmp = malloc(sizeof(char)*strlen(tmp)+1);
	strcpy(pathTmp,tmp);
}

//ritorna il path temporaneo settato
char *returnPathTmp(){
	return pathTmp;
}

//setta la directory per le espulsioni
void setDirExp(char *string){
	if(!string){ errno = EAGAIN; }
	DIRECTORY_EXP_C = malloc(sizeof(char)*strlen(string));
	strcpy(DIRECTORY_EXP_C,string);
}
//setta la directory per le read
void setDirRead(char *string){
	if(!string){ errno = EAGAIN; }
	DIRECTORY_READ_C = malloc(sizeof(char)*strlen(string));
	strcpy(DIRECTORY_READ_C,string);
}

//genera un nome per un file esportato
char *getNameForFile(){
	char numeroInString[16];
	char numeroCount[10];
	char *name = malloc(sizeof(char)*23);
	int secondaAtm = time(NULL);
	sprintf(numeroInString,"%d",secondaAtm);
	strcpy(name,"file");
	strcat(name,numeroInString);
	sprintf(numeroCount,"%d",countGenName);
	strcat(name,numeroCount);
	countGenName = countGenName + 1;
	return name;
}

/** 
 * Esporta una file 
 * In base al flag passato capiamo che tipo di salvataggio stiamo facendo
 * se dopo una read, una readN oppure un espulsione
*/
void exportInFile(char *string, int flags, const char *pathname){

	if(!string){ errno = EAGAIN; }
	if(flags != 1 || flags != 2){ errno = EPROTO; }

	FILE *tmpFile;

	char *token;
	int count = 0;
	char *content;

	char *tmpFilename = getNameForFile();
	printf("FILE NAME = %s\n", tmpFilename);

	token = strtok(string, "#");
	while(token != NULL){
		if(count == 0){
			//leggiamo la size
		}else{
			content = malloc(sizeof(char)*strlen(token));
			strcpy(content,token);
		}
		token = strtok(NULL, "#");
	    count = count + 1;
	}
	//qua andiamo a verificare il flag e che la cartella scelta sia stata settata
	if(flags == 1 && DIRECTORY_EXP_C != NULL){
		//siamo in export
	    char *filename = malloc(sizeof(char)*(strlen(tmpFilename)+strlen(DIRECTORY_EXP_C)+2));
	    strcpy(filename,DIRECTORY_EXP_C);
	    strcat(filename,"/");
	    strcat(filename,tmpFilename);
		if((tmpFile = fopen(filename,"w")) == NULL){
			printf("Errore nell apertura del file esportato\n");
		}else{
				    printf("-\n");
			fprintf(tmpFile, "%s\n", content);
			fclose(tmpFile);
		}
	}else if(flags == 2 && DIRECTORY_READ_C != NULL){
		//siamo in read
	    char *filename = malloc(sizeof(char)*(strlen(tmpFilename)+strlen(DIRECTORY_READ_C)+2));
	    strcpy(filename,DIRECTORY_READ_C);
	    strcat(filename,"/");
	    strcat(filename,tmpFilename);
		if((tmpFile = fopen(filename,"w")) == NULL){
			printf("Errore nell apertura del file letto\n");
		}else{
			fprintf(tmpFile, "%s\n", content);
			fclose(tmpFile);
		}

	}else if(flags == 3 && DIRECTORY_READ_C != NULL){
				//siamo in readn
	    char *filename = malloc(sizeof(char)*(strlen(tmpFilename)+strlen(DIRECTORY_READ_C)+2));
	    strcpy(filename,DIRECTORY_READ_C);
	    strcat(filename,"/");
	    strcat(filename,tmpFilename);
		if((tmpFile = fopen(filename,"w")) == NULL){
			printf("Errore nell apertura del file letto\n");
		}else{
			fprintf(tmpFile, "%s\n", string);
			fclose(tmpFile);
		}
	}else{
		printf("Non abbiamo salvato\n");
	}
	
}

//Funzione che serve per esportare N file letti dopo una readN
void exportInFileN(char *content){
	if(DIRECTORY_READ_C != NULL){
		char *token1;
		char *end_str;
		token1 = strtok_r(content, "#", &end_str);
		while( token1 != NULL ) {

			printf("token %s\n", token1);

			exportInFile(token1,3,NULL);

			token1 = strtok_r(NULL, "#",&end_str);
		}
	}
}

//setta il codice dell'ultima richiesta inviata
void setCodeLastRequest(int code){
	codeLastRequest = code;
}
//recupera il codice dell'ultima richiesta inviata
int getCodeLastRequest(){
	return codeLastRequest;
}

//--- Connessione ---//

int closeConnection(const char *sockname){
	
	if(!sockname){
		errno = EINVAL;
	}
	printf("Disconnessione\n");
	close(skt);
	exit(0);
	
	return 1;
}

int openConnection(const char *sockname, int msec, const struct timespec abstime){
	int count = 0;
	//
	sa.sun_family = AF_UNIX;
	strncpy(sa.sun_path,sockname, strlen(sockname)+1);
	skt = socket(AF_UNIX,SOCK_STREAM,0);
	while(connect(skt,(struct sockaddr*)&sa,sizeof(sa)) == -1){
		printf("Socket non trovata\n");
		time_t currentTime = time(0);
		if(currentTime >= abstime.tv_sec){
			errno = EAGAIN;
			return -1;
		}
		sleep(msec);
	}
	return 0;
}

//Invia la richiesta e ritorna la risposta
char *sendRequest(char *req){
	char buf[1000];
	write(skt,req,N);
	//	
	read(skt,buf,1000);
	printf("resp %s\n", buf);
	char *resp = malloc(sizeof(char)*strlen(buf));
	strcpy(resp,buf);

	if(strcmp(resp,"-1")==0){
		errno = EINVAL;
	}

	return resp;
}

//--- Help ---//

/*
@effects: manda una richiesta al server di help
*/
void help(){
	printf("*** HELP ***\n");
	//char *input = malloc(sizeof(char)*6);
	//strcpy(input,"0,xxx");
	//write(skt,input,6);
	//read(skt,buf,4);
	//printf("Client riceve: %s\n", buf);
}


//--- Generazione delle richieste ---//

/** 
 * Genera la richiesta
 * Concatenando i parametri di input
*/
char *generateRequest(char *codeToSend, const char *request, char *arg){
	if(!codeToSend || !request){ errno = EINVAL; return ""; } 

	int sizeOf;
	char *req;
	if(arg == NULL){
		sizeOf = (strlen(codeToSend)) + (strlen(request));
		req = malloc(sizeof(char)*(sizeOf+1));
		strcpy(req,"");
		strcat(req,codeToSend);
		strcat(req,",");
		strcat(req,request);
	}else{
		sizeOf = (strlen(codeToSend)) + (strlen(request)) + (strlen(arg));
		req = malloc(sizeof(char)*(sizeOf+2));
		strcpy(req,"");
		strcat(req,codeToSend);
		strcat(req,",");
		strcat(req,request);
		strcat(req,","); //oppure da sostituire con qualsiasi cosa per la separazione
		strcat(req,arg); 
	}
	char *input = malloc(sizeof(char)*N);
	int ci;
	for(ci = 0; ci < N; ci++){
		input[ci] = 126;
	}
	strncpy(input,req,strlen(req));
	return input;
}

/*
@params: filename -> nome file
@effects: ricerca un file in una cartella predefinita e lo legge
@return: ritorna la stringa contenuta nel file
*/
char *readInternalFile(const char *filename){

	if(!filename){ errno = EINVAL; return ""; }

	printf("file name -> %s\n", filename);
	int lenF1 = strlen(filename);
	char *result = malloc(sizeof(char)*MAX_FILE_SIZE);
	strcpy(result,"");
	char cwd[200];
	getcwd(cwd,200);
	FILE *f = fopen(filename,"r");
	if(f == NULL){
		errno = EAGAIN;
		printf("File Null\n");
		return "FileError";
	}
	char *line = malloc(sizeof(char)*MAX_LINE_FILE_SIZE);
	while(!feof(f)){
		fscanf(f,"%s",line);
		strcat(result,line);
		//strcat(result,"\\n");
	}
	printf("cosa scriviamo -> %s\n", result);
	return result;
}

//--- Lavoro con i file ---//

int writeFile(const char *pathname, const char *dirname){

	if(!pathname){
		errno = EINVAL;
		return -1;
	}

	char codeToSend[2] = "5";
	char *tmpStringReq;
	char *content;
	if(strchr(pathname,'/') == NULL){
		//abbiamo solo il nome file perche veniamo a una ricerca
		printf("Siamo nella ricerca %s\n", pathname);
		char cwd[100];
		getcwd(cwd,100);
		char *tmp = strrchr(cwd,'/');
		int newSizeOfFinalPath = (strlen(tmp)-1) + strlen(pathname) + 2;
		char *finalPath = malloc(sizeof(char)*newSizeOfFinalPath);
		strcpy(finalPath,tmp+1);
		strcat(finalPath,"/");
		strcat(finalPath,pathname);
		printf("Final path %s\n", finalPath);
		content = readInternalFile(pathname);
		printf("path to add %s \n", returnPathTmp());
		tmpStringReq = generateRequest(codeToSend,returnPathTmp(),content);
	}else{
		//abbiamo il path relativo delle cartelle perche proveniamo da una richieste semplice
		printf("Siamo nella richiesta semplice %s\n", pathname);
		content = readInternalFile(pathname);
		tmpStringReq = generateRequest(codeToSend,pathname,content);
	}
	printf("SEND: %s\n", tmpStringReq);
	char *resp = sendRequest(tmpStringReq);
	if((int)strlen(resp) > 3){
		exportInFile(resp,1,pathname);
	}

	if(strcmp(resp,"-1") != 0){
		return 1;
	}else{
		return -1;
	}
}

int readFile(const char *pathname, void **buff, size_t *size){
	char codeToSend[2] = "3";
	char *tmpStringReq = generateRequest(codeToSend,pathname,NULL);
	//printf("SEND: %s\n", tmpStringReq);
	char *resp = sendRequest(tmpStringReq);
	if((int)strlen(resp) > 2){
		exportInFile(resp,2,pathname);
	}
	if(strcmp(resp,"-1") != 0){
		return 1;
	}else{
		return -1;
	}
}

int readNfile(int enne, const char *dirname){
	char codeToSend[2] = "4";
	char numberString[3];
	sprintf(numberString,"%d",enne);
	char *tmpStringReq = generateRequest(codeToSend,numberString,NULL);
	printf("SEND %s\n", tmpStringReq);
	char *resp = sendRequest(tmpStringReq);
	printf("resp %s\n", resp);
	if((int)strlen(resp) > 2){
		printf("Exp\n");
		exportInFileN(resp);
	}
	if(strcmp(resp,"-1") != 0){
		return 1;
	}else{
		return -1;
	}
}

int lockFile(const char *dirname){
	char codeToSend[2] = "9";
	char *tmpStringReq = generateRequest(codeToSend,dirname,NULL);
	printf("SEND %s\n", tmpStringReq);
	char *resp = sendRequest(tmpStringReq);
	if(strcmp(resp,"-1") != 0){
		return 1;
	}else{
		return -1;
	}
}

int unlockFile(const char *dirname){
	char codeToSend[3] = "10";
	char *tmpStringReq = generateRequest(codeToSend,dirname,NULL);
	printf("SEND %s\n", tmpStringReq);
	char *resp = sendRequest(tmpStringReq);
	if(strcmp(resp,"-1") != 0){
		return 1;
	}else{
		return -1;
	}
}

int removeFile(const char *pathname){
	char codeToSend[2] = "8";
	char *tmpStringReq = generateRequest(codeToSend,pathname,NULL);
	printf("SEND %s\n", tmpStringReq);
	char *resp = sendRequest(tmpStringReq);
	if(strcmp(resp,"-1") != 0){
		return 1;
	}else{
		return -1;
	}
}

int closeFile(const char *pathname){
	char codeToSend[2] = "7";
	char *tmpStringReq = generateRequest(codeToSend,pathname,NULL);
	printf("SEND %s\n", tmpStringReq);
	char *resp = sendRequest(tmpStringReq);
	if(strcmp(resp,"-1") != 0){
		return 1;
	}else{
		return -1;
	}
}

int appendToFile(const char *pathname, void *buff, size_t size, const char *dirname){
	//scrivi su pathname i size dati di buff
	char codeToSend[2] = "6";
	char *request = malloc(sizeof(char)*size);
	strncpy(request,buff,size);
	char *tmpStringReq = generateRequest(codeToSend,pathname,request);
	printf("SEND %s\n", tmpStringReq); 
	char *resp = sendRequest(tmpStringReq);
	if(strcmp(resp,"-1") != 0){
		return 1;
	}else{
		return -1;
	}
}

int openFile(const char *pathname, int flags){
	char codeToInsert[2] = "2";
	char codeToOpen[2] = "1";

	if(flags == 3){
		setCodeLastRequest(-1);
		char *tmpStringReq1 = generateRequest("11",pathname,NULL);
		char *resp = sendRequest(tmpStringReq1);
		printf("RESP OPENLOCK %s\n", resp);
		if(strlen(resp) > 2){
			exportInFile(resp,1,NULL);
			return 1;
		}
		return atoi(resp);
	}else if(flags == 1){
		char *tmpStringReq1 = generateRequest(codeToInsert,pathname,NULL);
		char *tmpStringReq2 = generateRequest(codeToOpen,pathname,NULL);
		char *resp1 = sendRequest(tmpStringReq1);
		char *resp2 = sendRequest(tmpStringReq2);
		printf("resps insertOpen %s %s \n",resp1,resp2);
		return 1;
	}else if(flags == 2){
		char *tmpStringReq2 = generateRequest(codeToOpen,pathname,NULL);
		printf("SEND %s\n", tmpStringReq2);
		char *resp = sendRequest(tmpStringReq2);
		int res = lockFile(pathname);
		printf("resps open lock %s %d\n",resp,res );
		return 1;
	}else if(flags == 0){
		setCodeLastRequest(-1);
		char *tmpStringReq3 = generateRequest(codeToOpen,pathname,NULL);
		printf("SEND %s\n", tmpStringReq3);
		char *resp = sendRequest(tmpStringReq3);
		printf("resps open %s\n", resp);
		return 1;
	}
	return -1;
}

