#!/bin/bash
#qemu-system-i386 -m 1G -hda disk.img -d int,cpu_reset -D debug.log -machine q35 -smp 2
qemu-system-i386 -m 1G  -machine q35 -smp 1  -device ide-hd,drive=myhd -drive file=disk.img,id=myhd,format=raw -d int,cpu_reset -D debug.log

#-d int,cpu_reset -D debug.log