# dma_device_driver
This is a driver for a PCIe test device with DMA capabilities in QEMU.
> Warning: this is a kernel module, so use this in a safe environment (QEMU).

**QEMU Set-up**
- Clone and build the QEMU source containing hw/misc/pcie-testdev.c (https://github.com/liujennifer/qemu.git)

- To boot-up with no IOMMU/VT-d, you can run (sudo):
```shell
$ qemu/build/x86_64-softmmu/qemu-system-x86_64 -enable-kvm -hda <path to disk image> -m 2048
-device ioh3420,id=root_port,bus=pcie.0,addr=7.0 \
-device pcie-testdev,bus=root_port,id=testdev
```
- To boot-up with an IOMMU/VT-d, run:
```shell
$ qemu/build/x86_64-softmmu/qemu-system-x86_64 -enable-kvm -hda <path to disk image> -m 2048
-M q35,accel=kvm,kernel-irqchip=split \
-device intel-iommu,intremap=on,caching-mode=on,device-iotlb=on \
-device ioh3420,id=root_port,bus=pcie.0,addr=7.0 \
-device pcie-testdev,bus=root_port,id=testdev
```
You may need to enable the IOMMU in the guest:
- Navigate to /etc/default/grub and edit it so:
> GRUB_CMDLINE_LINUX="intel_iommu=on"
- Then run (may need to "sudo apt-get install update-grub" first)
> sudo update-grub


**Driver usage**
- After booting up QEMU, clone this repo inside your guest. Enter the folder and run make.

- Insert the module now with:
```shell
$ sudo insmod testdev_driver.ko
```
- You should see any printed output from the device, as well as kernel logs from running:
```
sudo dmesg
```
- If you booted QEMU without an IOMMU, you should see two successes (STATUS = 0). With an IOMMU, you should see the first transfer succeed, and the second one fail (non-zero status).

Remove your module with:
```shell
$ sudo rmmod testdev_driver
```

**Aggressive mode (driver not needed)**
- In aggressive mode, pcie-testdev will immediately and repeatedly make DMA transfers.
- You can enable this and set the source address, destination address, transfer size, and delay (in ms) between transfers like this:
```shell
-device pcie-testdev,bus=root_port1,id=testdev,aggressive,srcaddr=0x0009f000,dstaddr=0x0009fb00,size=1,aggression_delay_ms=2000
```
