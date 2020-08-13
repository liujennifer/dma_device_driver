/*
 * Driver to test pcie-testdev device's DMA transfer
 */

#include <linux/module.h>
#include <linux/pci.h>

#define PCI_EXPRESS_DEVICE_ID_TEST 0x0010
#define PCI_VENDOR_ID_REDHAT 0x1b36

#define BAR0 0
#define NAME "testdev_driver"
#define REGIONS 2

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

static void __iomem *dev_region;
static int flags = GFP_KERNEL; /* get free pages on behalf of kernel */

typedef struct RegionInfo {
    dma_addr_t bus_addr;
    void *cpu_addr;
    size_t size;
} RegionInfo;

static RegionInfo regions[REGIONS];

void create_region(struct pci_dev *pdev, RegionInfo *region, size_t size, int character, int bytes_filled)
{
    region->cpu_addr = dma_alloc_coherent(&pdev->dev, size, &region->bus_addr, flags);
    region->size = size;
    memset(region->cpu_addr, character, bytes_filled); /* Fill memory with 1mb data, to be transfered */
}

void dma_transfer(dma_addr_t src_addr, dma_addr_t dst_addr, dma_addr_t size)
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

    create_region(pdev, &regions[0], 1000 * 1024, 'a', 1000 * 1024);
    create_region(pdev, &regions[1], 1000 * 1024, 'b', 1000 * 1024);
    
    dma_transfer(regions[0].bus_addr, regions[1].bus_addr, 1000 * 1024);

    while (readq(dev_region + STATUS) != 0); /* Poll for transfer completion */
    printk("Transfer Status: %d\n", memcmp(regions[0].cpu_addr, regions[1].cpu_addr, 1000 * 1024));

    return 0;
}

static void remove(struct pci_dev *pdev)
{
    for (int i = 0; i < REGIONS; i++) {
        dma_free_coherent(&pdev->dev, regions[i].size, regions[i].cpu_addr, regions[i].bus_addr);
    }
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
