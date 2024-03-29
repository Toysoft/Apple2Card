/* Apple2Arduino firmware */
/* by Daniel L. Marks */

/*
   Copyright (c) 2022 Daniel Marks

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/* Note to self to create objdump:

  C:\Users\dmarks\Documents\ArduinoData\packages\arduino\tools\avr-gcc\4.8.1-arduino5\bin\avr-objdump.exe -x -t -s Apple2Arduino.ino.elf > s155143
*/

#include <Arduino.h>
#include <avr/pgmspace.h>
#include <EEPROM.h>
#include "diskio_sdc.h"
#include "mmc_avr.h"
#include "ff.h"
#include "pindefs.h"

#define USE_ETHERNET
#undef DEBUG_SERIAL

#ifdef USE_ETHERNET
#include "w5500.h"
#endif

#ifdef DEBUG_SERIAL
#include <SoftwareSerial.h>
SoftwareSerial softSerial(SOFTWARE_SERIAL_RX, SOFTWARE_SERIAL_TX);
#define SERIALPORT() (&softSerial)
#endif

#define EEPROM_INIT 0
#define EEPROM_SLOT0 1
#define EEPROM_SLOT1 2

#define SLOT_STATE_NODEV 0
#define SLOT_STATE_BLOCKDEV 1
#define SLOT_STATE_WIDEDEV 2
#define SLOT_STATE_FILEDEV 3

#ifdef USE_ETHERNET
uint8_t ethernet_initialized = 0;
Wiznet5500 eth(8);
#endif

FATFS   fs;
FIL     slotfile;
uint8_t last_drive = 255;

uint8_t slot0_state = SLOT_STATE_NODEV;
uint8_t slot0_fileno;

uint8_t slot1_state = SLOT_STATE_NODEV;
uint8_t slot1_fileno;

static char blockvolzero[] = "0:";
static char blockvolone[] = "1:";

static char blockdev0_filename[] = "0:BLKDEVXX.PO";
static char blockdev1_filename[] = "1:BLKDEVXX.PO";

extern "C" {
  void write_string(const char *c)
  {
#ifdef DEBUG_SERIAL
    SERIALPORT()->print(c);
    SERIALPORT()->flush();
#endif
  }
}

void read_eeprom(void)
{
  if (EEPROM.read(EEPROM_INIT) != 255)
  {
    slot0_fileno = EEPROM.read(EEPROM_SLOT0);
    slot1_fileno = EEPROM.read(EEPROM_SLOT1);
  } else
  {
    slot0_fileno = 0;
    slot1_fileno = 1;
  }
}

void write_eeprom(void)
{
  if (EEPROM.read(EEPROM_SLOT0) != slot0_fileno)
    EEPROM.write(EEPROM_SLOT0, slot0_fileno);
  if (EEPROM.read(EEPROM_SLOT1) != slot1_fileno)
    EEPROM.write(EEPROM_SLOT1, slot1_fileno);
  if (EEPROM.read(EEPROM_INIT) == 255)
    EEPROM.write(EEPROM_INIT, 0);
}

void setup_pins(void)
{
  INITIALIZE_CONTROL_PORT();
  DISABLE_RXTX_PINS();
  DATAPORT_MODE_RECEIVE();
}

void setup_serial(void)
{
  Serial.end();
  DISABLE_RXTX_PINS();
#ifdef DEBUG_SERIAL
#ifdef SOFTWARE_SERIAL
  softSerial.begin(9600);
  pinMode(SOFTWARE_SERIAL_RX, INPUT);
  pinMode(SOFTWARE_SERIAL_TX, OUTPUT);
#endif
#endif
}

void write_dataport(uint8_t ch)
{
  while (READ_IBFA() != 0);
  DATAPORT_MODE_TRANS();
  WRITE_DATAPORT(ch);
  STB_LOW();
  STB_HIGH();
  DATAPORT_MODE_RECEIVE();
}

uint8_t read_dataport(void)
{
  uint8_t byt;

  while (READ_OBFA() != 0);
  ACK_LOW();
  byt = READ_DATAPORT();
  ACK_HIGH();
  return byt;
}

static uint8_t unit;
static uint16_t buf;
static uint16_t blk;

