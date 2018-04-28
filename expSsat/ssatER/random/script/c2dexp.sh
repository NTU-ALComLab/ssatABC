home=`echo ${HOME}`
dir=${home}/ssat/ssatABC
bin=${dir}/bin/c2d_linux
bch=${dir}/expSsat/ssatER/random/CNFs
log=${dir}/expSsat/ssatER/random/log
tot=100
k_arr=( 3 4 5 6 7 8 9 )

for k in "${k_arr[@]}"
do
   echo "Do k = ${k}"
   echo "Profile c2d compiler on random ${k}CNF" > ${log}/c2d_ran_${k}.log
   for file in `ls ${bch}/${k}CNF/sdimacs`
   do
      number=`echo ${file} | cut -f2 -d"."`
      if (( number % 5 == 1 || number % 5 == 3 )); then
         echo "number: ${number}"
         echo "  > On file ${file}" >> ${log}/c2d_ran_${k}.log
         timeout ${tot} ${bin} -in ${bch}/${k}CNF/sdimacs/${file} -dt_method 0 -in_memory -count -smooth_all &>> ${log}/c2d_ran_${k}.log
         RET="$?"
         if [[ "$RET" = "124" ]]
         then echo "timeout on ${file} , Total Time: ${tot}" >> ${log}/c2d_ran_${k}.log
         fi
      fi
   done
done
