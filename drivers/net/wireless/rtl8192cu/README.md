# Realtek driver for 8192cu / 8188cu devices #


The in-tree kernel drivers for these devices do not work so the official realtek
drivers have to be used. Gladly, these were released as GPL code so we can
publish them freely. Unfortunately, they have some issues with 64bit machines so
this repository contains the fixes to make them work.

Please note that these fixes are not officially from realtek so there is no
guarantee that they work. However, I run this driver on my 64bit x86_64 linux
machine and they work like a charm. Furthermore, I try to keep them compatible
with linux-next. Therefore, they should work with every new kernel release.

If the current code does not compile with your kernel, please check out a
previous version that was known to work. I try to add tags for every kernel
release.

If there are issues, please do not hesitate to contact me. Furthermore, if any
of you can confirm that the in-tree kernel drivers work with these devices, I
will gladly drop this repository as it isn't really fun to maintain this.

## WHAT IS NEW? ##
Added support for kernel 3.10+ (3.11 working, 3.12 not tested)
DKMS method

## Install ##


### Easy Install on Ubuntu using DKMS ###


On Ubuntu 13.10, the following commands ought to download and install this
module, replacing the buggy built-in rtl8192cu driver. This will also automatically
rebuild kernel module on kernel upgrade:
    
    sudo apt-get install git build-essential linux-headers-generic dkms
    git clone https://github.com/dz0ny/rt8192cu.git --depth 1
    cd rt8192cu
    sudo make dkms

### Hard Install ###

    sudo apt-get install git build-essential linux-headers-generic dkms
    git clone https://github.com/dz0ny/rt8192cu.git --depth 1
    cd rt8192cu
    sudo make install

The current Realtek driver version that this repository is based on is:
    
    v4.0.2_9000

## License / Copying ##

The Realtek tarball didn't include license-terms, however, all code files that I
added to this repository are licensed as GPL-v2, therefore, there shouldn't be
any issues if you clone this.
