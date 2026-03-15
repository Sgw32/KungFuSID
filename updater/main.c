#include <c64.h>
#include <conio.h>
#include <peekpoke.h>
#include <stdint.h>
#include "firmware_blob.h"

#define FW_REG                  54301u      /* $D41D = $D400 + 29 */
#define FW_UPDATE_START_MAGIC   0xA5
#define FW_UPDATE_START_ACK     0x5A
#define FW_UPDATE_END_ACK       0x5A

#define FW_UPDATE_SECTOR_SIZE   (16UL * 1024UL)

#define WRITE_DELAY_INNER       8u
#define DELAY_AFTER_START       2000u
#define DELAY_AFTER_SECTOR_SEL  3000u
#define DELAY_BETWEEN_SECTORS   12000u
#define DELAY_BEFORE_FINAL_ACK  4000u

#define MAX_READ_RETRIES        3u

static void print_hex8(uint8_t v)
{
    static const char hex[] = "0123456789ABCDEF";
    cputc(hex[(v >> 4) & 0x0F]);
    cputc(hex[v & 0x0F]);
}

static void print_crlf(void)
{
    cputc('\r');
    cputc('\n');
}

static void delay_loops(unsigned int loops)
{
    volatile unsigned int i;
    for (i = 0; i < loops; ++i) {
    }
}

static void fw_write(uint8_t value)
{
    POKE(FW_REG, value);
}

static uint8_t fw_read(void)
{
    return PEEK(FW_REG);
}

static void progress_bar(uint8_t done, uint8_t total)
{
    uint8_t i;
    gotoxy(0, 18);
    cputs("Progress: [");

    for (i = 0; i < total; ++i) {
        cputc(i < done ? '#' : '.');
    }

    cputs("] ");
    cputc('0' + done);
    cputc('/');
    cputc('0' + total);
    cputs("   ");
}

static void sector_progress(uint16_t chunk, uint16_t total_chunks)
{
    gotoxy(0, 20);
    cputs("Chunk ");
    cputc((chunk / 1000) % 10 + '0');
    cputc((chunk / 100) % 10 + '0');
    cputc((chunk / 10) % 10 + '0');
    cputc((chunk % 10) + '0');
    cputs("/");
    cputc((total_chunks / 1000) % 10 + '0');
    cputc((total_chunks / 100) % 10 + '0');
    cputc((total_chunks / 10) % 10 + '0');
    cputc((total_chunks % 10) + '0');
    cputs("      ");
}

static uint8_t ask_confirm_update(void)
{
    char key;

    cputs("Start update? (Y/N)");
    print_crlf();

    while (1) {
        key = cgetc();

        if (key == 'y' || key == 'Y') {
            cputs("Confirmed.");
            print_crlf();
            print_crlf();
            return 1;
        }

        if (key == 'n' || key == 'N') {
            cputs("Canceled.");
            print_crlf();
            return 0;
        }
    }
}

static uint8_t fw_start(void)
{
    uint8_t ack;
    uint8_t attempt;

    cputs("Step 1: send START magic $");
    print_hex8(FW_UPDATE_START_MAGIC);
    print_crlf();

    fw_write(FW_UPDATE_START_MAGIC);

    for (attempt = 0; attempt < MAX_READ_RETRIES; ++attempt) {
        delay_loops(DELAY_AFTER_START);
        ack = fw_read();

        cputs("Read ACK: $");
        print_hex8(ack);
        print_crlf();

        if (ack == FW_UPDATE_START_ACK) {
            cputs("Start OK");
            print_crlf();
            print_crlf();
            return 1;
        }
    }

    cputs("ERROR: invalid start ACK");
    print_crlf();
    return 0;
}

