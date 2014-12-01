#!/bin/bash
#./obj/KERNEL_OBJ/arch/arm/boot/zImage
rm -f zImage
cp ../../../kernel/arch/arm/boot/zImage .
cd ramdisk
find . | cpio -o -H newc | gzip > ../newramdisk.gz
cd ..
../../mkbootimg_dtb --kernel zImage --ramdisk newramdisk.gz --dt dt.img --cmdline "console=ttyHSL0,115200,n8 androidboot.console=ttyHSL0 androidboot.hardware=qcom user_debug=31 msm_rtb.filter=0x37" --base 0x00000000 --ramdisk_offset 0x01000000 -o neoboot.img
