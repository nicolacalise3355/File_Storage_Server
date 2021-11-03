#!/bin/bash

#Questo Srcipt viene lanciato da test3.sh
# per poter essere lanciato abbiamo dovuto cambiare il permesso del file ad "eseguibile"
# lo script lancia in generale 3 client 
# uno tenta di mandare dagli 1 agli 8 files al server
# uno tenta di leggerne 1/5
# uno chiede sempre la lettura di due file specifichi

while true
do

./client -t 500 -w cfiles,$((1 + RANDOM % 8))

./client -t 500 -R $((1 + RANDOM % 5))

./client -t 500 -r cfiles/hosting,cfiles/docs.txt

done

#fra la w, la R e r vengono mandate circa 10 richieste al server

