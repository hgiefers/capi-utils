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

#include "common.h"

int main (int argc, char *argv[])
{

  struct capiFlash flash;

  uint32_t vsec = 0x404;
  uint32_t addr_reg = 0x450;
  uint32_t size_reg = 0x454;
  uint32_t cntl_reg = 0x458;
  uint32_t data_reg = 0x45c;
  CHECK( flash_init(argc, argv, &flash, vsec, addr_reg, size_reg, cntl_reg,
      data_reg) );

  CHECK( flash_wait(&flash) );
  
  uint32_t address = 0x800000;  //user partion.
  CHECK( flash_setup(&flash, address) );

  CHECK( flash_program(&flash) );

  CHECK( flash_verify(&flash) );

  CHECK( flash_close(&flash) );

  return 0;

}

