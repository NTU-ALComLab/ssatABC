import numpy as np
import sys

mp = {}
var_id = 1

with open(sys.argv[1], 'r') as f1:
    with open(sys.argv[2], 'w') as f2:
        for idx, line in enumerate(f1):
            if line[-1] == '\n':
                line = line[:-1]
            if line[0] == 'r':
                vals = line.split(' ')
                r_val = vals[1]
                r_vars = vals[2:]
                for var in r_vars:
                    if var != ' ' and var != '\n' and var != '' and var != '0':
                        f2.write('{} x{} R {}\n'.format(var_id, var_id, r_val))
                        var_id += 1
                        mp[var] = var_id
            elif line[0] == 'e':
                vals = line.split(' ')
                e_vars = vals[1:]
                for var in e_vars:
                    if var != ' ' and var != '\n' and var != '' and var != '0':
                        f2.write('{} x{} E\n'.format(var_id, var_id))
                        mp[var] = var_id
                        var_id += 1
            elif line[0] == 'p':
                var_num, cls_num = map(int, line[5:].split())
                print(var_num, cls_num)
                f2.write('%d\n' % (var_num))
                f2.write('%d\n' % (cls_num))
            elif line[0] != 'c':
                for var in line.split(' '):
                    if var != '0' and var != '':
                        f2.write(('-' if int(var) < 0 else '') + str(mp[str(abs(int(var)))]) + ' ')
                    elif var == '0':
                        f2.write('0')
                f2.write('\n')

print(mp)
