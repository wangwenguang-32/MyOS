#!/bin/bash
#qemu-system-i386 -m 1G -hda disk.img -d int,cpu_reset -D debug.log -machine q35 -smp 2
# qemu-system-i386 -m 1G  -machine q35 -smp 1  -device ide-hd,drive=myhd -drive file=disk.img,id=myhd,format=raw -d int,cpu_reset -D debug.log

# qemu-system-i386 -m 1G  -machine q35 -smp 1  -device ahci,id=ahci0  -device ide-hd,drive=myhd,bus=ahci0.0  -drive file=disk.img,id=myhd,format=raw -d int,cpu_reset -D debug.log -vga vmware
qemu-system-i386 -m 1G  -machine q35 -smp 1  -drive file=disk.img,format=raw,if=none,id=disk0 -device ide-hd,bus=ide.0,drive=disk0 -d int,cpu_reset -D debug.log -vga vmware

#-d int,cpu_reset -D debug.log
