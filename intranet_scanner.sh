#!/bin/bash
# a simple script for detect alive intranet ips
# plz download fscan & masscan binary under the same directory
# https://github.com/shadow1ng/fscan/releases
# https://github.com/robertdavidgraham/masscan/releases
#
# Usage:
# $1  progress file, e.g: progress250508065524.txt
#
# author: icingfire
#################

con_flag=0
masscan_rate=3000

fscan_subnet() {
  if [ ${con_flag} -ne 0 ]; then
    grep -q "fscan $1" $progress_file
	if [ $? -eq 0 ]; then
	  echo "----- skip fscan $1 -----"
	  return
	fi
  fi
  echo "===== start fscan $1 ====="
  ./fscan -m icmp -p 6379 -h "$1" | tee -a ${fscan_file}
  echo "fscan $1" >> $progress_file
}


fscan_sub() {
  fscan_subnet 192.168.0.0/16
  
  for i in {16..31};
  do
    fscan_subnet 172.${i}.0.0/16
  done
  
  for i in {0..255};
  do
    fscan_subnet 10.${i}.0.0/16
  done
}

fscan_main() {
  fscan_sub
  fscan_sub
  grep ICMP ${fscan_file} | awk '{print $5}' | sort -u >> ${ips_tmp}
}


masscan_subnet() {
  if [ ${con_flag} -ne 0 ]; then
    grep -q "masscan $1" $progress_file
	if [ $? -eq 0 ]; then
	  echo "----- skip masscan $1 -----"
	  return
	fi
  fi
  echo "===== start masscan $1 ====="
  ./masscan -p80,443,22,8080,25,21,23,3389,445,53 --open "$1" --rate=${masscan_rate} >> ${masscan_file}
  echo "masscan $1" >> $progress_file
}

masscan_sub() {
  masscan_subnet 192.168.0.0/16
  
  for i in {16..31};
  do
    masscan_subnet 172.${i}.0.0/16
  done
  
  for i in {0..255};
  do
    masscan_subnet 10.${i}.0.0/16
  done
}

masscan_main() {
  masscan_sub
  awk '{print $NF}' ${masscan_file} | sort -u >> ${ips_tmp}
}


################ Main #################

cd $(dirname $0)

dir_name=$(date "+%y%m%d%H%M%S")
if [ -n "$1" ]; then
  if [ -d "$1" ]; then
    con_flag=1
    echo "continue from ${dir_name}"
  else
    echo "can not find $1"
    exit 1
  fi
fi

mkdir -p ${dir_name}
progress_file="${dir_name}/progress.txt"
fscan_file="${dir_name}/fscan.txt"
masscan_file="${dir_name}/masscan.txt"
ips_tmp="${dir_name}/ips_tmp.txt"
ips="${dir_name}/ips.txt"


# start scanning
fscan_main
masscan_main
sort -u ${ips_tmp} > ${ips}

echo "scan finished, please check ${ips}"
rm -f ${progress_file} ${ips_tmp}
