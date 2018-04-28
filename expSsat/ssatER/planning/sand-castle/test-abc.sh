#!/bin/bash
DIR=/home/users/danny/ssatABC/bin
CURDIR="/home/users/danny/ssatABC/expSsat/ssatER/sand-castle"
ABCPROGRAM=${DIR}/abc
# SSATPROGRAM=${DIR}/ssat
SSATPROGRAM=/home/users/danny/ssat
LOGFILE=log/SC-sdimacs${1}.log
TIMEOUT=1000
echo "Start." > ${LOGFILE}
ulimit -Sv 4000000
echo "ulimit -Sv 4000000" >> ${LOGFILE}
for i in $(seq 1 25)
do
  echo "  Run test-${i}-${j}." >> ${LOGFILE}
  echo "  timeout ${TIMEOUT} ${ABCPROGRAM} -c \"ssat $1 ${CURDIR}/sdimacs/SC-${i}.sdimacs\" >> ${LOGFILE}" >> ${LOGFILE}
  timeout ${TIMEOUT} ${ABCPROGRAM} -c "ssat $1 ${CURDIR}/sdimacs/SC-${i}.sdimacs" >> ${LOGFILE}
done