void get_unit_buf_blk(void)
{
  unit = read_dataport();
#ifdef DEBUG_SERIAL
  SERIALPORT()->print("0000 unit=");
  SERIALPORT()->println(unit, HEX);
#endif
  buf = read_dataport();
  buf |= (((uint16_t)read_dataport()) << 8);
#ifdef DEBUG_SERIAL
  SERIALPORT()->print("0000 buf=");
  SERIALPORT()->println(buf, HEX);
#endif
  blk = read_dataport();
  blk |= (((uint16_t)read_dataport()) << 8);
#ifdef DEBUG_SERIAL
  SERIALPORT()->print("0000 blk=");
  SERIALPORT()->println(blk, HEX);
#endif
}

uint8_t hex_digit(uint8_t ch)
{
  if (ch < 10) return ch + '0';
  return ch - 10 + 'A';
}

void set_blockdev_filename(char *blockdev_filename, uint8_t fileno)
{
  blockdev_filename[8] = hex_digit(fileno >> 4);
  blockdev_filename[9] = hex_digit(fileno & 0x0F);
}

uint8_t check_change_filesystem(uint8_t current_filesystem)
{
  if (last_drive == current_filesystem)
    return 1;

  if (last_drive < 2)
  {
    f_close(&slotfile);
    f_unmount(last_drive == 0 ? blockvolzero : blockvolone);
  }
  last_drive = 255;
  if (current_filesystem < 2)
  {
    if (f_mount(&fs, current_filesystem == 0 ? blockvolzero : blockvolone, 0) != FR_OK)
      return 0;
    if (f_open(&slotfile, current_filesystem == 0 ? blockdev0_filename : blockdev1_filename, FA_READ | FA_WRITE) != FR_OK)
    {
      f_unmount(current_filesystem == 0 ? blockvolzero : blockvolone);
      return 0;
    }
  }
  last_drive = current_filesystem;
  return 1;
}

void initialize_drive(void)
{
  if (unit & 0x80)
  {
    if (slot1_state == SLOT_STATE_NODEV)
    {
      if (slot1_fileno == 255)
      {
        if (slot0_state == SLOT_STATE_NODEV)
        {
          if (disk_initialize(0) == 0)
            slot0_state = slot1_state = SLOT_STATE_WIDEDEV;
        }
      } else if (slot1_fileno == 0)
      {
        if (disk_initialize(1) == 0)
          slot1_state = SLOT_STATE_BLOCKDEV;
      } else
      {
        check_change_filesystem(255);
        set_blockdev_filename(blockdev1_filename, slot1_fileno);
        if (check_change_filesystem(1))
          slot1_state = SLOT_STATE_FILEDEV;
      }
    }
  } else
  {
    if (slot0_state == SLOT_STATE_NODEV)
    {
      if (slot0_fileno == 255)
      {
        if (slot1_state == SLOT_STATE_NODEV)
        {
          if (disk_initialize(0) == 0)
            slot0_state = slot1_state = SLOT_STATE_WIDEDEV;
        }
      } else if (slot0_fileno == 0)
      {
        if (disk_initialize(0) == 0)
          slot0_state = SLOT_STATE_BLOCKDEV;
      } else
      {
        check_change_filesystem(255);
        set_blockdev_filename(blockdev0_filename, slot0_fileno);
        if (check_change_filesystem(0))
          slot0_state = SLOT_STATE_FILEDEV;
      }
    }
  }
}

void unmount_drive(void)
{
  if (unit & 0x80)
  {
    switch (slot1_state)
    {
      case SLOT_STATE_NODEV:
        return;
      case SLOT_STATE_WIDEDEV:
      case SLOT_STATE_BLOCKDEV:
        slot1_state = SLOT_STATE_NODEV;
        return;
      case SLOT_STATE_FILEDEV:
        check_change_filesystem(255);
        slot1_state = SLOT_STATE_NODEV;
        return;
    }
  } else
  {
    switch (slot0_state)
    {
      case SLOT_STATE_NODEV:
        return;
      case SLOT_STATE_WIDEDEV:
      case SLOT_STATE_BLOCKDEV:
        slot0_state = SLOT_STATE_NODEV;
        return;
      case SLOT_STATE_FILEDEV:
        check_change_filesystem(255);
        slot0_state = SLOT_STATE_NODEV;
        return;
    }
  }
}

