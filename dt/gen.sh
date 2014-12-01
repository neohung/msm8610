#./dtc -p 1024 -O dtb -o out/msm8610-rumi.dtb arch/arm/boot/dts/msm8610-rumi.dts
#./dtc -p 1024 -O dtb -o out/msm8610-sim.dtb arch/arm/boot/dts/msm8610-sim.dts
#./dtc -p 1024 -O dtb -o out/msm8610-v1-cdp.dtb arch/arm/boot/dts/msm8610-v1-cdp.dts
#./dtc -p 1024 -O dtb -o out/msm8610-v1-mtp.dtb arch/arm/boot/dts/msm8610-v1-mtp.dts
#./dtc -p 1024 -O dtb -o out/msm8610-v1-qrd-skuaa.dtb arch/arm/boot/dts/msm8610-v1-qrd-skuaa.dts
#./dtc -p 1024 -O dtb -o out/msm8610-v1-qrd-skuab.dtb arch/arm/boot/dts/msm8610-v1-qrd-skuab.dts
#./dtc -p 1024 -O dtb -o out/msm8610-v1-qrd-skuab-dvt2.dtb arch/arm/boot/dts/msm8610-v1-qrd-skuab-dvt2.dts
#./dtc -p 1024 -O dtb -o out/msm8610-v2-cdp.dtb arch/arm/boot/dts/msm8610-v2-cdp.dts
#./dtc -p 1024 -O dtb -o out/msm8610-v2-mtp.dtb arch/arm/boot/dts/msm8610-v2-mtp.dts
#./dtc -p 1024 -O dtb -o out/msm8610-v2-qrd-skuaa.dtb arch/arm/boot/dts/msm8610-v2-qrd-skuaa.dts
#./dtc -p 1024 -O dtb -o out/msm8610-v2-qrd-skuab-dvt2.dtb arch/arm/boot/dts/msm8610-v2-qrd-skuab-dvt2.dts
rm out/*
./dtc -p 1024 -O dtb -o out/msm8610-v2-mtp.dtb neodts/msm8610-v2-mtp.dts
./dtbTool -o neodt.img  -s 2048 -p ./ out/
cp neodt.img ../mkboot/img/unpack/neodt.img
