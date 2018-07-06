# SSAT Solver: Stochastic Boolean Satisfiability Solver

## Introduction
This is the C++ implemantation in the [Solving Stochastic Boolean Satisfiability under Random-Exist Quantification](https://www.ijcai.org/proceedings/2017/0096.pdf) and [Solving Exist-Random Quantified Stochastic Boolean Satisfiability via Clause Selection](#).

## Contents
1. [Installation](#installation)
2. [How to Run](#howtorun)
3. [Benchmarks](#benchmarks)

### Installation
Just type in `make` to complie and the output execution will be `bin/abc`
```
make
```
It has been tested successful under CentOS 7.3.1611 with GCC\_VERSION=4.8.5

### How to Run
Run command `ssat` in execution file `./bin/abc` can solve both RE-SSAT and ER-SSAT. `ssat -h` will show the detailed argument lists that can be passed into solver.
1. RE-SSAT
```
./bin/abc -c "ssat ./expSsat/ssatRE/random/3CNF/sdimacs/rand-3-40-120-20.165.sdimacs"
```

2. ER-SSAT
```
./bin/abc -c "ssat ./expSsat/ssatER/planning/ToiletA/sdimacs/toilet_a_02_01.2.sdimacs"
```

### Benchmarks
All the benchmarks are under `expSsat/` directory. There are two formats (sdimacs and ssat) for RE-SSAT and three formats (sdimacs, ssat and maxcount) for ER-SSAT.
