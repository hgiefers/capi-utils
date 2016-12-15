#
# Copyright 2016 International Business Machines
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

CC=gcc
CFLAGS=-Wall -W -g -O2

ARCH:= $(shell uname -p)
ifneq ($(ARCH),ppc64le)
  $(error $(ARCH) does not support CAPI)
endif

prefix=/usr/local

TARGETS=capi-flash-AlphaData7v3 capi-flash-AlphaDataKU60 capi-flash-AlphaDataKU115 capi-flash-BittwareVU095 capi-flash-Nallatech

COMMON=src/common.c
COMMON_OBJS=$(COMMON:.c=.o)

install_files = $(TARGETS) capi-flash-script.sh psl-devices

.PHONY: all 
all: $(TARGETS)

%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@

capi-flash-AlphaData7v3: $(COMMON_OBJS) src/capi_flash_ad7v3_user.o
	$(CC) $(CFLAGS) $^ -o $@

capi-flash-AlphaDataKU60: $(COMMON_OBJS) src/capi_flash_adku3_user.o
	$(CC) $(CFLAGS) $^ -o $@

capi-flash-AlphaDataKU115: $(COMMON_OBJS) src/capi_flash_adku115_user.o
	$(CC) $(CFLAGS) $^ -o $@

capi-flash-BittwareVU095: $(COMMON_OBJS) src/capi_flash_bwvu095_user.o
	$(CC) $(CFLAGS) $^ -o $@

capi-flash-Nallatech: $(COMMON_OBJS) src/capi_flash_nallatech_user.o
	$(CC) $(CFLAGS) $^ -o $@

install: $(TARGETS)
	@chmod a+x capi-flash-*
	@mkdir -p $(prefix)/capi-utils
	@cp $(install_files) $(prefix)/capi-utils
	@ln -sf $(prefix)/capi-utils/capi-flash-script.sh \
		$(prefix)/bin/capi-flash-script

.PHONY: uninstall
uninstall:
	@rm -rf $(prefix)/capi-utils
	@rm $(prefix)/bin/capi-flash-script

.PHONY: clean
clean:
	@rm -rf $(TARGETS) src/*.o

