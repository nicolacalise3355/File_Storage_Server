#!/bin/bash

sleep 2

echo -------------CLIENTS---------------------------------------------------------------
# Inseriamo subito 10 files (Limite del server)
./client -t 200 -w cfiles,10
echo ------------------------------------------------------------------------------------
# Andiamo a inserire esplicitamente un file che sicuramente non abbiamo gia importato
# con questo verifichimo se il riampiazzo e' avvenuto
# salviamo il file in DirExp
./client -t 200 -D dirExp -W cfiles/dir/anotherDir/testFile
echo ------------------------------------------------------------------------------------