uint8_t check_unit_nodev(void)
{
  if (unit & 0x80)
  {
    if (slot1_state == SLOT_STATE_NODEV)
    {
      write_dataport(0x28);
      return 0;
    }
  } else
  {
    if (slot0_state == SLOT_STATE_NODEV)
    {
      write_dataport(0x28);
      return 0;
    }
  }
  return 1;
}

void do_status(void)
{
  get_unit_buf_blk();
  if (check_unit_nodev())
    write_dataport(0x00);
}

static uint32_t blockloc;

uint32_t calculate_block_location(uint8_t voltype)
{
  uint8_t unitshift = unit & ((voltype == SLOT_STATE_WIDEDEV) ? 0xF0 : 0x70);
  blockloc = ((uint32_t)blk) | (((uint32_t)(unitshift)) << 12);
}

uint32_t calculate_file_location(void)
{
  blockloc = ((uint32_t)blk) << 9;
}

void do_read(void)
{
  UINT br;
  uint8_t buf[512];

  get_unit_buf_blk();
  if (check_unit_nodev() == 0) return;
  if (unit & 0x80)
  {
    switch (slot1_state)
    {
      case SLOT_STATE_WIDEDEV:
      case SLOT_STATE_BLOCKDEV:
        calculate_block_location(slot1_state);
        if (disk_read(slot1_state == SLOT_STATE_BLOCKDEV ? 1 : 0, buf, blockloc, 1) != 0)
        {
          write_dataport(0x27);
          return;
        }
        break;
      case SLOT_STATE_FILEDEV:
        if (!check_change_filesystem(1))
        {
          write_dataport(0x27);
          return;
        }
        calculate_file_location();
        if ((f_lseek(&slotfile, blockloc) != FR_OK) ||
            (f_read(&slotfile, buf, 512, &br) != FR_OK) ||
            (br != 512))
        {
          write_dataport(0x27);
          return;
        }
        break;
    }
  } else
  {
    switch (slot0_state)
    {
      case SLOT_STATE_WIDEDEV:
      case SLOT_STATE_BLOCKDEV:
        calculate_block_location(slot0_state);
        if (disk_read(0, buf, blockloc, 1) != 0)
        {
          write_dataport(0x27);
          return;
        }
        break;
      case SLOT_STATE_FILEDEV:
        if (!check_change_filesystem(0))
        {
          write_dataport(0x27);
          return;
        }
        calculate_file_location();
        if ((f_lseek(&slotfile, blockloc) != FR_OK) ||
            (f_read(&slotfile, buf, 512, &br) != FR_OK) ||
            (br != 512))
        {
          write_dataport(0x27);
          return;
        }
        break;
    }
  }
  write_dataport(0x00);
  DATAPORT_MODE_TRANS();
  for (uint16_t i = 0; i < 512; i++)
  {
    while (READ_IBFA() != 0);
    WRITE_DATAPORT(buf[i]);
    STB_LOW();
    STB_HIGH();
  }
  DATAPORT_MODE_RECEIVE();
}

void do_write(void)
{
  UINT br;
  uint8_t buf[512];

  get_unit_buf_blk();
  if (check_unit_nodev() == 0) return;
  write_dataport(0x00);

  for (uint16_t i = 0; i < 512; i++)
  {
    while (READ_OBFA() != 0);
    ACK_LOW();
    buf[i] = READ_DATAPORT();
    ACK_HIGH();
  }

  if (unit & 0x80)
  {
    switch (slot1_state)
    {
      case SLOT_STATE_WIDEDEV:
      case SLOT_STATE_BLOCKDEV:
        calculate_block_location(slot1_state);
        disk_write(slot1_state == SLOT_STATE_BLOCKDEV ? 1 : 0, buf, blockloc, 1);
        break;
      case SLOT_STATE_FILEDEV:
        if (!check_change_filesystem(1))
          return;
        calculate_file_location();
        if ((f_lseek(&slotfile, blockloc) != FR_OK) ||
            (f_write(&slotfile, buf, 512, &br) != FR_OK) ||
            (br != 512))
          return;
        break;
    }
  } else
  {
    switch (slot0_state)
    {
      case SLOT_STATE_WIDEDEV:
      case SLOT_STATE_BLOCKDEV:
        calculate_block_location(slot0_state);
        disk_write(0, buf, blockloc, 1);
        break;
      case SLOT_STATE_FILEDEV:
        if (!check_change_filesystem(0))
          return;
        calculate_file_location();
        if ((f_lseek(&slotfile, blockloc) != FR_OK) ||
            (f_write(&slotfile, buf, 512, &br) != FR_OK) ||
            (br != 512))
          return;
        break;
    }
  }
  return;
}

