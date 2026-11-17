# AHCI transport

`ahci.c` implements polling AHCI commands using physical contiguous DMA buffers.
The shared block cache and unified buffer I/O API are in `../../block/`.

Current scope: Intel 8086:2922 controller, port 0, one disk, 512-byte logical
sectors, LBA48 and serialized calls. Interrupt completion, hotplug, timeout
recovery, SMP locking and ATA FLUSH CACHE are not implemented.

Validate with a 32-bit freestanding build and QEMU read/write tests on a
throwaway image before relying on storage writes.
