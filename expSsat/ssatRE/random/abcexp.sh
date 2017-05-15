dir=/home/users/nianze/ssat/ssatABC
bin=${dir}/bin/abc
bch=${dir}/expSsat/ssatRE/random
log=${dir}/expSsat/ssatRE/random/log
tot=1000
#k_arr=( 3 5 7 9 )
k_arr=( 3 4 5 6 7 8 9 )
#k_arr=( 3 )


for k in "${k_arr[@]}"
do
   echo "Profile our method on random ${k}CNF" > ${log}/our_ran_${k}.log
   for file in `ls ${bch}/${k}CNF/sdimacs`
   do
      echo "  > On file ${file}" >> ${log}/our_ran_${k}.log
      timeout ${tot} ${bin} -c "ssat -bm ${bch}/${k}CNF/sdimacs/${file}" &>> ${log}/our_ran_${k}.log
      RET="$?"
      if [[ "$RET" = "124" ]]
         then echo "timeout on ${file} , time: ${tot}" >> ${log}/our_ran_${k}.log
      fi
   done
done
