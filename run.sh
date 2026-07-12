#!/bin/bash
# i think i may delete the file because make run does the same but i will decide
make clean && make all && qemu-system-x86_64 -drive format=raw,file=some-tiny-os.img -m 64M -serial stdio