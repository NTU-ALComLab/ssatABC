dir=/home/users/nianze/ssat/ssatABC
exp=${dir}/expSsat/ssatER/sand-castle
bin=${dir}/bin/abc
bch=${exp}/sdimacs
log=${exp}/log
N=25
tot=3600

for ((t=25;t<=${N};t++))
do
   file=SC-${t}.sdimacs
   echo "  > On file ${file}" > ${log}/${file}.our.log
   timeout ${tot} time ${bin} -c "ssat ${bch}/${file}" &>> ${log}/${file}.our.log
   RET="$?"
   if [[ "$RET" = "124" ]]
      then echo "timeout on ${file} , solve time: ${tot}" >> ${log}/${file}.our.log
   fi
done
