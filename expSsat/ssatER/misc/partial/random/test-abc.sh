#!/bin/bash
DIR=/home/users/danny/ssatABC/bin
CURDIR="/home/users/danny/ssatABC/expSsat/ssatER/partial/test-2"
ABCPROGRAM=${DIR}/abc
# SSATPROGRAM=${DIR}/ssat
SSATPROGRAM=/home/users/danny/ssat
LOGFILE=log/test-sdimacs${1}.log
TIMEOUT=1000
echo "Start." > ${LOGFILE}
ulimit -Sv 4000000
echo "ulimit -Sv 4000000" >> ${LOGFILE}
for i in $(seq 16 35)
do
  for j in $(seq 1 2)
  do
    echo "  Run test-${i}-${j}." >> ${LOGFILE}
    echo "  timeout ${TIMEOUT} ${ABCPROGRAM} -c \"ssat $1 ${CURDIR}/test-${i}-${j}.sdimacs\" >> ${LOGFILE}" >> ${LOGFILE}
    timeout ${TIMEOUT} ${ABCPROGRAM} -c "ssat $1 ${CURDIR}/test-${i}-${j}.sdimacs" >> ${LOGFILE}
  done
done
