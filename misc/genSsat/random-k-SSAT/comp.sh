#! /bin/bash
if [ "$#" -ne 4 ]; then
  echo "Usage: ./comp.sh <k> <n> <m> <l>"
  exit 1
fi
./testingScript/run.sh rand-$1-$2-$3-$4
echo " "
echo "  Get from our ssat solver"
cat log/sdimacs-rand-$1-$2-$3-$4.log
echo " "
echo "  Get from DC-SSAT solver"
tail -n 1 log/ssat-rand-$1-$2-$3-$4.log
head -n 1 log/ssat-rand-$1-$2-$3-$4.log
echo " "
