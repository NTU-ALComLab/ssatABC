./bin/ssat ssat/$1.ssat | grep "total time\|Pr\[SAT\]" | awk 'NR==1{print "  > Time   = " $4} NR==2{print "  > Answer = "$3}' > log/ssat-$1.log
./bin/abc -c "ssat sdimacs/$1.sdimacs" | grep "Time\|Answer" | awk 'NR==2{print "  > Time   = " $4} NR==1{print "  > Answer = "$4}' > log/sdimacs-$1.log
