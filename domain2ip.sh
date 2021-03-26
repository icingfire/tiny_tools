#!/bin/bash
# $1  file contains domain names

if [ ! -f "$1" ]; then
    echo "file $1 not found, exit"
    exit
fi

while read one_host
do
    (ret_ip=$(ping -n -c 1 ${one_host} |head -1|awk '{print $3}'|sed 's/[\(\)]//g'); [ -n "${ret_ip}" ] && echo "${ret_ip}  ${one_host}" >> "${1}".ip ) &
    sleep 0.1
done < $1
