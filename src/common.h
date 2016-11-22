/*
 * Copyright 2016 International Business Machines
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __COMMON_INCLUDED__
#define __COMMON_INCLUDED__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <termios.h>
#include <endian.h>
#include <time.h>
#include <assert.h>

#define CHECK(X) (assert((X) >= 0))
#define CF_FILE_SIZE 1024
#define CF_SUCCESS 0
#define CF_FLASH_TIME_OUT -1
#define CF_ERASE_TIME_OUT -2
#define CF_PROG_ERROR -4
#define CF_FLASH_NOT_READY -6
#define CF_FLASH_PORT_NOT_READY -99

struct capiFlash
{
  int CFG;
  int fpgaBinary;
  time_t ct, lt;
  time_t st, et;
  time_t eet, set;
  time_t ept, spt;
  time_t svt, evt;
  uint32_t address;
  char fpgaBinary_file[CF_FILE_SIZE];
  char cfg_file[CF_FILE_SIZE];
  int addr_reg, size_reg, cntl_reg, data_reg;
  int num_blocks;
};

int flash_init(int argc, char *argv[], struct capiFlash *flash);

int flash_wait(struct capiFlash *flash);

int flash_setup(struct capiFlash *flash, uint32_t user_partition);

int flash_program(struct capiFlash *flash);

int flash_verify(struct capiFlash *flash);

int flash_close(struct capiFlash *flash);

#endif

