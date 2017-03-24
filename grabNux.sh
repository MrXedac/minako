#!/bin/bash
sudo cp ~/These/VMShared/bzImage src/partitions/x86/minako/images/bzImage
sudo chown xedac src/partitions/x86/minako/images/bzImage
PWD=`pwd`
cd src/partitions/x86/minako
make clean 
make
cd $PWD
