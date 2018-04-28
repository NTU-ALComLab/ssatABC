#!/bin/bash
for i in $(seq 16 25)
do
  for j in $(seq 1 4)
  do
    ./greedy $i test-$i-$j $((i*j))
  done
done
