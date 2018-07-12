import numpy as np
import sys

var_num = 0
cla_num = 0
term = 'N'
e_vars = []

with open(sys.argv[1], 'r') as f1:
    with open(sys.argv[2], 'w') as f2:
        f2.write('c sdimacs converted from {}\n'.format(sys.argv[1]))
        lines = iter(f1)
        var_num = int(next(lines).strip())
        cla_num = int(next(lines).strip())
        f2.write('p cnf {} {}\n'.format(var_num, cla_num))
        for i in range(var_num):
            line = next(lines).strip()
            args = line.split(' ')
            if args[2] == 'E':
                term = 'E'
                e_vars.append(args[0])
            elif args[2] == 'R':
                if term == 'E':
                    f2.write('e ' + ' '.join(e_vars) + ' 0\n')
                    e_vars = []
                term = 'R'
                f2.write('r ' + args[3]+ ' ' + args[0] + ' 0\n')
        if term == 'E':
            f2.write('e ' + ' '.join(e_vars) + ' 0\n')
            e_vars = []
        for i in range(cla_num):
            line = next(lines)
            f2.write(line)