void do_format(void)
{
  get_unit_buf_blk();
  if (check_unit_nodev() == 0) return;
  write_dataport(0x00);
}

void write_zeros(uint16_t num)
{
  DATAPORT_MODE_TRANS();
  while (num > 0)
  {
    while (READ_IBFA() != 0);
    WRITE_DATAPORT(0x00);
    STB_LOW();
    STB_HIGH();
    num--;
  }
  DATAPORT_MODE_RECEIVE();
}

void do_set_volume(uint8_t cmd)
{
  get_unit_buf_blk();
  unit = 0x80;
  unmount_drive();
  unit = 0x0;
  unmount_drive();
  slot0_fileno = blk & 0xFF;
  slot1_fileno = (blk >> 8);
  if (cmd != 6) write_eeprom();
  unit = 0x80;
  initialize_drive();
  unit = 0x0;
  initialize_drive();
  write_dataport(0x00);
  if (cmd != 4) write_zeros(512);
}

void do_get_volume(void)
{
  get_unit_buf_blk();
  write_dataport(0x00);
  write_dataport(slot0_fileno);
  write_dataport(slot1_fileno);
  write_zeros(510);
}

#ifdef USE_ETHERNET
void do_initialize_ethernet(void)
{
  uint8_t mac_address[6];
#ifdef DEBUG_SERIAL
  SERIALPORT()->println("initialize ethernet");
#endif
  for (uint8_t i = 0; i < 6; i++)
  {
    mac_address[i] = read_dataport();
#ifdef DEBUG_SERIAL
    SERIALPORT()->print(mac_address[i], HEX);
    SERIALPORT()->print(" ");
#endif
  }
  if (ethernet_initialized)
    eth.end();
  if (eth.begin(mac_address))
  {
#ifdef DEBUG_SERIAL
    SERIALPORT()->println("initialized");
#endif
    ethernet_initialized = 1;
    write_dataport(0);
    return 0;
  }
#ifdef DEBUG_SERIAL
  SERIALPORT()->println("not initialized");
#endif
  ethernet_initialized = 0;
  write_dataport(1);
  return 1;
}

void do_poll_ethernet(void)
{
  uint16_t len;
#ifdef DEBUG_SERIAL
  SERIALPORT()->println("poll eth");
#endif
  if (ethernet_initialized)
  {
    len = read_dataport();
    len |= ((uint16_t)read_dataport()) << 8;
#ifdef DEBUG_SERIAL
    SERIALPORT()->print("read len ");
    SERIALPORT()->println(len, HEX);
#endif
    len = eth.readFrame(NULL, len);
#ifdef DEBUG_SERIAL
    SERIALPORT()->print("recv len ");
    SERIALPORT()->println(len, HEX);
#endif
  } else
  {
    write_dataport(0);
    write_dataport(0);
  }
}

void do_send_ethernet(void)
{
  uint16_t len;
#ifdef DEBUG_SERIAL
  SERIALPORT()->println("send eth");
#endif
  if (ethernet_initialized)
  {
    len = read_dataport();
    len |= ((uint16_t)read_dataport()) << 8;
#ifdef DEBUG_SERIAL
    SERIALPORT()->print("len ");
    SERIALPORT()->println(len, HEX);
#endif
    eth.sendFrame(NULL, len);
  }
  write_dataport(0);
}
#endif

