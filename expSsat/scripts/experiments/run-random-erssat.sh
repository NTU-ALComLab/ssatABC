COUNT=0
LOGFILE="randomER-sdimacs${1}.log"
CSVFILE="randomER-sdimacs${1}.csv"
echo "Start randomER-sdimacs${1}" > ${LOGFILE}
echo "name, value, abc time, #call, len(base), len(sub), len(par), len(drop), real time" > ${CSVFILE}

for k in $(seq 3 9)
do
  TESTCASES=$(ls ~/ssatABC/expSsat/ssatER/random/${k}CNF/sdimacs)
  for name in ${TESTCASES}
  do
    number=`echo ${name} | cut -f2 -d"."`
    if (( number % 5 == 1 || number % 5 == 3 )); then
      printf "%s, " ${name} >> ${CSVFILE}
      { timeout 1000 time -p ./bin/abc -c "ssat ${1} expSsat/ssatER/random/${k}CNF/sdimacs/${name}" ; } >> ${LOGFILE} 2>&1 && tail -n 32 ${LOGFILE} | grep "Lower\|calls to\|Average\|Time  \|real " | awk 'NR==1{printf "%s, ", $5} NR==2{printf "%s, ", $4} NR==3{printf "%s, ", $9} NR==4{printf "%s, ", $8} NR==5{printf "%s, ", $8} NR==6{printf "%s, ", $8} NR==7{printf "%s, ", $8} NR==8{printf "%s\n", $2}' >> ${CSVFILE} || echo "-, -, -, -, -, -, -, -" >> ${CSVFILE}
      echo "case ${COUNT} done."
      COUNT=$((${COUNT} + 1))
    fi
  done
done
