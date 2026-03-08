# SPI Layout:
## GPIO layout 
- Bit 0: CLK
- Bit 1: Slave Out
- Bit 2: Slave In
- Bit 3: CS

BASE GPIO address is 0x300
NEED to set direction of pins:
0 - Output pin, 1 - input pin so:
0b0100


https://nodeloop.org/guides/sd-card-spi-init-guide/
# On Boot sequence:
## Initialise for CMD sending
- Set the **MOSI** and **CS** lines to high (logic '1').
(0b1100)
- Send at least **74 clock pulses** on the **SCK** line. Sending 80 pulses (10 bytes of 0xFF) is a common and safe practice.

## Set SPI mode
- Pull **CS** low. (0b0xxx)
- Send the 6-byte CMD0 command frame: 0x40 0x00 0x00 0x00 0x00 0x95.
- Keep sending clock pulses and read **MISO**. The card should respond with an **R1 response**. A value of 0x01 indicates the card is in an idle state and has successfully entered SPI mode.




For FAT:
typedef struct {
    uint32_t partition_start;

    uint32_t fat_start_lba;
    uint32_t data_start_lba;

    uint32_t sectors_per_cluster;
    uint32_t bytes_per_sector;

    uint32_t root_cluster;
} fat32_fs_t;
