COUNT=0
echo "name, number, value, time" >> randomER-sdimacs.csv
for k in $(seq 3 9)
do
  TESTCASES=$(ls expSsat/ssatER/random/${k}CNF/sdimacs)
  for name in ${TESTCASES}
  do
    printf "%s, " ${name} >> randomER-sdimacs.csv
    { timeout 1000 time -p ./bin/abc -c "ssat expSsat/ssatER/random/${k}CNF/sdimacs/${name}" ; } > tt2.log 2>&1 && cat tt2.log | grep "calls\|Lower\|user" | awk 'NR==1{printf "%d, ", $7} NR==2{printf "%s, ", $5} NR==3{printf "%s\n", $2}' >> randomER-sdimacs.csv || echo "-, -, -" >> randomER-sdimacs.csv
  echo "case ${COUNT} done."
  COUNT=$((${COUNT} + 1))
  done
done
rm -f tt2.log
