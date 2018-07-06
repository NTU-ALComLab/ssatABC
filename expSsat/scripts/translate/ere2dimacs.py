import sys

def er2dimacs(src, dest):
    with open(src, 'r') as s:
        with open(dest, 'w') as d:
            for line in s:
                if (line[0] != 'e' and line[0] != 'r' and line[0] != 'a'):
                    d.write(line)

if __name__ == '__main__':
    er2dimacs(sys.argv[1], sys.argv[2])
