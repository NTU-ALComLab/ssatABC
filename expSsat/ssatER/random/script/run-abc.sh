COUNT=0
LOGFILE="randomER-sdimacs${1}.log"
CSVFILE="randomER-sdimacs${1}.csv"
echo "Start randomER-sdimacs${1}" > ${LOGFILE}
echo "name, value, abc time, #call, real time" > ${CSVFILE}

for k in $(seq 3 9)
do
  TESTCASES=$(ls ~/ssatABC/expSsat/ssatER/random/${k}CNF/sdimacs)
  for name in ${TESTCASES}
  do
    printf "%s, " ${name} >> ${CSVFILE}
    { timeout 1000 time -p ./bin/abc -c "ssat ${1} expSsat/ssatER/random/${k}CNF/sdimacs/${name}" ; } >> ${LOGFILE} 2>&1 && tail -n 32 ${LOGFILE} | grep "calls\|Lower\|real\|Total elapsed time" | awk 'NR==1{printf "%s, ", $5} NR==2{printf "%s, ", $6} NR==3{printf "%s, ", $9} NR==4{printf "%s\n", $2}' >> ${CSVFILE} || echo "-, -, -, -" >> ${CSVFILE}
    # { timeout 1000 time -p ./bin/abc -c "ssat ${1} expSsat/ssatER/random/${k}CNF/sdimacs/${name}" ; } >> $LOGFILE 2>&1 && tail -n 30 $LOGFILE | grep "calls\|Lower\|real\|Time" | awk 'NR==1{printf "%d, ", $7} NR==2{printf "%s, ", $5} NR==3{printf "%s, ", $4} NR==4{printf "%s\n", $2}' >> randomER-sdimacs.csv || echo "-, -, -, -" >> randomER-sdimacs.csv
  echo "case ${COUNT} done."
  COUNT=$((${COUNT} + 1))
  done
done
