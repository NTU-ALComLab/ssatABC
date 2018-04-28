dir=/home/users/nianze/ssat/ssatABC
exp=${dir}/expSsat/ssatER/sand-castle
bin=${dir}/bin/abc
bch=${exp}/blif
log=${exp}/log/abccb_sand.log
N=25
tot=3600


echo "Profile command cb on sand-castle domain" > ${log}
for ((t=1;t<=${N};t++))
do
   file=SC-${t}_s.blif
   echo $'\n' "> On file ${file}" >> ${log}
   timeout ${tot} ${bin} -c "cktbddsp ${bch}/${file}" &>> ${log}
   RET="$?"
   if [[ "$RET" = "124" ]]
      then echo "timeout on ${file} , solve time: ${tot}" >> ${log}
   fi
done
