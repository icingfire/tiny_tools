import sys

arg_count = len(sys.argv)
if arg_count < 3:
    print("Usage: $0 file1 file2 file3 ...  \n    result: content_file1 - file2 - file3")

file2_lines = []
for i in range(2, arg_count):
    with open(sys.argv[i]) as f_h:
        for one in f_h:
            t_line = one.strip()
            if t_line != "":
                file2_lines.append(t_line)

diff_lines = []
with open(sys.argv[1]) as f_h:
    for two in f_h:
        t_line = two.strip()
        if t_line != "" and t_line not in file2_lines:
            diff_lines.append(t_line)

for one in diff_lines:
    print(one)
