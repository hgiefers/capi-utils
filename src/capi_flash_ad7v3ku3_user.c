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

  CHECK( flash_init(argc, argv, &flash) );

  CHECK( flash_wait(&flash) );
  
  uint32_t address = 0x800000;  //user partion.
  CHECK( flash_setup(&flash, address) );

  CHECK( flash_program(&flash) );

  CHECK( flash_verify(&flash) );

  CHECK( flash_close(&flash) );

  return 0;

}

