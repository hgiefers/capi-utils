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

int flash_init(int argc, char *argv[], struct capiFlash *flash, uint32_t vsec,
    uint32_t addr_reg, uint32_t size_reg, uint32_t cntl_reg, uint32_t data_reg,
    uint32_t subsys_pci)
{
  if (argc < 3) {
    printf("Usage: capi_flash <rbf_file> <card#>\n\n");
    exit(-1);
  }
  strncpy (flash->fpgaBinary_file, argv[1], CF_FILE_SIZE);

  if ((flash->fpgaBinary = open(flash->fpgaBinary_file, O_RDONLY)) < 0) {
    printf("Can not open %s\n",flash->fpgaBinary_file);
    exit(-1);
  }

  strcpy(flash->cfg_file, "/sys/class/cxl/card");
  strncat(flash->cfg_file, argv[2], (CF_FILE_SIZE - 64));
  strcat(flash->cfg_file, "/device/config");

  if ((flash->CFG = open(flash->cfg_file, O_RDWR)) < 0) {
    printf("Can not open %s\n",flash->cfg_file);
    exit(-1);
  }

  uint32_t temp,vendor,device;
  lseek(flash->CFG, 0, SEEK_SET);
  CHECK( read(flash->CFG, &temp, 4) );
  vendor = temp & 0xFFFF;
  device = (temp >> 16) & 0xFFFF;  
  printf("Device ID: %04X\n", device);
  printf("Vendor ID: %04X\n", vendor);

  if ( (vendor != 0x1014) || (( device != 0x0477) && (device != 0x04cf))) {
	printf("Unknown Vendor or Device ID\n");
	exit(-1);
  }
  uint32_t subsys;
  lseek(flash->CFG, 44, SEEK_SET);
  CHECK( read(flash->CFG, &temp, 4) );
  subsys = (temp >> 16) & 0xFFFF;
  printf("Subsys ID: %04X\n", subsys);
 
  if ( (subsys != 0x04af) && (subsys != subsys_pci) ) {
    printf("Unknown Subsystem ID\n");
    exit(-1);
  }
 
  lseek(flash->CFG, vsec, SEEK_SET);
  CHECK( read(flash->CFG, &temp,4) );
  printf("  VSEC Length/VSEC Rev/VSEC ID: 0x%08X\n", temp);
  if ( (temp & 0x08000000) == 0x08000000 ) {
    printf("    Version 0.12\n\n");
    flash->addr_reg = addr_reg;
    flash->size_reg = size_reg;
    flash->cntl_reg = cntl_reg;
    flash->data_reg = data_reg;
  } else {
    printf("    Version 0.10\n\n");
    flash->addr_reg = 0x920;
    flash->size_reg = 0x924;
    flash->cntl_reg = 0x928;
    flash->data_reg = 0x92c;
  }
 
  // Set stdout to autoflush
  setvbuf(stdout, NULL, _IONBF, 0);

// -------------------------------------------------------------------------------
// Reset Any Previously Aborted Sequences
// -------------------------------------------------------------------------------
  temp = 0;
  lseek(flash->CFG, flash->cntl_reg, SEEK_SET);
  CHECK( write(flash->CFG,&temp,4) );
  return CF_SUCCESS;
}

int flash_wait(struct capiFlash *flash)
{
  int temp = 0;
  lseek(flash->CFG, flash->cntl_reg, SEEK_SET);
  CHECK( read(flash->CFG,&temp,4) );

  flash->st = time(NULL);
  flash->lt = flash->st;
  int ec = CF_SUCCESS;
  int cp = 1;

  while (cp == 1) {
    lseek(flash->CFG, flash->cntl_reg, SEEK_SET);
    CHECK( read(flash->CFG,&temp,4) );
    if ( (temp & 0x80000000) == 0x80000000 ) {
      cp = 0;
    }
    flash->ct = time(NULL);
    if ((flash->ct - flash->lt) > 5) {
      printf(".");
      flash->lt = flash->ct;
    }
    if ((flash->ct - flash->st) > 120) {
      printf ("\nFAILURE --> Flash not ready after 2 min\n");
      cp = 0;
      ec = CF_FLASH_TIME_OUT;
    }
  }
  printf("\n");

  return ec;
}

