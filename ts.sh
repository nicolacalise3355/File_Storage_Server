#!/bin/bash


starter(){
	echo Statistiche
	echo "Attendi, potrebbe richiedere fino a qualche minuto..."
}


mediaBytesRead=0
mediaBytesWrite=0
totaleBytesRead=0
totaleBytesWrite=0
numeroDiRead=0
numeroDiWrite=0
numeroLock=0
numeroOpenLock=0
numeroUnlock=0
numeroCLose=0
dimensioneMaxRaggiunta=0
dimensioneMaxFiles=0
numeroVittine=0
numeroRichiestePerThread=0
maxConnessioniContemporanee=0


input_file="server_logs"

printResult(){
	ct=0
	echo "> Media Bytes per Reads [$mediaBytesRead]"
	echo "> Media Bytes per Write [$mediaBytesWrite]"
	echo "> Totale Bytes Read [$totaleBytesRead]"
	echo "> Totale Bytes Write [$totaleBytesWrite]"
	echo "> Numero di Read [$numeroDiRead]"
	echo "> Numero di Write [$numeroDiWrite]"
	echo "> Numero di Lock Singole [$numeroLock]"
	echo "> Numero di OpenLock [$numeroOpenLock]"
	echo "> Numero di Unlock [$numeroUnlock]"
	echo "> Numero di Close [$numeroCLose]"
	echo "> Numero Rimpiazzi [$numeroVittine]"
	echo "> Numero Max Connessioni Contemporanee [$maxConnessioniContemporanee]"
	echo "> Dimensione Max in file [$dimensioneMaxFiles]"
	echo "> Dimensione Max in bytes [$dimensioneMaxRaggiunta]"
}

calcThread(){
	x=$1   # Nome parametro
	y=$2   # Nome parametro
	echo "Richieste gestited da "$x "=" $y
}

calcolatore(){
	x=$1   # Nome parametro
	y=$2   # Nome parametro

	#echo $x
	if [[ "$x" == *"NumeroRimpiazzi"* ]]; then 
		numeroVittine=$y
	elif [[ "$x" == *"NumeroMaxConnessioniContemporanee"* ]]; then
		maxConnessioniContemporanee=$y
	elif [[ "$x" == *"BytesMassimiRaggiunti"* ]]; then
		dimensioneMaxRaggiunta=$y
	elif [[ "$x" == *"NumeroMassimoDiFile"* ]]; then
		dimensioneMaxFiles=$y
	elif [[ "$x" == *"openLock"* ]]; then
		numeroOpenLock=$((numeroOpenLock+1))
	elif [[ "$x" == "lock"*  ]]; then
		numeroLock=$((numeroLock+1))
	elif [[ "$x" == *"unlock"*  ]]; then
		numeroUnlock=$((numeroUnlock+1))
	elif [[ "$x" == *"closeFile"*  ]]; then
		numeroCLose=$((numeroCLose+1))
	elif [[ "$x" == *"writeFile"*  ]]; then
		numeroDiWrite=$((numeroDiWrite+1))
		totaleBytesWrite=$(($totaleBytesWrite+$y))
	elif [[ "$x" == *"readFile"*  ]]; then
		numeroDiRead=$((numeroDiRead+1))
		totaleBytesRead=$(($totaleBytesRead+$y))
	elif [[ "$x" == *"thread"* ]]; then
		calcThread $x $y
	fi 
}

handler(){
	y=\$"$1"   # Nome parametro
    #echo $y    # 
	contents=$(echo $y | tr "," "\n")
	count=0
	opname=""
	opcontent=""
	for addr in $contents
	do
		case $count in
		0)
			#echo "0> [$addr]"
			opname=$addr;;
		1)
			#echo "1> [$addr]"
			opcontent=$addr;;
		esac
		count=$((count+1))
	done
	calcolatore $opname $opcontent
}

echo "-----------------------------"
starter
echo "-----------------------------"
sleep 1

while read p; do
  handler "$p"
done < server_logs

mediaBytesRead="$(($totaleBytesRead/$numeroDiRead))"
mediaBytesWrite="$(($totaleBytesWrite/$numeroDiWrite))"

printResult