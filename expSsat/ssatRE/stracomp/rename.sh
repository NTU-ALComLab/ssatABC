for file in `cat bench_list`
do
   #mv sdimacs/${file}.qdimacs sdimacs/${file}.sdimacs
   #vim -s script.vim sdimacs/${file}.sdimacs
   ./sdimacs-to-ssat sdimacs/${file}.sdimacs ssat/${file}.ssat
done
