dir=/home/users/nianze/ssat/ssatABC
bin=${dir}/bin/abc
bch=${dir}/expSsat/ssatRE/stracomp
log=${dir}/expSsat/ssatRE/stracomp/log/our.log
tot=1000


echo "[INFO] Profile our method on strategic companies" > ${log}
for file in `cat ${bch}/bench_list`
do
   printf "\n  > On file ${file}.sdimacs\n" >> ${log}
   timeout ${tot} ${bin} -c "ssat -bm ${bch}/sdimacs/${file}.sdimacs" &>> ${log}
   RET="$?"
   if [[ "$RET" = "124" ]]
      then echo "[WARNING] Timeout on ${file}.sdimacs , time: ${tot}" >> ${log}
   fi
done
