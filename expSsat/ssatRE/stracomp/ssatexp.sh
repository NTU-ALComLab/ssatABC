dir=/home/users/nianze/ssat/ssatABC
bin=${dir}/bin/ssat_ai_qbf
bch=${dir}/expSsat/ssatRE/stracomp
log=${dir}/expSsat/ssatRE/stracomp/log/ssat.log
tot=1000

echo "[INFO] Profile DC-SSAT on strategic companies" > ${log}
for file in `cat ${bch}/bench_list`
do
   printf "\n  > On file ${file}.ssat\n" >> ${log}
   timeout ${tot} ${bin} ${bch}/ssat/${file}.ssat &>> ${log}
   RET="$?"
   if [[ "$RET" = "124" ]]
      then echo "[WARNING] Timeout on ${file}.ssat , time: ${tot}" >> ${log}
   fi
done
