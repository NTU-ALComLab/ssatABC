./testingScript/gen-sdimacs 2ssat $1 $2 $3 $4 $5
./testingScript/sdimacs-to-ssat rand-$1-$2-$3-$4.sdimacs ssat/rand-$1-$2-$3-$4.ssat
mv -f rand-$1-$2-$3-$4.sdimacs sdimacs
