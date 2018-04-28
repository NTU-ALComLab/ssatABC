#!/bin/sh
DIR=`echo ${HOME}`
SSAT=ssat/ssatABC
FILEPATH=expSsat/ssatER/random/log
KCNF=(3 4 5 6 7 8 9)
OUTPUTFILE=random_with${1}.csv

# $1 is flag
# Start !
echo "Start grep.sh, generate to ${OUTPUTFILE}"
echo `cat ${DIR}/${SSAT}/${FILEPATH}/our_ran_3${1}.log | grep "Using.*for counting" | awk 'NR==1{print $0}'` > ${OUTPUTFILE}
echo ",Name, Answer, Counting Time, Build Sop Time, Strash Time, Build Bdd Time, Total Time" >> ${OUTPUTFILE}

for k in "${KCNF[@]}"
do
  cat ${DIR}/${SSAT}/${FILEPATH}/our_ran_${k}${1}.log | grep "On file\|Lower\|on counting\|build  sop\|strash sop\|build  bdd\|Total elapsed time" | awk 'NR%7==1{printf ",%s, ", $4} 
    NR%7==2{
       if($6=="") printf "%s, ", $5
       else printf "%s, ", $6
    } 
    NR%7==3{printf "%s, ", $7} 
    NR%7==4{printf "%s, ", $8} 
    NR%7==5{printf "%s, ", $8} 
    NR%7==6{printf "%s, ", $8} 
    NR%7==0{printf "%s\n", $6}' >> ${OUTPUTFILE}
done
