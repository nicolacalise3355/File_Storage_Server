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
#include <limits.h>
#include <math.h>
#include <stdarg.h>

#include "server_utils.h"

/*
@params: stringa
@effects: conta il numero delle parole separate da t -> #
@return: numero delle parole
*/
int contaParole(char *string){
	//#
	char t[1] = "#";
	int len = strlen(string);
	int i;
	int count = 1;
	for(i = 0; i < len; i++){
		if(string[i] == t[0]){
			count = count + 1;
		}
	}
	return count;
}

/*
@params: char -tmp
@effects: controlla che tmp sia un numero
@return: ritorna il numero
*/
int controllaChar(char tmp){ 
	int c = 0;
	if(tmp >= 65 && tmp <= 90){
		c = 1;
	}
	if(tmp >= 97 && tmp <= 122){
		c = 1;
	}
	return c;
}

/*
*/
int max(int args, ...){
	int i, max, cur;
	va_list valist;
	va_start(valist, args);
	max = INT_MIN;
	for(i=0; i<args; i++){
		cur = va_arg(valist, int); 
		if(max < cur){
			max = cur;
		}
	}
	va_end(valist);
	return max;
}

int findCifre(int numero){
	if(numero < 0){
		return 0;
	}else if(numero >= 0 && numero <= 9){
		return 1;
	}else if(numero >= 10 && numero <= 99){
		return 2;
	}else if(numero >= 100 && numero <= 999){
		return 3;
	}else if(numero >= 1000 && numero <= 9999){
		return 4;
	}else if(numero >= 10000 && numero <= 99999){
		return 5;
	}else if(numero >= 100000 && numero <= 999999){
		return 6;
	}else if(numero >= 1000000 && numero <= 9999999){
		return 7;
	}
}