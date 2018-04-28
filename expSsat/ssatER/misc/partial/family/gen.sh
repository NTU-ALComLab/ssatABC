#!/bin/bash
for i in $(seq 11 40)
do
  ./gen3 $i family-$i
done
