#!/bin/bash
ssh -p 2222 pip@localhost "cd Downloads/linux-4.10.4 && make"
cp linux/arch/x86/boot/bzImage src/partitions/x86/minako/images/bzImage
PWD=`pwd`
cd src/partitions/x86/minako
make clean 
make
cd $PWD
