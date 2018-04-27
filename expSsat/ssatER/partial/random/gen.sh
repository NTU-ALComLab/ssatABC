#!/bin/bash
for i in $(seq 16 35)
do
  for j in $(seq 1 2)
  do
    ./gen22 $i test-$i-$j $((i*j))
  done
done
