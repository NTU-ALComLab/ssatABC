#!/bin/bash
if [[ "$#" -ne 2 ]]; then
  echo "usage: ./run-c2d.sh <log-name> <data-dir>";
  exit 0;
fi

COUNT=0
LOGFILE="${1}-c2d.log"
CSVFILE="${1}-c2d.csv"
echo "Start ${1}-c2d" > ${LOGFILE}
echo "name, time" > ${CSVFILE}

TESTCASES=$(ls ${2})
for name in ${TESTCASES}
do
  printf "%s, " ${name} >> ${CSVFILE}
  printf "Run %s\n" ${name} >> ${LOGFILE}
  timeout 1000 ./bin/c2d_linux -in ${2}/${name} -dt_method 0 -in_memory -count -smooth_all >> ${LOGFILE} && tail -n 8 ${LOGFILE} | grep "Total Time" | awk 'NR==1{printf "%s\n", $3}' >> ${CSVFILE} || echo "-" >> ${CSVFILE}
  echo "case ${COUNT} done."
  COUNT=$((${COUNT} + 1))
done
