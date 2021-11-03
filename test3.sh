#!/bin/bash

sleep 1

SERVER_PID=$!
export SERVER_PID
#bash -c 'sleep 30 && kill -2 ${SERVER_PID}' &



#Creiamo una lista di id di Processi
pids=()
for i in {1..10}; do
	#lanciamo 10 runner e salviamo il pid del pr in modo da chiuderli dopo
	bash -c './test3_runner.sh' &
	pids+=($!)
	sleep 0.2
done

#dormiamo 30 secondi
sleep 30
#Andiamo a uccidere i processi 
for i in "${pids[@]}"; do
	kill -9 ${i}
	wait ${i}
done

sleep 5

while read p; do
  echo "$p"
  kill -2 $p
done < server.PID

./ts.sh

#alle fine dei 30 secondi il server dovrebbe aver ricevuto diverse richieste con errori
#questo perche' per molte volte tentiamo di scrivere file gia' scritti 
#essendoci 10 clients in contemporanea possiamo testare il funzionamento delle lock
#alla fine del test il sever dovrebbe ricevere qualche decina di migliaia di richieste