const uint8_t bootblocks[] PROGMEM = {  
  0xea, 0xa9, 0x20, 0x85, 0xf0, 0xa9, 0x60, 0x85, 0xf3, 0xa5, 0x43, 0x4a,
  0x4a, 0x4a, 0x4a, 0x29, 0x07, 0x09, 0xc0, 0x85, 0xf2, 0xa0, 0x00, 0x84,
  0xf1, 0x88, 0xb1, 0xf1, 0x85, 0xf1, 0x20, 0x93, 0xfe, 0x20, 0x89, 0xfe,
  0x20, 0x58, 0xfc, 0x20, 0xa2, 0x09, 0xa9, 0x00, 0x85, 0x25, 0x20, 0x22,
  0xfc, 0xa5, 0x25, 0x85, 0xf5, 0x85, 0xf6, 0x20, 0x90, 0x09, 0xa9, 0x00,
  0x85, 0x24, 0xa5, 0x25, 0x20, 0xe3, 0xfd, 0xe6, 0x24, 0x20, 0x7a, 0x09,
  0x20, 0x04, 0x09, 0xa9, 0x14, 0x85, 0x24, 0xa5, 0x25, 0x20, 0xe3, 0xfd,
  0xe6, 0x24, 0xa5, 0x43, 0x09, 0x80, 0x85, 0x43, 0x20, 0x7a, 0x09, 0x20,
  0x04, 0x09, 0xa5, 0x43, 0x29, 0x7f, 0x85, 0x43, 0xe6, 0x25, 0xa5, 0x25,
  0xc9, 0x10, 0x90, 0xbe, 0xa9, 0x00, 0x85, 0x24, 0xa9, 0x12, 0x85, 0x25,
  0x20, 0x22, 0xfc, 0xa2, 0x14, 0x20, 0x66, 0x09, 0x20, 0x61, 0x09, 0xa9,
  0x0a, 0x85, 0x24, 0xa5, 0xf7, 0x20, 0xf8, 0x08, 0xa9, 0x14, 0x85, 0x24,
  0x20, 0x5c, 0x09, 0xa9, 0x1e, 0x85, 0x24, 0xa5, 0xf8, 0x20, 0xf8, 0x08,
  0xa9, 0x0a, 0x85, 0x24, 0x20, 0xca, 0x08, 0x85, 0xf5, 0x20, 0xf8, 0x08,
  0xa9, 0x1e, 0x85, 0x24, 0x20, 0xca, 0x08, 0x85, 0xf6, 0x20, 0xf8, 0x08,
  0x20, 0x8c, 0x09, 0x4c, 0xb7, 0x09, 0xa5, 0xf7, 0x85, 0xf5, 0xa5, 0xf8,
  0x85, 0xf6, 0x20, 0x90, 0x09, 0x68, 0x68, 0x4c, 0xb7, 0x09, 0x20, 0x0c,
  0xfd, 0xc9, 0x9b, 0xf0, 0xe9, 0xc9, 0xa1, 0xf0, 0x20, 0xc9, 0xe1, 0x90,
  0x03, 0x38, 0xe9, 0x20, 0xc9, 0xc1, 0x90, 0x04, 0xc9, 0xc7, 0x90, 0x0b,
  0xc9, 0xb0, 0x90, 0xe2, 0xc9, 0xba, 0xb0, 0xde, 0x29, 0x0f, 0x60, 0x38,
  0xe9, 0x07, 0x29, 0x0f, 0x60, 0xa9, 0xff, 0x60, 0xc9, 0xff, 0xf0, 0x03,
  0x4c, 0xe3, 0xfd, 0xa9, 0xa1, 0x4c, 0xed, 0xfd, 0xa2, 0x00, 0xb0, 0x25,
  0xad, 0x05, 0x10, 0x30, 0x20, 0xad, 0x04, 0x10, 0x29, 0xf0, 0xc9, 0xf0,
  0xd0, 0x17, 0xad, 0x04, 0x10, 0x29, 0x0f, 0xf0, 0x10, 0x85, 0xf9, 0xbd,
  0x05, 0x10, 0x09, 0x80, 0x20, 0xed, 0xfd, 0xe8, 0xe4, 0xf9, 0xd0, 0xf3,
  0x60, 0x4c, 0x66, 0x09, 0xbc, 0xce, 0xcf, 0xa0, 0xd6, 0xcf, 0xcc, 0xd5,
  0xcd, 0xc5, 0xbe, 0x00, 0xc3, 0xc1, 0xd2, 0xc4, 0xa0, 0xb1, 0xba, 0x00,
  0xc4, 0xc1, 0xce, 0xa0, 0xdd, 0xdb, 0xa0, 0xd6, 0xcf, 0xcc, 0xd5, 0xcd,
  0xc5, 0xa0, 0xd3, 0xc5, 0xcc, 0xc5, 0xc3, 0xd4, 0xcf, 0xd2, 0x8d, 0x00,
  0xa9, 0xb2, 0x8d, 0x41, 0x09, 0xa2, 0x0c, 0x4c, 0x66, 0x09, 0xbd, 0x30,
  0x09, 0xf0, 0x0e, 0x20, 0xed, 0xfd, 0xe8, 0xd0, 0xf5, 0xa9, 0x00, 0x85,
  0x44, 0xa9, 0x10, 0x85, 0x45, 0x60, 0xa9, 0x01, 0x85, 0x42, 0x20, 0x71,
  0x09, 0xa9, 0x02, 0x85, 0x46, 0xa9, 0x00, 0x85, 0x47, 0x4c, 0xf0, 0x00,
  0xa9, 0x07, 0xd0, 0x02, 0xa9, 0x06, 0x85, 0x42, 0x20, 0x71, 0x09, 0xa5,
  0xf5, 0x85, 0x46, 0xa5, 0xf6, 0x85, 0x47, 0x4c, 0xf0, 0x00, 0xa9, 0x05,
  0x85, 0x42, 0x20, 0x71, 0x09, 0x20, 0xf0, 0x00, 0xad, 0x00, 0x10, 0x85,
  0xf7, 0xad, 0x01, 0x10, 0x85, 0xf8, 0x60, 0xa9, 0x00, 0x85, 0xf1, 0x6c,
  0xf1, 0x00
};  

