dir=/home/users/nianze/ssat/ssatABC
exp=${dir}/expSsat/ssatER/tiger
bin=${dir}/bin/abc
bch=${exp}/sdimacs
log=${exp}/log
N=( 5 10 15 20 25 )
#N=25
tot=3600

#for ((t=1;t<=${N};t++))
for t in "${N[@]}"
do
   file=Tiger-${t}.sdimacs
   echo "  > On file ${file}" > ${log}/${file}.our.log
   timeout ${tot} time ${bin} -c "ssat ${bch}/${file}" &>> ${log}/${file}.our.log
   RET="$?"
   if [[ "$RET" = "124" ]]
      then echo "timeout on ${file} , solve time: ${tot}" >> ${log}/${file}.our.log
   fi
done
