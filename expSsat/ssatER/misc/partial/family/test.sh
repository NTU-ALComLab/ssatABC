#!/bin/bash
DIR=/home/users/danny/ssatABC/bin
CURDIR="/home/users/danny/generate/family"
#SSATPROGRAM=${DIR}/ssat
SSATPROGRAM=/home/users/danny/ssat
LOGFILE=log/family-ssat.log
TIMEOUT=1000
echo "Start." > ${LOGFILE}
ulimit -Sv 4000000
echo "ulimit -Sv 4000000" >> ${LOGFILE}
for i in $(seq 11 40)
do
  echo "  Run family-${i}." >> ${LOGFILE}
  echo "  timeout ${TIMEOUT} ${SSATPROGRAM} ${CURDIR}/family-${i}.ssat >> ${LOGFILE}" >> ${LOGFILE}
  timeout ${TIMEOUT} ${SSATPROGRAM} ${CURDIR}/family-${i}.ssat >> ${LOGFILE}
done
