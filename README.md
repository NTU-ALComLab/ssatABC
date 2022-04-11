# Stochastic Boolean Satisfiability (SSAT) Solver

## Introduction

This is a C++ implemantation of the algorithms proposed in [Solving Stochastic Boolean Satisfiability under Random-Exist Quantification](https://www.ijcai.org/proceedings/2017/0096.pdf) and [Solving Exist-Random Quantified Stochastic Boolean Satisfiability via Clause Selection](https://www.ijcai.org/proceedings/2018/0186.pdf) to solve random-exist SSAT (RE-SSAT) and exist-random SSAT (ER-SSAT) formulas, respectively.

## Installation

Type `make` to complie and the executable is `bin/abc`

```
make
```

It has been tested successfully under Ubuntu 20.04.1 with GCC_VERSION=9.3.0

## Execution

Run `./bin/abc` from your console and execute command `ssat` to solve both RE-SSAT and ER-SSAT formulas. `ssat -h` shows detailed arguments of the command.
You can also run the `ssat` command from the console, as the following examples show.

### RE-SSAT

```
./bin/abc -c "ssat" test/test-cases/ssatRE/random/3CNF/sdimacs/rand-3-40-120-20.165.sdimacs
```

### ER-SSAT

```
./bin/abc -c "ssat" test/test-cases/ssatER/planning/ToiletA/sdimacs/toilet_a_02_01.2.sdimacs
```

### Early Termination

Due to the high computational complexity of SSAT solving, early termination is supported. The solver can be stopped via signals `SIGINT` or `SIGTERM`, and the already-obtained results will still be output. This way, the spent resource will not be lost and a pair of upper and lower bounds of the exact answer can be derived.

## Benchmarks

All benchmarks are in directory `test/test-cases/`. There are two formats (sdimacs and ssat) for RE-SSAT and three formats (sdimacs, ssat, and maxcount) for ER-SSAT. A brief description for each file format and benchmark family is as follows.

### Format

#### sdimacs

The input file format for the implementation in this repository. It adapts the qdimacs format for quantified Boolean formulas and encodes a randomly quantified (with probability\* `p`) variable `x` as `r p x 0`. For example, the SSAT query `exist x1, exist x2, random p=0.5 x3, random p=0.5 x4. (x1 or x3) and (x2 != x4).` is encoded as follows.

```
p cnf 4 3
e 1 0
e 2 0
r 0.5 3 0
r 0.5 4 0
1 3 0
2 4 0
-2 -4 0
```

_Note: the current parsing implementaion only supports floating-point numbers up to six digits after the decimal point._

#### ssat

The input file format for the software implementing the algorithm proposed in "DC-SSAT: A Divide-and-conquer Approach to Solving Stochastic Satisfiability Problems Efficiently" by Stephen M. Majercik and Byron Boots. The SSAT query mentioned above is encoded as follows.

```
4
3
1 x1 E
2 x2 E
3 x3 R 0.5
4 x4 R 0.5
1 3 0
2 4 0
-2 -4 0
```

#### maxcount

The input file format for the software implementing the algorithm proposed in "Maximum Model Counting" by Daniel J. Fremont, Markus N. Rabe, and Sanjit A. Seshia. The SSAT query mentioned above can be converted to a maximum model counting query and encoded as follows.

```
c max 1 0
c max 2 0
c ind 3 0
c ind 4 0
p cnf 4 3
1 3 0
2 4 0
-2 -4 0
```

### Family

#### RE-SSAT (in directory `ssatRE`)

1. random: Randomly generated k-CNF formulas with a prefix where a half of variables are randomly quantified with probability 0.5, followed by the other half existentially quantified.
2. PEC: SSAT formulas encoding the probabilistic equivalence checking problem proposed in "Towards Formal Evaluation and Verification of Probabilistic Design" by Nian-Ze Lee and Jie-Hong R. Jiang.
3. stracomp: SSAT formulas generated by changing the universal quantifers to random quantifiers of the QBFs encoding the strategic company problem proposed in "Default Logic as a Query Language" by M. Cadoli, T. Eiter, and G. Gottlob.

#### ER-SSAT (in directory `ssatER`)

1. random: Randomly generated k-CNF formulas with a prefix where a half of variables are existentially quantified, followed by the other half randomly quantified with probability 0.5.
2. MPEC: SSAT formulas encoding the maximum probabilistic equivalence checking problem proposed in "Towards Formal Evaluation and Verification of Probabilistic Design" by Nian-Ze Lee and Jie-Hong R. Jiang.
3. planning: SSAT formulas generated by changing the universal quantifers to random quantifiers of the QBFs encoding various planning problems in QBFLIB.
4. MaxCount: SSAT formulas generated by converting the benchmarks used in "Maximum Model Counting" by Daniel J. Fremont, Markus N. Rabe, and Sanjit A. Seshia.

## Miscellany

This section lists some tasks that 1) this repository can also perform but 2) are not directly related to SSAT solving.

### Approximate Circuit Analysis

The command `bddsp` can be used for approxiamte circuit analysis. It can compute the error probability, i.e., the probability for an approximate circuit to behave differently from its golden version, under the assumption that primary input patterns follow a uniform distribution (the option `-u`).

Example usages are given below.

#### Example 1

To compute the difference probability between a 3-input AND gate and a 3-input OR gate:

```
UC Berkeley, ABC 1.01 (compiled Dec 17 2020 22:32:32)
abc 01> miter test/test-cases/probverify/and3.blif test/test-cases/probverify/or3.blif
abc 02> bddsp -u
  > Pb_BddComputeSp() : build bdd for 0-th Po
  > Bdd construction =     0.01 sec
  > Prob computation =     0.00 sec
  > 0-th Po , prob = 7.500000e-01
abc 02> quit
```

#### Example 2

Note that the miter can be simplified (here by command `dc2`) before the analysis to enhance the efficiency.

```
UC Berkeley, ABC 1.01 (compiled Dec 17 2020 22:32:32)
abc 01> miter test/test-cases/probverify/c17
c17-approx.bench  c17.bench
abc 01> miter test/test-cases/probverify/c17.bench test/test-cases/probverify/c17-approx.bench
abc 02> ps
test/test-cases/probverify/c17_test/test-cases/probverify/c17-approx_miter: i/o =    5/    1  lat =    0  and =     14  lev =  6
abc 02> dc2
abc 03> ps
test/test-cases/probverify/c17_test/test-cases/probverify/c17-approx_miter: i/o =    5/    1  lat =    0  and =      4  lev =  3
abc 03> bddsp -u
  > Pb_BddComputeSp() : build bdd for 0-th Po
  > Bdd construction =     0.01 sec
  > Prob computation =     0.00 sec
  > 0-th Po , prob = 9.687500e-01
abc 03> quit
```

## Suggestions, Questions, Bugs, etc

You are welcome to [create an issue](https://github.com/nianzelee/ssatABC/issues) to make suggestions, ask questions, or report bugs, etc.

## Contact

Nian-Ze Lee: nian-ze.lee@sosy.ifi.lmu.de
