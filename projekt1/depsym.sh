#!/bin/bash

# autor: Sara SKUTOVA xskuto00@stud.fit.vutbr.cz
# skript vypisuje zavislosti modulu na modulech podle definice/umisteni globalnich symbolu

#funkce ktera vytiskne napovedu
help()
{	echo "depsym.sh [-g] [-r OBJECT_ID| -d OBJECT_ID] FILEs" >&2
	exit 1
}

#kontrola, zda skrypt byl spusten s nejakymi parametry
if [ "$#" = 0 ]; then
	help
fi

grafic=false
vypisR=false
vypisD=false
objectID=

#zpracovani parametru
while getopts ":gr:d:" arg; do
	case "$arg" in
		g)
		  grafic=true
		  ;;
		r)
		  vypisR=true
		  objectID="$OPTARG"
		  ;;
		d)
		  vypisD=true
		  objectID="$OPTARG"
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

if [ "$vypisR" = true -o "$vypisD" = true ]; then
	if [ $(echo "$objectID" | grep -c '.*\.o') == 1 ]; then
		:
	else
		echo "Chyba: modul u prepinace -d|-r nema pozadovany format"
	fi
fi

#zpracovani soboru	
shift $((OPTIND - 1))
if [ "$#" = 0 ]; then
	echo "Chyba: chyni soubor/y FILE" >&2
	help
else
	i="$#"
	soubory=$(while [ "$i" != 0 ]; do
			echo "$1"
			i=$(($i-1))
			shift
	done
	)
fi
	


#vytvoreni a zabezpeceni smazani docasneho pomocneho souboru
trap 'rm $temp' SIGHUP SIGINT SIGQUIT SIGKILL SIGTERM
temp=$(mktemp /tmp/xskuto00.XXXXXX)

#zpracovani zadanych souboru a vypsani zavislych symbolu s nazvy modulu
zkouska2=$(echo "$soubory" | while read radekS; do
	if [ "$grafic" = true ]; then
		file=" $radekS"
	else
		file="$radekS"
	fi
	echo "$file"
	nm "$radekS" | awk '$1 ~ /U/'		
done)

#zpracovani zadanych souboru a vypsani definujicich symbolu s nazvy modulu
zkouska=$(echo "$soubory" | while read radekS; do
	if [ "$grafic" = true ]; then
		file="$radekS"
	else
		file="$radekS"
	fi
	echo "$file"
	nm "$radekS" | awk '$2 ~ /[TBCDG]/'
done)


#cerna magie a temna strana Sily
#cte ze souboru, hleda jmena modulu = object1
#kdyz nalezne nedefinovane symboly = sym
#druhy while hleda pro nedefinovane symboly jejich definovane protejsky
vypis=$(echo "$zkouska2" | while read radek; do
	if [ $(echo "$radek" | grep -c "[TBCDGU] ") == 1 ]; then
		if [ $(echo "$radek" | grep -c -w "U") == 1  ]; then
			sym=$(echo "$radek" | awk '{print $2}')
				echo "$zkouska" | while read radek_s2; do
					if [ $(echo "$radek_s2" | grep -c '[TBCDGU] ') == 1 ]; then	
						if [ $(echo "$radek_s2" | grep -c '[TBCDG] ') == 1 ]; then
							sym2=$(echo "$radek_s2" | awk '{print $3}')
							if [ "$sym" == "$sym2" ]; then
								if [ "$grafic" = true ]; then
									echo "$(basename $object1) -> $(basename $object2) ($sym)" >> $temp
									echo "$object1 -> $object2 ($sym)"
								else
									echo "$object1 -> $object2 ($sym)"
								fi
							fi
						fi
					else
						object2="$radek_s2"
					fi
				done
				
		fi
	else
		 object1="$radek"
	fi
done)

vysledek=$(echo "$vypis" | sort -u)

vypis_graf="$(sort -u $temp)"


#dunkce na dosazeni potrebneho formatu pro vypis puvodnich nazvu modulu
fun_moduly_graf()
{
	if [ "$vypisR" = false -a "$vypisD" = false ]; then
		echo "$vysledek"

	elif [ "$vypisD" = true ]; then
		echo "$vysledek" | while read radek; do
			if [ $(echo "$radek" | awk '{print $1}' | grep -c -w "$objectID") == 1 ]; then
				echo "$radek"
			fi
		done
	elif [ "$vypisR" = true ]; then
		echo "$vysledek" | while read radek; do
			if [ $(echo "$radek" | awk '{print $3}' | grep -c -w "$objectID") == 1 ]; then
				echo "$radek"
			fi
		done
	fi	 
}

#vytvori pozadovany format pro graf
if [ "$grafic" = true ]; then
	vysledek_graf="$(echo "$vypis_graf" | sed 's/-/_/g' | sed 's/\./D/g' | sed 's/+/P/g')"
	graf="$(echo "$vysledek_graf" | sed 's/(/[label=\"/g' | sed 's/)/\"];/g' | sed 's/_>/->/g')"
	
	#vytvori pozadovany format pro vypis puvodnich modulu do grafu
	pomoc="$(fun_moduly_graf)"

	if [ "$pomoc" = "" ]; then
	:
	else
		echo "$pomoc" | awk '{print $1}' > $temp
		echo "$pomoc" | awk '{print $3}' >> $temp
		moduly=$(while read radek; do
			echo "$radek"
		done < $temp)

		moduly_origo="$(echo "$moduly" | sort -u)"
		moduly_graf=$(echo "$moduly_origo" | while read radek; do
			modul="$(echo "$radek" | sed 's/-/_/g' | sed 's/\./D/g' | sed 's/+/P/g')"
			echo "$(basename $modul) [label=\"$radek\"];"
			done)
	fi
	objectID="$(echo "$objectID" | sed 's/-/_/g' | sed 's/\./D/g' | sed 's/+/P/g')"

fi

#vypis na standartni vystup
if [ "$vypisR" = false -a "$vypisD" = false ]; then
	if [ "$grafic" = true ]; then
		echo "digraph GSYM {"
			echo "$graf"
			if [ "$pomoc" = "" ]; then
				:
			else
				echo "$moduly_graf"
			fi
		echo "}"
	else
		echo "$vysledek"
	fi
elif [ "$vypisD" = true ]; then
	if [ "$grafic" = true ]; then
		echo "digraph GSYM {"
		echo "$graf" | while read radek; do
			if [ $(echo "$radek" | awk '{print $1}' | grep -c -w "$objectID") == 1 ]; then
				echo "$radek"
			fi
		done
		if [ "$pomoc" = "" ]; then
			:
		else
			echo "$moduly_graf"
		fi

		echo "}"
	else
		echo "$vysledek" | while read radek; do
			if [ $(echo "$radek" | awk '{print $1}' | grep -c -w "$objectID") == 1 ]; then
				echo "$radek"
			fi
		done
	fi
elif [ "$vypisR" = true ]; then
	if [ "$grafic" = true ]; then
		echo "digraph GSYM {"
		echo "$graf" | while read radek; do
			if [ $(echo "$radek" | awk '{print $3}' | grep -c -w "$objectID") == 1 ]; then
				echo "$radek"
			fi
		done
		if [ "$pomoc" = "" ]; then
			:
		else
			echo "$moduly_graf"
		fi

		echo "}"
	else
		echo "$vysledek" | while read radek; do
			if [ $(echo "$radek" | awk '{print $3}' | grep -c -w "$objectID") == 1 ]; then
				echo "$radek"
			fi
		done
	fi

fi 

#smazani pocasneho pomocnego souboru
rm "$temp"

























