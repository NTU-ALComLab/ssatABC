#!/bin/bash
for i in $(seq 8 18)
do
  double=$((2*i))
  for j in $(seq 1 4)
  do
    ./subsume ${double} test-${double}-$j $((i*j))
  done
done
