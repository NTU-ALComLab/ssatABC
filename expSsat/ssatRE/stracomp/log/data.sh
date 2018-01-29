dir=/home/users/nianze/ssat/ssatABC/expSsat/ssatRE/stracomp

if [ ${1} = 'log' ]; then
   bch=(`ls ${dir}/${2}CNF/sdimacs`)
   log1=${dir}/log/our_ran_${2}.log
   log2=${dir}/log/ssat_ran_${2}.log
   ourp=(`grep "Upper" ${log1} | awk '{print $5}'`)
   ourt=(`grep "Time" ${log1} | awk '{print $4}'`)
   dcsp=(`grep "Pr" ${log2} | awk '{print $3}'`)
   dcst=(`grep "solve time" ${log2} | awk '{print $4}'`)
   res=${dir}/log/result_${2}.log
   printf "==========     Comparing our method with DC-SSAT on random %s-CNF     ==========\n\n" "${1}" > ${res}
   printf "%30s   %12s   %12s   %12s   %12s\n" "benchmark" "our_p" "dcs_p" "our_t" "dcs_t" >> ${res}
   for i in {0..99}
   do
      printf "%30s   %12s   %12s   %12s   %12s\n" "${bch[i]}" "${ourp[i]}" "${dcsp[i]}" "${ourt[i]}" "${dcst[i]}" >> ${res}
   done
elif [ ${1} = 'csv' ]; then
   res=${dir}/log/excel.csv
   printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\n" "benchmark" "our_p" "SAT" "UNSAT" "dcs_p" "our_t" "dcs_t" > ${res}
   bch=(`cat ${dir}/bench_list`)
   log1=${dir}/log/our.log
   log2=${dir}/log/ssat.log
   ourp=(`grep "Upper" ${log1} | awk '{print $5}'`)
   satc=(`grep " SAT cubes"  ${log1} | awk '{print $6}'`)
   unsc=(`grep "UNSAT cubes" ${log1} | awk '{print $6}'`)
   ourt=(`grep "Time" ${log1} | awk '{print $4}'`)
   dcsp=(`grep "Pr" ${log2} | awk '{print $3}'`)
   dcst=(`grep "solve time" ${log2} | awk '{print $4}'`)
   for i in {0..59}
   do
      printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\n" "${bch[i]}" "${ourp[i]}" "${satc[i]}" "${unsc[i]}" "${dcsp[i]}" "${ourt[i]}" "${dcst[i]}" >> ${res}
   done
else
   echo "[Error] Unknown report type: ${2}"
fi
