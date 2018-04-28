#!/bin/bash
DIR=/home/users/danny/ssatABC/bin
CURDIR="/home/users/danny/ssatABC/expSsat/ssatER/partial/subsume"
ABCPROGRAM=${DIR}/abc
# SSATPROGRAM=${DIR}/ssat
SSATPROGRAM=/home/users/danny/ssat
LOGFILE=log/test-ssat.log
TIMEOUT=1000
echo "Start." > ${LOGFILE}
ulimit -Sv 4000000
echo "ulimit -Sv 4000000" >> ${LOGFILE}
for i in $(seq 8 18)
do
  double=$((2*i))
  for j in $(seq 1 4)
  do
    echo "  Run test-${double}-${j}." >> ${LOGFILE}
    echo "  timeout ${TIMEOUT} ${SSATPROGRAM} ${CURDIR}/test-${double}-${j}.ssat >> ${LOGFILE}" >> ${LOGFILE}
    timeout ${TIMEOUT} ${SSATPROGRAM} ${CURDIR}/test-${double}-${j}.ssat >> ${LOGFILE}
  done
done
