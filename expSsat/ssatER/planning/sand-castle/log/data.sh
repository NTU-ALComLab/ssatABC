dir=/home/users/nianze/ssat/ssatABC
exp=${dir}/expSsat/ssatER/sand-castle
log=${exp}/log
N=25

for ((t=1;t<=${N};t++))
do
   file=SC-${t}.sdimacs.our.log
   grep "system" ${log}/${file} | awk '{print $1 $2}'
done
