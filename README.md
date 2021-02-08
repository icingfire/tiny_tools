# tiny_tools
a collection of small tools

## scan.sh
2 stage scan, 1xx ports by nmap, 6xxxx ports by masscan

**usage:**

scan.sh 119.118.117.116

scan.sh ip.txt

ip.txt contains simple ips, it does not support format like: 119.118.117.1-220 or 119.118.117.0/24

ip.txt e.g:
```
119.118.117.116
119.118.117.110
```

## domain2ip.sh
Convert domain name to ip address.
```
bash domain2ip.sh domains.txt | tee -a ips.txt
```

