COUNT=0
for k in $(seq 3 9)
do
  TESTCASES=$(ls expSsat/ssatER/random/${k}CNF/sdimacs)
  echo "name, number, value, time" >> randomER-sdimacs.csv
  for name in ${TESTCASES}
  do
    printf "%s, " ${name} >> randomER-sdimacs.csv
    timeout 1000 ./bin/abc -c "ssat expSsat/ssatER/random/${k}CNF/sdimacs/${name}" > tt.log && cat tt.log | grep "calls\|Lower\|Time" | awk 'NR==1{printf "%d, ", $7} NR==2{printf "%s, ", $5} NR==3{printf "%s\n", $4}' >> randomER-sdimacs.csv || echo "-, -, -" >> randomER-sdimacs.csv
  echo "case ${COUNT} done."
  COUNT=$((${COUNT} + 1))
  done
done
rm -f tt.log
