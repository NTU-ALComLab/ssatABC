#!/bin/bash
if [[ "$#" -ne 3 ]]; then
  echo "usage: ./run-er-one.sh <flag> <log-name> <testcase>";
  exit 0;
fi

LOGFILE="${2}-sdimacs${1}.log"
CSVFILE="${2}-sdimacs${1}.csv"
echo "Start ${2}-sdimacs${1}" > ${LOGFILE}
echo "name, value, abc time, #call, len(base), len(sub), len(par), len(drop), real time" > ${CSVFILE}

name=${3}
printf "%s, " ${name} >> ${CSVFILE}
printf "Run %s\n" ${name} >> ${LOGFILE}
{ time -p ./bin/abc -c "ssat ${1} ${name}" ; } >> ${LOGFILE} 2>&1 && tail -n 32 ${LOGFILE} | grep "Lower\|calls to\|Average\|Time  \|real " | awk 'NR==1{printf "%s, ", $5} NR==2{printf "%s, ", $4} NR==3{printf "%s, ", $9} NR==4{printf "%s, ", $8} NR==5{printf "%s, ", $8} NR==6{printf "%s, ", $8} NR==7{printf "%s, ", $8} NR==8{printf "%s\n", $2}' >> ${CSVFILE} || echo "-, -, -, -, -, -, -, -" >> ${CSVFILE}
