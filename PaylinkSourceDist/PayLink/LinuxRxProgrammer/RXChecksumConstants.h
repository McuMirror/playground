// These constants defined the interface between the POC, APP, FCP, PCP and RXC routine. No other links should be used.
// If any value is changed the effect on all 5 programs should be considered.

// POC = Power on checks program
// APP = The application program eg PHub
// FCP = Flash code programmer
// PCP = PC Programmer
// RXC = RxChecksum routine. Combines POC, APP and FCP programs.

// POC must be built as a standalone module.
// APP is build with POC source files and RXC will verify that the POC area is the same.
// FCP is build with POC source files and RXC will verify that the POC area is the same.
// RXC merges the FCP and APP files to generate a full SYSTEM file.
// PCP is "wrapped" with the full SYSTEM binary file.


// Physical Memory Definitions for trh RX621
#define  START_OF_FLASH_AREA         0xFFF80000     // 512K Chip
#define  SIZE_OF_FLASH_AREA          0x80000        // 512K Chip (up to 512K. All systems work with 256K, 384K, 512K.

// Memory Allocation for Each program

// POC ROM space - 1K
#define  POCROM_START                0xFFFFFC00     // Last 1K Flash Block
#define  POCROM_LENGTH               0x00000400     // Last 1K Flash Block

// FCP copy in ROM space - copied to RAM before calling - 19K
#define  FCPROM_START                0xFFFF8000     // starts at -20K and is 19K long
#define  FCPROM_LENGTH               0x00007C00

// APP area. Max of 492K. NB PHub limited to 236K
#define  APPROM_START                0xFFF80000
#define  APPROM_LENGTH               0x00078000

// Standard definitions
#define  AARDVARK_FLAG               0xaa8d4a8c
#define  DEFAULT_CHECKSUM            0x12345678
#define  DEFAULT_CHECKSUM_ADJUSTMENT (0x12 + 0x34 + 0x56 + 0x78)



// APP Fixed Locations - the 128 bytes below the FCP start
#define  APP_SECTION_CHECKSUM        (APPROM_START + APPROM_LENGTH - 0x80)  // 128 bytes of fixed location data
#define    APP_AARDVARK_FLAG           (APP_SECTION_CHECKSUM + 0x00)        // confirmation that area is programmed
#define    APP_AREA_START_ADDR         (APP_SECTION_CHECKSUM + 0x04)        // first checksum address
#define    APP_AREA_END_ADDR           (APP_SECTION_CHECKSUM + 0x08)        // last checksum address
#define    APP_FIRST_SPARE_ADDR        (APP_SECTION_CHECKSUM + 0x0C)        // allows unused space to be calculated. Form here to APP_LAST_SPARE_ADDR is unused
#define    APP_LAST_SPARE_ADDR         (APP_SECTION_CHECKSUM + 0x10)        // allows unused space to be calculated.
#define    APP_RESET_VECTOR            (APP_SECTION_CHECKSUM + 0x14)        // the address of the startup routine
#define    APP_SPARE_64_BYTES          (APP_SECTION_CHECKSUM + 0x18)        // spare
#define    APP_CHECKSUM                (APP_SECTION_CHECKSUM + 0x58)        // checksum from Area Start to Area End inclusive
#define    APP_VERSION                 (APP_SECTION_CHECKSUM + 0x5C)        // 4 byte version
#define    APP_COMPILE_DATE            (APP_SECTION_CHECKSUM + 0x60)        // date and time not in the checksum area
#define    APP_COMPILE_TIME            (APP_SECTION_CHECKSUM + 0x70)        // date and time not in the checksum area


// POC Fixed Locations - last 32 bytes before Fixed vectors.
//#define  POC_CODE                    0xFFFFF000
#define  POC_SECTION_CHECKSUM        (POCROM_START + (POCROM_LENGTH - 0xA0))
#define    POC_AARDVARK_FLAG           (POC_SECTION_CHECKSUM + 0x00)        // confirmation that area is programmed
#define    POC_AREA_START_ADDR         (POC_SECTION_CHECKSUM + 0x04)        // first address in checksum area
#define    POC_AREA_END_ADDR           (POC_SECTION_CHECKSUM + 0x08)        // last address in checksum area. NB Length End - Start + 1.
#define    POC_CHECKSUM                (POC_SECTION_CHECKSUM + 0x0C)        // checksum from Area Start to Area End inclusive
#define    POC_VERSION                 (POC_SECTION_CHECKSUM + 0x10)        // 4 byte version
#define    POC_ACTIVATE_BOOTLOADER_PTR (POC_SECTION_CHECKSUM + 0x14)        // the address of the routine that will activate the boot loader
#define    POC_RESTART_APP_PTR         (POC_SECTION_CHECKSUM + 0x18)        // the address of the routine that will restart the application
#define    POC_SPARE_LONG_3            (POC_SECTION_CHECKSUM + 0x1C)        //
#define    POC_FVECTORS                (POC_SECTION_CHECKSUM + 0x20)        // the fixed vextors are located here


// FCP Fixed Locations - last 32 bytes of the FCP area.
#define  FCP_SECTION_CHECKSUM        (FCPROM_START + FCPROM_LENGTH - 0x20)
#define    FCP_AARDVARK_FLAG           (FCP_SECTION_CHECKSUM + 0x00)        // confirmation that area is programmed
#define    FCP_AREA_START_ADDR         (FCP_SECTION_CHECKSUM + 0x04)        // first address in checksum area
#define    FCP_AREA_END_ADDR           (FCP_SECTION_CHECKSUM + 0x08)        // last address in checksum area. NB Length End - Start + 1.
#define    FCP_CHECKSUM                (FCP_SECTION_CHECKSUM + 0x0C)        // checksum from Area Start to Area End inclusive
#define    FCP_VERSION                 (FCP_SECTION_CHECKSUM + 0x10)        // 4 byte version
#define    FCP_SPARE_LONG_1            (FCP_SECTION_CHECKSUM + 0x14)        //
#define    FCP_SPARE_LONG_2            (FCP_SECTION_CHECKSUM + 0x18)        //
#define    FCP_SPARE_LONG_3            (FCP_SECTION_CHECKSUM + 0x1C)        //


// Flash Code Programmer RAM location
#define  FCPRAM_START                0x00000000     // the address teh FCP is copied to at power on
#define  FCP_RESET_VECTOR            0x00000000     // the address of the start routine



// External Flash Chip fixed locations
// Common to PHub and FCP. Code stored in last 512K of flash chip
// Settings in the 16 bytes before the code data
//#define  FCP_FLASH_CHIP_SETTINGS     0x0037FFF0     // chip address for this data
//#define    FCP_FLASH_CHIP_FLAG         0x0037FFF0   // chip address for this data
//#define    FCP_FLASH_CHIP_START        0x0037FFF4   // chip address for this data
//#define    FCP_FLASH_CHIP_XSUM         0x0037FFF8   // chip address for this data
//#define    FCP_FLASH_CHIP_LENGTH       0x0037FFFC   // chip address for this data
//#define  FCP_FLASH_CHIP_CODE         0x00380000     // start address of the code in the flash chip
