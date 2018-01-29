#!/bin/bash
DIR=/home/users/danny/ssatABC/bin
CURDIR="/home/users/danny/generate/family"
ABCPROGRAM=${DIR}/abc
LOGFILE=log/family-sdimacs${1}.log
TIMEOUT=1000
echo "Start." > ${LOGFILE}
ulimit -Sv 4000000
echo "ulimit -Sv 4000000" >> ${LOGFILE}
for i in $(seq 11 40)
do
  echo "  Run family-${i}." >> ${LOGFILE}
  echo "  timeout ${TIMEOUT} ${ABCPROGRAM} -c \"ssat $1 ${CURDIR}/family-${i}.sdimacs\" >> ${LOGFILE}" >> ${LOGFILE}
  timeout ${TIMEOUT} ${ABCPROGRAM} -c "ssat $1 ${CURDIR}/family-${i}.sdimacs" >> ${LOGFILE}
done
