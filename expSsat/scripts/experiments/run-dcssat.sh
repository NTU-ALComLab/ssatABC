#!/bin/bash
if [[ "$#" -ne 2 ]]; then
  echo "usage: ./run-dcssat.sh <log-name> <data-dir>";
  exit 0;
fi

COUNT=0
LOGFILE="${1}-ssat.log"
CSVFILE="${1}-ssat.csv"
echo "Start ${1}-ssat" > ${LOGFILE}
echo "name, time, value" > ${CSVFILE}

TESTCASES=$(ls ${2})
for name in ${TESTCASES}
do
  printf "%s, " ${name} >> ${CSVFILE}
  printf "Run %s\n" ${name} >> ${LOGFILE}
  timeout 1000 ./bin/ssat ${2}/${name} >> ${LOGFILE} && tail -n 8 ${LOGFILE} | grep "total time:\|Pr\[SAT\]" | awk 'NR==1{printf "%s, ", $4} NR==2{printf "%s\n", $3}' >> ${CSVFILE} || echo "-, -" >> ${CSVFILE}
  echo "case ${COUNT} done."
  COUNT=$((${COUNT} + 1))
done
