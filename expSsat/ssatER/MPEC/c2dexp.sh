dir=/home/users/nianze/ssat/ssatABC
bin=${dir}/bin/c2d_linux
bch=${dir}/expSsat/ssatER/MPEC/dimacs
log=${dir}/expSsat/ssatER/MPEC/c2d_log
declare -a ckt=("c432" "c499" "c880" "c1355" "c1908" "c3540" "c5315" "c7552")
tot=100

for file in "${ckt[@]}"
do
   rep=${log}/${file}_${tot}_0.rep
   echo "  > On file ${file}-er.dimacs" > ${rep}
   timeout ${tot} ${bin} -in ${bch}/${file}-er.dimacs -dt_method 0 -in_memory -count -smooth_all &>> ${rep}
   RET="$?"
   if [[ "$RET" = "124" ]]
      then echo "timeout on ${file} , solve time: ${tot}" >> ${rep}
   fi
done