static uint8_t fw_send_sector(uint8_t sector,
                              const uint8_t* sector_data,
                              uint16_t sector_data_len)
{
    unsigned long i;
    uint8_t echo = 0xFF;
    uint8_t checksum = 0;
    uint8_t dev_checksum = 0xFF;
    uint16_t chunk;
    uint8_t attempt;
    const uint16_t total_chunks = 64;   /* 64 * 256 = 16384 bytes */

    cputs("Sector ");
    cputc('0' + sector);
    cputs(": select sector");
    print_crlf();

    for (attempt = 0; attempt < 2; ++attempt) {
        delay_loops(DELAY_AFTER_SECTOR_SEL * 10u);
        fw_write(sector);
        delay_loops(DELAY_AFTER_SECTOR_SEL);

        echo = fw_read();

        cputs("Sector echo: $");
        print_hex8(echo);
        print_crlf();

        if (echo == sector) {
            break;
        }

        if (attempt == 0) {
            cputs("Retry sector select...");
            print_crlf();
        }
    }

    if (echo != sector) {
        cputs("ERROR: sector echo mismatch");
        print_crlf();
        return 0;
    }

    cputs("Writing 16KB");
    print_crlf();

    for (chunk = 0; chunk < total_chunks; ++chunk) {
        sector_progress((uint16_t)(chunk + 1), total_chunks);

        for (i = 0; i < 256UL; ++i) {
            unsigned long absolute_index = ((unsigned long)chunk << 8) + i;
            uint8_t value = 0xFF;

            if (absolute_index < sector_data_len) {
                value = sector_data[absolute_index];
            }

            fw_write(value);
            checksum ^= value;

            delay_loops(WRITE_DELAY_INNER);
        }
    }

    cputs("Write done");
    print_crlf();

    for (attempt = 0; attempt < MAX_READ_RETRIES; ++attempt) {
        dev_checksum = fw_read();

        cputs("Checksum expected: $");
        print_hex8(checksum);
        print_crlf();

        cputs("Checksum read:     $");
        print_hex8(dev_checksum);
        print_crlf();

        if (dev_checksum == checksum) {
            break;
        }

        if (attempt + 1u < MAX_READ_RETRIES) {
            cputs("Retry checksum read...");
            print_crlf();
            delay_loops(DELAY_AFTER_SECTOR_SEL);
        }
    }

    if (dev_checksum != checksum) {
        cputs("ERROR: checksum mismatch");
        print_crlf();
        return 0;
    }

    cputs("Sector ");
    cputc('0' + sector);
    cputs(" OK");
    print_crlf();
    print_crlf();

    progress_bar((uint8_t)(sector + 1), FW_BLOB_SECTOR_COUNT);
    delay_loops(DELAY_BETWEEN_SECTORS);

    return 1;
}

static uint8_t fw_finalize(void)
{
    uint8_t i;
    uint8_t ack;
    uint8_t attempt;
    static const char end_sequence[] = "KFSID_END";

    cputs("Final step: send end sequence");
    print_crlf();

    for (i = 0; end_sequence[i] != '\0'; ++i) {
        fw_write((uint8_t)end_sequence[i]);
        delay_loops(WRITE_DELAY_INNER * 8u);
    }

    for (attempt = 0; attempt < MAX_READ_RETRIES; ++attempt) {
        delay_loops(DELAY_BEFORE_FINAL_ACK);
        ack = fw_read();

        cputs("Final ACK: $");
        print_hex8(ack);
        print_crlf();

        if (ack == FW_UPDATE_END_ACK) {
            cputs("Finalize OK");
            print_crlf();
            print_crlf();
            return 1;
        }
    }

    cputs("ERROR: invalid final ACK");
    print_crlf();
    return 0;
}

int main(void)
{
    uint8_t sector;

    clrscr();
    bordercolor(COLOR_BLACK);
    bgcolor(COLOR_BLACK);
    textcolor(COLOR_WHITE);

    cputs("Firmware updater");
    print_crlf();
    cputs("----------------");
    print_crlf();
    print_crlf();

    cputs("Blob size: ");
    cprintf("%lu", (unsigned long)FW_BLOB_SIZE);
    cputs(" bytes");
    print_crlf();

    cputs("Sectors: ");
    cprintf("%u", (unsigned int)FW_BLOB_SECTOR_COUNT);
    print_crlf();
    print_crlf();

    progress_bar(0, FW_BLOB_SECTOR_COUNT);
    sector_progress(0, 64);

    if (!ask_confirm_update()) {
        return 0;
    }

    if (!fw_start()) {
        cputs("Update failed at start.");
        print_crlf();
        return 0;
    }

    for (sector = 0; sector < FW_BLOB_SECTOR_COUNT; ++sector) {
        unsigned long offset = (unsigned long)sector * FW_UPDATE_SECTOR_SIZE;
        uint16_t remaining;
        uint16_t send_len;

        if (FW_BLOB_SIZE > offset) {
            remaining = (uint16_t)(FW_BLOB_SIZE - offset);
            send_len = (remaining > FW_UPDATE_SECTOR_SIZE)
                ? (uint16_t)FW_UPDATE_SECTOR_SIZE
                : remaining;
        } else {
            send_len = 0;
        }

        if (!fw_send_sector(sector, &firmware_blob[offset], send_len)) {
            cputs("Update failed.");
            print_crlf();
            return 0;
        }
    }

    if (!fw_finalize()) {
        cputs("Update failed at finalize.");
        print_crlf();
        return 0;
    }

    gotoxy(0, 22);
    cputs("Firmware update complete.");
    print_crlf();

    return 0;
}
