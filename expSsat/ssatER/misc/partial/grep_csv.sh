#!/bin/bash
if [[ "${3}" == "ssat" ]]; then
cat ${1}/log/${2}-${3}.log | grep "Run\|total time" | awk '$1=="Run"{printf "\n%s, ", $2} $1!="Run"{printf "%s,", $4}' | grep " " | tee csv/${1}-${3}.csv
else
cat ${1}/log/${2}-${3}.log  | grep "Run \|time     " | awk '$1=="Run"{printf "%s, ", $2} $1!="Run"{printf "%s, \n", $6}' | tee csv/${1}-${3}.csv
fi
