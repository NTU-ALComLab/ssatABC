#!/bin/bash
if [[ "$#" -ne 3 ]]; then
  echo "usage: ./run-erssat.sh <flag> <log-name> <data-dir>";
  exit 0;
fi

COUNT=0
LOGFILE="${2}-sdimacs${1}.log"
CSVFILE="${2}-sdimacs${1}.csv"
echo "Start ${2}-sdimacs${1}" > ${LOGFILE}
echo "name, value, abc time, #call, len(base), len(sub), len(par), len(drop), real time" > ${CSVFILE}

TESTCASES=$(ls ${3})
for name in ${TESTCASES}
do
  printf "%s, " ${name} >> ${CSVFILE}
  printf "Run %s\n" ${name} >> ${LOGFILE}
  { timeout 1000 time -p ./bin/abc -c "ssat ${1} ${3}/${name}" ; } >> ${LOGFILE} 2>&1 && tail -n 32 ${LOGFILE} | grep "Lower\|calls to\|Average\|Time  \|real " | awk 'NR==1{printf "%s, ", $5} NR==2{printf "%s, ", $4} NR==3{printf "%s, ", $9} NR==4{printf "%s, ", $8} NR==5{printf "%s, ", $8} NR==6{printf "%s, ", $8} NR==7{printf "%s, ", $8} NR==8{printf "%s\n", $2}' >> ${CSVFILE} || echo "-, -, -, -, -, -, -, -" >> ${CSVFILE}
  echo "case ${COUNT} done."
  COUNT=$((${COUNT} + 1))
done
