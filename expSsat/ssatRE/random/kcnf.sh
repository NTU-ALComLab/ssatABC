dir=/home/users/nianze/ssat/ssatABC/expSsat/ssatRE/random
gen=gen-sdimacs
conv=sdimacs-to-ssat
k_arr=( 3 4 5 6 7 8 9 )
n_arr=( 10 20 30 40 50 )
r_arr=( -1 0 1 2 ) # m/n = k + r
N=5 # number of files generated under the parameters

for i in "${k_arr[@]}"
do
   index=100
   rm -rf ${i}CNF/sdimacs/*
   rm -rf ${i}CNF/ssat/*
   for j in "${n_arr[@]}"
   do
      for k in "${r_arr[@]}"
      do
         for ((n=1;n<=${N};n++))
         do
            ${dir}/${gen} 2ssat ${i} ${j} $((j*(i+k))) $((j/2)) 0.5
            ${dir}/${conv} rand-${i}-${j}-$((j*(i+k)))-$((j/2)).sdimacs ${i}CNF/ssat/rand-${i}-${j}-$((j*(i+k)))-$((j/2)).${index}.ssat
            mv rand-${i}-${j}-$((j*(i+k)))-$((j/2)).sdimacs ${i}CNF/sdimacs/rand-${i}-${j}-$((j*(i+k)))-$((j/2)).${index}.sdimacs
            ((index++))
         done
      done
   done
done
