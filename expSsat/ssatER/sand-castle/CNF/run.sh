#!/bin/bash
echo "Start." > log/c2d.log
for file in $(ls *.cnf)
do
  timeout 1000 ./c2d_linux -in ${file} -dt_method 0 -in_memory -count -smooth_all >> log/c2d.log
done