int flash_setup(struct capiFlash *flash, uint32_t user_partition)
{
  int temp = 0;
  int ec = CF_SUCCESS;
  off_t fsize;
  struct stat tempstat;
  int num_blocks;
  flash->address = user_partition;  //user partion.
  if (stat(flash->fpgaBinary_file, &tempstat) != 0) {
    fprintf(stderr, "Cannot determine size of %s: %s\n", 
        flash->fpgaBinary_file, strerror(errno));
    exit(-1);
  } else {
    fsize = tempstat.st_size;
  }
  num_blocks = flash->num_blocks = fsize / (64 * 1024 *4);
  printf("Programming User Partition with %s\n", flash->fpgaBinary_file);
  printf("  Program ->  for Size: %d in blocks (32K Words or 128K Bytes)\n\n",
      num_blocks);

  flash->set = time(NULL);  
  //# -------------------------------------------------------------------------------
  //# Setup for Program From Flash
  //# -------------------------------------------------------------------------------
  lseek(flash->CFG, flash->addr_reg, SEEK_SET);
  CHECK( write(flash->CFG,&flash->address,4) );
  
  lseek(flash->CFG, flash->size_reg, SEEK_SET);
  CHECK( write(flash->CFG,&num_blocks,4) );

  temp = 0x04000000;
  lseek(flash->CFG, flash->cntl_reg, SEEK_SET);
  CHECK( write(flash->CFG,&temp,4) );

  printf("Erasing Flash\n");

  //# -------------------------------------------------------------------------------
  //# Wait for Flash Erase to complete.
  //# -------------------------------------------------------------------------------
  flash->st = flash->lt = time(NULL);
  int cp = 1;

  while (cp == 1) {
    lseek(flash->CFG, flash->cntl_reg, SEEK_SET);
    CHECK( read(flash->CFG,&temp,4) );
    if ( ( (temp & 0x00008000) == 0x00000000 ) &&
         ( (temp & 0x00004000) == 0x00004000 ) ){
      cp = 0;
    }
    flash->ct = time(NULL);
    if ((flash->ct - flash->lt) > 5) {
      printf(".");
      flash->lt = flash->ct;
    }
    if ((flash->ct - flash->st) > 240) {
      printf ("\nFAILURE --> Erase did not complete after 4 min\n");
      cp = 0;
      ec = CF_ERASE_TIME_OUT;
    }
  }
  printf("\n");

  return ec;
}

int flash_program(struct capiFlash *flash)
{
  int dat, dif;
  int temp = 0;
  int cp = 0;
  int ec = CF_SUCCESS;
  flash->eet = flash->spt = time(NULL);
  int num_blocks = flash->num_blocks;
  //# -------------------------------------------------------------------------------
  //# Program Flash                        
  //# -------------------------------------------------------------------------------

  int prtnr = 0;
  if (ec == 0) {
    printf("\n\nProgramming Flash\n");
  
    int bc = 0;
    int i;
    printf("Writing Block: %d        \r", bc);
    for(i=0; i<(64*1024*(num_blocks+1)); i++) {
      dif = read(flash->fpgaBinary,&dat,4);
      if (!(dif)) {
        dat = 0xFFFFFFFF;
      }

      // -------------------------------------------------------------------------------
      // Poll for flash port to be ready - offset 0x58 bit 12(LE) = 1 means busy
      // -------------------------------------------------------------------------------

      flash->st = time(NULL);
      flash->lt = flash->st;
      cp = 1;

      while (cp == 1) {
	      lseek(flash->CFG, flash->cntl_reg, SEEK_SET);
	      CHECK( read(flash->CFG,&temp,4) );
	      if ( (temp & 0x00001000) == 0x00000000 ) {
	        cp = 0;
	      } else {
	        ++prtnr;
	      }

	      flash->ct = time(NULL);
	      if ((flash->ct - flash->lt) > 5) {
	        printf("...");
	        flash->lt = flash->ct;
	      }
	      if ((flash->ct - flash->st) > 30) {
	        printf ("\nFAILURE --> Flash Port not ready after 30 seconds\n");
	        cp = 0;
	        ec = CF_FLASH_PORT_NOT_READY;
	        i = (64*1024*(num_blocks+1));
	      }
      }

      lseek(flash->CFG, flash->data_reg, SEEK_SET);
      CHECK( write(flash->CFG,&dat,4) );
    
      if (((i+1) % (512)) == 0) {
        printf("Writing Buffer: %d        \r",bc);
        bc++;
      }
    }
  }

  printf("\n\n");
  
  //# -------------------------------------------------------------------------------
  //# Wait for Flash Program to complete.
  //# -------------------------------------------------------------------------------
  flash->st = flash->lt = time(NULL);
  cp = 1;

  while (cp == 1) {
    lseek(flash->CFG, flash->cntl_reg, SEEK_SET);
    CHECK( read(flash->CFG,&temp,4) );
    if ( (temp & 0x40000000) == 0x40000000 ) { 
      cp = 0;
    }
    flash->ct = time(NULL);
    if ((flash->ct - flash->lt) > 5) {
      printf(".");
      flash->lt = flash->ct;
    }
    if ((flash->ct - flash->st) > 120) {
      printf ("\nFAILURE --> Programming did not complete after 2 min\n");
      cp = 0;
      ec = CF_PROG_ERROR;
    }
  }
  printf("\n");

  flash->ept = time(NULL);

  //# -------------------------------------------------------------------------------
  //# Reset Program Sequence
  //# -------------------------------------------------------------------------------
  dat=0;
  lseek(flash->CFG,flash->cntl_reg,SEEK_SET);
  CHECK( write(flash->CFG,&dat,4) );

  //# -------------------------------------------------------------------------------
  //# Wait for Flash to be ready
  //# -------------------------------------------------------------------------------
  flash->st = flash->lt = time(NULL);
  cp = 1;

  while (cp == 1) {
    lseek(flash->CFG, flash->cntl_reg, SEEK_SET);
    CHECK( read(flash->CFG,&temp,4) );
    if ( (temp & 0x80000000) == 0x80000000 ) { 
      cp = 0;
    }
    flash->ct = time(NULL);
    if ((flash->ct - flash->lt) > 5) {
      printf(".");
      flash->lt = flash->ct;
    }
    if ((flash->ct - flash->st) > 120) {
      printf ("\nFAILURE --> Flash not ready after 2 min\n");
      cp = 0;
      ec = CF_FLASH_NOT_READY;
    }
  }
  printf("\n");

  return ec;
}

