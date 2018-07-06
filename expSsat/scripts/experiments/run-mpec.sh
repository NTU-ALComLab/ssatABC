#!/bin/bash
./run-erssat.sh "" MPEC expSsat/ssatER/MPEC/sdimacs &

./run-erssat.sh -pgs MPEC expSsat/ssatER/MPEC/sdimacs &

./run-dcssat.sh MPEC expSsat/ssatER/MPEC/ssat &

./run-c2d.sh MPEC expSsat/ssatER/MPEC/dimacs &
