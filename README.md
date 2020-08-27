# dma_device_driver
This is a driver for a PCIe test device with DMA capabilities in QEMU.
> Warning: this is a kernel module, so use this in a safe environment (QEMU).

**QEMU Set-up**
- Clone and build the QEMU source containing hw/misc/pcie-testdev.c (https://github.com/liujennifer/qemu.git)

- To boot-up with no IOMMU/VT-d, you can run:
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
- Then run update-grub

**Driver usage**
After booting up QEMU, clone this repo inside your guest.
Enter the folder and run make.

If you insert the module now with:
> sudo insmod testdev_driver.ko

You should see any printed output from the device, as well kernel logs indicating transfer status.
If you ran with without an IOMMU, you should see two successes (STATUS = 0).
With an IOMMU, you see the first transfer succeed, and the second one fail.

Remove your module with:
> sudo rmmod testdev_driver
