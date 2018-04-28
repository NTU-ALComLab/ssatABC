COUNT=0
echo "name, time, value" >> randomER-ssat.csv
for k in $(seq 3 9)
do
  TESTCASES=$(ls expSsat/ssatER/random/${k}CNF/ssat)
  for name in ${TESTCASES}
  do
     number=`echo ${name} | cut -f2 -d"."`
     if (( number % 5 == 1 || number % 5 == 3 )); then
        printf "%s, " ${name} >> randomER-ssat.csv
        timeout 1000 ./bin/ssat expSsat/ssatER/random/${k}CNF/ssat/${name} > tt1.log && cat tt1.log | grep "total time:\|Pr\[SAT\]" | awk 'NR==1{printf "%s, ", $4} NR==2{printf "%s\n", $3}' >> randomER-ssat.csv || echo "-, -" >> randomER-ssat.csv
        echo "case ${COUNT} done."
        COUNT=$((${COUNT} + 1))
     fi
  done
done
rm -f tt1.log
