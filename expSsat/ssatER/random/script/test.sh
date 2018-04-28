dir=/home/users/nianze/ssat/ssatABC
exp=${dir}/expSsat/ssatER/random
list=${exp}/bench.list
tot=1000

if [ ${1} = 'dc' ]; then
   bin=${dir}/bin/ssat
   bch=${exp}/4CNF/ssat
   log=${exp}/ssat.log
   echo "Profile DC-SSAT on random ER cases" > ${log}
   for file in `cat ${list}`
   do
      echo "On file ${file}.ssat" >> ${log}
      timeout ${tot} ${bin} ${bch}/${file}.ssat &>> ${log}
   done
elif [ ${1} = 'our' ]; then
   bin=${dir}/bin/abc
   bch=${exp}/4CNF/sdimacs
   log=${exp}/our.log
   cmd='ssat -v'
   echo "Profile our solver on random ER cases" > ${log}
   for file in `cat ${list}`
   do
      echo "On file ${file}.sdimacs" >> ${log}
      timeout ${tot} ${bin} -c " ${cmd} ${bch}/${file}.sdimacs" &>> ${log}
   done
else
   echo "[Error] Unknown solver type: ${1}"
fi
