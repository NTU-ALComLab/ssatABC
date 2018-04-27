dir=/home/users/nianze/ssat/ssatABC/expSsat/ssatER/sand-castle
gen=gen_sand
ckt=new_gen
sdi=${dir}/sdimacs
sat=${dir}/ssat
blf=${dir}/blif
N=25

rm -rf ${sdi}/*
rm -rf ${sat}/*
rm -rf ${blf}/*

for ((t=1;t<=${N};t++))
do
   ${dir}/${gen} sdimacs ${t}
   ${dir}/${gen} ssat ${t}
   ${dir}/${ckt} blif ${t} single
   mv SC-${t}.sdimacs ${sdi}
   mv SC-${t}.ssat ${sat}
   mv SC-${t}_s.blif ${blf}
done
