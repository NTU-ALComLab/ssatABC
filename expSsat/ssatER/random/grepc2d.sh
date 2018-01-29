#!/bin/sh
DIR=`echo ${HOME}`
SSAT=ssat/ssatABC
FILEPATH=expSsat/ssatER/random/log
KCNF=(3 4 5 6 7 8 9)
OUTPUTFILE=c2d_result.csv

printf "%s\t%s\n" "File" "Total Time" > ${OUTPUTFILE}
for k in "${KCNF[@]}"
do
  grep "On file\|Total Time" ${DIR}/${SSAT}/${FILEPATH}/c2d_ran_${k}.log | awk 'NR%2==1{printf "%s\t" ,$4} NR%2==0{printf "%s\n" ,$3}' >> ${OUTPUTFILE}
done

