dir=/home/users/nianze/ssat/ssatABC
exp=${dir}/expSsat/ssatER/tiger
bin=${dir}/bin/ssat
bch=${exp}/ssat
log=${exp}/log/ssat_tiger.log
N=( 5 10 15 20 25 )
#N=25
tot=3600

echo "Profile DC-SSAT on tiger domain" > ${log}
#for ((t=1;t<=${N};t++))
for t in "${N[@]}"
do
   file=Tiger-${t}.ssat
   echo "  > On file ${file}" >> ${log}
   timeout ${tot} ${bin} ${bch}/${file} &>> ${log}
   RET="$?"
   if [[ "$RET" = "124" ]]
      then echo "timeout on ${file} , solve time: ${tot}" >> ${log}
   fi
done
