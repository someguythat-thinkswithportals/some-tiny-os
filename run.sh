#!/bin/bash
make clean && make all && qemu-system-x86_64 -drive format=raw,file=some-tiny-os.img -m 64M -serial stdio
