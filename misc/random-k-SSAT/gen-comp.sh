#! /bin/bash
if [ "$#" -ne 5 ]; then
  echo "Usage: ./gen-comp.sh <k> <n> <m> <l> <p>"
  exit 1
fi
./testingScript/gen.sh $1 $2 $3 $4 $5
./comp.sh $1 $2 $3 $4
