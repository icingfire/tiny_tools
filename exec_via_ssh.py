import paramiko
import sys
import time
######################
#while read one_line
#do
#        cmd="proxychains python ssh_cmd.py ${one_line}"
#        echo "----- ${cmd} -----"
#        eval "${cmd}"
#done < ./ip_user_pass.txt
######################


def ssh_send_recv(ssh_connect, cmd):
	ssh_connect.send(cmd + "\n")
	while True:
		try:
			recv = shell.recv(2048).decode()
			if recv:
				print(recv)
			else:
				continue
		except:
			return 
	return 

############### main ###
host = sys.argv[1]
username = sys.argv[2]
passwd = sys.argv[3]

ssh = paramiko.SSHClient()
know_host = paramiko.AutoAddPolicy()
ssh.set_missing_host_key_policy(know_host)

ssh.connect(
    hostname = host,
    port = 22,
    username = username,
    password = passwd
)


if username != "root":
	shell = ssh.invoke_shell()
	shell.settimeout(1)
	shell.recv(2048)
	shell.send("su - root\n")
	time.sleep(0.2)
	shell.send("password\n")
	time.sleep(0.2)
	print(shell.recv(512).decode())
	ssh_send_recv(shell, "netstat -ltnp\n")
	ssh_send_recv(shell, "netstat -tnp\n")
else:
	stdin,stdout,stderr = ssh.exec_command("netstat -ltnp")
	print(stdout.readlines())
	stdin,stdout,stderr = ssh.exec_command("netstat -tnp")
	print(stdout.readlines())

ssh.close()