#define NUMBER_BOOTBLOCKS (sizeof(bootblocks)/(512*sizeof(uint8_t)))

void do_send_bootblock()
{
  get_unit_buf_blk();
  write_dataport(0x00);
  if (blk >= NUMBER_BOOTBLOCKS) blk = 0;
  blk <<= 9;
  DATAPORT_MODE_TRANS();
  for (uint16_t i = 0; i < 512; i++)
  {
    while (READ_IBFA() != 0);
    WRITE_DATAPORT(pgm_read_byte(&bootblocks[blk + i]));
    STB_LOW();
    STB_HIGH();
  }
  DATAPORT_MODE_RECEIVE();
}

void do_command()
{
  uint8_t cmd = read_dataport();
#ifdef DEBUG_SERIAL
  SERIALPORT()->print("0000 cmd=");
  SERIALPORT()->println(cmd, HEX);
#endif
  switch (cmd)
  {
    case 0:    do_status();
      break;
    case 1:    do_read();
      break;
    case 2:    do_write();
      break;
    case 3:    do_format();
      break;
    case 4:
    case 6:
    case 7:    do_set_volume(cmd);
      break;
    case 5:    do_get_volume();
      break;
#ifdef USE_ETHERNET
    case 0x10: do_initialize_ethernet();
      break;
    case 0x11: do_poll_ethernet();
      break;
    case 0x12: do_send_ethernet();
      break;
#endif
    case 13+128:
    case 32+128:  do_send_bootblock();
      break;
    default:      write_dataport(0x27);
      break;
  }
}

int freeRam ()
{
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void setup()
{
  setup_pins();
  setup_serial();
  read_eeprom();

  unit = 0;
  initialize_drive();
  unit = 0x80;
  initialize_drive();

#ifdef DEBUG_SERIAL
  SERIALPORT()->println("0000");
  SERIALPORT()->print("d=");
  SERIALPORT()->print(sizeof(fs));
  SERIALPORT()->print(" f=");
  SERIALPORT()->print(freeRam());
  SERIALPORT()->print(" s=");
  SERIALPORT()->print(slot0_state);
  SERIALPORT()->print(" ");
  SERIALPORT()->println(slot1_state);
  SERIALPORT()->flush();
#endif

  DATAPORT_MODE_RECEIVE();
}

void loop()
{
  uint8_t instr = read_dataport();
  if (instr == 0xAC) do_command();
}
