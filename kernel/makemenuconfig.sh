#!/bin/bash
arm-eabi-gcc
if [ ! $? -eq 0 ];then
	export PATH=$PATH:$(pwd)/../toolchain/arm-eabi-4.7/bin
fi
make ARCH=arm CORSS_COMPILE=arm-eabi- menuconfig
