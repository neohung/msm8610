#!/bin/bash
arm-eabi-gcc
if [ ! $? -eq 0 ];then
	export PATH=$PATH:$(pwd)/../toolchain/arm-eabi-4.7/bin
	#export PATH=$PATH:$(pwd)/../toolchain/arm-linux-androideabi-4.7/bin
fi
make ARCH=arm CROSS_COMPILE=arm-eabi- zImage -j4
cd ../mkboot/img/unpack
./gen.sh
