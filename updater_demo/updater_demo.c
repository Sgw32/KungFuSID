#include <c64.h>
#include <conio.h>
#include <peekpoke.h>
#include <stdint.h>

#define FW_REG                  54301u      /* $D41D = $D400 + 29 */
#define FW_UPDATE_START_MAGIC   0xA5
#define FW_UPDATE_START_ACK     0x5A
#define FW_UPDATE_END_ACK       0x5A

//16k*3 = 48K for program
#define FW_UPDATE_SECTOR_COUNT  3 
#define FW_UPDATE_SECTOR_SIZE   (16UL * 1024UL)

#define WRITE_DELAY_INNER       8u
#define DELAY_AFTER_START       2000u
#define DELAY_AFTER_SECTOR_SEL  3000u
#define DELAY_BETWEEN_SECTORS   12000u
#define DELAY_BEFORE_FINAL_ACK  4000u

static const uint8_t sector_pattern[FW_UPDATE_SECTOR_COUNT] = {
     0xA0, 0xB0, 0xC0//, 0xD0
};

static const char end_sequence[] = "KFSID_END";

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

static uint8_t fw_start(void)
{
    uint8_t ack;

    cputs("Step 1: send START magic $");
    print_hex8(FW_UPDATE_START_MAGIC);
    print_crlf();

    fw_write(FW_UPDATE_START_MAGIC);
    delay_loops(DELAY_AFTER_START);

    ack = fw_read();

    cputs("Read ACK: $");
    print_hex8(ack);
    print_crlf();

    if (ack != FW_UPDATE_START_ACK) {
        cputs("ERROR: invalid start ACK");
        print_crlf();
        return 0;
    }

    cputs("Start OK");
    print_crlf();
    print_crlf();
    return 1;
}

static uint8_t fw_send_sector(uint8_t sector, uint8_t base)
{
    unsigned long i;
    uint8_t echo;
    uint8_t checksum = 0;
    uint8_t dev_checksum;
    uint16_t chunk;
    const uint16_t total_chunks = 64;   /* 64 * 256 = 16384 bytes */
    uint8_t pattern_index = 0;
    uint8_t value;
    uint8_t attempt;

    cputs("Sector ");
    cputc('0' + sector);
    cputs(": select sector");
    print_crlf();
    
    for (attempt = 0; attempt < 2; ++attempt)
    {
        delay_loops(DELAY_AFTER_SECTOR_SEL * 10);
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

    cputs("Writing 16KB pattern, base $");
    print_hex8(base);
    print_crlf();

    cputs("First 16 bytes: ");
    for (i = 0; i < 16UL; ++i) {
        // value = (uint8_t)(base + (pattern_index & 0x0F));
        // value ^= (uint8_t)(0x11 * (pattern_index & 0x03));
        // value ^= (uint8_t)(sector << 1);
        value = base+i;
        print_hex8(value);
        cputc(' ');

        pattern_index++;
    }
    print_crlf();

    pattern_index = 0;

    for (chunk = 0; chunk < total_chunks; ++chunk) {
        sector_progress((uint16_t)(chunk + 1), total_chunks);

        for (i = 0; i < 256UL; ++i) {
            // value = (uint8_t)(base + (pattern_index & 0x0F));
            // value ^= (uint8_t)(0x11 * (pattern_index & 0x03));
            // value ^= (uint8_t)(sector << 1);
            value = base+i;
            fw_write(value);
            checksum ^= value;
            pattern_index++;

            delay_loops(WRITE_DELAY_INNER);
        }
    }

    cputs("Write done");
    print_crlf();

    dev_checksum = fw_read();

    cputs("Checksum expected: $");
    print_hex8(checksum);
    print_crlf();

    cputs("Checksum read:     $");
    print_hex8(dev_checksum);
    print_crlf();

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

    progress_bar((uint8_t)(sector + 1), FW_UPDATE_SECTOR_COUNT);
    delay_loops(DELAY_BETWEEN_SECTORS);

    return 1;
}

static uint8_t fw_finalize(void)
{
    uint8_t i;
    uint8_t ack;

    cputs("Final step: send end sequence");
    print_crlf();

    for (i = 0; end_sequence[i] != '\0'; ++i) {
        fw_write((uint8_t)end_sequence[i]);
        delay_loops(WRITE_DELAY_INNER * 8u);
    }

    delay_loops(DELAY_BEFORE_FINAL_ACK);
    ack = fw_read();

    cputs("Final ACK: $");
    print_hex8(ack);
    print_crlf();

    if (ack != FW_UPDATE_END_ACK) {
        cputs("ERROR: invalid final ACK");
        print_crlf();
        return 0;
    }

    cputs("Finalize OK");
    print_crlf();
    print_crlf();
    return 1;
}

int main(void)
{
    uint8_t sector;

    clrscr();
    bordercolor(COLOR_BLACK);
    bgcolor(COLOR_BLACK);
    textcolor(COLOR_WHITE);

    cputs("Firmware updater demo");
    print_crlf();
    cputs("---------------------");
    print_crlf();
    print_crlf();

    cputs("Sectors to program:");
    print_crlf();
   cputs("  0 -> complex A0-based pattern");
    print_crlf();
    cputs("  1 -> complex B0-based pattern");
    print_crlf();
    cputs("  2 -> complex C0-based pattern");
    print_crlf();
    cputs("  3 -> complex D0-based pattern");
    print_crlf();
    print_crlf();

    progress_bar(0, FW_UPDATE_SECTOR_COUNT);
    sector_progress(0, 64);

    if (!fw_start()) {
        cputs("Update failed at start.");
        print_crlf();
        return 0;
    }

    for (sector = 0; sector < FW_UPDATE_SECTOR_COUNT; ++sector) {
        if (!fw_send_sector(sector, sector_pattern[sector])) {
            cputs("Update failed.");
            print_crlf();
            return 0;
        }
    }

    // if (!fw_finalize()) {
    //     cputs("Update failed at finalize.");
    //     print_crlf();
    //     return 0;
    // }

    gotoxy(0, 22);
    cputs("Firmware update complete.");
    print_crlf();

    return 0;
}