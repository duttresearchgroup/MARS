#!/bin/bash
# build the odroid linux image

make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- odroidxu3_defconfig
make -j8 ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
make -j8 ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- modules
