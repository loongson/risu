#!/bin/bash

rm -rf build
mkdir build
cd build
ARCH=loongarch64 ../configure --static
#ARCH=loongarch64 ../configure
make

# rand insn: 
# ./risugen --numinsn 100000 loongarch.risu test.bin

# master : 
# ./build/risu --master test.bin

# client
# [./qemu-loongarch ] ./build/risu  --host ip  test.bin

# dump test.bin:
#  objdump -D -b binary -m loongarch64 gs.bin 2>&1 | tee 1.txt
