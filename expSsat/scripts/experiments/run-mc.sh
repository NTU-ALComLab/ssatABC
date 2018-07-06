#!/bin/bash
if [[ "$#" -ne 3 ]]; then
  echo "usage: ./run-mc.sh <k> <log-name> <data-dir>";
  exit 0;
fi

COUNT=0
LOGFILE="${2}-maxcount-${1}.log"
CSVFILE="${2}-maxcount-${1}.csv"
echo "Start ${2}-maxcount-${1}" > ${LOGFILE}
echo "name, quality, answer, upper, lower, time" > ${CSVFILE}


TESTCASES=$(ls ${3}/*.maxcount)
for name in ${TESTCASES}
do
  printf "%s, " ${name} >> ${CSVFILE}
  printf "Run %s\n" ${name} >> ${LOGFILE}
  { timeout 1000 time -p python maxcount.py ${name} ${1} ; } >> ${LOGFILE} 2>&1 && tail -n 16 ${LOGFILE} | grep "Witness quality\|Estimated max\|Max-count is <=\|Max-count is >=\|Total runtime" | awk 'NR==1{printf "%s, ", $0} NR==2{printf "%s, ", $0} NR==3{printf "%s, ", $0} NR==4{printf "%s, ", $0} NR==5{printf "%s, ", $0}' >> ${CSVFILE} || echo "-, -, -, -, -" >> ${CSVFILE}
  echo "case ${COUNT} done."
  COUNT=$((${COUNT} + 1))
done
