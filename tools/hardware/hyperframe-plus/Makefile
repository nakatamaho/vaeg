# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

ifeq ($(strip $(OUT)),)
$(error Set OUT to an absolute, task-specific directory outside the checkout)
endif
ROOT := $(abspath .)
REPO := $(abspath ../../..)
BUILD := $(abspath $(OUT))
ifneq ($(filter $(REPO) $(REPO)/%,$(BUILD)),)
$(error OUT must be outside the checkout)
endif
CC ?= cc
PYTHON ?= python3
CFLAGS ?= -O2
STRICT = -std=c89 -pedantic -Wall -Wextra -Werror
INCLUDES = -Isrc -Itests

.PHONY: all test
all: $(BUILD)/test_hf $(BUILD)/hftest_mock

$(BUILD):
	mkdir -p "$@"

$(BUILD)/hf_cli.o: src/hf_test.c src/hf_io.h | $(BUILD)
	$(CC) $(CFLAGS) $(STRICT) $(INCLUDES) -Dmain=hf_cli_main -c $< -o $@

$(BUILD)/test_hf: src/hf_io.c tests/hf_mock_io.c tests/test_hf.c src/hf_io.h tests/hf_mock_io.h | $(BUILD)
	$(CC) $(CFLAGS) $(STRICT) $(INCLUDES) src/hf_io.c tests/hf_mock_io.c tests/test_hf.c -o $@

$(BUILD)/hftest_mock: src/hf_io.c tests/hf_mock_io.c tests/mock_main.c $(BUILD)/hf_cli.o src/hf_io.h tests/hf_mock_io.h | $(BUILD)
	$(CC) $(CFLAGS) $(STRICT) $(INCLUDES) src/hf_io.c tests/hf_mock_io.c tests/mock_main.c $(BUILD)/hf_cli.o -o $@

test: all
	$(BUILD)/test_hf
	$(PYTHON) -B tests/test_cli.py $(BUILD)/hftest_mock
	$(PYTHON) -B -m unittest discover -s tests -p test_reference.py -v
	$(PYTHON) -B -m unittest discover -s tests -p test_trace.py -v
