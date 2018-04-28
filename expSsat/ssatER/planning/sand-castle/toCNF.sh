#!/bin/bash
files=$(ls ${1}/*.sdimacs)
for file in ${files}
do
  name=$(echo ${file} | cut -f2 -d'/')
  echo ${name}
  cat ${file} | awk 'NR==1{print $0} NR>3{print $0}' > CNF/${name}-sub.cnf
done
