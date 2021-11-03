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

int contaParole(char *string);
int controllaChar(char tmp);
int max(int args, ...);
int findCifre(int numero);