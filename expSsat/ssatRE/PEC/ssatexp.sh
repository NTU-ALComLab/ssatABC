dir=/home/users/danny/ssatABC
bin=${dir}/bin/ssat
bch=${dir}/expSsat/ssatRE/PEC/ssat
log=${dir}/expSsat/ssatRE/PEC/log
declare -a ckt=("c432" "c499" "c880" "c1355" "c1908" "c3540" "c5315" "c7552")
tot=1000

#echo "Test DC-SSAT on PEC domain" > ${log}
for file in "${ckt[@]}"
do
   rep=${log}/${file}_ssat_re.rep
   echo "  > On file ${file}_re.ssat" > ${rep}
   timeout ${tot} ${bin} ${bch}/${file}_re.ssat &>> ${rep} &
   RET="$?"
   if [[ "$RET" = "124" ]]
      then echo "timeout on ${file}_re , solve time: ${tot}" >> ${rep}
   fi
done
