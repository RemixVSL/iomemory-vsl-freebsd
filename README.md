# IOMemory-VSL
This is an unsupported update of the original driver source for FusionIO
cards. It comes with no warranty, it may cause DATA LOSS or CORRUPTION.
Therefore it is NOT meant for production use, just for testing purposes.

## Background
Driver support for FusionIO cards has been lagging behind FreeBSD releases, effectively making these cards an expensive paperweight.

## Current version
The current version is derived from iomemory-vsl-3.2.10, with several fixes for compiling on FreeBSD 12.

## Releases
There are no releases yet, just master and the original checkin branch.

## Important note!!!
Commits to master are not "always" write tested, just compile tested. Releases have gone through testing with Flexible I/O Tester. Testing for page_cache errors, and generic FIO checksumming on read and write and different block sizes. For now only the fio-3.2.10.1509 branch has been tested!!

## Building
### Source
```
git clone https://github.com/snuf/iomemory-vsl-freebsd
cd iomemory-vsl-freebsd/
git checkout <release-tag|branch>
cd driver
make
kldload ./iomemory-vsl
```
## Installation
According to the INSTALL file:
``` 
sudo cp iomemory-vsl.ko /boot/modules
echo iomemory-vsl_load=\"YES\" | sudo tee -a /boot/loader.conf
```

# Utils (WIP)
For now the utils don't work as they require `libstdc++.so.6`:
```[vagrant@bazinga ~/iomemory-vsl-freebsd/bin]$ ./fio-status 
ld-elf.so.1: Shared object "libstdc++.so.6" not found, required by "fio-status"
```

The devices are seen as follows by FreeBSD in `pciconf -l -v`:
```
none0@pci0:0:6:0:      class=0x018000 card=0x10101aed chip=0x10051aed rev=0x01 hdr=0x00
    vendor     = 'SanDisk'
    device     = 'ioDimm3'
    class      = mass storage
none1@pci0:0:7:0:      class=0x018000 card=0x10101aed chip=0x10051aed rev=0x01 hdr=0x00
    vendor     = 'SanDisk'
    device     = 'ioDimm3'
    class      = mass storage
```

## Sysctl
Unlike Linux, which uses `/proc/fctX` for statistics and card information, FreeBSD uses sysctl to provide statistics and configuration options. The relevant information can be found in `sysctl -a dev.fct`, `sysctl -a hw.fusion`.


# How to Get Help
- Open an issue in this Github repo
- Join our Discord server at https://discord.gg/EAcujJkt


