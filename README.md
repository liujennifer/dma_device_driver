# dma_device_driver
This is a driver for a PCIe test device with DMA capabilities in QEMU.
> Warning: this is currently an out-of-tree kernel module, so use this in a safe environment (QEMU).

## QEMU Set-up
- Clone and build the QEMU source containing hw/misc/pcie-testdev.c (https://github.com/liujennifer/qemu.git)
- Create a disk image/install a guest OS in the VM. More information [here](https://wiki.qemu.org/Hosts/Linux).

### Run with no IOMMU/VT-d:
```shell
$ qemu/build/x86_64-softmmu/qemu-system-x86_64 -enable-kvm -hda <disk image> -m 2048
-device ioh3420,id=root_port,bus=pcie.0,addr=7.0 \
-device pcie-testdev,bus=root_port,id=testdev
```
> -device ioh3420 is a PCIe root port complex, to which PCIe endpoints like pcie-testdev can be connected to. You can see this relationship by running 'lspci -t' in the guest.

> The id property allows you to reference the specific device (like "root_port" is used here, and also if you want to reference it in the QEMU Monitor).
### Run with IOMMU/VT-d enabled:
```shell
$ qemu/build/x86_64-softmmu/qemu-system-x86_64 -enable-kvm -hda <disk image> -m 2048
-M q35,accel=kvm,kernel-irqchip=split \
-device intel-iommu,intremap=on,caching-mode=on,device-iotlb=on \
-device ioh3420,id=root_port,bus=pcie.0,addr=7.0 \
-device pcie-testdev,bus=root_port,id=testdev
```
> -M q35 is necessary here to support the IOMMU (More info about using [Intel VT-d](https://wiki.qemu.org/Features/VT-d)/intel-iommu)
- You may need to enable the IOMMU in the guest. Navigate to /etc/default/grub and edit it to be:
```shell
GRUB_CMDLINE_LINUX_DEFAULT="intel_iommu=on"
```
- Then run (may need to "sudo apt-get install update-grub" first)
```shell
$ sudo update-grub
```

## Driver usage
- After booting up QEMU, clone this repo inside your guest. Enter the folder and run make.

- Insert the module now with:
```shell
$ sudo insmod testdev_driver.ko
```
- View the kernel logs by running:
```
$ sudo dmesg
```
- If you booted QEMU without an IOMMU, you should see two successes (STATUS = 0). With an IOMMU, you should see the first transfer succeed, and the second one fail (non-zero status). This is because the first transfer is using the DMAR-safe [DMA-API](https://www.kernel.org/doc/Documentation/DMA-API-HOWTO.txt) mappings, and the second one uses the deprecated [virt-to-bus](https://www.kernel.org/doc/Documentation/bus-virt-phys-mapping.txt) ones.

- Remove your module with:
```shell
$ sudo rmmod testdev_driver
```

## Aggressive mode (driver not needed)
- In aggressive mode, pcie-testdev will immediately and repeatedly make DMA transfers, which may be a useful test for vulnerabilities
- You can enable this and set the source address, destination address, transfer size, and delay (in ms) between transfers. For example (you can pick appropriate addresses from the output of running "sudo cat /proc/iomem" in the guest):
```shell
-device pcie-testdev,bus=root_port1,id=testdev,aggressive,srcaddr=0x0009f000,dstaddr=0x0009fb00,size=1,aggression_delay_ms=2000
```
