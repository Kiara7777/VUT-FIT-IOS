#!/bin/bash

#autor: Sara SKUTOVA xskuto00@stud.fit.vutbr.cz
#skript ma za ukol vypsat zavislosti funkci na volani funkci

#funkce ktera vytiskne napovedu
help()
{
	echo "depcg.sh [-g] [-p] [-r FUNCTION_ID|-d FUNCTION_ID] FILE" >&2
	exit 1
}

#kontrola poctu parametru skriptu
if [ "$#" = 0 -o "$#" -gt 5 ]; then
	help
fi

grafic=false
prepP=false
vypisR=false
vypisD=false
functionID=
soubor=

#zpracovani parametru
while getopts ":gpr:d:" arg; do
	case "$arg" in
		g)
		  grafic=true
		  ;;
		p)
		  prepP=true
		  ;;
		r)
		  vypisR=true
		  functionID="$OPTARG"
		  ;;
		d)
		  vypisD=true
		  functionID="$OPTARG"
                  ;;
		:)
		  echo "Chyba: Parametr -$OPTARG ocekava argument" >&2
		  help
		  ;;
		\?)
		   echo "Nespravny parametr: -$OPTARG" >&2
		   help
		   ;; 
	esac
done

#kontrola, zda neni skript spusten zaroven s parametrem -d jak i -r
if [ "$vypisR" = true -a "$vypisD" = true ]; then
	echo "Chyba: Parametr -r a -d nemohou byt spolecne"
	help
fi

#zpracovani souboru
shift $((OPTIND - 1))
if [ "$#" = 0 ]; then
	echo "Chyba: chybi soubor FILE"
	help
else
	soubor="$1"
fi

if [ -f "$soubor" ]; then
:
else
	echo "Chyba: soubor neexistuje"	
	help
fi

#vytvoreni a zabezpeceni smazani docasneho pomocneho souboru
trap 'rm $temp' SIGHUP SIGINT SIGQUIT SIGKILL SIGTERM 
temp=$(mktemp /tmp/xskuto00.XXXXXX)


#zpracovani souboru, aby odpovidal pozadavkum prepinace
if [ "$prepP" = false ]; then
	objdump -d -j .text "$soubor" | grep -E '<.*>:|callq' | grep '<' | awk '$9 !~ /@plt>$/' | awk '$9 !~ /+/' | awk '$9 !~ /^[0-9]/' > $temp
else
	if [ "$grafic" = true ]; then
		objdump -d -j .text "$soubor" | grep -E '<.*>:|callq' | grep '<'| awk '$9 !~ /+/'| awk '$9 !~ /^[0-9]/' | sed 's/@plt/_PLT/g' > $temp
	else
		objdump -d -j .text "$soubor" | grep -E '<.*>:|callq' | grep '<' | awk '$9 !~ /+/' | awk '$9 !~ /^[0-9]/' > $temp
	fi
fi

#nalezeni volajici a volane funkce
#zpracoani do pozadovaneho formatu
vystup=$(while read radek; do
	if [ $(echo "$radek" | grep -c 'callq ') == 1 ];then
		echo "$volajici -> $(echo "$radek" | awk '{print $9}' | sed 's/^<//' | sed 's/>$//')"
	else
		volajici=$(echo "$radek" | awk '{print $2}' | sed 's/^<//g' | sed 's/>:$//g')
	fi
	done < $temp)

#zamezeni duplicite
vypis=$(echo "$vystup" | sort -u)

#vypis na obrazovku v pozadovanem tvaru dle parametru
if [ "$vypisR" = false -a "$vypisD" = false  ]; then
	if [ "$grafic" = true ]; then
		echo "digraph CG {"
		echo "$vypis" | sed 's/$/;/g'
		echo "}"
	else
		echo "$vypis"
	fi
elif [ "$vypisD" = true ]; then
	if [ "$grafic" = true ]; then
		echo "digraph CG {"
	fi
	echo "$vypis" | while read radek; do
		if [ $(echo "$radek" | awk '{print $1}'| grep -c -w "$functionID") == 1 ]; then
			if [ "$grafic" = true ]; then
				echo "$radek;"
			else
				echo "$radek"
			fi
		fi
	done
	if [ "$grafic" = true ]; then
		echo "}"
	fi
elif [ "$vypisR" = true ]; then
	if [ "$grafic" = true ]; then
		echo "digraph CG {"
	fi
	echo "$vypis" | while read radek; do
		if [ $(echo "$radek" | awk '{print $3}' | grep -c -w "$functionID") == 1 ]; then
			if [ "$grafic" = true ]; then
				echo "$radek;"
			else
				echo "$radek"
			fi
		fi
	done
	if [ "$grafic" = true ]; then
		echo "}"
	fi		
fi

#smazani docasneho pomocneho souboru
rm $temp


























