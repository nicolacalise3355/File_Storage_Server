#!/bin/bash

sleep 2

echo -------------CLIENTS---------------------------------------------------------------
# Inviamo al server 6 files
./client -f ./cs_sock -p -t 200 -w cfiles,6
echo ------------------------------------------------------------------------------------
# Inviamo al server un file esplicitamente
./client -p -t 200 -W cfiles/dir/anotherDir/testFile
echo ------------------------------------------------------------------------------------
# Andiamo a leggere un file
./client -p -t 200 -r cfiles/hosting
echo ------------------------------------------------------------------------------------
# Andiamo a bloccare e sucessivamente eliminare un file
./client -p -t 200 -l cfiles/prog.php -c cfiles/prog.php
echo ------------------------------------------------------------------------------------
# Andiamo a leggere un file salvandolo nella directory 
./client -p -t 200 -d dirExp -r cfiles/lorem
echo ------------------------------------------------------------------------------------
# Andiamo a leggere due file casuali dal server
./client -p -t 200 -R 2