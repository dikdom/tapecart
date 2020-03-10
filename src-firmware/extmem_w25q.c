/* tapecart - a tape port storage pod for the C64

   Copyright (C) 2013-2017  Ingo Korb <ingo@akana.de>
   All rights reserved.
   Idea by enthusi

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
   OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
   OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
   SUCH DAMAGE.


   extmem_w25q.c: Winbond W25Q flash chip external memory

*/

#include "config.h"
#include "bitbanging.h"
#include "spi.h"
#include "timer.h"
#include "watchdog.h"
#include "extmem.h"

#define CMD_WRITE_ENABLE       0x06
#define CMD_PAGE_PROGRAM       0x02
#define CMD_SECTOR_ERASE       0x20
#define CMD_READ_DATA          0x03
#define CMD_READ_STATUS_1      0x05
#define CMD_READ_DEVICE_ID     0x9F

#define STATUS1_BUSY           (1<<0)

static uint24 extmem_size;

static void spi_send_24bit(uint24 address) {
  spi_exchange_byte((address >> 16) & 0xff);
  spi_exchange_byte((address >>  8) & 0xff);
  spi_exchange_byte( address        & 0xff);
}

static void send_writeenable(void) {
  spi_set_ss(SPISS_LOW);
  spi_exchange_byte(CMD_WRITE_ENABLE);
  spi_set_ss(SPISS_HIGH);
}

void extmem_init(void) {
  spi_init();

  spi_set_ss(SPISS_LOW);
  spi_exchange_byte(CMD_READ_DEVICE_ID);
  uint8_t manufacturer_id = spi_exchange_byte(0xff);
  spi_exchange_byte(0xff); // drop flash type
  uint8_t size         = spi_exchange_byte(0xff);
  spi_set_ss(SPISS_HIGH);
  extmem_size = (uint24)2097152; // defaulting to 2MB
  if (manufacturer_id == 0xef) {
    if (size == 0x14) {
	  extmem_size = (uint24)1048576; // 1024*1024, W25Q80
	} else if (size == 0x15) {
	  extmem_size = (uint24)2097152; // 2*1024*1024, W25Q16
	} else if (size == 0x16) {
	  extmem_size = (uint24)4194304; // 4*1024*1024, W25Q32
	} else if (size == 0x17) {
	  extmem_size = (uint24)8388608; // 8*1024*1024, W25Q64
	} else if (size == 0x18) {
	  extmem_size = (uint24)16777215; // 16*1024*1024-1, W25Q128 - highest addressable space
	}
  }
}

void extmem_read_start(uint24 address) {
  spi_set_ss(SPISS_LOW);
  spi_exchange_byte(CMD_READ_DATA);
  spi_send_24bit(address);
}

uint8_t extmem_read_byte(bool last_byte) {
  return spi_exchange_byte(0xff);
}

void extmem_read_stop(void) {
  spi_set_ss(SPISS_HIGH);
}

void extmem_write_start(uint24 address) {
  send_writeenable();

  spi_set_ss(SPISS_LOW);
  spi_exchange_byte(CMD_PAGE_PROGRAM);
  spi_send_24bit(address);
}

void extmem_write_byte(uint8_t byte) {
  spi_exchange_byte(byte);
}

void extmem_write_stop(void) {
  spi_set_ss(SPISS_HIGH);
}

bool extmem_write_isbusy(void) {
  uint8_t result;

  spi_set_ss(SPISS_LOW);
  spi_exchange_byte(CMD_READ_STATUS_1);
  result = spi_exchange_byte(0xff);
  spi_set_ss(SPISS_HIGH);

  return (result & STATUS1_BUSY);
}

void extmem_erase(uint24 address) {
  send_writeenable();

  spi_set_ss(SPISS_LOW);
  spi_exchange_byte(CMD_SECTOR_ERASE);
  spi_send_24bit(address);
  spi_set_ss(SPISS_HIGH);
}

inline uint24 extmem_getsize() {
  return extmem_size;
}
