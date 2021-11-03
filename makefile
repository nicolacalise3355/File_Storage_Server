# ---------------------------------------------------------------------------
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 2 as 
#  published by the Free Software Foundation.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
#  As a special exception, you may use this file as part of a free software
#  library without restriction.  Specifically, if other files instantiate
#  templates or use macros or inline functions from this file, or you compile
#  this file and link it with other files to produce an executable, this
#  file does not by itself cause the resulting executable to be covered by
#  the GNU General Public License.  This exception does not however
#  invalidate any other reasons why the executable file might be covered by
#  the GNU General Public License.
#
# ---------------------------------------------------------------------------

SERVERPID = ./server.PID

CC		=  gcc
CFLAGS	        = -std=c99 -Wall -pedantic
LIBS            = -lpthread

ms: masterServer.o server_conf.o server_utils.o server_files.o logger_funcs.o server_request_list.o client.o clientApi.o
	gcc -o ms masterServer.o server_conf.o server_utils.o server_files.o logger_funcs.o server_request_list.o -lpthread
	gcc -o client client.o clientApi.o
#
server_conf.o: server_conf.c
	gcc -c server_conf.c
#
server_utils.o: server_utils.c
	gcc -c server_utils.c
#
server_files.o: server_files.c
	gcc -c server_files.c
#
logger_funcs.o: logger_funcs.c
	gcc -c logger_funcs.c
#
server_request_list.o: server_request_list.c
	gcc -c server_request_list.c
#
masterServer.o: masterServer.c
	gcc -c masterServer.c


clientApi.o: clientApi.c
	gcc -c clientApi.c 

client.o: client.c 
	gcc -c client.c

test1: ms config1.txt
	(valgrind --leak-check=full ./ms ./config1.txt) & echo $$! > $(SERVERPID) &
	./test1.sh
	if [ -e server.PID ]; then \
		kill -1 $$(cat server.PID) || true; \
	fi;
	sleep 2 
	./ts.sh
	cat /dev/null > server_logs

test2: ms config2.txt
	(./ms ./config2.txt) & echo $$! > $(SERVERPID) &
	./test2.sh
	if [ -e server.PID ]; then \
		kill -1 $$(cat server.PID) || true; \
	fi;
	sleep 2 
	./ts.sh
	cat /dev/null > server_logs

test3: ms config3.txt
	(./ms ./config3.txt) & echo $$! > $(SERVERPID) &
	./test3.sh

	#if [ -e server.PID ]; then \
		#kill -2 $$(cat server.PID) || true; \
	#fi;
	#./ts.sh
	cat /dev/null > server_logs


clean: 
	rm ./*.o 
	rm ./*.out