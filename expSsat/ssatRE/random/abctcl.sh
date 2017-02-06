dir=/users/nianze/ssat/ssatABC/expSsat/ssatRE/random
k_arr=( 3 5 7 9 )
#bch=${dir}/3CNF/sdimacs
#tcl=${dir}/3cnf.tcl

for file in `ls ${dir}/5CNF/sdimacs`
do
   echo "ss -U 65536 -L 65536 ${dir}/5CNF/sdimacs/${file}" >> ${dir}/5cnf.tcl
done
: '
for i in "${k_arr[@]}"
do
   rm -f ${dir}/${i}cnf.tcl
   for file in `ls ${dir}/${i}CNF/sdimacs`
   do
      echo "ss ${dir}/${i}CNF/sdimacs/${file}" >> ${dir}/${i}cnf.tcl
   done
done
'
