some-tiny-os is a unix-like operating system that was made from scratch. has a custom kernel, custom libc known as ```some-libc``` and a custom filesystem. it currently supports x86_64 but in the future there might be 32-bit version of the operating system.

# getting started
before you can try out the operating system on a virtual machine, you need to install the following software:

* GCC
* NASM
* QEMU
* make

after you installed all of the essential software, its time to compile the OS and run it inside of QEMU
```
git clone https://github.com/someguythat-thinkswithportals/some-tiny-os.git
cd some-tiny-os
make all
qemu-system-x86_64 some-tiny-os.img
```
alternatively you can do this
```
git clone https://github.com/someguythat-thinkswithportals/some-tiny-os.git
cd some-tiny-os
./run.sh
```