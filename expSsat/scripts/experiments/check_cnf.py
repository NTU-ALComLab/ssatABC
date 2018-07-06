import numpy as np
import sys

eVars = []
rVars = []
e2Vars = []
var_num = None
cls_num = None
start = True
beforeInd = True

with open(sys.argv[1], 'r') as f1:
    with open(sys.argv[2], 'a') as f2:
        # f2.write('name, #clause, #variable, #E, #R, #E\n')
        for idx, line in enumerate(f1):
            if line[0] not in 'cpera':
                f2.write('%s, %d, %d, %d, %d, %d\n' %
                         (sys.argv[1].split('/')[-1], cls_num, var_num, len(eVars), len(rVars), len(e2Vars)))
                if len(eVars) + len(rVars) + len(e2Vars) != var_num:
                    print('Number of variables is inconsistent!')
                    print('File: ', sys.argv[1])
                    print(len(eVars), len(rVars), len(e2Vars), var_num)
                break

            if line[-1] == '\n':
                line = line[:-1]

            if line[:5] == 'p cnf':
                var_num, cls_num = map(int, line[5:].split())
            elif line[0] == 'e':
                # print('ELINE: ', line)
                if beforeInd:
                    eVars += [ ch for ch in line.split(' ') if ch.isdigit() and int(ch) != 0]
                else:
                    e2Vars += [ ch for ch in line.split(' ') if ch.isdigit() and int(ch) != 0]
            elif line[0] == 'r':
                beforeInd = False
                # print('RLINE: ', line)
                rVars += [ ch for ch in line.split(' ') if ch.isdigit() and int(ch) != 0]
