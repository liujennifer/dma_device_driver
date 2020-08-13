/*
 * Driver to test pcie-testdev device's DMA transfer
 */

#include <linux/module.h>
#include <linux/pci.h>

#define PCI_EXPRESS_DEVICE_ID_TEST 0x0010
#define PCI_VENDOR_ID_REDHAT 0x1b36

#define BAR0 0
#define NAME "testdev_driver"
#define COHERENT_REGION_SIZE 32

/* Offsets into device's BAR */
#define SRCADDR 0x00
#define DSTADDR 0x08
#define SIZE 0x10
#define DOORBELL 0x18
#define STATUS 0x20
#define RING 0x01


static struct pci_device_id id_table[] = {
    { PCI_DEVICE(PCI_VENDOR_ID_REDHAT, PCI_EXPRESS_DEVICE_ID_TEST), },
    { 0, }
};

MODULE_LICENSE("GPL");
MODULE_DEVICE_TABLE(pci, id_table);

static dma_addr_t dma_addr;
static void *vaddr;
static void __iomem *dev_region;
static int flags = GFP_KERNEL; /* get free pages on behalf of kernel */

void dma_transfer(void __iomem *region, dma_addr_t src_addr, dma_addr_t dst_addr, dma_addr_t size)
{
    iowrite32(src_addr, dev_region + SRCADDR);
    iowrite32(dst_addr, dev_region + DSTADDR);
    iowrite32(size, dev_region + SIZE);
    iowrite32(RING, dev_region + DOORBELL); /* Start DMA */
    printk("DMA Transfer\n");
}

static int probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
    printk("Probe\n");

    if (pci_enable_device(pdev) < 0) {
        printk("Error: pci_enable_device\n");
        return 1;
    }
    if (pci_request_region(pdev, BAR0, "bar0")) {
        printk("Error: pci_request_region\n");
        return 1;
    }

    dev_region = pci_iomap(pdev, BAR0, pci_resource_len(pdev, BAR0));
    if (!dev_region) {
        printk("Error: pci_iomap\n");
        return 1;
    }

    pci_set_master(pdev); /* Enable bus mastering bit in device for DMA */
    vaddr = dma_alloc_coherent(&pdev->dev, COHERENT_REGION_SIZE, &dma_addr, flags);

    memset(vaddr, 'a', 1000); /* Fill memory with 1mb data, to be transfered */

    dma_transfer(dev_region, dma_addr, dma_addr + 1020, 1000);

    while (readq(dev_region + STATUS) != 0); /* Poll for transfer completion */

    printk("Transfer memcmp Status: %d\n", memcmp(vaddr, vaddr + 1020, 1000));

    return 0;
}

static void remove(struct pci_dev *pdev)
{
    dma_free_coherent(&pdev->dev, COHERENT_REGION_SIZE, vaddr, dma_addr);
    pci_release_region(pdev, BAR0);
}

static struct pci_driver pci_driver = {
    .name     = NAME,
    .id_table = id_table,
    .probe    = probe,
    .remove   = remove,
};

static int my_init(void)
{
    printk("Init Driver\n");
    if (pci_register_driver(&pci_driver) < 0) {
        printk("Error: pci_register_driver");
        return 1;
    }
    return 0;
}

static void my_exit(void)
{
    pci_unregister_driver(&pci_driver);
}

module_init(my_init);
module_exit(my_exit);
