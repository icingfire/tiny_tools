#!/bin/bash
# $1  ip addr or file contains ips

function check_ip()
{
	echo $1 |grep "^[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}$" >/dev/null
	return $?
}

function scan_ip()
{
	# tcp top100 + serval special ports
	tcp_fast_ports="7,9,13,21,22,23,25,26,37,53,79,80,81,88,106,110,111,113,119,135,139,143,144,179,199,389,427,443,444,445,465,513,514,515,543,544,548,554,587,631,646,837,873,888,990,993,995,1025,1026,1027,1028,1029,1110,1433,1720,1723,1755,1900,2000,2001,2049,2121,2181,2375,2717,3000,3128,3306,3389,3986,4899,5000,5009,5051,5060,5101,5190,5357,5432,5601,5631,5666,5800,5900,5901,5984,5985,6000,6001,6379,6646,7070,8000,8008,8009,8080,8081,8161,8443,8888,9100,9200,9990,9999,10000,11211,27017,32768,49152,49153,49154,49155,49156,49157,50070"

	# fast scan
	nmap -sC -sV --script=vuln -p${tcp_fast_ports} -n -Pn $1 -oN nmap_${1}.txt
	nmap -sUC --top-ports 30 --open -n -Pn $1 2>/dev/null | sed -n '4,$p' | tee -a nmap_${1}.txt

	# stage2 masscan full ports
	masscan -p1-65535 -n -Pn $1 | tee -a masscan_${1}.txt
	awk '{if($1 ~ "/tcp" && $2=="open") print $1}' nmap_${1}.txt|awk -F/ '{print $1}'|sort -n > t_nmap_${1}.txt
	awk '{print $4}' masscan_${1}.txt|awk -F/ '{print $1}'|sort -n -u > t_masscan_${1}.txt
	target_ports=$(diff t_nmap_${1}.txt t_masscan_${1}.txt | sed -n '/>/p'|awk '{print $2}')
	if [ "${target_ports}" != "" ]; then
		nmap -sC -sV --script=vuln -p$(echo ${target_ports}|sed 's/ /,/g') -n -Pn $1  |tee -a nmap_${1}.txt
	fi 

	# stage3 masscan again to check potential missing ports
	cat t_nmap_${1}.txt t_masscan_${1}.txt | sort -n -u > t_pre_whole.txt
	masscan -p1-65535 -n -Pn $1 | tee -a masscan2_${1}.txt
	awk '{print $4}' masscan2_${1}.txt|awk -F/ '{print $1}'|sort -n -u > t_masscan2_${1}.txt
	target_ports=$(diff t_pre_whole.txt t_masscan2_${1}.txt | sed -n '/>/p'|awk '{print $2}')
	if [ "${target_ports}" != "" ]; then
		nmap -sC -sV --script=vuln -p$(echo ${target_ports}|sed 's/ /,/g') -n -Pn $1  |tee -a nmap_${1}.txt
	fi 

	# clear
	rm t_masscan_${1}.txt t_nmap_${1}.txt t_pre_whole.txt t_masscan2_${1}.txt masscan_${1}.txt masscan2_${1}.txt
}

################ main ##################


if [ ! -n "$1" ]; then
	echo "input ip.txt file or ip addr"
	exit 1
fi

if [ -f "$1" ]; then
	while read one_ip
	do
		if check_ip ${one_ip}; then
			scan_ip ${one_ip}
		else
			echo "invalid ip ${one_ip}"
		fi
	done < $1
elif check_ip ${1}; then
	scan_ip ${1}
fi
