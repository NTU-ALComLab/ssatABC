dir=/home/users/nianze/ssat/ssatABC
bin=${dir}/bin/abc
bch=${dir}/expSsat/ssatER/MPEC/sdimacs
log=${dir}/expSsat/ssatER/MPEC/our_log
declare -a ckt=("c432" "c499" "c880" "c1355" "c1908" "c3540" "c5315" "c7552")
tot=1000

for file in "${ckt[@]}"
do
   rep=${log}/${file}_${tot}.rep
   echo "  > On file ${file}-er.sdimacs" > ${rep}
   timeout ${tot} ${bin} -c "ssat ${bch}/${file}-er.sdimacs" &>> ${rep}
   RET="$?"
   if [[ "$RET" = "124" ]]
      then echo "timeout on ${file} , solve time: ${tot}" >> ${rep}
   fi
done
