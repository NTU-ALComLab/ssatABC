home=`echo ${HOME}`
dir=${home}/ssat/ssatABC/expSsat/ssatER/random/CNFs
k_arr=( 3 4 5 6 7 8 9 )

for k in "${k_arr[@]}"
do
   for file in `ls ${dir}/${k}CNF/sdimacs`
   do
      vim -s script.vim ${dir}/${k}CNF/sdimacs/${file}
   done
done
