A userspace implementation of a PCI driver for the Intel Precise Touch; as found in the Microsoft Surface Pro 4.

**N.B.** this is currently not usable and in the reverse engineering stage

## Related Links

 * [Reverse engineering Windows or Linux PCI drivers with Intel VT-d and QEMU](https://hakzsam.wordpress.com/2015/02/21/471/)
 * [The Userspace I/O HOWTO](https://www.kernel.org/doc/htmldocs/uio-howto/index.html)
 * [Getting started with uinput: the user level input subsystem](http://thiemonge.org/getting-started-with-uinput)

# Preflight

These instructions assume you are using Debian `jessie` 8.x, but may work with other distributions too.

You will require a kernel (Debian and probably other distros already do) with the following modules enabled:

 * `CONFIG_UIO_PCI_GENERIC`
 * `CONFIG_INPUT_UINPUT`

Fetch a copy of this project:

    cd /usr/src
    git clone https://github.com/jimdigriz/intel-precise-touch.git
    cd intel-precise-touch

Now install the dependencies:

    sudo apt-get install libsys-mmap-perl

# Usage

Set things up ready for the driver:

    sudo modprobe uinput
    sudo modprobe uio_pci_generic
    echo 8086 9d3e | sudo tee /sys/bus/pci/drivers/uio_pci_generic/new_id >/dev/null

Before we start the driver, check that the output of `lspci -k -d 8086:9d3e` shows the kernel driver in use is `uio_pci_generic`.

Now we can run the userspace driver with:

    sudo ./intel-precise-touch

# Development

To try and work out how the Microsoft Windows driver works, we tie the PCI card directly to a Win10 QEMU VM and record the requests that pass through, from there we stretch our heads and try to work out what means what and where.

Thanks go out to Olivier Fauchon for noting down the setting up of QEMU sparing me from having to do so.

## Windows 10

You will need to download:

 * [Windows 10 ISO](https://www.microsoft.com/en-US/software-download/windows10ISO)
 * [Virtio Drivers for Windows](https://fedoraproject.org/wiki/Windows_Virtio_Drivers) - at the time, only the latest release of `virtio-win.iso` had Windows 10 drivers

We also require an EFI BIOS (VFIO seems to stall otherwise):

    sudo apt-get install rpm2cpio
    wget https://www.kraxel.org/repos/jenkins/edk2/edk2.git-ovmf-x64-0-20160129.b1452.g62c9131.noarch.rpm
    rpm2cpio edk2.git-ovmf-x64-0-20160129.b1452.g62c9131.noarch.rpm > payload.cpio
    cpio -id < payload.cpio
    cp usr/share/edk2.git/ovmf-x64/OVMF{,_VARS}-pure-efi.fd .

Now install Windows 10 with:

    sudo apt-get install qemu-system-x86 qemu-utils spice-client
    qemu-img create -f qcow2 -o size=40G,lazy_refcounts=on mswin10.qcow2
    /usr/src/qemu/x86_64-softmmu/qemu-system-x86_64 \
        -machine accel=kvm \
        -cpu host -smp 2 -m 2G \
        -drive if=pflash,format=raw,readonly,file=OVMF-pure-efi.fd \
        -drive if=pflash,format=raw,file=OVMF_VARS-pure-efi.fd \
        -vga qxl -spice addr=127.0.0.1,port=5901,disable-ticketing \
        -drive file=mswin10.qcow2,if=virtio,snapshot=off \
        -net nic,model=virtio -net user \
        -device usb-ehci,id=ehci -usbdevice tablet \
        -soundhw hda \
        -rtc base=localtime \
        -drive file=Win10_1511_English_x64.iso,media=cdrom \
        -drive file=virtio-win.iso,media=cdrom \
        -boot once=d

You can access the VM with:

    spicec -h 127.0.0.1 -p 5901

I recommend you get all the regular disk, video, network and sound drivers working, optionally applying any Windows 10 updates, once done run:

    qemu-img convert -c -p -O qcow2 mswin10.qcow2 mswin10.compressed.qcow2
    mv mswin10.compressed.qcow2 mswin10.qcow2
    qemu-img snapshot -c fresh mswin10.qcow2

Now you have a snapshot point you can start from each time.

## Kernel

You will require a kernel with `CONFIG_VFIO_PCI` and `CONFIG_INTEL_IOMMU` enabled (Debian and probably other distros already do), plus you will need to add `intel_iommu=on` to your kernel parameters list; remember to reboot.

## QEMU

Although you can install and use Windows 10 in QEMU with Debian's packages, for the VFIO support, you will need a recent QEMU from git:

    cd /usr/src
    git clone git://git.qemu-project.org/qemu.git
    cd qemu
    ./configure --python=/usr/bin/python2 --enable-trace-backends=stderr --target-list=x86_64-softmmu --enable-spice
    make -j$(($(getconf _NPROCESSORS_ONLN)+1))

## PCI 

Now we need to perform the slightly tricky process of reserving the target PCI card for QEMU, tricky as you will also need to grab any parent PCI devices.

For example on the Microsoft Surface Pro 4, the Intel Precise Touch has the PCI ID `8086:9d3e` and hangs off the parent PCI card at `00:16.0`:

    $ lspci -k
    [snipped]
    00:16.0 Communication controller [0780]: Intel Corporation Device [8086:9d3a] (rev 21)
            Subsystem: Intel Corporation Device [8086:7270]
            Kernel driver in use: mei_me
            Kernel modules: mei_me
    00:16.4 Communication controller [0780]: Intel Corporation Device [8086:9d3e] (rev 21)

To reserve the PCI cards, you have to first free them up from any drivers that may have grabbed them already; here you can see that `mei_me` is in use, so will need need unloading:

    sudo modprobe -r mei_me

Now we can grab the PCI card (and its parent) for use with QEMU with:

    sudo modprobe vfio-pci ids=8086:9d3a,8086:9d3e

## Running

Now you should have everything you need so you can spin up QEMU and look at the tracing output:

    cat /usr/src/qemu/trace-events | sed -n 's/^\(vfio_[^(]*\).*/\1/ p' > events
    sudo /usr/src/qemu/x86_64-softmmu/qemu-system-x86_64 \
        -machine accel=kvm \
        -cpu host -smp 2 -m 2G \
        -drive if=pflash,format=raw,readonly,file=OVMF-pure-efi.fd \
        -drive if=pflash,format=raw,file=OVMF_VARS-pure-efi.fd \
        -vga qxl -spice addr=127.0.0.1,port=5901,disable-ticketing \
        -drive file=mswin10.qcow2,if=virtio,snapshot=off \
        -net nic,model=virtio -net user \
        -device usb-ehci,id=ehci -usbdevice tablet \
        -soundhw hda \
        -rtc base=localtime \
        -device vfio-pci,host=00:16.4,x-no-mmap=on \
        -trace events=events 2>&1 | tee trace

**N.B.** has to run with `sudo` so QEMU can directly access the PCI card

You will need to go to Device Manager and search for drivers online for your new unknown PCI communications device; Windows 10 will then download the driver and set up the card.  If everything works, you should be able to touch the screen now and move the mouse around in the VM.

Remember, if you want to run the userspace tool, you will first need to free up the PCI card with:

    sudo modprobe -r vfio-pci
