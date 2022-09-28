#!/bin/bash
# set white ip list for security, leave ssh port in case that ip changed
#   by icingfire
###########################

iptables -F

SSH_PORT=22
white_list=(6.6.6.6 8.8.8.8)

iptables -A INPUT -p tcp --dport "${SSH_PORT}"  -j ACCEPT
for one in "${!white_list[@]}";
do
  iptables -A INPUT -p tcp -s "${white_list[${one}]}" -j ACCEPT
done
iptables -A INPUT -j DROP
