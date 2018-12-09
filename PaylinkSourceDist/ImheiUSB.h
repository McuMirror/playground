#ifndef __IMHEIUSB_H
#define __IMHEIUSB_H

/***************************************************************

Definitions for the USB interface provided for the:
Aardvark Embedded Solutions Intelligent Money Handling Equipment Interface




All data is transmitted as 6 byte packets.
These are normally buffered by the FTDI / Windows system, but this is incidental -
there is no higher structure.
The packets are identical in both directions - bit order is as for a PC.

Note:   A packet with an initial byte of 0xff is ignored.
        Both ends send 6 x 0xff at start of world to reset / flush the link.

Each packet is interpreted as a 16 bit control word and a 32 bit value:
_____________________________________________________
|                 Control Word                      |
_____________________________________________________
| 3 bit code | 11 bit longword Address | 2 bit code |
_____________________________________________________

  2/8/2007    AJT   Addition of message to query the kernel version.

****************************************************************/

enum {
    /* Frame Structure */
    USB_RESET            = -1,
    USB_CONTROL_MASK     = 0xe003,
    USB_ADDRESS_MASK     = 0x1ffc,
    USB_CONFIG_MASK      = 0x1fff,


    /********* Frame Identities / Codes *********/
    USB_VALUE            = 0x0000,          // Value is a replacement value
    USB_INCREMENT        = 0x0001,          // Value is a (signed) increment
    USB_COPY             = 0x0002,          // Value is in fact an Address to be copied into this
    //                   = 0x0003,

    USB_CONFIG           = 0x2000,          // See configuration header file.
    USB_CONFIG_1         = 0x2001,          // This uses the botton 3 bits !!
    USB_CONFIG_2         = 0x2002,
    USB_CONFIG_3         = 0x2003,

    USB_FIRST_LOCKOUT    = 0x4000,          // This is the first in a set of “locked out” updates.
    USB_LAST_LOCKOUT     = 0x4001,          // This is the last in a set of “locked out” updates.
    //                   = 0x4002,
    //                   = 0x4003,

    //                   = 0x6000,
    //                   = 0x6001,
    //                   = 0x6002,
    //                   = 0x6003,

    //                   = 0x8000,
    //                   = 0x8001,
    //                   = 0x8002,
    //                   = 0x8003,

    //                   = 0xA000,
    //                   = 0xA001,
    //                   = 0xA002,
    //                   = 0xA003,

    //                   = 0xC000,
    //                   = 0xC001,
    //                   = 0xC002,
    //                   = 0xC003,

    USB_QUERY_REPLY      = 0xE000,          // Reply to “Action Code” (Action code is shifted into Address)
    #define USB_QUERY_REPLY_CODE(x)     (USB_QUERY_REPLY + (x << 2))

    USB_SET_COMMAND      = 0xE001,          // SelfTest / Programming / "Protocol" Commands - Address contains a sub-code.
    //                   = 0xE002,          //
    USB_ACTION           = 0xE003,          // Value is an “action code.” (Address is zero!)


    /********* Action codes *********/
    USB_MEMORY_CLEAR         = 0  ,         // Memory Reset
    USB_RESTART              = 1  ,         // Restart Link (PC=>USB)
    USB_NEW_MEMORY_CLEAR     = 2  ,         // Start of new protocol - Clear memory
    USB_NEW_RESTART          = 3  ,         // Restart Link (PC=>USB) & use new protocol if possible
    USB_SEND_CONIFG = USB_SET_COMMAND | 64  , // This should be an Action value, but that confuses old drivers !!!!

    USB_H8_RESPONSE          = 10 ,         // if (H8 => PC) Crash Driver (see whoops!!!!!!)
    USB_POLL                 = 11 ,         // No op
    USB_RESET_HUB            = 12 ,         // Issue hardware reset to the hub

    USB_QUERY_APPLICATION    = 100,         // Application Enquiry (PC=>USB)
    USB_QUERY_VERSION        = 101,         // Version Enquiry (PC=>USB)
    USB_QUERY_SELFTEST       = 102,         // SelfTest Enquiry
    USB_QUERY_SERIAL         = 103,         // Serial Number Enquiry
    USB_QUERY_ACCEPTOR_COUNT = 104,         // Acceptor Enquiry
    USB_QUERY_HOPPER_COUNT   = 105,         // Hopper Enquiry
    USB_QUERY_METER_STATUS   = 106,         // Meter Enquiry
    USB_GREEN_ON             = 107,         // Turn on Green LED
    USB_RESET_SELFTEST       = 108,         // Reset Selftest / Processor
    USB_QUERY_CHECKSUM       = 109,         // Program Checksum
    USB_QUERY_SWITCHES       = 110,         // Current Switch status
    USB_QUERY_KERNEL_VERSION = 111,         // Get the kernel version if it is present
    USB_QUERY_PLATFORM       = 112,         // Hardware platform

    USB_FLASH_START          = 201,         // Start Flash Programming (PC=>USB)
    USB_FLASH_ACK            = 202,         // Block Acknowledge (USB=>PC)
    USB_FLASH_END            = 203,         // Programming ended.

    USB_CONFIG_START         = 301,         // Start Configuration data             (PC=>USB)
    USB_CONFIG_END           = 302,         // End Configuration data               (PC=>USB)
    USB_CONFIG_UPDATE        = 303,         // Configuration good - H8 restarting   (USB=>PC)
    USB_CONFIG_NAK           = 304,         // Configuration bad - rejected         (USB=>PC)
    USB_CONFIG_MATCH         = 305,         // Configuration matches stored version (USB=>PC)


    /********* Command codes *********/
    USB_FLASH_ADDRESS     = USB_SET_COMMAND | 0, // Value is address for following data.
    USB_FLASH_DATA        = USB_SET_COMMAND | 4, // Value is next 4 byte for programming.
    USB_SET_SERIAL        = USB_SET_COMMAND | 8, // Value is new serial number for the H8.
    USB_ECHO_TEST         = USB_SET_COMMAND | 12,// Test message from test program - invert value & echo it
    USB_ECHO_TEST_FLUSH   = USB_SET_COMMAND | 16,// Test message - invert value & echo it, then flush buffer

    USB_DIAGNOSTIC        = USB_SET_COMMAND | 20,// Diagnostic text - value is 1 - 4 bytes of text (NULL filled)
    USB_CHECKPOINT        = USB_SET_COMMAND | 24,// Value is Checksum | Seq << 24
    USB_ACK               = USB_SET_COMMAND | 28,// Value is Last good Seq
    USB_RESEND            = USB_SET_COMMAND | 32,// Value is Last good Seq
    USB_RESEND_START      = USB_SET_COMMAND | 36,// Value is Seq on next frame

    USB_READ_ADDRESS      = USB_SET_COMMAND | 40, // Value is address to read 128 bytes from (sent as USB_FLASH_DATA)






    // USB protocol buffer sizes (needed to ensure PC & Milan allow same size buffers

    MILAN_OUT_ITEMS       = 16,                    // Must be a power of 2 so that.......
    MILAN_OUT_ITEM_MASK   = (MILAN_OUT_ITEMS - 1), // this can be a mask for modulo arithmetic
    MILAN_IN_ITEMS        = 8

    } ;
#endif
