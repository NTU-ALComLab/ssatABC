dir=/users/nianze/ssat/ssatABC
bin=${dir}/bin/ssat_exp
bch=${dir}/expSsat/ssatRE/random
log=${dir}/expSsat/ssatRE/random/log
tot=1000
#k_arr=( 3 5 7 9 )
k_arr=( 4 6 8 )


for k in "${k_arr[@]}"
do
   echo "Test DC-SSAT on random ${k}CNF" > ${log}/ssat_ran_${k}.log
   for file in `ls ${bch}/${k}CNF/ssat`
   do
      echo "  > On file ${file}" >> ${log}/ssat_ran_${k}.log
      timeout ${tot} ${bin} ${bch}/${k}CNF/ssat/${file} &>> ${log}/ssat_ran_${k}.log
      RET="$?"
      if [[ "$RET" = "124" ]]
         then echo "timeout on ${file} , solve time: ${tot}" >> ${log}/ssat_ran_${k}.log
      fi
   done
done
