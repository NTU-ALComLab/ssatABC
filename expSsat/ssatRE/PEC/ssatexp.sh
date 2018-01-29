dir=/home/users/nianze/ssat/ssatABC
bin=${dir}/bin/ssat
bch=${dir}/expSsat/ssatRE/PEC/old_ssat
log=${dir}/expSsat/ssatRE/PEC/log
declare -a ckt=("c432" "c499" "c880" "c1355" "c1908" "c3540" "c5315" "c7552")
tot=1000

#echo "Test DC-SSAT on PEC domain" > ${log}
for file in "${ckt[@]}"
do
   rep=${log}/${file}_ssat.rep
   echo "  > On file ${file}.ssat" > ${rep}
   timeout ${tot} ${bin} ${bch}/${file}.ssat &>> ${rep}
   RET="$?"
   if [[ "$RET" = "124" ]]
      then echo "timeout on ${file} , solve time: ${tot}" >> ${rep}
   fi
done
