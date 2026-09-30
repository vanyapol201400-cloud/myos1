#include "ata.h"
#include "io.h"

extern void print(const char *str);
extern void putchar(char c);
extern void print_uint(uint32_t n);
extern void print_hex8(uint8_t n);

static uint16_t BASE  = 0x1F0;
static uint16_t CTRL  = 0x3F6;
static uint8_t  DRIVE = 0xA0;

static uint32_t total_sectors = 0;
static char     model[41] = {0};
static uint16_t raw_ident[256];

#define R_DATA    (BASE + 0)
#define R_SECCNT  (BASE + 2)
#define R_LBA0    (BASE + 3)
#define R_LBA1    (BASE + 4)
#define R_LBA2    (BASE + 5)
#define R_DRIVE   (BASE + 6)
#define R_STATUS  (BASE + 7)
#define R_CMD     (BASE + 7)

#define ST_BSY 0x80
#define ST_DRQ 0x08
#define ST_ERR 0x01

static void io_delay(void) { inb(CTRL); inb(CTRL); inb(CTRL); inb(CTRL); }

static int try_identify(uint16_t base, uint16_t ctrl, uint8_t drive_sel) {
    BASE = base; CTRL = ctrl; DRIVE = drive_sel;

    outb(CTRL, 0x04); io_delay();
    outb(CTRL, 0x00); io_delay();
    for (int i = 0; i < 1000000; i++) {
        uint8_t s = inb(R_STATUS);
        if (s == 0x00 || s == 0xFF) break;
        if (!(s & ST_BSY)) break;
    }

    outb(R_DRIVE, DRIVE); io_delay();
    outb(R_SECCNT, 0);
    outb(R_LBA0, 0);
    outb(R_LBA1, 0);
    outb(R_LBA2, 0);
    outb(R_CMD, 0xEC); io_delay();

    uint8_t s1 = inb(R_STATUS);
    if (s1 == 0x00 || s1 == 0xFF) return -1;

    int ok = 0;
    for (int i = 0; i < 1000000; i++) {
        uint8_t s = inb(R_STATUS);
        if (s == 0x00 || s == 0xFF) return -1;
        if (!(s & ST_BSY)) { ok = 1; break; }
    }
    if (!ok) return -1;

    if (inb(R_LBA1) != 0 || inb(R_LBA2) != 0) return -1;

    ok = 0;
    for (int i = 0; i < 1000000; i++) {
        uint8_t s = inb(R_STATUS);
        if (s & ST_ERR) return -1;
        if (s & ST_BSY) continue;
        if (s & ST_DRQ) { ok = 1; break; }
    }
    if (!ok) return -1;

    for (int i = 0; i < 256; i++) raw_ident[i] = inw(R_DATA);

    for (int i = 0; i < 20; i++) {
        model[i*2]   = (raw_ident[27+i] >> 8) & 0xFF;
        model[i*2+1] = raw_ident[27+i] & 0xFF;
    }
    model[40] = 0;
    total_sectors = ((uint32_t)raw_ident[61] << 16) | raw_ident[60];
    return 0;
}

int ata_init(void) {
    print("\nATA probe:");
    if (try_identify(0x1F0, 0x3F6, 0xA0) == 0) {
        print("\n  IDENTIFY OK, model=");
        print(model);
        print("\n  sectors=");
        print_uint(total_sectors);
        return 0;
    }
    if (try_identify(0x1F0, 0x3F6, 0xB0) == 0) { print("\n  slave OK"); return 0; }
    if (try_identify(0x170, 0x376, 0xA0) == 0) { print("\n  secondary OK"); return 0; }
    if (try_identify(0x170, 0x376, 0xB0) == 0) { print("\n  secondary slave OK"); return 0; }
    print("\n  no ATA disk found");
    return -1;
}

void ata_read_sector(uint32_t lba, uint8_t *buf) {
    for (int w = 0; w < 1000000; w++) if (!(inb(R_STATUS) & ST_BSY)) break;
    outb(R_DRIVE, DRIVE | ((lba >> 24) & 0x0F)); io_delay();
    outb(R_SECCNT, 1);
    outb(R_LBA0, lba & 0xFF);
    outb(R_LBA1, (lba >> 8) & 0xFF);
    outb(R_LBA2, (lba >> 16) & 0xFF);
    outb(R_CMD, 0x20);
    for (int w = 0; w < 1000000; w++) {
        uint8_t s = inb(R_STATUS);
        if (!(s & ST_BSY) && (s & ST_DRQ)) break;
    }
    for (int i = 0; i < 256; i++) {
        uint16_t w = inw(R_DATA);
        buf[i*2] = w & 0xFF;
        buf[i*2+1] = (w >> 8) & 0xFF;
    }
}

void ata_write_sector(uint32_t lba, const uint8_t *buf) {
    /* Ждём BSY=0 */
    for (int w = 0; w < 1000000; w++) {
        uint8_t s = inb(R_STATUS);
        if (s == 0x00 || s == 0xFF) { print("\nWrite: no disk"); return; }
        if (!(s & ST_BSY)) break;
    }

    outb(R_DRIVE, DRIVE | ((lba >> 24) & 0x0F)); io_delay();
    outb(R_SECCNT, 1);
    outb(R_LBA0, lba & 0xFF);
    outb(R_LBA1, (lba >> 8) & 0xFF);
    outb(R_LBA2, (lba >> 16) & 0xFF);
    outb(R_CMD, 0x30);
    io_delay();

    uint8_t s = inb(R_STATUS);
    if (s == 0x00 || s == 0xFF) {
        print("\nWrite: dead disk st=0x");
        print_hex8(s);
        return;
    }

    int ok = 0;
    for (int w = 0; w < 1000000; w++) {
        s = inb(R_STATUS);
        if (s & ST_ERR) {
            print("\nWrite: ERR st=0x");
            print_hex8(s);
            return;
        }
        if (s & ST_BSY) continue;
        if (s & ST_DRQ) { ok = 1; break; }
    }
    if (!ok) {
        print("\nWrite: no DRQ st=0x");
        print_hex8(inb(R_STATUS));
        return;
    }

    for (int i = 0; i < 256; i++) {
        outb(R_DATA, buf[i*2]);
        outb(R_DATA, buf[i*2+1]);
    }

    outb(R_CMD, 0xE7);
    for (int w = 0; w < 1000000; w++) {
        s = inb(R_STATUS);
        if (s == 0x00 || s == 0xFF) break;
        if (!(s & ST_BSY)) break;
    }
}

uint32_t ata_total_sectors(void) { return total_sectors; }
void ata_drive_info(char *m, uint32_t *s) {
    for (int i = 0; i < 41; i++) m[i] = model[i];
    *s = total_sectors;
}
