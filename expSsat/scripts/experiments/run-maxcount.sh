#!/bin/bash
./run-erssat.sh "" MaxCount ../maxcount/sdimacs-bench/ &

./run-erssat.sh -pgs MaxCount ../maxcount/sdimacs-bench/ &

./run-dcssat.sh MaxCount ../maxcount/ssat-bench/ &

./run-c2d.sh MaxCount ../maxcount/dimacs-bench/ &
