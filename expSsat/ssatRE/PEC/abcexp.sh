dir=/home/users/nianze/ssat/ssatABC
bin=${dir}/bin/abc
bch=${dir}/expSsat/ssatRE/PEC/old_sdimacs
log=${dir}/expSsat/ssatRE/PEC/log
declare -a ckt=("c432" "c499" "c880" "c1355" "c1908" "c3540" "c5315" "c7552")
tot=60

for file in "${ckt[@]}"
do
   rep=${log}/${file}.rep
   echo "  > On file ${file}.sdimacs" > ${rep}
   timeout ${tot} ${bin} -c "ssat ${bch}/${file}.sdimacs" &>> ${rep}
   RET="$?"
   if [[ "$RET" = "124" ]]
      then echo "timeout on ${file} , solve time: ${tot}" >> ${rep}
   fi
done
