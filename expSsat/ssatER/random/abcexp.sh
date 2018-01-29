dir=/home/users/nianze/ssat/ssatABC
bin=${dir}/bin/abc
bch=${dir}/expSsat/ssatER/random
log=${dir}/expSsat/ssatER/random/log
tot=1000
k_arr=( 3 4 5 6 7 8 9 )
#k_arr=( 3 )

echo "Remember to add \$1 option !"
for k in "${k_arr[@]}"
do
   echo "Do k = ${k}"
   echo "Profile our method on random ${k}CNF" > ${log}/our_ran_${k}${1}.log
   for file in `ls ${bch}/${k}CNF/sdimacs`
   do
      number=`echo ${file} | cut -f2 -d"."`
      if (( number % 5 == 1 || number % 5 == 3 )); then
         echo "number: ${number}"
         echo "  > On file ${file}" >> ${log}/our_ran_${k}${1}.log
         timeout ${tot} ${bin} -c "ssat $1 ${bch}/${k}CNF/sdimacs/${file}" &>> ${log}/our_ran_${k}${1}.log
         RET="$?"
         if [[ "$RET" = "124" ]]
         then echo "timeout on ${file} , Time: ${tot}" >> ${log}/our_ran_${k}${1}.log
         fi
      fi
   done
done
