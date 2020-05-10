#!/usr/bin/env bash

echo "Usage: $0 [ SRC [ DST ] ]"
echo "Where SRC and DST are optional ssh hostnames (or user@hostname)"
echo "For your computer, please enable sshd and use 'localhost'"
echo "default is localhost"
echo "----"

set -e

SRC=$1
[ -z $SRC ] && read -p "Source host: " SRC
[ -z $SRC ] && SRC=localhost
echo "Source: $SRC"

DST=$2
[ -z $DST ] && read -p "Destination host: " DST
[ -z $DST ] && DST=localhost
echo "Destination: $DST"

echo "----"
echo "Select to work on MulitMC instances or worlds of 'native' minecraft"
select M_PATH in .local/share/multimc/instances/ .minecraft/saves/; do
	if [ -z $M_PATH ]; then
                echo "ERROR: No selection"
                exit 1
        fi
        break
done


echo "Getting list from $SRC"
LIST=( "$(ssh $SRC find $M_PATH -maxdepth 1 -mindepth 1 -type d -printf \"$SRC:%p\\n\" |grep -v _MMC_TEMP)" )

IFS=$'\n'
select I in ${LIST[@]}; do
	if [ -z $I ]; then
		echo "ERROR: No selection"
		exit 1
	fi
	SRC_PATH=${I// /\\ }
	break
done

echo ""
echo "Do you want to DELETE it on $DST if it exists or make a backup?"
select ACTION in delete backup; do
	if [ -z $ACTION ]; then
                echo "Then I'll do a backup..."
		ACTION="backup"
        fi
        break
done


TMP_DIR=$(mktemp -d)
scp -r "$SRC_PATH" $TMP_DIR
ssh $DST mkdir -p $M_PATH
NAME=$(ls $TMP_DIR)


[ $ACTION == "backup" ] && echo Trying to create a backup
[ $ACTION == "backup" ] && ssh $DST [ ! -f $M_PATH/${NAME// /\\ } ] || mv $M_PATH/${NAME// /\\ } $M_PATH/${NAME// /\\ }-$(date +%F_%H-%M-%S)
[ $ACTION == "delete" ] && ssh $DST rm -rf $M_PATH/${NAME// /\\ }
scp -r $TMP_DIR/* "$DST:$M_PATH"
