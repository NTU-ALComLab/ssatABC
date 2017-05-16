COUNT=0
LOGFILE=randomER-sdimacs${1}.csv
echo "Start." > ${LOGFILE}
for k in $(seq 3 9)
do
  TESTCASES=$(ls ~/ssatABC/expSsat/ssatER/random/${k}CNF/sdimacs)
  for name in ${TESTCASES}
  do
    number=`echo ${name} | cut -f2 -d"."`
    if (( number % 5 == 1 || number % 5 == 3 )); then
       printf "%s, " ${name} >> ${LOGFILE}
       timeout 1000 ~/ssatABC/bin/abc -c "ssat ${1} /home/users/danny/ssatABC/expSsat/ssatER/random/${k}CNF/sdimacs/${name}" >> ${LOGFILE}
       echo "case ${COUNT} done."
       COUNT=$((${COUNT} + 1))
    fi
  done
done
