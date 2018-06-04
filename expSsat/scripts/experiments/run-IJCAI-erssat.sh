#!/bin/bash
./run-erssat.sh "" ToiletA expSsat/ssatER/planning/ToiletA/ &
./run-erssat.sh "" conformant expSsat/ssatER/planning/conformant/ &
./run-erssat.sh "" SandCastle expSsat/ssatER/planning/sand-castle/sdimacs/ &
./run-erssat.sh "" MPEC expSsat/ssatER/MPEC/sdimacs/ &
./run-erssat.sh "" MaxCount ../maxcount/sdimacs-bench/ &

./run-erssat.sh -pgs ToiletA expSsat/ssatER/planning/ToiletA/ &
./run-erssat.sh -pgs conformant expSsat/ssatER/planning/conformant/ &
./run-erssat.sh -pgs SandCastle expSsat/ssatER/planning/sand-castle/sdimacs/ &
./run-erssat.sh -pgs MPEC expSsat/ssatER/MPEC/sdimacs/ &
./run-erssat.sh -pgs MaxCount ../maxcount/sdimacs-bench/ &
