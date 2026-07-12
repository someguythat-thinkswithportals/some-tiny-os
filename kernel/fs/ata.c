#include "ata.h"
#include "../io.h"

#define ATA_DATA        0x1F0
#define ATA_ERROR       0x1F1
#define ATA_SECT_COUNT  0x1F2
#define ATA_LBA_LO      0x1F3
#define ATA_LBA_MID     0x1F4
#define ATA_LBA_HI      0x1F5
#define ATA_DRIVE       0x1F6
#define ATA_CMD_STAT    0x1F7
#define ATA_ALT_STAT    0x3F6

#define ATA_CMD_READ    0x20
#define ATA_CMD_WRITE   0x30

#define ATA_MASTER      0xE0

#define ATA_STATUS_BSY  0x80
#define ATA_STATUS_DRQ  0x08
#define ATA_STATUS_ERR  0x01
#define ATA_STATUS_DRDY 0x40

#define ATA_TIMEOUT     10000000

static int ata_wait_busy(void) {
    int timeout = ATA_TIMEOUT;
    while (inb(ATA_CMD_STAT) & ATA_STATUS_BSY) {
        if (--timeout == 0) return -1;
    }
    return 0;
}

static int ata_wait_drq(void) {
    int timeout = ATA_TIMEOUT;
    while (!(inb(ATA_CMD_STAT) & ATA_STATUS_DRQ)) {
        uint8_t stat = inb(ATA_CMD_STAT);
        if (stat & ATA_STATUS_ERR) return -1;
        if (stat & ATA_STATUS_BSY) return -1;
        if (--timeout == 0) return -1;
    }
    return 0;
}

int ata_read_sectors(uint32_t lba, uint8_t count, void* buf) {
    uint16_t* word_buf = (uint16_t*)buf;

    if (ata_wait_busy() < 0) return -1;

    outb(ATA_DRIVE, ATA_MASTER | ((lba >> 24) & 0x0F));
    outb(ATA_SECT_COUNT, count);
    outb(ATA_LBA_LO, lba & 0xFF);
    outb(ATA_LBA_MID, (lba >> 8) & 0xFF);
    outb(ATA_LBA_HI, (lba >> 16) & 0xFF);
    outb(ATA_CMD_STAT, ATA_CMD_READ);

    for (int s = 0; s < count; s++) {
        if (ata_wait_busy() < 0) return -1;
        if (ata_wait_drq() < 0) return -1;
        for (int i = 0; i < 256; i++) {
            word_buf[s * 256 + i] = inw(ATA_DATA);
        }
    }

    return 0;
}

int ata_write_sectors(uint32_t lba, uint8_t count, const void* buf) {
    const uint16_t* word_buf = (const uint16_t*)buf;

    if (ata_wait_busy() < 0) return -1;

    outb(ATA_DRIVE, ATA_MASTER | ((lba >> 24) & 0x0F));
    outb(ATA_SECT_COUNT, count);
    outb(ATA_LBA_LO, lba & 0xFF);
    outb(ATA_LBA_MID, (lba >> 8) & 0xFF);
    outb(ATA_LBA_HI, (lba >> 16) & 0xFF);
    outb(ATA_CMD_STAT, ATA_CMD_WRITE);

    for (int s = 0; s < count; s++) {
        if (ata_wait_busy() < 0) return -1;
        if (ata_wait_drq() < 0) return -1;
        for (int i = 0; i < 256; i++) {
            outw(ATA_DATA, word_buf[s * 256 + i]);
        }
    }

    return 0;
}
