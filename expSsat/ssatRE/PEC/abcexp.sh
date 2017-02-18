dir=/home/users/nianze/ssat/ssatABC
bin=${dir}/bin/abc
bch=${dir}/expSsat/ssatRE/PEC/sdimacs
log=${dir}/expSsat/ssatRE/PEC/log
declare -a ckt=("c432" "c499" "c880" "c1355" "c1908" "c3540" "c5315" "c7552")
tot=7200

for file in "${ckt[@]}"
do
   rep=${log}/${file}_re.rep
   echo "  > On file ${file}_re.sdimacs" > ${rep}
   timeout ${tot} ${bin} -c "ssat ${bch}/${file}_re.sdimacs" &>> ${rep} &
   RET="$?"
   if [[ "$RET" = "124" ]]
      then echo "timeout on ${file}_re , solve time: ${tot}" >> ${rep}
   fi
done
