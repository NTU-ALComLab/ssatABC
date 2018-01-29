dir=/home/users/nianze/ssat/ssatABC
exp=${dir}/expSsat/ssatER/sand-castle
bin=${dir}/bin/ssat_to
bch=${exp}/ssat
log=${exp}/log
N=25
tot=180

#echo "Profile DC-SSAT on sand-castle domain" > ${log}/ssat_sand.log
for ((t=21;t<=${N};t++))
do
   file=SC-${t}.ssat
   res=${log}/${file}.log
   echo "  > On file ${file}" > ${res}
   timeout ${tot} ${bin} ${bch}/${file} &>> ${res} &
   RET="$?"
   if [[ "$RET" = "124" ]]
      then echo "timeout on ${file} , solve time: ${tot}" >> ${res}
   fi
done
