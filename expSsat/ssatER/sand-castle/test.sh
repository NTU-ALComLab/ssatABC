#!/bin/bash
DIR=/home/users/danny/ssatABC/bin
CURDIR="/home/users/danny/ssatABC/expSsat/ssatER/sand-castle"
ABCPROGRAM=${DIR}/abc
# SSATPROGRAM=${DIR}/ssat
SSATPROGRAM=/home/users/danny/ssat
LOGFILE=log/SC-ssat.log
TIMEOUT=1000
echo "Start." > ${LOGFILE}
ulimit -Sv 4000000
echo "ulimit -Sv 4000000" >> ${LOGFILE}
for i in $(seq 1 25)
do
  echo "  Run SC-${i}.ssat" >> ${LOGFILE}
  echo "  timeout ${TIMEOUT} ${SSATPROGRAM} ${CURDIR}/ssat/SC-${i}.ssat >> ${LOGFILE}" >> ${LOGFILE}
  timeout ${TIMEOUT} ${SSATPROGRAM} ${CURDIR}/ssat/SC-${i}.ssat >> ${LOGFILE}
done