int flash_verify(struct capiFlash *flash)
{
  int dat, dif, edat;
  int temp = 0;
  int ec = CF_SUCCESS;
  flash->svt = time(NULL);

  //# -------------------------------------------------------------------------------
  //# Verify Flash Programmming
  //# -------------------------------------------------------------------------------

  if (ec == 0) {
    printf("Verifying Flash\n");

    lseek(flash->fpgaBinary, 0, SEEK_SET);   // Reset to beginning of file

    int i, bc = 0;
    uint32_t raddress = flash->address;
    for(i=0; i<(64*1024*(flash->num_blocks+1)); i++) {

      dif = read(flash->fpgaBinary,&edat,4);
      if (!(dif)) {
        edat = 0xFFFFFFFF;
      }

      if ((i % 512) == 0) {
        dat = 0;
        lseek(flash->CFG, flash->cntl_reg, SEEK_SET);
        CHECK( write(flash->CFG,&dat,4) );

        //# -------------------------------------------------------------------------------
        //# Wait for Flash to be ready
        //# -------------------------------------------------------------------------------
        flash->st = flash->lt = time(NULL);
        int cp = 1;
      
        while (cp == 1) {
          lseek(flash->CFG, flash->cntl_reg, SEEK_SET);
          CHECK( read(flash->CFG,&temp,4) );
          if ( (temp & 0x80000000) == 0x80000000 ) { 
            cp = 0;
          }
          flash->ct = time(NULL);
          if ((flash->ct - flash->lt) > 5) {
            printf(".");
            flash->lt = flash->ct;
          }
          if ((flash->ct - flash->st) > 120) {
            printf ("\nFAILURE --> Flash not ready after 2 min\n");
            cp = 0;
            ec = CF_FLASH_NOT_READY;
          }
        }

        //# -------------------------------------------------------------------------------
        //# Setup for Reading From Flash
        //# -------------------------------------------------------------------------------
        lseek(flash->CFG, flash->addr_reg, SEEK_SET);
        CHECK( write(flash->CFG,&raddress,4) );
        raddress += 0x200;
      
        dat = 0x1FF;
        lseek(flash->CFG, flash->size_reg, SEEK_SET);
        CHECK( write(flash->CFG,&dat,4) );
      
        dat = 0x08000000;
        lseek(flash->CFG, flash->cntl_reg, SEEK_SET);
        CHECK( write(flash->CFG,&dat,4) );
      }
  
      lseek(flash->CFG, flash->data_reg, SEEK_SET);
      CHECK( read(flash->CFG,&dat,4) );
  
      if (edat != dat) {
        int ma = raddress + (i % 512) - 0x200;
        printf("Data Miscompare @: %08x --> %08x expected %08x\r",ma, dat, edat);
      }
  
      if (((i+1) % (64*1024)) == 0) {
        printf("Reading Block: %d        \r", bc);
        bc++;
      }
    }
  }
  flash->evt = time(NULL);
  printf("\n\n");

  return ec;
}

int flash_close(struct capiFlash *flash)
{

  //# -------------------------------------------------------------------------------
  //# Calculate and Print Elapsed Times
  //# -------------------------------------------------------------------------------
  time_t et = flash-> evt - flash->set;
  time_t eet = flash-> eet - flash->set;
  time_t ept = flash->ept - flash->spt;
  time_t evt = flash->evt - flash->svt;
  
  printf("Erase Time:   %d seconds\n", (int)eet);
  printf("Program Time: %d seconds\n", (int)ept);
  printf("Verify Time:  %d seconds\n", (int)evt);
  printf("Total Time:   %d seconds\n\n", (int)et);

  //# -------------------------------------------------------------------------------
  //# Reset Read Sequence
  //# -------------------------------------------------------------------------------
  int dat = 0;
  lseek(flash->CFG, flash->cntl_reg, SEEK_SET);
  CHECK( write(flash->CFG,&dat,4) );


  close(flash->fpgaBinary);
  close(flash->CFG);

  return CF_SUCCESS;
}

