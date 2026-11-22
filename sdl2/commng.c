/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "compiler.h"
#include "np2.h"
#include "commng.h"
#include "cmver.h"
#include "cmjasts.h"

static UINT ncread(COMMNG self, BYTE *data) {
	(void)self;
	(void)data;
	return (0);
}

static UINT ncwrite(COMMNG self, BYTE data) {
	(void)self;
	(void)data;
	return (0);
}

static BYTE ncgetstat(COMMNG self) {
	(void)self;
	return (0xf0);
}

static VAEG_INTPTR ncmsg(COMMNG self, UINT msg, VAEG_INTPTR param) {
	(void)self;
	(void)msg;
	(void)param;
	return (0);
}

static void ncrelease(COMMNG self) {
	(void)self;
}

static const _COMMNG com_nc = {COMCONNECT_OFF, ncread, ncwrite, ncgetstat, ncmsg, ncrelease};

static FILE *printer_out;
static FILE *serial_in;
static FILE *serial_out;
static long serial_remaining;
static UINT printer_busy_frames;
static BOOL files_failed;

static BOOL close_file(FILE **file) {
	BOOL ok = (*file == NULL || fclose(*file) == 0);
	*file = NULL;
	return ok;
}

BOOL commng_files_close(void) {
	if (!close_file(&printer_out)) {
		files_failed = TRUE;
	}
	if (!close_file(&serial_in)) {
		files_failed = TRUE;
	}
	if (!close_file(&serial_out)) {
		files_failed = TRUE;
	}
	printer_busy_frames = 0;
	serial_remaining = 0;
	return files_failed ? FAILURE : SUCCESS;
}

/* Unbuffered captures are observable even while the emulator is paused. */
static BOOL open_file(FILE **file, const char *path, BOOL output) {
	if (path == NULL) {
		return TRUE;
	}
	*file = fopen(path, output ? "ab" : "rb");
	if (*file == NULL || (output && setvbuf(*file, NULL, _IONBF, 0) != 0)) {
		fprintf(stderr, "Error: cannot open device file: %s\n", path);
		return FALSE;
	}
	return TRUE;
}

BOOL commng_files_open(const char *printer, UINT busy_frames, const char *input,
                       const char *output) {
	files_failed = FALSE;
	if (!open_file(&serial_in, input, FALSE)) {
		return FAILURE;
	}
	/* Replay a finite input, not a tail of a growing host stream. */
	if (serial_in != NULL &&
	    (fseek(serial_in, 0, SEEK_END) != 0 || (serial_remaining = ftell(serial_in)) < 0 ||
	     fseek(serial_in, 0, SEEK_SET) != 0)) {
		fprintf(stderr, "Error: cannot size serial input file: %s\n", input);
		commng_files_close();
		return FAILURE;
	}
	if (!open_file(&serial_out, output, TRUE) || !open_file(&printer_out, printer, TRUE)) {
		commng_files_close();
		return FAILURE;
	}
	printer_busy_frames = busy_frames;
	return SUCCESS;
}

void commng_files_frame(void) {
	if (printer_busy_frames != 0) {
		printer_busy_frames--;
	}
}

BOOL commng_printer_attached(void) {
	return printer_out != NULL;
}

BOOL commng_printer_busy(void) {
	return printer_busy_frames != 0;
}

static UINT capture_byte(FILE *file, BYTE data) {
	if (file == NULL || ferror(file)) {
		return 0;
	}
	if (fputc(data, file) == EOF) {
		files_failed = TRUE;
		return 0;
	}
	return 1;
}

void commng_printer_strobe(BYTE data) {
	capture_byte(printer_out, data);
}

BOOL commng_serial_attached(void) {
	return serial_in != NULL || serial_out != NULL;
}

static UINT file_read(COMMNG self, BYTE *data) {
	int value;
	(void)self;
	if (serial_remaining == 0) {
		return 0;
	}
	value = fgetc(serial_in);
	if (value == EOF) {
		files_failed = TRUE;
		serial_remaining = 0;
		return 0;
	}
	serial_remaining--;
	*data = (BYTE)value;
	return 1;
}

static UINT file_write(COMMNG self, BYTE data) {
	(void)self;
	return capture_byte(serial_out, data);
}

static BYTE file_status(COMMNG self) {
	(void)self;
	/* CI inactive; active-low CTS and DCD asserted. Bit 5 also feeds
	 * the existing USART DSR status conversion in rs232c_i32. */
	return 0x90;
}

static const _COMMNG com_file = {COMCONNECT_SERIAL, file_read, file_write,
                                 file_status,       ncmsg,     ncrelease};

void commng_initialize(void) {
	cmvermouth_initialize();
}

COMMNG commng_create(UINT device) {
	COMMNG ret;

	ret = NULL;
	if (device == COMCREATE_MPU98II) {
		ret = cmvermouth_create();
	} else if (device == COMCREATE_SERIAL && commng_serial_attached()) {
		ret = (COMMNG)&com_file;
	} else if (device == COMCREATE_PRINTER) {
		if (np2oscfg.jastsnd) {
			ret = cmjasts_create();
		}
	}
	if (ret == NULL) {
		ret = (COMMNG)&com_nc;
	}
	return (ret);
}

void commng_destroy(COMMNG hdl) {
	if (hdl) {
		hdl->release(hdl);
	}
}
