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
#include "selftest.h"
#include "codecnv.h"
#include "commng.h"
#include "machine/clockscale.h"
#include "bmsio.h"
#include "emsio.h"
#include "bkupmemva.h"
#include "cliopts.h"
#include "debug_harness.h"
#include "dosio.h"
#include "dropmedia.h"
#include "fddfile.h"
#include "fdd_d88.h"
#include "fdd_xdf.h"
#include "framedisp.h"
#include "hostfat.h"
#include "hostfat_snapshot.h"
#include "hostfat_manager.h"
#include "ini.h"
#include "machine/pccore.h"
#include "cpu/upd9002/memory.h"
#include "cpucore.h"
#include "diagnostics/upd9002_debug.h"
#include "iocore.h"
#include "iocoreva.h"
#include "kbdmap.h"
#include "kbdpaste.h"
#include "memoryva.h"
#include "mousestate.h"
#include "newdisk.h"
#include "np2.h"
#include "pacing.h"
#include "sound.h"
#include "opngen.h"
#include "profile.h"
#include "romcheck.h"
#include "romankana.h"
#include "sgp.h"
#include "sxsi.h"
#include "scsiio.h"
#include "scsicmd.h"
#include "scrnmng.h"
#include "splash.h"
#include "gui/gui.h"
#include "scrndraw.h"
#include "scrndrawva.h"
#include "sdrawva.h"
#include "soundmng.h"
#include "soundopts.h"
#include "strres.h"
#include "viewport.h"
#include "ymfmbridge.h"
#if defined(VAEG_UPD9002_M42_TESTING)
#include "tests/upd9002/direct_harness.h"
#include "tests/upd9002/fixtures.h"
#endif
#if defined(VAEG_UPD9002_M44_TESTING)
#include "tests/upd9002/state_scenario.h"
#include "tests/upd9002/statsave_boundary.h"
#endif
#if defined(VAEG_UPD9002_M46_TESTING)
#include "tests/upd9002/dispatch_normalization.h"
#endif
#if defined(VAEG_UPD780_INTEGRATION_TESTING)
#include "io/subsystem.h"
#include "tests/upd780/subsystem_integration.h"
#endif

static int fail(const char *name, const char *detail) {
	fprintf(stderr, "selftest: %s failed: %s\n", name, detail);
	return (FAILURE);
}

static int test_codecnv(void) {
	const char sjis_wave[] = {(char)0x81, (char)0x60, '\0'};
	UINT16 utf[4];
	char euc[8];
	char sjis[8];

	ZeroMemory(utf, sizeof(utf));
	codecnv_sjis2utf(utf, NELEMENTS(utf), sjis_wave, sizeof(sjis_wave));
	if ((utf[0] != 0xff5e) || (utf[1] != 0)) {
		return (fail("codecnv", "CP932 wave-dash maps incorrectly"));
	}

	ZeroMemory(euc, sizeof(euc));
	ZeroMemory(sjis, sizeof(sjis));
	codecnv_sjis2euc(euc, sizeof(euc), sjis_wave, sizeof(sjis_wave));
	codecnv_euc2sjis(sjis, sizeof(sjis), euc, sizeof(euc));
	if ((memcmp(sjis, sjis_wave, 2) != 0) || (sjis[2] != '\0')) {
		return (fail("codecnv", "SJIS/EUC round-trip changed bytes"));
	}
	fprintf(stderr, "selftest: codecnv ok\n");
	return (SUCCESS);
}

static int test_romcheck(void) {
	static const char vector[] = "123456789";
	ROMCHECKSUM result;
	char sha1[41];

	romcheck_buffer(vector, sizeof(vector) - 1, &result);
	romcheck_sha1_string(result.sha1, sha1);
	if ((result.size != 9) || (result.crc32 != 0xcbf43926) ||
	    strcmp(sha1, "f7c3bc1d808e04732adf679965ccc34ca7ae3441")) {
		return (fail("romcheck", "CRC32/SHA-1 test vector failed"));
	}
	fprintf(stderr, "selftest: romcheck ok\n");
	return (SUCCESS);
}

static int test_cli_boot_model(void) {
	if ((np2_cli_boot_model("va") != str_VA1) || (np2_cli_boot_model("VA") != str_VA1) ||
	    (np2_cli_boot_model("va2") != str_VA2) || (np2_cli_boot_model("VA2") != str_VA2) ||
	    (np2_cli_boot_model("va1") != NULL) || (np2_cli_boot_model("va3") != NULL) ||
	    (np2_cli_boot_model("") != NULL) || (np2_cli_boot_model(NULL) != NULL)) {
		return (fail("CLI boot model", "model name parsing failed"));
	}
	fprintf(stderr, "selftest: CLI boot model ok\n");
	return (SUCCESS);
}

static int test_cli_options(void) {
	char *valid[] = {"vaeg",
	                 "--model",
	                 "VA",
	                 "--fmbackend",
	                 "NP2",
	                 "--fmsound",
	                 "OPNA",
	                 "--ymfm-fidelity",
	                 "MAXIMUM",
	                 "--samplerate",
	                 "44100",
	                 "--soundbuffer",
	                 "40",
	                 "--mute",
	                 "--fdd1",
	                 "boot.d88",
	                 "--fdd2",
	                 "none",
	                 "--sasi1",
	                 "disk.hdi",
	                 "--sasi2",
	                 "NONE",
	                 "--scsi0",
	                 "disk.hdd",
	                 "--scsi1",
	                 "none",
	                 "--scsi2",
	                 "none",
	                 "--scsi3",
	                 "none",
	                 "--scsi4",
	                 "none",
	                 "--scsi5",
	                 "none",
	                 "--scsi6",
	                 "disk6.hdi",
	                 "--hostfat-dir",
	                 "host-root",
	                 "--roms",
	                 "rom-root",
	                 "--cpumult",
	                 "32",
	                 "--sgp",
	                 "16",
	                 "--nowait",
	                 "--frameskip",
	                 "4",
	                 "--fullscreen",
	                 "--effect",
	                 "crt-lite",
	                 "--scaling",
	                 "fit-8dot",
	                 "--controller",
	                 "mouse",
	                 "--keyboard-layout",
	                 "custom",
	                 "--debug",
	                 "--fdctrace",
	                 "--pacelog",
	                 "--trace-cpu",
	                 "17",
	                 "--headless-input-script",
	                 "input.txt",
	                 "--debug-script",
	                 "debug.txt",
	                 "--debug-output-dir",
	                 "debug-output",
	                 "--screen-dump",
	                 "rendered.bmp",
	                 "--screenshot",
	                 "1800:frame.BMP",
	                 "--screenshot",
	                 "600:C:\\tmp\\frame.PNG",
	                 "--screen-tvram-dump",
	                 "tvram.bin",
	                 "--scsitrace-cmdreq-windows",
	                 "--scsitrace-jitter-seed",
	                 "1234",
	                 "--scsitrace-jitter-span",
	                 "200",
	                 "--cfg",
	                 "session.cfg",
	                 "--bkupmem",
	                 "session.bak",
	                 "--smoke"};
	char *positional[] = {"vaeg", "boot.d88"};
	char *invalid_model[] = {"vaeg", "--model", "va3"};
	char *invalid_backend[] = {"vaeg", "--fmbackend", "mame"};
	char *invalid_rate[] = {"vaeg", "--samplerate", "48000"};
	char *invalid_sgp[] = {"vaeg", "--sgp", "17"};
	char *invalid_scaling[] = {"vaeg", "--scaling", "nearest"};
	char *missing_value[] = {"vaeg", "--fdd1"};
	char *missing_config[] = {"vaeg", "--cfg"};
	char *disabled_persistence[] = {"vaeg", "--no-cfg", "--no-bkupmem"};
	char *config_conflict[] = {"vaeg", "--cfg", "test.cfg", "--no-cfg"};
	char *bkupmem_conflict[] = {"vaeg", "--bkupmem", "test.dat", "--no-bkupmem"};
	char *hostfat_disabled[] = {"vaeg", "--smoke"};
	char *invalid_screenshot_frame[] = {"vaeg", "--screenshot", "abc:a.png"};
	char *zero_screenshot_frame[] = {"vaeg", "--screenshot", "0:a.png"};
	char *negative_screenshot_frame[] = {"vaeg", "--screenshot", "-1:a.png"};
	char *empty_screenshot_path[] = {"vaeg", "--screenshot", "1:"};
	char *invalid_screenshot_format[] = {"vaeg", "--screenshot", "1:a.jpg"};
	VAEG_CLI_OPTIONS options;
	char error[256];

	if (vaeg_cli_parse((int)NELEMENTS(valid), valid, &options, error, sizeof(error)) != SUCCESS) {
		return (fail("CLI options", error));
	}
	if ((options.model != VAEG_CLI_MODEL_VA) || (options.fm_backend != VAEG_CLI_FM_BACKEND_NP2) ||
	    (options.fm_sound != VAEG_CLI_FM_SOUND_OPNA) ||
	    (options.ymfm_fidelity != VAEG_CLI_FIDELITY_MAXIMUM) || (options.sample_rate != 44100) ||
	    (options.sound_buffer != 40) || !options.mute ||
	    (options.fdd_mode[0] != VAEG_CLI_MEDIA_PATH) || strcmp(options.fdd_path[0], "boot.d88") ||
	    (options.fdd_mode[1] != VAEG_CLI_MEDIA_NONE) ||
	    (options.sasi_mode[0] != VAEG_CLI_MEDIA_PATH) || strcmp(options.sasi_path[0], "disk.hdi") ||
	    (options.sasi_mode[1] != VAEG_CLI_MEDIA_NONE) ||
	    (options.scsi_mode[0] != VAEG_CLI_MEDIA_PATH) || strcmp(options.scsi_path[0], "disk.hdd") ||
	    (options.scsi_mode[1] != VAEG_CLI_MEDIA_NONE) ||
	    (options.scsi_mode[2] != VAEG_CLI_MEDIA_NONE) ||
	    (options.scsi_mode[3] != VAEG_CLI_MEDIA_NONE) ||
	    (options.scsi_mode[4] != VAEG_CLI_MEDIA_NONE) ||
	    (options.scsi_mode[5] != VAEG_CLI_MEDIA_NONE) ||
	    (options.scsi_mode[6] != VAEG_CLI_MEDIA_PATH) ||
	    strcmp(options.scsi_path[6], "disk6.hdi") || (options.hostfat_path == NULL) ||
	    strcmp(options.hostfat_path, "host-root") || (options.roms_path == NULL) ||
	    strcmp(options.roms_path, "rom-root") || (options.cpu_multiplier != 32) ||
	    (options.sgp_mode != VAEG_CLI_SGP_CUSTOM) || (options.sgp_multiplier != 16) ||
	    !options.nowait || (options.frame_skip != VAEG_CLI_FRAMESKIP_QUARTER) ||
	    (options.display_mode != VAEG_CLI_DISPLAY_FULLSCREEN) ||
	    (options.effect != VAEG_CLI_EFFECT_CRT_LITE) ||
	    (options.scaling != VAEG_CLI_SCALING_FIT_8DOT) ||
	    (options.controller != VAEG_CLI_CONTROLLER_MOUSE) ||
	    (options.keyboard_layout != VAEG_CLI_KEYBOARD_CUSTOM) || (options.trace_cpu != 17) ||
	    (options.headless_input_script == NULL) ||
	    strcmp(options.headless_input_script, "input.txt") || (options.debug_script == NULL) ||
	    strcmp(options.debug_script, "debug.txt") || (options.debug_output_dir == NULL) ||
	    strcmp(options.debug_output_dir, "debug-output") || (options.screen_dump_path == NULL) ||
	    strcmp(options.screen_dump_path, "rendered.bmp") ||
	    (options.screenshot_count != 2) || (options.screenshot_requests[0].frame != 1800) ||
	    strcmp(options.screenshot_requests[0].path, "frame.BMP") ||
	    (options.screenshot_requests[0].format != VAEG_SCREENSHOT_FORMAT_BMP) ||
	    (options.screenshot_requests[1].frame != 600) ||
	    strcmp(options.screenshot_requests[1].path, "C:\\tmp\\frame.PNG") ||
	    (options.screenshot_requests[1].format != VAEG_SCREENSHOT_FORMAT_PNG) ||
	    (options.screen_tvram_dump_path == NULL) ||
	    strcmp(options.screen_tvram_dump_path, "tvram.bin") || (options.config_path == NULL) ||
	    strcmp(options.config_path, "session.cfg") || (options.bkupmem_path == NULL) ||
	    strcmp(options.bkupmem_path, "session.bak") || !options.debug || !options.fdctrace ||
	    !options.pacelog || !options.scsitrace_cmdreq_windows || !options.scsitrace_jitter ||
	    (options.scsitrace_jitter_seed != 1234) || (options.scsitrace_jitter_span != 200) ||
	    !options.smoke) {
		return (fail("CLI options", "accepted values were parsed incorrectly"));
	}
	if ((vaeg_cli_parse((int)NELEMENTS(hostfat_disabled), hostfat_disabled, &options, error,
	                    sizeof(error)) != SUCCESS) ||
	    (options.hostfat_path != NULL) || options.scsitrace_cmdreq_windows) {
		return (fail("CLI options", "HOSTFAT was not disabled by default"));
	}
	if ((vaeg_cli_parse((int)NELEMENTS(disabled_persistence), disabled_persistence, &options, error,
	                    sizeof(error)) != SUCCESS) ||
	    !options.no_config || !options.no_bkupmem || (options.config_path != NULL) ||
	    (options.bkupmem_path != NULL)) {
		return (fail("CLI options", "persistence disable flags were parsed incorrectly"));
	}
	if ((vaeg_cli_parse((int)NELEMENTS(positional), positional, &options, error, sizeof(error)) ==
	     SUCCESS) ||
	    (strstr(error, "positional FDD arguments were removed") == NULL) ||
	    (vaeg_cli_parse((int)NELEMENTS(invalid_model), invalid_model, &options, error,
	                    sizeof(error)) == SUCCESS) ||
	    (vaeg_cli_parse((int)NELEMENTS(invalid_backend), invalid_backend, &options, error,
	                    sizeof(error)) == SUCCESS) ||
	    (vaeg_cli_parse((int)NELEMENTS(invalid_rate), invalid_rate, &options, error,
	                    sizeof(error)) == SUCCESS) ||
	    (vaeg_cli_parse((int)NELEMENTS(invalid_sgp), invalid_sgp, &options, error, sizeof(error)) ==
	     SUCCESS) ||
	    (vaeg_cli_parse((int)NELEMENTS(invalid_scaling), invalid_scaling, &options, error,
	                    sizeof(error)) == SUCCESS) ||
	    (vaeg_cli_parse((int)NELEMENTS(missing_value), missing_value, &options, error,
	                    sizeof(error)) == SUCCESS) ||
	    (vaeg_cli_parse((int)NELEMENTS(missing_config), missing_config, &options, error,
	                    sizeof(error)) == SUCCESS) ||
	    (vaeg_cli_parse((int)NELEMENTS(config_conflict), config_conflict, &options, error,
	                    sizeof(error)) == SUCCESS) ||
	    (vaeg_cli_parse((int)NELEMENTS(bkupmem_conflict), bkupmem_conflict, &options, error,
                    sizeof(error)) == SUCCESS) ||
	    (vaeg_cli_parse((int)NELEMENTS(invalid_screenshot_frame), invalid_screenshot_frame,
                    &options, error, sizeof(error)) == SUCCESS) ||
	    (vaeg_cli_parse((int)NELEMENTS(zero_screenshot_frame), zero_screenshot_frame, &options,
                    error, sizeof(error)) == SUCCESS) ||
	    (vaeg_cli_parse((int)NELEMENTS(negative_screenshot_frame), negative_screenshot_frame,
                    &options, error, sizeof(error)) == SUCCESS) ||
	    (vaeg_cli_parse((int)NELEMENTS(empty_screenshot_path), empty_screenshot_path, &options,
                    error, sizeof(error)) == SUCCESS) ||
	    (vaeg_cli_parse((int)NELEMENTS(invalid_screenshot_format), invalid_screenshot_format,
                    &options, error, sizeof(error)) == SUCCESS)) {
		return (fail("CLI options", "invalid input was accepted"));
	}
	fprintf(stderr, "selftest: CLI options ok\n");
	return (SUCCESS);
}

typedef struct {
	UINT32 frames[8];
	const char *paths[8];
	UINT count;
} SCREENSHOT_TEST_CAPTURE;

static BOOL test_screenshot_capture(UINT32 frame, const VAEG_SCREENSHOT_REQUEST *request,
	                                   void *opaque) {
	SCREENSHOT_TEST_CAPTURE *capture = (SCREENSHOT_TEST_CAPTURE *)opaque;

	if ((capture == NULL) || (request == NULL) || (capture->count >= NELEMENTS(capture->frames))) {
		return FAILURE;
	}
	capture->frames[capture->count] = frame;
	capture->paths[capture->count] = request->path;
	capture->count++;
	return SUCCESS;
}

static int test_screenshot_scheduler(void) {
	VAEG_SCREENSHOT_REQUEST requests[] = {
		{11, "eleven.png", VAEG_SCREENSHOT_FORMAT_PNG},
		{3, "three.bmp", VAEG_SCREENSHOT_FORMAT_BMP},
		{7, "seven-a.bmp", VAEG_SCREENSHOT_FORMAT_BMP},
		{7, "seven-b.bmp", VAEG_SCREENSHOT_FORMAT_BMP},
	};
	VAEG_SCREENSHOT_SCHEDULER scheduler;
	SCREENSHOT_TEST_CAPTURE capture;
	char *expected_paths[] = {"three.bmp", "seven-a.bmp", "seven-b.bmp", "eleven.png"};
	UINT i;

	ZeroMemory(&capture, sizeof(capture));
	vaeg_screenshot_scheduler_init(&scheduler, requests, NELEMENTS(requests));
	if (!vaeg_screenshot_scheduler_wants_frame(&scheduler, 3) ||
	    (vaeg_screenshot_scheduler_after_frame(&scheduler, 3, test_screenshot_capture,
	                                           &capture) != SUCCESS) ||
	    !vaeg_screenshot_scheduler_wants_frame(&scheduler, 7) ||
	    (vaeg_screenshot_scheduler_after_frame(&scheduler, 7, test_screenshot_capture,
                                           &capture) != SUCCESS) ||
	    !vaeg_screenshot_scheduler_wants_frame(&scheduler, 11) ||
	    (vaeg_screenshot_scheduler_after_frame(&scheduler, 11, test_screenshot_capture,
                                           &capture) != SUCCESS) ||
	    !vaeg_screenshot_scheduler_done(&scheduler) || (capture.count != 4)) {
		return (fail("screenshot scheduler", "frame ordering or completion failed"));
	}
	for (i = 0; i < capture.count; i++) {
		if ((capture.frames[i] != ((i == 0) ? 3 : (i < 3) ? 7 : 11)) ||
		    strcmp(capture.paths[i], expected_paths[i])) {
			return (fail("screenshot scheduler", "capture order was not stable"));
		}
	}
	ZeroMemory(&capture, sizeof(capture));
	vaeg_screenshot_scheduler_init(&scheduler, requests, NELEMENTS(requests));
	if (vaeg_screenshot_scheduler_after_frame(&scheduler, 4, test_screenshot_capture,
	                                          &capture) == SUCCESS) {
		return (fail("screenshot scheduler", "missed frame was accepted"));
	}
	fprintf(stderr, "selftest: screenshot scheduler ok\n");
	return (SUCCESS);
}

static void hostfat_send_string(const char *value) {
	while (*value != '\0') {
		iocore_out8(0x07ef, (REG8)*value++);
	}
}

static void hostfat_send_string_generic(const char *value) {
	while (*value != '\0') {
		iocore_out8(0x07ef, (REG8)*value++);
	}
}

static void hostfat_send_far_pointer(UINT32 value) {
	int index;

	for (index = 0; index < 4; index++) {
		iocore_out8(0x07ed, (REG8)value);
		value >>= 8;
	}
}

static int test_hostfat_transport(void) {
	BYTE *image;
	BYTE packet[22];
	BYTE saved_packet[sizeof(packet)];
	BYTE saved_destination[HOSTFAT_SECTOR_SIZE];
	BYTE unchanged[HOSTFAT_SECTOR_SIZE];
	UINT32 request_pointer;
	UINT32 packet_address;
	UINT32 destination_address;
	UINT8 result;
	SINT32 saved_remclock;
	int index;
	int status;

	image = (BYTE *)_MALLOC(HOSTFAT_IMAGE_SIZE, "hostfat-selftest-image");
	if (image == NULL) {
		return (fail("HOSTFAT transport", "image allocation failed"));
	}
	ZeroMemory(image, HOSTFAT_IMAGE_SIZE);
	for (index = 0; index < HOSTFAT_SECTOR_SIZE; index++) {
		image[(3 * HOSTFAT_SECTOR_SIZE) + index] = (BYTE)(index ^ 0x5a);
	}
	if (hostfat_mount_image(image, HOSTFAT_IMAGE_SIZE) != SUCCESS) {
		_MFREE(image);
		return (fail("HOSTFAT transport", "image mount failed"));
	}
	_MFREE(image);

	packet_address = 0x01000;
	destination_address = 0x02000;
	request_pointer = 0x01000000;
	MEML_READ(packet_address, saved_packet, sizeof(saved_packet));
	MEML_READ(destination_address, saved_destination, sizeof(saved_destination));
	ZeroMemory(packet, sizeof(packet));
	packet[0] = sizeof(packet);
	packet[2] = 4;
	/* Reserved header bytes must not be mistaken for IBM packet fields. */
	for (index = 5; index < 13; index++) {
		packet[index] = (BYTE)(0xa0 + index);
	}
	STOREINTELWORD(packet + 14, 0);
	STOREINTELWORD(packet + 16, 0x0200);
	STOREINTELWORD(packet + 18, 1);
	STOREINTELWORD(packet + 20, 3);
	MEML_WRITE(packet_address, packet, sizeof(packet));
	FillMemory(unchanged, sizeof(unchanged), 0xa5);
	MEML_WRITE(destination_address, unchanged, sizeof(unchanged));

	status = FAILURE;
	saved_remclock = CPU_REMCLOCK;
	iocore_create();
	if (iocore_build() != SUCCESS) {
		goto transport_cleanup;
	}
	np2sysp_reset();
	np2sysp_bind();
	iocore_bind();
	hostfat_send_string_generic("check_hostfat");
	if ((iocore_inp8(0x07ef) != 'H') || (iocore_inp8(0x07ef) != '1')) {
		goto transport_cleanup;
	}
	np2sysp_reset();
	hostfat_send_string("check_hostfat");
	if ((iocore_inp8(0x07ef) != 'H') || (iocore_inp8(0x07ef) != '1')) {
		goto transport_cleanup;
	}
	hostfat_send_far_pointer(request_pointer);
	hostfat_send_string("read_hostfat1");
	result = iocore_inp8(0x07ed);
	if (result != HOSTFAT_RESULT_OK) {
		goto transport_cleanup;
	}
	for (index = 0; index < HOSTFAT_SECTOR_SIZE; index++) {
		if (upd9002_memoryread(destination_address + index) != (BYTE)(index ^ 0x5a)) {
			goto transport_cleanup;
		}
	}
	STOREINTELWORD(packet + 20, HOSTFAT_TOTAL_SECTORS);
	MEML_WRITE(packet_address, packet, sizeof(packet));
	MEML_WRITE(destination_address, unchanged, sizeof(unchanged));
	hostfat_send_far_pointer(request_pointer);
	hostfat_send_string("read_hostfat1");
	result = iocore_inp8(0x07ed);
	if (result != HOSTFAT_RESULT_RANGE) {
		goto transport_cleanup;
	}
	for (index = 0; index < HOSTFAT_SECTOR_SIZE; index++) {
		if (upd9002_memoryread(destination_address + index) != 0xa5) {
			goto transport_cleanup;
		}
	}
	packet[0] = sizeof(packet) - 1;
	STOREINTELWORD(packet + 20, 3);
	MEML_WRITE(packet_address, packet, sizeof(packet));
	hostfat_send_far_pointer(request_pointer);
	hostfat_send_string("read_hostfat1");
	if (iocore_inp8(0x07ed) != HOSTFAT_RESULT_BAD_REQUEST) {
		goto transport_cleanup;
	}
	packet[0] = sizeof(packet);
	packet[1] = 1;
	MEML_WRITE(packet_address, packet, sizeof(packet));
	hostfat_send_far_pointer(request_pointer);
	hostfat_send_string("read_hostfat1");
	if (iocore_inp8(0x07ed) != HOSTFAT_RESULT_BAD_REQUEST) {
		goto transport_cleanup;
	}
	packet[1] = 0;
	packet[2] = 5;
	MEML_WRITE(packet_address, packet, sizeof(packet));
	hostfat_send_far_pointer(request_pointer);
	hostfat_send_string("read_hostfat1");
	if (iocore_inp8(0x07ed) != HOSTFAT_RESULT_BAD_REQUEST) {
		goto transport_cleanup;
	}
	packet[2] = 4;
	STOREINTELWORD(packet + 18, 129);
	MEML_WRITE(packet_address, packet, sizeof(packet));
	hostfat_send_far_pointer(request_pointer);
	hostfat_send_string("read_hostfat1");
	if (iocore_inp8(0x07ed) != HOSTFAT_RESULT_RANGE) {
		goto transport_cleanup;
	}
	STOREINTELWORD(packet + 18, 1);
	STOREINTELWORD(packet + 14, 0);
	STOREINTELWORD(packet + 16, 0x9ff0);
	MEML_WRITE(packet_address, packet, sizeof(packet));
	hostfat_send_far_pointer(request_pointer);
	hostfat_send_string("read_hostfat1");
	if (iocore_inp8(0x07ed) != HOSTFAT_RESULT_RANGE) {
		goto transport_cleanup;
	}
	for (index = 0; index < HOSTFAT_SECTOR_SIZE; index++) {
		if (upd9002_memoryread(destination_address + index) != 0xa5) {
			goto transport_cleanup;
		}
	}
	np2sysp_reset();
	if (!hostfat_is_mounted()) {
		goto transport_cleanup;
	}
	hostfat_unmount();
	hostfat_send_string("check_hostfat");
	if (iocore_inp8(0x07ef) != 0) {
		goto transport_cleanup;
	}
	hostfat_send_far_pointer(request_pointer);
	hostfat_send_string("read_hostfat1");
	if (iocore_inp8(0x07ed) != HOSTFAT_RESULT_NOT_MOUNTED) {
		goto transport_cleanup;
	}
	for (index = 0; index < HOSTFAT_SECTOR_SIZE; index++) {
		if (upd9002_memoryread(destination_address + index) != 0xa5) {
			goto transport_cleanup;
		}
	}
	status = SUCCESS;

transport_cleanup:
	iocore_destroy();
	CPU_REMCLOCK = saved_remclock;
	hostfat_unmount();
	MEML_WRITE(packet_address, saved_packet, sizeof(saved_packet));
	MEML_WRITE(destination_address, saved_destination, sizeof(saved_destination));
	if (status != SUCCESS) {
		return (fail("HOSTFAT transport", "VA 07EDH/07EFH read or atomic range rejection failed"));
	}
	fprintf(stderr, "selftest: HOSTFAT transport ok\n");
	return (SUCCESS);
}

static int test_hostfat_snapshot(void) {
	if ((hostfat_snapshot_selftest() != SUCCESS) || (hostfat_manager_selftest() != SUCCESS)) {
		return (fail("HOSTFAT snapshot",
		             "FAT generation, asynchronous commit, or rejection policy failed"));
	}
	fprintf(stderr, "selftest: HOSTFAT snapshot ok\n");
	return (SUCCESS);
}

static int test_va_tvram_window(void) {
	_MEMORYVA saved_memoryva;
	UINT8 saved_model_va;
	BYTE saved_bytes[4];
	BOOL saved_dirty;
	int result;

	saved_memoryva = memoryva;
	saved_model_va = pccore.model_va;
	saved_bytes[0] = textmem[0x0fffe];
	saved_bytes[1] = textmem[0x0ffff];
	saved_bytes[2] = textmem[0x10000];
	saved_bytes[3] = textmem[0x1fff0];
	saved_dirty = textmem_dirty;
	result = SUCCESS;

	memoryva.sysm_bank = 1;
	memoryva.dma_sysm_bank = 0;
	memoryva.dma_access = 0;
	pccore.model_va = PCMODEL_VA1;
	textmem[0x10000] = 0x5a;
	textmem[0x1fff0] = 0xa5;

	upd9002_memorywrite_va_w(0x0afffe, 0x1234);
	if (upd9002_memoryread_va_w(0x0afffe) != 0x1234) {
		result = fail("VA TVRAM", "valid word access failed");
	}
	upd9002_memorywrite_va_w(0x0affff, 0x5678);
	if ((upd9002_memoryread_va_w(0x0affff) != 0xff78) || (textmem[0x10000] != 0x5a)) {
		result = fail("VA TVRAM", "AFFFF boundary crossed into unused memory");
	}
	upd9002_memorywrite_va(0x0b0000, 0x6c);
	if ((upd9002_memoryread_va(0x0b0000) != 0xff) || (textmem[0x10000] != 0x5a)) {
		result = fail("VA TVRAM", "unused byte access did not use open bus");
	}
	upd9002_memorywrite_va_w(0x0bfff0, 0xabcd);
	if ((upd9002_memoryread_va_w(0x0bfff0) != 0xffff) || (textmem[0x1fff0] != 0xa5)) {
		result = fail("VA TVRAM", "B0000-DFFFF is not open bus");
	}

	pccore.model_va = PCMODEL_VA2;
	upd9002_memorywrite_va(0x0b0000, 0x6c);
	if ((upd9002_memoryread_va(0x0b0000) != 0x6c) || (textmem[0x10000] != 0x6c)) {
		result = fail("VA2 TVRAM", "legacy B0000 byte access was blocked");
	}
	upd9002_memorywrite_va_w(0x0bfff0, 0xabcd);
	if ((upd9002_memoryread_va_w(0x0bfff0) != 0xabcd) || (textmem[0x1fff0] != 0xcd)) {
		result = fail("VA2 TVRAM", "legacy B0000-DFFFF word access was blocked");
	}

	memoryva = saved_memoryva;
	pccore.model_va = saved_model_va;
	textmem[0x0fffe] = saved_bytes[0];
	textmem[0x0ffff] = saved_bytes[1];
	textmem[0x10000] = saved_bytes[2];
	textmem[0x1fff0] = saved_bytes[3];
	textmem_dirty = saved_dirty;
	if (result == SUCCESS) {
		fprintf(stderr, "selftest: model-specific TVRAM window ok\n");
	}
	return (result);
}

static int test_va_bms_window(void) {
	_BMSIOCFG saved_bmsiocfg;
	_BMSIO saved_bmsio;
	_BMSIOWORK saved_bmsiowork;
	BYTE *retained_mem;
	BYTE saved_main[3];
	BOOL io_created;
	int result;

	saved_bmsiocfg = bmsiocfg;
	saved_bmsio = bmsio;
	saved_bmsiowork = bmsiowork;
	saved_main[0] = mem[0x080000];
	saved_main[1] = mem[0x09fffe];
	saved_main[2] = mem[0x09ffff];
	mem[0x080000] = 0xa1;
	mem[0x09fffe] = 0xb2;
	mem[0x09ffff] = 0xc3;
	ZeroMemory(&bmsio, sizeof(bmsio));
	ZeroMemory(&bmsiowork, sizeof(bmsiowork));
	io_created = FALSE;
	result = SUCCESS;

	bmsiocfg.enabled = FALSE;
	if ((BMSIO_DEFAULT_ENABLED != TRUE) || (BMSIO_PORT_DEFAULT != 0x01d0) ||
	    (BMSIO_PORT_COMPAT != 0x00ec) || (BMSIO_BANK_BYTES != 0x20000) ||
	    (BMSIO_DEFAULT_BANKS != 0x80) ||
	    (((UINT32)BMSIO_DEFAULT_BANKS * BMSIO_BANK_BYTES) != 0x01000000)) {
		result = fail("VA BMS", "unexpected default configuration");
		goto bms_test_cleanup;
	}
	bmsiocfg.port = BMSIO_PORT_DEFAULT;
	bmsiocfg.portmask = BMSIO_PORT_MASK;
	bmsiocfg.numbanks = BMSIO_DEFAULT_BANKS;
	bmsio_set();
	bmsio_reset();
	upd9002_memorywrite_va(0x080000, 0x12);
	upd9002_memorywrite_va_w(0x09fffe, 0x3456);
	if ((upd9002_memoryread_va(0x080000) != 0x12) ||
	    (upd9002_memoryread_va_w(0x09fffe) != 0x3456) || (mem[0x080000] != 0x12) ||
	    (mem[0x09fffe] != 0x56) || (mem[0x09ffff] != 0x34) || (bmsiowork.bmsmem != NULL) ||
	    (bmsiowork.bmsmemsize != 0) || (bmsio.cfg.port != BMSIO_PORT_DEFAULT) ||
	    (bmsio.nomem != 0)) {
		result = fail("VA BMS", "disabled bank zero did not preserve main RAM");
		goto bms_test_cleanup;
	}

	bmsiocfg.enabled = TRUE;
	bmsiocfg.port = BMSIO_PORT_DEFAULT;
	bmsiocfg.numbanks = 2;
	bmsio_set();
	bmsio_reset();
	iocore_create();
	io_created = TRUE;
	if (iocore_build() != SUCCESS) {
		result = fail("VA BMS", "could not build selected-port I/O table");
		goto bms_test_cleanup;
	}
	bmsio_bind();
	iocore_out8(BMSIO_PORT_DEFAULT, 1);
	if ((bmsio.bank != 1) || (bmsio.nomem != 0) || (iocore_inp8(BMSIO_PORT_DEFAULT) != 1) ||
	    (iocore_inp8(BMSIO_PORT_COMPAT) != 0xff)) {
		result = fail("VA BMS", "native selection unexpectedly exposed another port");
		goto bms_test_cleanup;
	}
	iocore_destroy();
	iocore_create();
	if (iocore_build() != SUCCESS) {
		result = fail("VA BMS", "could not rebuild selected-port I/O table");
		goto bms_test_cleanup;
	}
	bmsiocfg.port = BMSIO_PORT_COMPAT;
	bmsio_set();
	bmsio_reset();
	bmsio_bind();
	iocore_out8(BMSIO_PORT_COMPAT, 2);
	if ((bmsio.bank != 2) || (bmsio.nomem != 0) || (iocore_inp8(BMSIO_PORT_COMPAT) != 2) ||
	    (iocore_inp8(BMSIO_PORT_DEFAULT) != 0xff)) {
		result = fail("VA BMS", "compatibility selection unexpectedly exposed another port");
		goto bms_test_cleanup;
	}
	iocore_destroy();
	io_created = FALSE;

	bmsiocfg.port = BMSIO_PORT_COMPAT;
	bmsio_set();
	bmsio_reset();
	if ((bmsiowork.bmsmem == NULL) || (bmsiowork.bmsmemsize != 0x40000) ||
	    (bmsio.cfg.enabled == FALSE) || (bmsio.cfg.port != BMSIO_PORT_COMPAT) ||
	    (bmsio.cfg.numbanks != 2) || (bmsio.bank != 0) || (bmsio.nomem != 0)) {
		result = fail("VA BMS", "enabled configuration was not applied");
		goto bms_test_cleanup;
	}
	ZeroMemory(bmsiowork.bmsmem, bmsiowork.bmsmemsize);
	if ((upd9002_memoryread_va(0x080000) != 0x12) ||
	    (upd9002_memoryread_va_w(0x09fffe) != 0x3456) || (bmsiowork.bmsmem[0] != 0) ||
	    (LOADINTELWORD(bmsiowork.bmsmem + 0x1fffe) != 0) || (mem[0x080000] != 0x12) ||
	    (mem[0x09fffe] != 0x56) || (mem[0x09ffff] != 0x34)) {
		result = fail("VA BMS", "enabled bank zero did not preserve main RAM");
		goto bms_test_cleanup;
	}

	retained_mem = bmsiowork.bmsmem;
	bmsio_set();
	bmsio_reset();
	if ((bmsiowork.bmsmem != retained_mem) || (upd9002_memoryread_va(0x080000) != 0x12) ||
	    (upd9002_memoryread_va_w(0x09fffe) != 0x3456)) {
		result = fail("VA BMS", "ordinary reset did not retain BMS contents");
		goto bms_test_cleanup;
	}

	bmsio.bank = 1;
	bmsio.nomem = 0;
	upd9002_memorywrite_va(0x080000, 0x78);
	upd9002_memorywrite_va_w(0x09fffe, 0x9abc);
	if ((upd9002_memoryread_va(0x080000) != 0x78) ||
	    (upd9002_memoryread_va_w(0x09fffe) != 0x9abc) || (bmsiowork.bmsmem[0] != 0x78) ||
	    (LOADINTELWORD(bmsiowork.bmsmem + 0x1fffe) != 0x9abc) || (mem[0x080000] != 0x12) ||
	    (mem[0x09fffe] != 0x56) || (mem[0x09ffff] != 0x34)) {
		result = fail("VA BMS", "bank 1 access failed");
		goto bms_test_cleanup;
	}
	bmsio.bank = 0;
	if ((upd9002_memoryread_va(0x080000) != 0x12) ||
	    (upd9002_memoryread_va_w(0x09fffe) != 0x3456)) {
		result = fail("VA BMS", "bank switch did not preserve bank 0");
		goto bms_test_cleanup;
	}

	bmsiocfg.enabled = FALSE;
	bmsio_set();
	bmsio_reset();
	if ((bmsiowork.bmsmem != NULL) || (bmsiowork.bmsmemsize != 0) || (bmsio.nomem != 0) ||
	    (upd9002_memoryread_va(0x080000) != 0x12) ||
	    (upd9002_memoryread_va_w(0x09fffe) != 0x3456) || (mem[0x080000] != 0x12) ||
	    (mem[0x09fffe] != 0x56) || (mem[0x09ffff] != 0x34)) {
		result = fail("VA BMS", "disable did not restore main RAM");
	}

bms_test_cleanup:
	if (io_created) {
		iocore_destroy();
	}
	if (bmsiowork.bmsmem != NULL) {
		_MFREE(bmsiowork.bmsmem);
	}
	bmsiocfg = saved_bmsiocfg;
	bmsio = saved_bmsio;
	bmsiowork = saved_bmsiowork;
	mem[0x080000] = saved_main[0];
	mem[0x09fffe] = saved_main[1];
	mem[0x09ffff] = saved_main[2];
	if (result == SUCCESS) {
		fprintf(stderr, "selftest: VA BMS config/window lifecycle ok\n");
	}
	return (result);
}

static int test_va_ems_board(void) {
	_EMSIO saved_emsio;
	BYTE *saved_ems[4];
	BYTE *saved_extmem;
	UINT32 saved_extsize;
	UINT8 saved_pccore_extmem;
	SINT32 saved_remclock;
	BYTE saved_main;
	BYTE saved_high0;
	BYTE saved_high1;
	BYTE *page_ptr;
	BYTE saved_page;
	BYTE pattern;
	UINT32 page_addr;
	UINT32 page_count;
	UINT frame;
	UINT i;
	UINT page_port;
	UINT target;
	int result;

	saved_emsio = emsio;
	for (i = 0; i < 4; i++) {
		saved_ems[i] = CPU_EMSPTR[i];
	}
	saved_extmem = CPU_EXTMEM;
	saved_extsize = CPU_EXTMEMSIZE;
	saved_pccore_extmem = pccore.extmem;
	saved_remclock = CPU_REMCLOCK;
	saved_main = mem[0xc0000];
	saved_high0 = mem[0x100000];
	saved_high1 = mem[0x104000];
	result = SUCCESS;
	iocore_create();
	if (iocore_build() != SUCCESS) {
		result = fail("VA EMS", "could not build isolated I/O tables");
		goto ems_test_cleanup;
	}

	if ((EMSIO_DEFAULT_MEGABYTES != 13) || (EMSIO_MIN_MEGABYTES != 1) ||
	    (EMSIO_MAX_MEGABYTES != 13)) {
		result = fail("VA EMS", "unexpected capacity bounds");
		goto ems_test_cleanup;
	}
	CPU_EXTMEM = NULL;
	CPU_EXTMEMSIZE = 0;
	CPU_SETEXTSIZE(EMSIO_MAX_MEGABYTES);
	if ((CPU_EXTMEM == NULL) || (CPU_EXTMEMSIZE != (EMSIO_MAX_MEGABYTES << 20))) {
		result = fail("VA EMS", "could not allocate isolated test memory");
		goto ems_test_cleanup;
	}

	pccore.extmem = 1;
	emsio_reset();
	emsio_bind();
	CPU_REMCLOCK = 0x100000;
	iocore_out8(0x08e9, 1);
	if ((iocore_inp8(0x08e9) != 0) || (emsio.target != 1)) {
		result = fail("VA EMS", "VA target register was not reachable");
		goto ems_test_cleanup;
	}
	iocore_out8(0x08e1, 0);
	upd9002_memorywrite(0x0c0000, 0x5a);
	if ((mem[0x100000] != 0x5a) || (emsio.addr[0] != 0x100000)) {
		result = fail("VA EMS", "first 16KB page did not map");
		goto ems_test_cleanup;
	}
	iocore_out8(0x08e1, 4);
	upd9002_memorywrite(0x0c0000, 0xa5);
	if ((mem[0x104000] != 0xa5) || (emsio.addr[0] != 0x104000) || (mem[0x100000] != 0x5a)) {
		result = fail("VA EMS", "page selection did not preserve distinct data");
		goto ems_test_cleanup;
	}
	iocore_out8(0x08e1, 0x10);
	upd9002_memorywrite(0x0c0000, 0x3c);
	if ((CPU_EXTMEM[0x10000] != 0x3c) || (emsio.addr[0] != 0x110000)) {
		result = fail("VA EMS", "page mapping failed above the 64KB boundary");
		goto ems_test_cleanup;
	}
	upd9002_memorywrite(0x0c0000, 0x96);
	if ((upd9002_memoryread(0x0c0000) != 0x96) || (CPU_EXTMEM[0x10000] != 0x96)) {
		result = fail("VA EMS", "native VA page-frame access did not map");
		goto ems_test_cleanup;
	}

	iocore_out8(0x08e9, 1);
	if (iocore_inp8(0x08e9) != 0) {
		result = fail("VA EMS", "native VA target register was not reachable");
		goto ems_test_cleanup;
	}
	iocore_out8(0x08e9, 2);
	if (iocore_inp8(0x08e9) != 0xff) {
		result = fail("VA EMS", "out-of-range target was accepted");
		goto ems_test_cleanup;
	}
	iocore_out8(0x08e1, 8);
	if (emsio.addr[0] != 0x110000) {
		result = fail("VA EMS", "out-of-range target changed the page frame");
		goto ems_test_cleanup;
	}
	iocore_out8(0x08e9, 0);
	iocore_out8(0x08e1, 0);
	if ((emsio.addr[0] != 0xc0000) || (CPU_EMSPTR[0] != mem + 0xc0000)) {
		result = fail("VA EMS", "target zero did not restore ordinary memory");
		goto ems_test_cleanup;
	}

	pccore.extmem = EMSIO_MAX_MEGABYTES;
	emsio_reset();
	emsio_bind();
	page_count = EMSIO_MAX_MEGABYTES << 6;
	for (i = 0; i < page_count; i++) {
		target = (i >> 6) + 1;
		frame = i & 3;
		page_port = 0x08e1 + (frame << 1);
		page_addr = (target << 20) + ((i & 0x3f) << 14);
		if (page_addr < USE_HIMEM) {
			page_ptr = mem + page_addr;
		} else {
			page_ptr = CPU_EXTMEM + page_addr - 0x100000;
		}
		iocore_out8(0x08e9, target);
		iocore_out8(page_port, (i & 0x3f) << 2);
		if ((iocore_inp8(0x08e9) != 0) || (emsio.addr[frame] != page_addr) ||
		    (CPU_EMSPTR[frame] != page_ptr)) {
			result = fail("VA EMS", "13MB page table did not map completely");
			goto ems_test_cleanup;
		}
		saved_page = *page_ptr;
		pattern = (BYTE)(i ^ (i >> 8) ^ 0xa5);
		if (pattern == saved_page) {
			pattern ^= 0xff;
		}
		upd9002_memorywrite(0x0c0000 + (frame << 14), pattern);
		if ((upd9002_memoryread(0x0c0000 + (frame << 14)) != pattern) || (*page_ptr != pattern)) {
			*page_ptr = saved_page;
			result = fail("VA EMS", "13MB native page-frame test failed");
			goto ems_test_cleanup;
		}
		upd9002_memorywrite(0x0c0000 + (frame << 14), saved_page);
		if (*page_ptr != saved_page) {
			result = fail("VA EMS", "13MB page test did not restore memory");
			goto ems_test_cleanup;
		}
	}

	pccore.extmem = 0;
	emsio_reset();
	emsio_bind();
	if ((emsio.maxmem != 0) || (emsio.target != 0) || (emsio.addr[0] != 0xc0000)) {
		result = fail("VA EMS", "disabled reset retained an EMS target");
	}

ems_test_cleanup:
	iocore_destroy();
	if (CPU_EXTMEM != saved_extmem) {
		if (CPU_EXTMEM != NULL) {
			_MFREE(CPU_EXTMEM);
		}
		CPU_EXTMEM = saved_extmem;
		CPU_EXTMEMSIZE = saved_extsize;
	}
	mem[0xc0000] = saved_main;
	pccore.extmem = saved_pccore_extmem;
	mem[0x100000] = saved_high0;
	mem[0x104000] = saved_high1;
	emsio = saved_emsio;
	for (i = 0; i < 4; i++) {
		CPU_EMSPTR[i] = saved_ems[i];
	}
	CPU_REMCLOCK = saved_remclock;
	if (result == SUCCESS) {
		fprintf(stderr, "selftest: VA EMS board mapping/config lifecycle ok\n");
	}
	return (result);
}

typedef struct {
	UINT format;
	UINT8 d88_type;
	UINT8 cylinders;
	UINT8 heads;
	UINT8 sectors;
	UINT8 sector_n;
	UINT16 sector_size;
	UINT8 sectors_per_cluster;
	UINT8 media;
	UINT16 root_entries;
	UINT16 fat_sectors;
} SELFTESTFDDGEOMETRY;

static const SELFTESTFDDGEOMETRY selftest_fdd_geometry[] = {
    {NEWDISK_FDD_MSDOS_2HD, 0x20, 77, 2, 8, 3, 1024, 1, 0xfe, 192, 2},
    {NEWDISK_FDD_MSDOS_2DD, 0x10, 80, 2, 8, 2, 512, 2, 0xfb, 112, 2}};

static int test_new_fdd_image(void) {
	const SELFTESTFDDGEOMETRY *geometry;
	_D88HEAD header;
	_D88SEC sector_header;
	_FDDFILE parsed;
	BYTE sector[1024];
	char path[MAX_PATH];
	FILEH fh;
	UINT32 data_offset;
	UINT32 expected_size;
	UINT32 fat_sector;
	UINT32 total_sectors;
	UINT index;
	UINT tracks;
	int result;

	result = SUCCESS;
	for (index = 0; index < NELEMENTS(selftest_fdd_geometry); index++) {
		geometry = selftest_fdd_geometry + index;
		SPRINTF(path, "vaeg-selftest-%lu-fdd-%u.d88", (unsigned long)getpid(), geometry->format);
		file_delete(path);
		if (newdisk_fdd_msdos(path, geometry->format) != SUCCESS) {
			result = fail("new-fdd", "formatted D88 creation failed");
			break;
		}
		if (newdisk_fdd_msdos(path, geometry->format) != FAILURE) {
			result = fail("new-fdd", "existing image was overwritten");
			file_delete(path);
			break;
		}
		ZeroMemory(&parsed, sizeof(parsed));
		if ((fddd88_set(&parsed, path, 0) != SUCCESS) ||
		    (parsed.inf.d88.fdtype_major != (geometry->d88_type >> 4))) {
			result = fail("new-fdd", "active D88 loader rejected the image");
			file_delete(path);
			break;
		}
		fh = file_open_rb(path);
		if (fh == FILEH_INVALID) {
			result = fail("new-fdd", "created D88 could not be opened");
			file_delete(path);
			break;
		}
		total_sectors = geometry->cylinders * geometry->heads * geometry->sectors;
		tracks = geometry->cylinders * geometry->heads;
		expected_size =
		    sizeof(header) + total_sectors * (sizeof(sector_header) + geometry->sector_size);
		if ((file_getsize(fh) != expected_size) ||
		    (file_read(fh, &header, sizeof(header)) != sizeof(header)) ||
		    (header.fd_type != geometry->d88_type) ||
		    (LOADINTELDWORD(header.fd_size) != expected_size) ||
		    (LOADINTELDWORD(header.trackp[0]) != sizeof(header)) ||
		    (LOADINTELDWORD(header.trackp[tracks - 1]) !=
		     sizeof(header) + (tracks - 1) * geometry->sectors *
		                          (sizeof(sector_header) + geometry->sector_size)) ||
		    ((tracks < NELEMENTS(header.trackp)) && (LOADINTELDWORD(header.trackp[tracks]) != 0))) {
			result = fail("new-fdd", "D88 header or track table is invalid");
			file_close(fh);
			file_delete(path);
			break;
		}
		if ((file_read(fh, &sector_header, sizeof(sector_header)) != sizeof(sector_header)) ||
		    (file_read(fh, sector, geometry->sector_size) != geometry->sector_size) ||
		    (sector_header.c != 0) || (sector_header.h != 0) || (sector_header.r != 1) ||
		    (sector_header.n != geometry->sector_n) ||
		    (LOADINTELWORD(sector_header.sectors) != geometry->sectors) ||
		    (LOADINTELWORD(sector_header.size) != geometry->sector_size) ||
		    (LOADINTELWORD(sector + 11) != geometry->sector_size) ||
		    (sector[13] != geometry->sectors_per_cluster) || (LOADINTELWORD(sector + 14) != 1) ||
		    (sector[16] != 2) || (LOADINTELWORD(sector + 17) != geometry->root_entries) ||
		    (LOADINTELWORD(sector + 19) != total_sectors) || (sector[21] != geometry->media) ||
		    (LOADINTELWORD(sector + 22) != geometry->fat_sectors) ||
		    (LOADINTELWORD(sector + 24) != geometry->sectors) ||
		    (LOADINTELWORD(sector + 26) != geometry->heads) || (sector[510] != 0x55) ||
		    (sector[511] != 0xaa)) {
			result = fail("new-fdd", "D88 sector or FAT12 BPB is invalid");
			file_close(fh);
			file_delete(path);
			break;
		}
		fat_sector = 1 + geometry->fat_sectors;
		data_offset = sizeof(header) + fat_sector * (sizeof(sector_header) + geometry->sector_size);
		if ((file_seek(fh, data_offset + sizeof(sector_header), SEEK_SET) !=
		     (long)(data_offset + sizeof(sector_header))) ||
		    (file_read(fh, sector, 3) != 3) || (sector[0] != geometry->media) ||
		    (sector[1] != 0xff) || (sector[2] != 0xff)) {
			result = fail("new-fdd", "second FAT was not initialized");
			file_close(fh);
			file_delete(path);
			break;
		}
		file_close(fh);
		file_delete(path);

		SPRINTF(path, "vaeg-selftest-%lu-fdd-%u.img", (unsigned long)getpid(), geometry->format);
		file_delete(path);
		if (newdisk_fdd_msdos_ex(path, geometry->format, NEWDISK_FDD_CONTAINER_RAW) != SUCCESS) {
			result = fail("new-fdd", "formatted raw creation failed");
			break;
		}
		if (newdisk_fdd_msdos_ex(path, geometry->format, NEWDISK_FDD_CONTAINER_RAW) != FAILURE) {
			result = fail("new-fdd", "existing raw image was overwritten");
			file_delete(path);
			break;
		}
		ZeroMemory(&parsed, sizeof(parsed));
		if ((fddxdf_set(&parsed, path, 0) != SUCCESS) || (parsed.inf.xdf.headersize != 0) ||
		    (parsed.inf.xdf.tracks != tracks) || (parsed.inf.xdf.sectors != geometry->sectors) ||
		    (parsed.inf.xdf.n != geometry->sector_n) ||
		    (parsed.inf.xdf.disktype != (geometry->d88_type >> 4))) {
			result = fail("new-fdd", "active raw loader rejected the image");
			file_delete(path);
			break;
		}
		fh = file_open_rb(path);
		if (fh == FILEH_INVALID) {
			result = fail("new-fdd", "created raw image could not be opened");
			file_delete(path);
			break;
		}
		expected_size = total_sectors * geometry->sector_size;
		if ((file_getsize(fh) != expected_size) ||
		    (file_read(fh, sector, geometry->sector_size) != geometry->sector_size) ||
		    (LOADINTELWORD(sector + 11) != geometry->sector_size) ||
		    (sector[13] != geometry->sectors_per_cluster) ||
		    (LOADINTELWORD(sector + 17) != geometry->root_entries) ||
		    (LOADINTELWORD(sector + 19) != total_sectors) || (sector[21] != geometry->media) ||
		    (LOADINTELWORD(sector + 22) != geometry->fat_sectors) ||
		    (LOADINTELWORD(sector + 24) != geometry->sectors) ||
		    (LOADINTELWORD(sector + 26) != geometry->heads) || (sector[510] != 0x55) ||
		    (sector[511] != 0xaa)) {
			result = fail("new-fdd", "raw FAT12 BPB is invalid");
			file_close(fh);
			file_delete(path);
			break;
		}
		data_offset = fat_sector * geometry->sector_size;
		if ((file_seek(fh, data_offset, SEEK_SET) != (long)data_offset) ||
		    (file_read(fh, sector, 3) != 3) || (sector[0] != geometry->media) ||
		    (sector[1] != 0xff) || (sector[2] != 0xff)) {
			result = fail("new-fdd", "raw second FAT was not initialized");
			file_close(fh);
			file_delete(path);
			break;
		}
		file_close(fh);
		file_delete(path);
	}
	if (result == SUCCESS) {
		fprintf(stderr, "selftest: formatted D88/raw images ok\n");
	}
	return (result);
}

typedef struct {
	const char *name;
	UINT8 d88_type;
	UINT8 cylinders;
	UINT8 sectors;
	UINT16 sector_size;
	UINT8 sector_n;
	UINT8 dskctl_2hd;
	UINT8 density_96;
	UINT8 clock_8mhz;
	UINT8 d88_track_stride;
} SELFTESTFDDPROFILE;

static const SELFTESTFDDPROFILE selftest_fdd_profiles[] = {
	{"2d-320", 0x00, 40, 8, 512, 2, 0, 0, 0, 2},
	{"2d-320-256-sector", 0x00, 40, 16, 256, 1, 0, 0, 0, 2},
	{"2d-360", 0x00, 40, 9, 512, 2, 0, 0, 0, 2},
	{"2d-320-double-step", 0x00, 40, 8, 512, 2, 0, 0, 0, 4},
	{"2dd-640", 0x10, 80, 8, 512, 2, 0, 1, 0, 2},
	{"2dd-720", 0x10, 80, 9, 512, 2, 0, 1, 0, 2},
	{"2hc-1200", 0x20, 80, 15, 512, 2, 1, 1, 1, 2},
	{"2hd-1232", 0x20, 77, 8, 1024, 3, 1, 1, 1, 2}};

static SINT32 selftest_fdc_profile_interval(const SELFTESTFDDPROFILE *profile) {
	const int data = profile->sector_size;
	const int gap3 = profile->dskctl_2hd ? 116 : 84;
	const int gap4 = profile->dskctl_2hd ? 654 : 182;
	const int sync = 12;
	const int sector_length = sync + 4 + 4 + 2 + 22 + sync + 4 + data + 2 + gap3;
	const int track_length = 80 + sync + 4 + 50 + sector_length * profile->sectors + gap4;

	return (SINT32)((UINT64)pccore.realclock * 60 / 360 / track_length * sector_length / data);
}

static BYTE selftest_fdd_profile_byte(UINT track, UINT sector, UINT offset) {
	return (BYTE)(0x5b + track * 31 + sector * 17 + offset * 13);
}

static BOOL selftest_fdd_create_profile(const char *path, const SELFTESTFDDPROFILE *profile) {
	_D88HEAD header;
	_D88SEC sector_header;
	BYTE data[1024];
	FILEH fh;
	UINT32 offset;
	UINT track;
	UINT sector;
	UINT index;
	UINT track_count;

	ZeroMemory(&header, sizeof(header));
	CopyMemory(header.fd_name, "VAEG test FDD", 13);
	header.fd_type = profile->d88_type;
	track_count = profile->cylinders * 2;
	offset = sizeof(header);
	for (track = 0; track < track_count; track++) {
		const UINT table_track = ((track >> 1) * profile->d88_track_stride) | (track & 1);
		STOREINTELDWORD(header.trackp[table_track], offset);
		offset += profile->sectors * (sizeof(sector_header) + profile->sector_size);
	}
	STOREINTELDWORD(header.fd_size, offset);

	fh = file_create(path);
	if (fh == FILEH_INVALID) {
		return (FAILURE);
	}
	if (file_write(fh, &header, sizeof(header)) != sizeof(header)) {
		file_close(fh);
		return (FAILURE);
	}
	for (track = 0; track < track_count; track++) {
		for (sector = 1; sector <= profile->sectors; sector++) {
			ZeroMemory(&sector_header, sizeof(sector_header));
			sector_header.c = (BYTE)(track >> 1);
			sector_header.h = (BYTE)(track & 1);
			sector_header.r = (BYTE)sector;
			sector_header.n = profile->sector_n;
			STOREINTELWORD(sector_header.sectors, profile->sectors);
			/* Match the controller's MFM lookup convention used by newdisk.c. */
			sector_header.mfm_flg = 0x00;
			STOREINTELWORD(sector_header.size, profile->sector_size);
			if (file_write(fh, &sector_header, sizeof(sector_header)) != sizeof(sector_header)) {
				file_close(fh);
				return (FAILURE);
			}
			for (index = 0; index < profile->sector_size; index++) {
				data[index] = selftest_fdd_profile_byte(track, sector, index);
			}
			if (file_write(fh, data, profile->sector_size) != profile->sector_size) {
				file_close(fh);
				return (FAILURE);
			}
		}
	}
	file_close(fh);
	return (SUCCESS);
}

static BYTE selftest_fdc_mode(const SELFTESTFDDPROFILE *profile, UINT drive, UINT density_96) {
	return (BYTE)((profile->dskctl_2hd ? (1 << drive) : 0) |
	              (density_96 ? (4 << drive) : 0) | (profile->clock_8mhz ? 0x20 : 0));
}

static BOOL selftest_fdd_profile_read(const SELFTESTFDDPROFILE *profile, UINT drive, UINT cylinder,
	                                  UINT head, UINT sector, UINT physical_cylinder,
	                                  UINT density_96, BOOL changed_data) {
	UINT index;
	UINT track;
	BYTE expected;

	fdc.us = (UINT8)drive;
	fdc.hd = (UINT8)head;
	fdc.ncn = (UINT8)physical_cylinder;
	fdc.C = (UINT8)cylinder;
	fdc.H = (UINT8)head;
	fdc.R = (UINT8)sector;
	fdc.N = profile->sector_n;
	fdc.mf = 0x40;
	fdc.rpm[drive] = 0;
	fdcsubsys_o_dskctl(selftest_fdc_mode(profile, drive, density_96));
	if (fdd_diskaccess() != SUCCESS) {
		fprintf(stderr, "selftest: %s media access rejected: CHS=%u/%u/%u\n", profile->name,
		        cylinder, head, sector);
		return (FAILURE);
	}
	if (fdd_seek() != SUCCESS) {
		fprintf(stderr,
		        "selftest: %s seek failed: CHS=%u/%u/%u physical=%u TPI=%u\n", profile->name,
		        cylinder, head, sector, physical_cylinder, density_96 ? 96 : 48);
		return (FAILURE);
	}
	if ((fdd_read() != SUCCESS) || (fdc.bufcnt != profile->sector_size)) {
		fprintf(stderr,
		        "selftest: %s read failed: spt=%u CHS=%u/%u/%u physical=%u TPI=%u\n",
		        profile->name, profile->sectors, cylinder, head, sector, physical_cylinder,
		        density_96 ? 96 : 48);
		return (FAILURE);
	}
	track = cylinder * 2 + head;
	for (index = 0; index < profile->sector_size; index++) {
		expected = changed_data ? (BYTE)(0x69 ^ (index * 7)) :
		                          selftest_fdd_profile_byte(track, sector, index);
		if (fdc.buf[index] != expected) {
			return (FAILURE);
		}
	}
	return (SUCCESS);
}

static void selftest_fdc_send_command(const BYTE *command, UINT length) {
	UINT index;

	for (index = 0; index < length; index++) {
		fdc_datawrite(command[index]);
	}
}

static BOOL selftest_fdc_wait_rqm(void) {
	UINT attempt;

	for (attempt = 0; attempt < 64; attempt++) {
		if (fdc.status & FDCSTAT_RQM) {
			return (SUCCESS);
		}
		if (fdc.rqminterval <= 0) {
			return (FAILURE);
		}
		fdc.rqmlastclock = CPU_CLOCK - fdc.rqminterval;
		fdc_statewatch(NULL);
	}
	return (FAILURE);
}

static BOOL selftest_fdc_take_result(BYTE expected_st0, BYTE expected_st1) {
	BYTE result[7];
	UINT index;

	if ((fdc.status & (FDCSTAT_RQM | FDCSTAT_DIO)) != (FDCSTAT_RQM | FDCSTAT_DIO)) {
		return (FAILURE);
	}
	for (index = 0; index < NELEMENTS(result); index++) {
		result[index] = fdc_dataread();
	}
	if (result[1] != expected_st1 || (!expected_st1 &&
	                                  ((result[0] != expected_st0) || (result[2] != 0)))) {
		fprintf(stderr, "selftest: unexpected FDC result ST0=%02x ST1=%02x ST2=%02x expected ST1=%02x\n",
		        result[0], result[1], result[2], expected_st1);
		return (FAILURE);
	}
	return (SUCCESS);
}


static void selftest_fdc_prepare(const SELFTESTFDDPROFILE *profile, UINT drive, UINT head,
	                            UINT physical_cylinder, UINT density_96) {
	const BYTE specify[] = {0x03, 0xdf, 0x01};
	BYTE seek[] = {0x0f, (BYTE)((head << 2) | drive), (BYTE)physical_cylinder};

	fdc.event = 0;
	fdc.status = FDCSTAT_RQM;
	fdc.rqm = FALSE;
	fdc.tcreserved = FALSE;
	fdc.cmdp = 0;
	fdc.cmdcnt = 0;
	fdc.bufp = 0;
	fdc.bufcnt = 0;
	fdc.intreq = 0;
	fdc.stat[0] = 0;
	fdc.ctrlreg = 0;
	fdc.equip |= (UINT8)(1 << drive);
	fdcsubsys_o_dskctl(selftest_fdc_mode(profile, drive, density_96));
	selftest_fdc_send_command(specify, NELEMENTS(specify));
	selftest_fdc_send_command(seek, NELEMENTS(seek));
}

static BOOL selftest_fdc_read_profile(const SELFTESTFDDPROFILE *profile, UINT drive, UINT cylinder,
	                                 UINT head, UINT sector, UINT physical_cylinder,
	                                 UINT density_96, BYTE *data, BYTE expected_st1) {
	BYTE command[9];
	UINT index;

	selftest_fdc_prepare(profile, drive, head, physical_cylinder, density_96);
	command[0] = 0x46;
	command[1] = (BYTE)((head << 2) | drive);
	command[2] = (BYTE)cylinder;
	command[3] = (BYTE)head;
	command[4] = (BYTE)sector;
	command[5] = profile->sector_n;
	command[6] = profile->sectors;
	command[7] = 0x2a;
	command[8] = 0xff;
	selftest_fdc_send_command(command, NELEMENTS(command));
	if (fdc.rqminterval != selftest_fdc_profile_interval(profile)) {
		fprintf(stderr, "selftest: %s FDC read interval=%d expected=%d\n", profile->name,
		        fdc.rqminterval, selftest_fdc_profile_interval(profile));
		return (FAILURE);
	}
	if (selftest_fdc_wait_rqm() != SUCCESS) {
		fprintf(stderr, "selftest: %s FDC read request wait failed status=%02x event=%d count=%d\n",
		        profile->name,
		        fdc.status, fdc.event, fdc.bufcnt);
		return (FAILURE);
	}
	if (fdc.bufcnt == 7) {
		if (!expected_st1 ||
		    (selftest_fdc_take_result((BYTE)((head << 2) | drive), expected_st1) != SUCCESS)) {
			fprintf(stderr, "selftest: %s FDC read returned unexpected result ST0=%02x\n",
			        profile->name, fdc.buf[0]);
			return (FAILURE);
		}
		return (SUCCESS);
	}
	if (expected_st1 || (fdc.bufcnt != profile->sector_size)) {
		fprintf(stderr, "selftest: %s FDC read got count=%d expected=%u ST=%02x\n", profile->name,
		        fdc.bufcnt, profile->sector_size, fdc.status);
		return (FAILURE);
	}
	for (index = 0; index < profile->sector_size; index++) {
		if (selftest_fdc_wait_rqm() != SUCCESS) {
			fprintf(stderr, "%s FDC read byte wait failed index=%u status=%02x event=%d\n",
			        profile->name, index, fdc.status, fdc.event);
			return (FAILURE);
		}
		data[index] = fdc_dataread();
	}
	fdcsubsys_o_tc();
	if (selftest_fdc_take_result((BYTE)((head << 2) | drive), 0) != SUCCESS) {
		fprintf(stderr, "selftest: %s FDC read result invalid status=%02x event=%d count=%d\n",
		        profile->name, fdc.status, fdc.event, fdc.bufcnt);
		return (FAILURE);
	}
	return (SUCCESS);
}

static BOOL selftest_fdc_write_profile(const SELFTESTFDDPROFILE *profile, UINT drive,
	                                  UINT cylinder, UINT head, UINT sector,
	                                  UINT physical_cylinder, UINT density_96, const BYTE *data,
	                                  BYTE expected_st1) {
	BYTE command[9];
	UINT index;

	selftest_fdc_prepare(profile, drive, head, physical_cylinder, density_96);
	command[0] = 0x45;
	command[1] = (BYTE)((head << 2) | drive);
	command[2] = (BYTE)cylinder;
	command[3] = (BYTE)head;
	command[4] = (BYTE)sector;
	command[5] = profile->sector_n;
	command[6] = profile->sectors;
	command[7] = 0x2a;
	command[8] = 0xff;
	selftest_fdc_send_command(command, NELEMENTS(command));
	if (fdc.rqminterval != selftest_fdc_profile_interval(profile)) {
		fprintf(stderr, "selftest: %s FDC write interval=%d expected=%d\n", profile->name,
		        fdc.rqminterval, selftest_fdc_profile_interval(profile));
		return (FAILURE);
	}
	if (selftest_fdc_wait_rqm() != SUCCESS) {
		return (FAILURE);
	}
	if (fdc.bufcnt == 7) {
		if (!expected_st1) {
			return (FAILURE);
		}
		return (selftest_fdc_take_result((BYTE)((head << 2) | drive), expected_st1));
	}
	if (expected_st1 || (fdc.bufcnt != profile->sector_size)) {
		return (FAILURE);
	}
	for (index = 0; index < profile->sector_size; index++) {
		if (selftest_fdc_wait_rqm() != SUCCESS) {
			return (FAILURE);
		}
		fdc_datawrite(data[index]);
	}
	fdcsubsys_o_tc();
	return (selftest_fdc_take_result((BYTE)((head << 2) | drive), 0));
}

static int test_fdd_d88_production_path(void) {
	const SELFTESTFDDPROFILE *profile;
	_FDC saved_fdc;
	_DMAC saved_dmac;
	_PIC saved_pic;
	_NEVENT saved_nevent;
	_FDDFILE saved_drives[2];
	BYTE saved_error;
	BYTE fdc_data[1024];
	UINT index;
	BYTE data[1024];
	char path[MAX_PATH];
	char path_b[MAX_PATH];
	UINT cylinder;
	UINT head;
	UINT sector;
	UINT profile_index;
	int result;

	saved_fdc = fdc;
	saved_dmac = dmac;
	saved_pic = pic;
	saved_nevent = nevent;
	saved_drives[0] = fddfile[0];
	saved_drives[1] = fddfile[1];
	saved_error = fddlasterror;
	path[0] = '\0';
	path_b[0] = '\0';
	result = SUCCESS;
	(void)fdd_eject(0);
	(void)fdd_eject(1);
	for (profile_index = 0; profile_index < NELEMENTS(selftest_fdd_profiles); profile_index++) {
		profile = selftest_fdd_profiles + profile_index;
		const UINT density = profile->density_96;
		const UINT write_cylinder = 2;
		const UINT write_sector = 5;

		SPRINTF(path, "vaeg-selftest-%lu-%s.d88", (unsigned long)getpid(), profile->name);
		file_delete(path);
		if (selftest_fdd_create_profile(path, profile) != SUCCESS) {
			result = fail("fdd-media", "synthetic D88 creation failed");
			goto fdd_2d_cleanup;
		}
		if (fdd_set(0, path, FTYPE_NONE, 0) != SUCCESS) {
			result = fail("fdd-media", "production D88 loader rejected synthetic media");
			goto fdd_2d_cleanup;
		}

		if ((selftest_fdd_profile_read(profile, 0, 0, 0, 1, 0, density, FALSE) != SUCCESS) ||
		    (selftest_fdd_profile_read(profile, 0, profile->cylinders - 1, 1, profile->sectors,
		                               profile->cylinders - 1, density, FALSE) != SUCCESS)) {
			result = fail("fdd-media", "first/last cylinder or head/sector boundary read failed");
			goto fdd_2d_cleanup;
		}

		if (profile->d88_type == 0x00) {
			/* 2D at 96 TPI reaches the recorded cylinder by stepping twice. */
			if (selftest_fdd_profile_read(profile, 0, 1, 1, profile->sectors, 2, 1, FALSE) !=
			    SUCCESS) {
				result = fail("fdd-2d", "96-TPI double-step 2D read failed");
				goto fdd_2d_cleanup;
			}
		}
		cylinder = (profile->d88_type == 0x00) ? write_cylinder : profile->cylinders - 1;
		head = (profile->d88_type == 0x00) ? 0 : 1;
		sector = (profile->d88_type == 0x00) ? write_sector : profile->sectors;
		if (selftest_fdc_read_profile(profile, 0, cylinder, head, sector, cylinder, density,
		                              fdc_data, 0) != SUCCESS) {
			result = fail("fdd-media", "FDC READ DATA command path rejected the profile");
			goto fdd_2d_cleanup;
		}
		for (index = 0; index < profile->sector_size; index++) {
			if (fdc_data[index] != selftest_fdd_profile_byte(cylinder * 2 + head, sector, index)) {
				result = fail("fdd-media", "FDC READ DATA returned incorrect bytes");
				goto fdd_2d_cleanup;
			}
		}
		if ((selftest_fdc_read_profile(profile, 0, profile->cylinders - 1, 1,
		                               profile->sectors, profile->cylinders - 1, density,
		                               fdc_data, 0) != SUCCESS)) {
			result = fail("fdd-media", "FDC READ DATA rejected the last track/head/sector");
			goto fdd_2d_cleanup;
		}
		for (index = 0; index < profile->sector_size; index++) {
			if (fdc_data[index] != selftest_fdd_profile_byte(profile->cylinders * 2 - 1,
			                                               profile->sectors, index)) {
				result = fail("fdd-media", "FDC READ DATA returned incorrect last-sector bytes");
				goto fdd_2d_cleanup;
			}
		}
		if (profile->d88_type == 0x00) {
			if (selftest_fdc_read_profile(profile, 0, 0, 0, 1, 1, 1, fdc_data, 0x04) != SUCCESS) {
				result = fail("fdd-2d", "FDC did not report an invalid 96-TPI half-track");
				goto fdd_2d_cleanup;
			}
		}
		for (index = 0; index < profile->sector_size; index++) {
			data[index] = (BYTE)(0xa5 ^ index);
		}
		fddfile[0].protect = TRUE;
		if (selftest_fdc_write_profile(profile, 0, 0, 0, 1, 0, density, data, 0x02) != SUCCESS) {
			fddfile[0].protect = FALSE;
			result = fail("fdd-media", "FDC WRITE DATA did not report write protection");
			goto fdd_2d_cleanup;
		}
		fddfile[0].protect = FALSE;
		if (selftest_fdc_read_profile(profile, 0, 0, 0, 1, 0, density, fdc_data, 0) != SUCCESS) {
			result = fail("fdd-media", "protected FDC write damaged the source sector");
			goto fdd_2d_cleanup;
		}
		for (index = 0; index < profile->sector_size; index++) {
			if (fdc_data[index] != selftest_fdd_profile_byte(0, 1, index)) {
				result = fail("fdd-media", "protected FDC write changed source bytes");
				goto fdd_2d_cleanup;
			}
		}
		for (index = 0; index < profile->sector_size; index++) {
			data[index] = (BYTE)(0x69 ^ (index * 7));
		}
		if ((selftest_fdc_write_profile(profile, 0, write_cylinder, 0, write_sector,
		                                write_cylinder, density, data, 0) != SUCCESS) ||
		    (fdd_eject(0) != SUCCESS) || (fdd_set(0, path, FTYPE_NONE, 0) != SUCCESS) ||
		    (selftest_fdc_read_profile(profile, 0, write_cylinder, 0, write_sector,
		                               write_cylinder, density, fdc_data, 0) != SUCCESS)) {
			result = fail("fdd-media", "FDC write did not persist after eject and reopen");
			goto fdd_2d_cleanup;
		}
		for (index = 0; index < profile->sector_size; index++) {
			if (fdc_data[index] != data[index]) {
				result = fail("fdd-media", "FDC writeback did not preserve bytes");
				goto fdd_2d_cleanup;
			}
		}
		if (fdd_eject(0) != SUCCESS) {
			result = fail("fdd-media", "synthetic media could not be cleanly ejected");
			goto fdd_2d_cleanup;
		}
		file_delete(path);
		path[0] = '\0';
	}

	/* Exercise every requested format on FDD2 while a different format remains on FDD1. */
	{
		const UINT profile_b_indices[] = {0, 2, 3, 4, 5};
		UINT pair_index;
		for (pair_index = 0; pair_index < NELEMENTS(profile_b_indices); pair_index++) {
			const SELFTESTFDDPROFILE *profile_b =
			    selftest_fdd_profiles + profile_b_indices[pair_index];
			const SELFTESTFDDPROFILE *profile_a =
			    selftest_fdd_profiles + ((profile_b_indices[pair_index] == 0) ? 2 : 0);
			const UINT cylinder_a = 1;
			const UINT cylinder_b = profile_b->cylinders - 1;
			const UINT head_b = 1;
			const UINT sector_b = profile_b->sectors;
			SPRINTF(path, "vaeg-selftest-%lu-drive-a-%s.d88", (unsigned long)getpid(),
			        profile_b->name);
			SPRINTF(path_b, "vaeg-selftest-%lu-drive-b-%s.d88", (unsigned long)getpid(),
			        profile_b->name);
			file_delete(path);
			file_delete(path_b);
			if ((selftest_fdd_create_profile(path, profile_a) != SUCCESS) ||
			    (selftest_fdd_create_profile(path_b, profile_b) != SUCCESS) ||
			    (fdd_set(0, path, FTYPE_NONE, 0) != SUCCESS) ||
			    (fdd_set(1, path_b, FTYPE_NONE, 0) != SUCCESS)) {
				result = fail("fdd-drives", "different-format synthetic media could not be mounted");
				goto fdd_2d_cleanup;
			}
			if ((selftest_fdc_read_profile(profile_b, 1, cylinder_b, head_b, sector_b,
			                               cylinder_b, profile_b->density_96, fdc_data, 0) != SUCCESS) ||
			    (fdc_data[0] != selftest_fdd_profile_byte(cylinder_b * 2 + head_b, sector_b, 0)) ||
			    (selftest_fdc_read_profile(profile_a, 0, cylinder_a, 0, 1, cylinder_a,
			                               profile_a->density_96, fdc_data, 0) != SUCCESS) ||
			    (fdc_data[0] != selftest_fdd_profile_byte(cylinder_a * 2, 1, 0)) ||
			    (selftest_fdc_read_profile(profile_b, 1, cylinder_b, head_b, sector_b,
			                               cylinder_b, profile_b->density_96, fdc_data, 0) != SUCCESS) ||
			    (fdc_data[0] != selftest_fdd_profile_byte(cylinder_b * 2 + head_b, sector_b, 0))) {
				result = fail("fdd-drives", "alternating FDC selection mixed independent media");
				goto fdd_2d_cleanup;
			}
			for (index = 0; index < profile_b->sector_size; index++) {
				data[index] = (BYTE)(0x36 + index * 9);
			}
			if ((selftest_fdc_write_profile(profile_b, 1, cylinder_b, head_b, sector_b,
			                                cylinder_b, profile_b->density_96, data, 0) != SUCCESS) ||
			    (selftest_fdc_read_profile(profile_a, 0, cylinder_a, 0, 1, cylinder_a,
			                               profile_a->density_96, fdc_data, 0) != SUCCESS) ||
			    (fdc_data[0] != selftest_fdd_profile_byte(cylinder_a * 2, 1, 0)) ||
			    (fdd_eject(1) != SUCCESS) || (fdd_set(1, path_b, FTYPE_NONE, 0) != SUCCESS) ||
			    (selftest_fdc_read_profile(profile_b, 1, cylinder_b, head_b, sector_b,
			                               cylinder_b, profile_b->density_96, fdc_data, 0) != SUCCESS)) {
				result = fail("fdd-drives", "FDD2 writeback or non-target preservation failed");
				goto fdd_2d_cleanup;
			}
			for (index = 0; index < profile_b->sector_size; index++) {
				if (fdc_data[index] != data[index]) {
					result = fail("fdd-drives", "FDD2 write did not persist after reopen");
					goto fdd_2d_cleanup;
				}
			}
			if ((fdd_eject(0) != SUCCESS) || (fdd_eject(1) != SUCCESS)) {
				result = fail("fdd-drives", "concurrent FDD media could not be ejected");
				goto fdd_2d_cleanup;
			}
			file_delete(path);
			file_delete(path_b);
			path[0] = '\0';
			path_b[0] = '\0';
		}
	}

fdd_2d_cleanup:
	(void)fdd_eject(0);
	(void)fdd_eject(1);
	if (path[0]) {
		file_delete(path);
	}
	if (path_b[0]) {
		file_delete(path_b);
	}
	fddfile[0] = saved_drives[0];
	fddfile[1] = saved_drives[1];
	fdc = saved_fdc;
	dmac = saved_dmac;
	pic = saved_pic;
	nevent = saved_nevent;
	fddlasterror = saved_error;
	if (result == SUCCESS) {
		fprintf(stderr, "selftest: floppy D88 production paths ok\n");
	}
	return (result);
}

static int test_sasi_image_validation(void) {
	char valid_path[MAX_PATH];
	char invalid_path[MAX_PATH];
	FILEH fh;
	SXSIDEV slot;
	_SXSIDEV saved_slot;
	HDIHDR valid_header;
	BOOL slot_overridden = FALSE;
	int result;

	SPRINTF(valid_path, "vaeg-selftest-%lu-sasi.hdi", (unsigned long)getpid());
	SPRINTF(invalid_path, "vaeg-selftest-%lu-invalid.hdi", (unsigned long)getpid());
	file_delete(valid_path);
	file_delete(invalid_path);
	newdisk_hdi(valid_path, 0);
	result = SUCCESS;
	if (sxsi_hddvalidate_sasi(valid_path) != SUCCESS) {
		result = fail("SASI image", "valid generated HDI was rejected");
		goto done;
	}
	fh = file_open_rb(valid_path);
	if ((fh == FILEH_INVALID) ||
	    (file_read(fh, &valid_header, sizeof(valid_header)) != sizeof(valid_header))) {
		if (fh != FILEH_INVALID) {
			file_close(fh);
		}
		result = fail("SASI image", "valid HDI header could not be read");
		goto done;
	}
	file_close(fh);
	fh = file_create(invalid_path);
	if ((fh == FILEH_INVALID) ||
	    (file_write(fh, &valid_header, sizeof(valid_header)) != sizeof(valid_header))) {
		if (fh != FILEH_INVALID) {
			file_close(fh);
		}
		result = fail("SASI image", "truncated test HDI could not be created");
		goto done;
	}
	file_close(fh);
	if (sxsi_hddvalidate_sasi(invalid_path) != FAILURE) {
		result = fail("SASI image", "truncated SASI HDI was accepted");
	}

	slot = sxsi_getptr(0);
	saved_slot = *slot;
	ZeroMemory(slot, sizeof(*slot));
	slot->fh = FILEH_INVALID;
	slot_overridden = TRUE;
	if ((result == SUCCESS) && (sxsi_hddopen(0, valid_path) != SUCCESS)) {
		result = fail("SASI image", "valid HDI could not be mounted");
	} else if ((result == SUCCESS) && ((slot->type & SXSITYPE_IFMASK) != SXSITYPE_SASI)) {
		result = fail("SASI image", "mounted HDI was not classified as SASI");
	}

done:
	if (slot_overridden) {
		if (slot->fh != FILEH_INVALID) {
			file_close(slot->fh);
		}
		*slot = saved_slot;
	}
	file_delete(valid_path);
	file_delete(invalid_path);
	if (result == SUCCESS) {
		fprintf(stderr, "selftest: SASI image validation ok\n");
	}
	return (result);
}

static int test_profile_ini(void) {
	char path[MAX_PATH];
	char name[32];
	char read_name[32];
	UINT8 flag;
	UINT8 read_flag;
	UINT16 count;
	UINT16 read_count;
	UINT8 bytes[3];
	UINT8 read_bytes[3];
	UINT8 effect;
	UINT8 read_effect;
	UINT16 window_width;
	UINT16 read_window_width;
	UINT16 pacing_ms;
	UINT16 read_pacing_ms;
	UINT8 ndp8087_enable;
	UINT8 read_ndp8087_enable;
	UINT32 ndp8087_clock_hz;
	UINT32 read_ndp8087_clock_hz;
	_BMSIOCFG write_bms;
	UINT8 ems_megabytes;
	UINT8 read_ems_megabytes;
	UINT16 main_ram;
	UINT16 read_main_ram;
	_BMSIOCFG read_bms;
	PFTBL write_tbl[] = {
	    {"name", PFTYPE_STR, name, sizeof(name)}, {"flag", PFTYPE_BOOL, &flag, 0},
	    {"count", PFTYPE_UINT16, &count, 0},      {"bytes", PFTYPE_BIN, bytes, sizeof(bytes)},
	    {"effect", PFTYPE_UINT8, &effect, 0},     {"win_width", PFTYPE_UINT16, &window_width, 0}};
	PFTBL read_tbl[] = {{"name", PFTYPE_STR, read_name, sizeof(read_name)},
	                    {"flag", PFTYPE_BOOL, &read_flag, 0},
	                    {"count", PFTYPE_UINT16, &read_count, 0},
	                    {"bytes", PFTYPE_BIN, read_bytes, sizeof(read_bytes)},
	                    {"effect", PFTYPE_UINT8, &read_effect, 0},
	                    {"win_width", PFTYPE_UINT16, &read_window_width, 0}};
	INITBL write_bms_tbl[] = {{"Use_BMS_", INITYPE_BOOL, &write_bms.enabled, 0},
	                          {"BMS_Port", INITYPE_HEX16, &write_bms.port, 0},
	                          {"BMS_Size", INITYPE_UINT8, &write_bms.numbanks, 0},
	                          {"ExMemory", INITYPE_UINT8, &ems_megabytes, 0},
	                          {"Main_RAM", INITYPE_UINT16, &main_ram, 0},
	                          {"PacingMs", INITYPE_UINT16, &pacing_ms, 0},
	                          {"NDP8087", INITYPE_BOOL, &ndp8087_enable, 0},
	                          {"NDP8087Hz", INITYPE_UINT32, &ndp8087_clock_hz, 0}};
	INITBL read_bms_tbl[] = {{"Use_BMS_", INITYPE_BOOL, &read_bms.enabled, 0},
	                         {"BMS_Port", INITYPE_HEX16, &read_bms.port, 0},
	                         {"BMS_Size", INITYPE_UINT8, &read_bms.numbanks, 0},
	                         {"ExMemory", INITYPE_UINT8, &read_ems_megabytes, 0},
	                         {"Main_RAM", INITYPE_UINT16, &read_main_ram, 0},
	                         {"PacingMs", INITYPE_UINT16, &read_pacing_ms, 0},
	                         {"NDP8087", INITYPE_BOOL, &read_ndp8087_enable, 0},
	                         {"NDP8087Hz", INITYPE_UINT32, &read_ndp8087_clock_hz, 0}};

	SPRINTF(path, "vaeg-selftest-%lu.ini", (unsigned long)getpid());
	file_delete(path);
	file_cpyname(name, "portable", sizeof(name));
	flag = 1;
	count = 0x1234;
	bytes[0] = 0x12;
	bytes[1] = 0x34;
	bytes[2] = 0xab;
	effect = VAEG_EFFECT_CRT_LITE;
	window_width = 1280;
	pacing_ms = 64;
	ZeroMemory(read_name, sizeof(read_name));
	ems_megabytes = 7;
	read_flag = 0;
	read_count = 0;
	ZeroMemory(read_bytes, sizeof(read_bytes));
	read_effect = 0;
	read_window_width = 0;
	read_pacing_ms = 0;
	write_bms.enabled = TRUE;
	read_ems_megabytes = 0;
	main_ram = 384;
	read_main_ram = 0;
	write_bms.port = BMSIO_PORT_COMPAT;
	write_bms.portmask = BMSIO_PORT_MASK;
	write_bms.numbanks = 32;
	ndp8087_enable = 1;
	ndp8087_clock_hz = 12345678U;
	read_ndp8087_enable = 0;
	read_ndp8087_clock_hz = 0;
	ZeroMemory(&read_bms, sizeof(read_bms));

	profile_iniwrite(path, "selftest", write_tbl, NELEMENTS(write_tbl), NULL);
	profile_iniread(path, "selftest", read_tbl, NELEMENTS(read_tbl), NULL);
	ini_write(path, "selftest-bms", write_bms_tbl, NELEMENTS(write_bms_tbl));
	ini_read(path, "selftest-bms", read_bms_tbl, NELEMENTS(read_bms_tbl));
	file_delete(path);

	if (strcmp(read_name, name) != 0) {
		return (fail("ini", "string value did not round-trip"));
	}
	if ((read_flag != flag) || (read_count != count) ||
	    (memcmp(read_bytes, bytes, sizeof(bytes)) != 0) || (read_effect != effect) ||
	    (read_window_width != window_width)) {
		return (fail("ini", "typed values did not round-trip"));
	}
	if ((read_bms.enabled != TRUE) || (read_bms.port != BMSIO_PORT_COMPAT) ||
	    (read_bms.numbanks != 32) || (read_pacing_ms != pacing_ms) ||
	    (read_ems_megabytes != ems_megabytes) || (read_main_ram != main_ram) ||
	    (read_ndp8087_enable != ndp8087_enable) ||
	    (read_ndp8087_clock_hz != ndp8087_clock_hz)) {
		return (fail("ini", "BMS/EMS settings did not round-trip"));
	}
	fprintf(stderr, "selftest: ini ok\n");
	return (SUCCESS);
}

static int test_persistence_controls(void) {
	char saved_crt[sizeof(np2oscfg.gui_shader_parameters)];
	char read_crt[sizeof(np2oscfg.gui_shader_parameters)] = {0};
	INITBL crt_table[] = {{"NativeCRTParameters", INITYPE_STR, read_crt, sizeof(read_crt)}};
	char config_path[MAX_PATH];
	char backup_path[MAX_PATH];
	char missing_backup_path[MAX_PATH];
	BYTE saved_backup_memory[0x04000];
	UINT16 saved_main_ram;
	const char *detail;

	SPRINTF(config_path, "vaeg-selftest-%lu.cfg", (unsigned long)getpid());
	SPRINTF(backup_path, "vaeg-selftest-%lu-bkup.dat", (unsigned long)getpid());
	SPRINTF(missing_backup_path, "vaeg-selftest-%lu-missing-bkup.dat", (unsigned long)getpid());
	file_delete(config_path);
	file_delete(backup_path);
	file_delete(missing_backup_path);
	CopyMemory(saved_backup_memory, backupmem, sizeof(saved_backup_memory));
	saved_main_ram = np2cfg.main_ram;
	detail = NULL;

	initsetpath(config_path);
	initsetenabled(TRUE);
	milstr_ncpy(saved_crt, np2oscfg.gui_shader_parameters, sizeof(saved_crt));
	milstr_ncpy(np2oscfg.gui_shader_parameters,
	            "VAEG_SHADER_PARAMETERS 1;SCREEN_SIZE=96.5;CURVATURE=0.0299999993;",
	            sizeof(np2oscfg.gui_shader_parameters));
	initsave();
	if (file_attr(config_path) < 0) {
		detail = "explicit configuration path was not written";
	}
	ini_read(config_path, "NekoProjectII", crt_table, NELEMENTS(crt_table));
	if (strcmp(read_crt, np2oscfg.gui_shader_parameters) != 0) {
		detail = "CRT parameters did not round-trip through the main config";
	}
	milstr_ncpy(np2oscfg.gui_shader_parameters, saved_crt, sizeof(saved_crt));
	file_delete(config_path);
	initsetenabled(FALSE);
	initsave();
	if ((detail == NULL) && (file_attr(config_path) >= 0)) {
		detail = "disabled configuration persistence wrote a file";
	}
	initsetpath(NULL);
	initsetenabled(TRUE);

	bkupmemva_setpath(backup_path);
	bkupmemva_setenabled(TRUE);
	bkupmemva_save();
	if ((detail == NULL) && (file_attr(backup_path) < 0)) {
		detail = "explicit backup-memory path was not written";
	}
	file_delete(backup_path);
	bkupmemva_setenabled(FALSE);
	bkupmemva_save();
	if ((detail == NULL) && (file_attr(backup_path) >= 0)) {
		detail = "disabled backup-memory persistence wrote a file";
	}
	bkupmemva_setpath(missing_backup_path);
	bkupmemva_setenabled(TRUE);
	np2cfg.main_ram = 640;
	ZeroMemory(backupmem, sizeof(backupmem));
	backupmem[0] = 0xa5;
	bkupmemva_load();
	if ((detail == NULL) && ((backupmem[0] != 0) || (backupmem[0x1fc4] != 0x64) ||
	                         (backupmem[0x1fc8] != 0x4b) || (backupmem[0x1fc9] != 0x5a) ||
	                         (backupmem[0x1fca] != 0x4d) || (backupmem[0x1fcd] != 0x48))) {
		detail = "missing backup-memory image was not seeded from Main_RAM";
	}
	np2cfg.main_ram = saved_main_ram;
	CopyMemory(backupmem, saved_backup_memory, sizeof(saved_backup_memory));
	bkupmemva_setpath(NULL);
	bkupmemva_setenabled(TRUE);
	file_delete(config_path);
	file_delete(backup_path);
	file_delete(missing_backup_path);

	if (detail != NULL) {
		return (fail("persistence controls", detail));
	}
	fprintf(stderr, "selftest: persistence controls ok\n");
	return (SUCCESS);
}

static int test_main_ram_configuration(void) {
	static const UINT16 capacities[] = {256, 384, 512, 640};
	BYTE saved_backup_memory[0x04000];
	UINT16 saved_main_ram;
	UINT16 capacity;
	UINT32 limit;
	BYTE saved_below;
	BYTE saved_above;
	BYTE checksum;
	int i;
	int j;

	saved_main_ram = np2cfg.main_ram;
	CopyMemory(saved_backup_memory, backupmem, sizeof(saved_backup_memory));
	for (i = 0; i < NELEMENTS(capacities); i++) {
		capacity = capacities[i];
		np2cfg.main_ram = capacity;
		limit = pccore_mainram_limit();
		if ((pccore_mainram_kb() != capacity) || (limit != (UINT32)capacity * 1024)) {
			np2cfg.main_ram = saved_main_ram;
			CopyMemory(backupmem, saved_backup_memory, sizeof(saved_backup_memory));
			return (fail("Main_RAM", "capacity normalization failed"));
		}

		/* Build a BIOS-shaped record only for this host-side configuration test. */
		ZeroMemory(backupmem, sizeof(backupmem));
		backupmem[0x1fc2] = 0xdb;
		backupmem[0x1fc3] = 0x02;
		backupmem[0x1fc4] = (BYTE)(0x60 | ((capacity / 128) - 1));
		backupmem[0x1fc5] = 0x04;
		backupmem[0x1fc6] = 0xf9;
		backupmem[0x1fc7] = 0x0a;
		backupmem[0x1fc8] = 0x4b;
		backupmem[0x1fc9] = 0x5a;
		backupmem[0x1fca] = 0x4d;
		backupmem[0x1fcb] = 0xff;
		backupmem[0x1fcc] = 0xff;
		checksum = 0;
		for (j = 0; j < 8; j++) {
			checksum = (BYTE)(checksum + backupmem[0x1fc0 + j]);
		}
		backupmem[0x1fcd] = checksum;
		if (backupmem[0x1fc4] != (BYTE)(0x60 | ((capacity / 128) - 1)) ||
		    backupmem[0x1fc8] != 0x4b || backupmem[0x1fc9] != 0x5a || backupmem[0x1fca] != 0x4d ||
		    backupmem[0x1fcd] != checksum) {
			np2cfg.main_ram = saved_main_ram;
			CopyMemory(backupmem, saved_backup_memory, sizeof(saved_backup_memory));
			return (fail("Main_RAM", "backup-memory capacity record failed"));
		}

		saved_below = mem[limit - 1];
		upd9002_mainram_write(limit - 1, 0x5a);
		if (upd9002_mainram_read(limit - 1) != 0x5a) {
			np2cfg.main_ram = saved_main_ram;
			mem[limit - 1] = saved_below;
			CopyMemory(backupmem, saved_backup_memory, sizeof(saved_backup_memory));
			return (fail("Main_RAM", "last installed byte was not writable"));
		}
		mem[limit - 1] = saved_below;
		if (limit < UPD9002_MAINRAM_LIMIT) {
			saved_above = mem[limit];
			upd9002_mainram_write(limit, 0xa5);
			if (upd9002_mainram_read(limit) != 0xff) {
				np2cfg.main_ram = saved_main_ram;
				mem[limit] = saved_above;
				CopyMemory(backupmem, saved_backup_memory, sizeof(saved_backup_memory));
				return (fail("Main_RAM", "uninstalled byte was accessible"));
			}
			mem[limit] = saved_above;
		}
	}
	np2cfg.main_ram = saved_main_ram;
	CopyMemory(backupmem, saved_backup_memory, sizeof(saved_backup_memory));
	fprintf(stderr, "selftest: Main_RAM physical ceiling ok\n");
	return (SUCCESS);
}

static int test_va_layer_display(void) {
	BOOL saved[VAEG_VA_LAYER_COUNT];
	UINT layer;

	for (layer = 0; layer < VAEG_VA_LAYER_COUNT; layer++) {
		saved[layer] = scrndrawva_layer_enabled(layer);
		if (!saved[layer]) {
			return (fail("VA layer display", "default layer is disabled"));
		}
		scrndrawva_set_layer_enabled(layer, FALSE);
		if (scrndrawva_layer_enabled(layer)) {
			return (fail("VA layer display", "layer disable was ignored"));
		}
		scrndrawva_set_layer_enabled(layer, TRUE);
		if (!scrndrawva_layer_enabled(layer)) {
			return (fail("VA layer display", "layer enable was ignored"));
		}
	}
	if (scrndrawva_layer_enabled(VAEG_VA_LAYER_COUNT)) {
		return (fail("VA layer display", "invalid layer was accepted"));
	}
	for (layer = 0; layer < VAEG_VA_LAYER_COUNT; layer++) {
		scrndrawva_set_layer_enabled(layer, saved[layer]);
	}
	fprintf(stderr, "selftest: VA layer display ok\n");
	return (SUCCESS);
}

static int test_framedisp(void) {
	VAEG_FRAMEDISP state;

	vaeg_framedisp_reset(&state, 1000, 100);
	if (vaeg_framedisp_update(&state, 2999, 220) != FALSE) {
		return (fail("framedisp", "measurement renewed before two seconds"));
	}
	if ((vaeg_framedisp_update(&state, 3000, 220) != TRUE) || (state.fps_tenths != 600)) {
		return (fail("framedisp", "60.0 FPS measurement is incorrect"));
	}
	if (vaeg_framedisp_update(&state, 4000, 280) != FALSE) {
		return (fail("framedisp", "measurement interval was not restarted"));
	}
	if ((vaeg_framedisp_update(&state, 5000, 280) != TRUE) || (state.fps_tenths != 300)) {
		return (fail("framedisp", "30.0 FPS measurement is incorrect"));
	}
	vaeg_framedisp_reset(&state, 0, 0);
	if ((vaeg_framedisp_update(&state, 2000, 0) != TRUE) || (state.fps_tenths != 0)) {
		return (fail("framedisp", "zero-draw interval is incorrect"));
	}
	vaeg_framedisp_reset(&state, 0xffffffffU - 1000U, 0xffffffffU - 3U);
	if ((vaeg_framedisp_update(&state, 999, 116) != TRUE) || (state.fps_tenths != 600)) {
		return (fail("framedisp", "counter wrap handling is incorrect"));
	}
	if (vaeg_framedisp_update(NULL, 0, 0) != FALSE) {
		return (fail("framedisp", "NULL state was accepted"));
	}
	vaeg_framedisp_reset(NULL, 0, 0);
	fprintf(stderr, "selftest: frame display ok\n");
	return (SUCCESS);
}

static int test_clockscale(void) {
	CLOCKSCALE scale;
	UINT64 total;
	UINT index;
	UINT saved_config_multiple;
	UINT saved_baseclock;
	static const UINT cpu_multipliers[] = {1, 2, 4, 8, 32};

	if ((clockscale_configure(&scale, 1, 0) != FAILURE) ||
	    (clockscale_configure(NULL, 1, 1) != FAILURE)) {
		return (fail("clockscale", "invalid ratio was accepted"));
	}
	if (clockscale_configure(&scale, 2, 3) != SUCCESS) {
		return (fail("clockscale", "x3 CPU ratio was rejected"));
	}
	total = 0;
	for (index = 0; index < 30000; index++) {
		total += clockscale_apply(&scale, 1);
	}
	if ((total != 20000) || (scale.remainder != 0)) {
		return (fail("clockscale", "fractional carry drifted"));
	}
	clockscale_configure(&scale, 2, 32);
	if ((clockscale_apply(&scale, 15) != 0) || (clockscale_apply(&scale, 1) != 1)) {
		return (fail("clockscale", "small CPU slices lost their remainder"));
	}
	clockscale_configure(&scale, 32, 1);
	if (clockscale_apply(&scale, 0xffffffffU) != (UINT64)0xffffffffU * 32) {
		return (fail("clockscale", "wide multiplication overflowed"));
	}
	clockscale_configure(&scale, 2, 3);
	(void)clockscale_apply(&scale, 1);
	clockscale_reset(&scale);
	if (scale.remainder != 0) {
		return (fail("clockscale", "reset retained a remainder"));
	}
	if (!pccore_cpu_multiple_valid(1) || !pccore_cpu_multiple_valid(2) ||
	    !pccore_cpu_multiple_valid(3) || !pccore_cpu_multiple_valid(4) ||
	    !pccore_cpu_multiple_valid(8) || !pccore_cpu_multiple_valid(32) ||
	    pccore_cpu_multiple_valid(0) || pccore_cpu_multiple_valid(33)) {
		return (fail("clockscale", "CPU multiplier validation failed"));
	}
	saved_config_multiple = np2cfg.multiple;
	saved_baseclock = pccore.baseclock;
	pccore.baseclock = PCBASECLOCK40;
	for (index = 0; index < NELEMENTS(cpu_multipliers); index++) {
		np2cfg.multiple = cpu_multipliers[index];
		pccore_clockrestore();
		if ((pccore.multiple != PCCORE_STANDARD_MULTIPLE) ||
		    (pccore.realclock != PCBASECLOCK40 * PCCORE_STANDARD_MULTIPLE) ||
		    (pccore_cpu_multiple() != cpu_multipliers[index]) ||
		    (pccore_cpu_clock() != PCBASECLOCK40 * cpu_multipliers[index])) {
			np2cfg.multiple = saved_config_multiple;
			pccore.baseclock = saved_baseclock;
			pccore_clockrestore();
			return (fail("clockscale", "CPU scaling changed machine time"));
		}
	}
	np2cfg.multiple = saved_config_multiple;
	pccore.baseclock = saved_baseclock;
	pccore_clockrestore();
	fprintf(stderr, "selftest: clockscale ok\n");
	return (SUCCESS);
}

static int test_sgp_speed(void) {
	UINT32 numerator;
	UINT32 denominator;
	UINT64 total;
	UINT index;
	UINT saved_model_va;
	UINT saved_sgp_speed_mode;
	UINT saved_sgp_multiplier;
	UINT saved_config_multiple;
	UINT32 saved_baseclock;

	if (!sgp_speed_mode_valid(SGP_SPEED_MODEL_DEFAULT) ||
	    !sgp_speed_mode_valid(SGP_SPEED_FOLLOW_CPU) || !sgp_speed_mode_valid(SGP_SPEED_CUSTOM) ||
	    sgp_speed_mode_valid(SGP_SPEED_MODE_COUNT) || !sgp_speed_multiplier_valid(1) ||
	    !sgp_speed_multiplier_valid(2) || !sgp_speed_multiplier_valid(3) ||
	    !sgp_speed_multiplier_valid(16) || sgp_speed_multiplier_valid(0) ||
	    sgp_speed_multiplier_valid(17)) {
		return (fail("sgp-speed", "SGP setting validation failed"));
	}
	if ((sgp_speed_ratio(SGP_SPEED_MODEL_DEFAULT, 1, 32, &numerator, &denominator) != SUCCESS) ||
	    (numerator != 1) || (denominator != 1)) {
		return (fail("sgp-speed", "Model default ratio changed"));
	}
	if ((sgp_speed_ratio(SGP_SPEED_FOLLOW_CPU, 1, 1, &numerator, &denominator) != SUCCESS) ||
	    (numerator != 1) || (denominator != 2)) {
		return (fail("sgp-speed", "Follow CPU x1 ratio failed"));
	}
	if ((sgp_speed_ratio(SGP_SPEED_FOLLOW_CPU, 1, 4, &numerator, &denominator) != SUCCESS) ||
	    (numerator != 4) || (denominator != 2)) {
		return (fail("sgp-speed", "Follow CPU x4 ratio failed"));
	}
	if ((sgp_speed_ratio(SGP_SPEED_CUSTOM, 3, 2, &numerator, &denominator) != SUCCESS) ||
	    (numerator != 3) || (denominator != 1) ||
	    (sgp_speed_ratio(SGP_SPEED_CUSTOM, 0, 2, &numerator, &denominator) != FAILURE)) {
		return (fail("sgp-speed", "Custom ratio failed"));
	}
	if ((sgp_model_clock(PCMODEL_VA1) != PCBASECLOCK40) ||
	    (sgp_model_clock(PCMODEL_VA2) != (PCBASECLOCK40 * 2))) {
		return (fail("sgp-speed", "Model clock selection failed"));
	}

	saved_model_va = pccore.model_va;
	saved_sgp_speed_mode = np2cfg.sgp_speed_mode;
	saved_sgp_multiplier = np2cfg.sgp_multiplier;
	saved_config_multiple = np2cfg.multiple;
	saved_baseclock = pccore.baseclock;
	pccore.baseclock = PCBASECLOCK40;
	pccore.model_va = PCMODEL_VA1;
	np2cfg.sgp_speed_mode = SGP_SPEED_MODEL_DEFAULT;
	np2cfg.sgp_multiplier = 1;
	np2cfg.multiple = PCCORE_STANDARD_MULTIPLE;
	pccore_clockrestore();
	sgp_configure_speed();
	if ((pccore_cpu_clock() != PCBASECLOCK40 * PCCORE_STANDARD_MULTIPLE) ||
	    (sgp_effective_clock() != PCBASECLOCK40)) {
		return (fail("sgp-speed", "effective default clocks are incorrect"));
	}
	if (sgp_scale_elapsed(20000) != 20000) {
		return (fail("sgp-speed", "VA Model default timing changed"));
	}
	pccore.model_va = PCMODEL_VA2;
	sgp_configure_speed();
	if (sgp_scale_elapsed(20000) != 40000) {
		return (fail("sgp-speed", "VA2 Model default timing failed"));
	}

	np2cfg.sgp_speed_mode = SGP_SPEED_FOLLOW_CPU;
	np2cfg.sgp_multiplier = 1;
	np2cfg.multiple = 4;
	pccore_clockrestore();
	sgp_configure_speed();
	if ((pccore_cpu_clock() != PCBASECLOCK40 * 4) || (sgp_effective_clock() != PCBASECLOCK40 * 4)) {
		return (fail("sgp-speed", "full-speed clocks did not increase"));
	}
	np2cfg.multiple = 3;
	pccore_clockrestore();
	sgp_configure_speed();
	total = 0;
	for (index = 0; index < 20000; index++) {
		total += sgp_scale_elapsed(1);
	}
	if (total != 60000) {
		return (fail("sgp-speed", "Follow CPU fractional timing drifted"));
	}
	pccore.model_va = saved_model_va;
	np2cfg.sgp_speed_mode = saved_sgp_speed_mode;
	np2cfg.sgp_multiplier = saved_sgp_multiplier;
	np2cfg.multiple = saved_config_multiple;
	pccore.baseclock = saved_baseclock;
	pccore_clockrestore();
	sgp_configure_speed();
	fprintf(stderr, "selftest: SGP speed ok\n");
	return (SUCCESS);
}

static int test_pacing(void) {
	VAEG_PACING_STATE state;
	BOOL configured_nowait;
	UINT configured_drawskip;

	configured_nowait = FALSE;
	configured_drawskip = 3;
	vaeg_pacing_reset(&state);
	if (state.fast_forward_held || vaeg_pacing_effective_nowait(&state, configured_nowait) ||
	    (vaeg_pacing_effective_drawskip(&state, configured_drawskip) != 3)) {
		return (fail("pacing", "reset state changed configured pacing"));
	}
	if (!vaeg_pacing_key(&state, SDL_SCANCODE_F11, TRUE, FALSE) || !state.fast_forward_held ||
	    !vaeg_pacing_effective_nowait(&state, configured_nowait) ||
	    (vaeg_pacing_effective_drawskip(&state, configured_drawskip) != 16)) {
		return (fail("pacing", "F11 keydown did not enable fast-forward"));
	}
	if (!vaeg_pacing_key(&state, SDL_SCANCODE_F11, TRUE, TRUE) || !state.fast_forward_held) {
		return (fail("pacing", "F11 repeat damaged held state"));
	}
	if (!vaeg_pacing_key(&state, SDL_SCANCODE_F11, FALSE, FALSE) || state.fast_forward_held) {
		return (fail("pacing", "F11 keyup did not disable fast-forward"));
	}
	if (vaeg_pacing_key(&state, SDL_SCANCODE_RALT, FALSE, FALSE)) {
		return (fail("pacing", "unrelated keyup was consumed as a shortcut"));
	}
	if (!vaeg_pacing_key(&state, SDL_SCANCODE_F9, TRUE, FALSE) ||
	    !vaeg_pacing_key(&state, SDL_SCANCODE_F9, FALSE, FALSE)) {
		return (fail("pacing", "remapped fast-forward key was not accepted"));
	}
	vaeg_pacing_reset(&state);
	if (state.fast_forward_held || configured_nowait || (configured_drawskip != 3)) {
		return (fail("pacing", "focus/reset cleanup changed saved settings"));
	}
	fprintf(stderr, "selftest: pacing ok\n");
	return (SUCCESS);
}

static int expect_viewport(int drawable_width, int drawable_height, int menu_inset, int scaling,
                           BOOL aspect, int x, int y, int width, int height) {
	VAEG_VIEWPORT_INPUT input;
	VAEG_VIEWPORT viewport;

	input.guest_width = 640;
	input.guest_height = 400;
	input.drawable_width = drawable_width;
	input.drawable_height = drawable_height;
	input.menu_inset = menu_inset;
	input.scaling = scaling;
	input.aspect = aspect;
	if ((vaeg_viewport_calculate(&input, &viewport) != SUCCESS) || (viewport.x != x) ||
	    (viewport.y != y) || (viewport.width != width) || (viewport.height != height)) {
		fprintf(stderr,
		        "selftest: viewport expected=%d,%d %dx%d actual=%d,%d %dx%d "
		        "drawable=%dx%d menu=%d mode=%d aspect=%d\n",
		        x, y, width, height, viewport.x, viewport.y, viewport.width, viewport.height,
		        drawable_width, drawable_height, menu_inset, scaling, aspect);
		return (FAILURE);
	}
	return (SUCCESS);
}

static int test_viewport(void) {
	if (splash_selftest() != SUCCESS) return fail("splash", "fit geometry failed");
	if (scrnmng_display_capture_selftest() != SUCCESS) return FAILURE;
	if (gui_overlay_selftest() != SUCCESS) return FAILURE;
	if (scrnmng_window_rebind_selftest() != SUCCESS) return FAILURE;
	VAEG_VIEWPORT_INPUT input;
	VAEG_VIEWPORT viewport;
	BOOL masked;
	int guest_x;
	int guest_y;
	int width;
	int height;
	UINT value;

	if ((expect_viewport(640, 400, 0, VAEG_SCALING_FIT, FALSE, 0, 0, 640, 400) != SUCCESS) ||
	    (expect_viewport(800, 600, 0, VAEG_SCALING_FIT, FALSE, 0, 50, 800, 500) != SUCCESS) ||
	    (expect_viewport(1024, 768, 0, VAEG_SCALING_FIT, FALSE, 0, 64, 1024, 640) != SUCCESS) ||
	    (expect_viewport(1280, 720, 0, VAEG_SCALING_FIT, FALSE, 64, 0, 1152, 720) != SUCCESS) ||
	    (expect_viewport(1920, 1080, 0, VAEG_SCALING_FIT, FALSE, 96, 0, 1728, 1080) != SUCCESS)) {
		return (fail("viewport", "Fit geometry failed"));
	}
	if ((expect_viewport(800, 600, 0, VAEG_SCALING_NATIVE, FALSE, 80, 100, 640, 400) != SUCCESS) ||
	    (expect_viewport(1280, 900, 0, VAEG_SCALING_INTEGER, FALSE, 0, 50, 1280, 800) != SUCCESS) ||
	    (expect_viewport(1003, 700, 0, VAEG_SCALING_FIT_8DOT, FALSE, 1, 37, 1000, 625) !=
	     SUCCESS) ||
	    (expect_viewport(800, 600, 0, VAEG_SCALING_STRETCH, FALSE, 0, 0, 800, 600) != SUCCESS) ||
	    (expect_viewport(640, 422, 22, VAEG_SCALING_FIT, FALSE, 0, 22, 640, 400) != SUCCESS) ||
	    (expect_viewport(1280, 844, 44, VAEG_SCALING_FIT, FALSE, 0, 44, 1280, 800) != SUCCESS)) {
		return (fail("viewport", "mode or inset geometry failed"));
	}
	input.guest_width = 640;
	input.guest_height = 400;
	input.drawable_width = 800;
	input.drawable_height = 600;
	input.menu_inset = 0;
	input.scaling = VAEG_SCALING_FIT;
	input.aspect = FALSE;
	if ((vaeg_viewport_calculate(&input, &viewport) != SUCCESS) ||
	    (vaeg_viewport_map_point(&viewport, 640, 400, 400, 300, &guest_x, &guest_y) != SUCCESS) ||
	    (guest_x != 320) || (guest_y != 200) ||
	    (vaeg_viewport_map_point(&viewport, 640, 400, 400, 25, &guest_x, &guest_y) != FAILURE)) {
		return (fail("viewport", "inverse coordinate transform failed"));
	}
	input.drawable_width = 0;
	if (vaeg_viewport_calculate(&input, &viewport) != FAILURE) {
		return (fail("viewport", "zero drawable size was accepted"));
	}
	for (value = 0; value <= 7; value++) {
		if ((vaeg_fscrnmod_sanitize(value, &masked) != value) || masked) {
			return (fail("viewport", "valid fscrnmod changed"));
		}
	}
	if ((vaeg_fscrnmod_sanitize(0x87, &masked) != 7) || !masked) {
		return (fail("viewport", "fscrnmod upper bits were not masked"));
	}
	vaeg_fullscreen_size(0, 0, 0, 1920, 1080, &width, &height);
	if ((width != 640) || (height != 400)) {
		return (fail("viewport", "legacy fullscreen fallback failed"));
	}
	vaeg_fullscreen_size(1280, 720, 4, 1920, 1080, &width, &height);
	if ((width != 1920) || (height != 1080)) {
		return (fail("viewport", "current display fallback failed"));
	}
	scrnmng_initialize();
	for (value = 0; value < VAEG_EFFECT_COUNT; value++) {
		scrnmng_set_effect((int)value);
		if (scrnmng_get_effect() != (int)value) {
			return (fail("viewport", "effect selection failed"));
		}
	}
	scrnmng_set_effect(VAEG_EFFECT_COUNT);
	if (scrnmng_get_effect() != VAEG_EFFECT_UNFILTERED) {
		return (fail("viewport", "invalid effect did not fall back"));
	}
	for (value = 0; value < VAEG_SCALING_COUNT; value++) {
		scrnmng_set_scaling((int)value);
		if (scrnmng_get_scaling() != (int)value) {
			return (fail("viewport", "scaling selection failed"));
		}
	}
	scrnmng_set_scaling(VAEG_SCALING_COUNT);
	if (scrnmng_get_scaling() != VAEG_SCALING_FIT) {
		return (fail("viewport", "invalid scaling did not fall back"));
	}
	fprintf(stderr, "selftest: viewport ok\n");
	return (SUCCESS);
}

static int test_va_raster_guard(void) {
	BYTE output[(SURFACE_WIDTH + SCRNMNG_SURFACE_GUARD_LEFT) * 2];
	SCRNSURF surf;
	_SDRAWVA draw;
	SDRAWFNVA drawfn;

	memset(output, 0x5a, sizeof(output));
	ZeroMemory(&surf, sizeof(surf));
	surf.bpp = 16;
	drawfn = sdrawva_getproctbl(&surf);
	if (drawfn == NULL) {
		return (fail("VA raster", "16bpp converter is unavailable"));
	}
	ZeroMemory(vabitmap, SURFACE_WIDTH * sizeof(vabitmap[0]));
	vabitmap[0] = 1;
	vabitmap[SURFACE_WIDTH - 1] = 2;
	drawcolor16[1] = 0x1234;
	drawcolor16[2] = 0xabcd;
	ZeroMemory(&draw, sizeof(draw));
	draw.dst = output;
	draw.width = SURFACE_WIDTH;
	draw.xbytes = SURFACE_WIDTH * 2;
	draw.xalign = 2;
	draw.yalign = sizeof(output);
	drawfn(&draw, 1);
	if ((LOADINTELWORD(output) != 0) ||
	    (LOADINTELWORD(output + (SCRNMNG_SURFACE_GUARD_LEFT * 2)) != 0x1234) ||
	    (LOADINTELWORD(output + ((SCRNMNG_SURFACE_GUARD_LEFT + SURFACE_WIDTH - 1) * 2)) !=
	     0xabcd)) {
		return (fail("VA raster", "guard or visible edge pixels are wrong"));
	}
	fprintf(stderr, "selftest: VA raster guard ok\n");
	return (SUCCESS);
}

static int read_whole_file(const char *path, BYTE **out, UINT *out_size) {
	FILEH fh;
	UINT size;
	BYTE *buf;
	int ret;

	*out = NULL;
	*out_size = 0;
	fh = file_open_rb(path);
	if (fh == FILEH_INVALID) {
		return (FAILURE);
	}
	size = file_getsize(fh);
	buf = (BYTE *)_MALLOC(size, "selftest-file");
	if (buf == NULL) {
		file_close(fh);
		return (FAILURE);
	}
	ret = SUCCESS;
	if (file_read(fh, buf, size) != size) {
		ret = FAILURE;
	}
	file_close(fh);
	if (ret != SUCCESS) {
		_MFREE(buf);
		return (FAILURE);
	}
	*out = buf;
	*out_size = size;
	return (SUCCESS);
}

static BOOL statsave_section_body_stable(const BYTE *hdr) {
	if (!memcmp(hdr, "CGWINDOW", 8) || !memcmp(hdr, "TEXTRAM", 7)) {
		return (FALSE);
	}
	return (TRUE);
}

static int make_statsave_with_unsupported_subcpu(const char *source, const char *destination) {
	BYTE *data;
	UINT size;
	UINT pos;
	BOOL found;
	FILEH fh;
	int ret;

	data = NULL;
	if (read_whole_file(source, &data, &size) != SUCCESS) {
		return (FAILURE);
	}
	found = FALSE;
	pos = 0x30;
	while ((pos + 16) <= size) {
		UINT body_size;
		UINT padded;

		body_size = LOADINTELDWORD(data + pos + 12);
		padded = (body_size + 15) & ~15U;
		if ((pos + 16 + padded) > size) {
			break;
		}
		if (!memcmp(data + pos, "SUBCPU", 6) && (body_size == 68)) {
			data[pos + 16 + 59] = 2;
			found = TRUE;
			break;
		}
		pos += 16 + padded;
	}
	ret = FAILURE;
	fh = file_create(destination);
	if (found && (fh != FILEH_INVALID) && (file_write(fh, data, size) == size)) {
		ret = SUCCESS;
	}
	if (fh != FILEH_INVALID) {
		file_close(fh);
	}
	_MFREE(data);
	return (ret);
}

static int make_statsave_with_retired_fmboard(const char *source, const char *destination) {
	BYTE *data;
	UINT size;
	UINT pos;
	BOOL found;
	FILEH fh;
	int ret;

	data = NULL;
	if (read_whole_file(source, &data, &size) != SUCCESS) {
		return (FAILURE);
	}
	found = FALSE;
	pos = 0x30;
	while ((pos + 16) <= size) {
		UINT body_size;
		UINT padded;

		body_size = LOADINTELDWORD(data + pos + 12);
		padded = (body_size + 15) & ~15U;
		if ((pos + 16 + padded) > size) {
			break;
		}
		if (!memcmp(data + pos, "FMBOARD", 7) && (body_size >= sizeof(UINT32))) {
			data[pos + 16] = 0x04;
			data[pos + 17] = 0x00;
			data[pos + 18] = 0x00;
			data[pos + 19] = 0x00;
			found = TRUE;
			break;
		}
		pos += 16 + padded;
	}
	ret = FAILURE;
	fh = file_create(destination);
	if (found && (fh != FILEH_INVALID) && (file_write(fh, data, size) == size)) {
		ret = SUCCESS;
	}
	if (fh != FILEH_INVALID) {
		file_close(fh);
	}
	_MFREE(data);
	return (ret);
}

static int compare_statsave_stable_sections(const char *left, const char *right) {
	BYTE *ldata;
	BYTE *rdata;
	UINT lsize;
	UINT rsize;
	UINT pos;
	int ret;

	ldata = NULL;
	rdata = NULL;
	if ((read_whole_file(left, &ldata, &lsize) != SUCCESS) ||
	    (read_whole_file(right, &rdata, &rsize) != SUCCESS)) {
		if (ldata != NULL) {
			_MFREE(ldata);
		}
		if (rdata != NULL) {
			_MFREE(rdata);
		}
		return (FAILURE);
	}
	ret = FAILURE;
	if ((lsize != rsize) || (lsize < 0x30) || memcmp(ldata, rdata, 0x30)) {
		goto cmp_done;
	}
	pos = 0x30;
	while (pos < lsize) {
		UINT size;
		UINT padded;
		UINT body;

		if ((pos + 16) > lsize) {
			goto cmp_done;
		}
		if (memcmp(ldata + pos, rdata + pos, 16)) {
			goto cmp_done;
		}
		size = LOADINTELDWORD(ldata + pos + 12);
		padded = (size + 15) & ~15U;
		body = pos + 16;
		if ((body + padded) > lsize) {
			goto cmp_done;
		}
		if (statsave_section_body_stable(ldata + pos) &&
		    memcmp(ldata + body, rdata + body, padded)) {
			UINT diff;

			for (diff = 0; diff < padded; diff++) {
				if (ldata[body + diff] != rdata[body + diff]) {
					fprintf(stderr, "selftest: statsave section %.10s differs at %u: %02x/%02x\n",
					        ldata + pos, diff, ldata[body + diff], rdata[body + diff]);
					break;
				}
			}
			goto cmp_done;
		}
		pos = body + padded;
	}
	ret = SUCCESS;

cmp_done:
	_MFREE(ldata);
	_MFREE(rdata);
	return (ret);
}

#if defined(VAEG_UPD9002_SSTS_TESTING)
static BOOL test_8087_documented_slot(unsigned opcode, unsigned modrm) {
	unsigned reg = (modrm >> 3) & 7;
	unsigned rm = modrm & 7;

	/* Keep this membership predicate independent from upd8087_classify().
	 * It is the production-CPU coverage side of the frozen P01 inventory. */
#define MEMORY_FORM(op, r) \
	(((op) == 0xd8) || ((op) == 0xda) || ((op) == 0xdc) || \
	 ((op) == 0xde) || \
	 (((op) == 0xd9) && ((r) != 1)) || \
	 (((op) == 0xdb) && ((r) == 0 || (r) == 2 || (r) == 3 || \
	                    (r) == 5 || (r) == 7)) || \
	 (((op) == 0xdd) && ((r) == 0 || (r) == 2 || (r) == 3 || \
	                    (r) == 4 || (r) == 6 || (r) == 7)) || \
	 (((op) == 0xdf) && ((r) != 1)))
#define REGISTER_FORM(op, r, m) \
	(((op) == 0xd8) || \
	 (((op) == 0xd9) && \
	  (((r) == 0) || ((r) == 1) || ((r) == 2 && (m) == 0) || \
	   ((r) == 4 && ((m) == 0 || (m) == 1 || (m) == 4 || (m) == 5)) || \
	   ((r) == 5 && (m) <= 6) || \
	   ((r) == 6 && ((m) <= 4 || (m) >= 6)) || \
	   ((r) == 7 && ((m) == 0 || (m) == 1 || (m) == 2 || \
	                (m) == 4 || (m) == 5)))) || \
	 (((op) == 0xdb) && (r) == 4 && (m) <= 3) || \
	 (((op) == 0xdc) && ((r) == 0 || (r) == 1 || (r) == 4 || \
	                    (r) == 5 || (r) == 6 || (r) == 7)) || \
	 (((op) == 0xdd) && ((r) == 0 || (r) == 2 || (r) == 3)) || \
	 (((op) == 0xde) && ((r) == 0 || (r) == 1 || (r) == 4 || \
	                    (r) == 5 || (r) == 6 || (r) == 7 || \
	                    ((r) == 3 && (m) == 1))))

	BOOL result = ((modrm & 0xc0) != 0xc0) ?
		MEMORY_FORM(opcode, reg) : REGISTER_FORM(opcode, reg, rm);

#undef MEMORY_FORM
#undef REGISTER_FORM
	return result;
}

static int test_8087_production_inventory(void) {
	NP2CFG saved_config;
	unsigned opcode;
	unsigned modrm;
	unsigned executed = 0;

	saved_config = np2cfg;
	upd9002_test_flat_memory_set(TRUE);
	np2cfg.upd8087_enable = 1;
	np2cfg.upd8087_clock_hz = UPD8087_DEFAULT_CLOCK_HZ;
	pccore_reset();

	for (opcode = 0xd8; opcode <= 0xdf; opcode++) {
		for (modrm = 0; modrm <= 0xff; modrm++) {
			BOOL memory_form;
			BOOL rewrites_saved_opcode;
			unsigned expected_ip;

			if (!test_8087_documented_slot(opcode, modrm)) {
				continue;
			}
			memory_form = (modrm & 0xc0) != 0xc0;
			rewrites_saved_opcode =
				(memory_form && (((opcode == 0xd9) || (opcode == 0xdd)) &&
				                 (((modrm >> 3) & 7) == 4))) ||
				(memory_form && (opcode == 0xdd) &&
				                 (((modrm >> 3) & 7) == 6)) ||
				((opcode == 0xdb) && (((modrm >> 3) & 7) == 4) &&
				 (((modrm & 7) == 3)));
			if (!memory_form) {
				expected_ip = 2U;
			} else {
				switch (modrm & 0xc0) {
				case 0x00:
					expected_ip = ((modrm & 7) == 6) ? 4U : 2U;
					break;
				case 0x40:
					expected_ip = 3U;
					break;
				default:
					expected_ip = 4U;
					break;
				}
			}

			/* Reset only the CPU/device architectural state between cases.
			 * The flat test bus remains the production memory callback used by
			 * _esc; no direct upd8087_execute call is made here. */
			CPU_RESET();
			upd8087_reset(&upd8087);
			CPU_CS = 0;
			CPU_DS = 0;
			CPU_SS = 0;
			/* Keep every effective address away from the instruction bytes.  In
			 * particular FLDENV/FRSTOR legitimately replace the saved opcode
			 * from their memory image. */
			CPU_BX = 0x0100;
			CPU_BP = 0x0100;
			CPU_SI = 0x0100;
			CPU_DI = 0x0100;
			CS_BASE = 0;
			DS_BASE = 0;
			SS_BASE = 0;
			CPU_IP = 0;
			CPU_REMCLOCK = 1000000;
			memset(mem, 0, 0x1000);
			mem[0] = (BYTE)opcode;
			mem[1] = (BYTE)modrm;
			/* Safe displacement for mod=00/rm=110 and for mod=10. */
			mem[2] = 0x00;
			mem[3] = 0x03;
			upd9002_core_step();
			executed++;
			if ((CPU_IP != expected_ip) ||
			    (!rewrites_saved_opcode &&
			     (upd8087.opcode != (UINT16)(((opcode & 7U) << 8) | modrm))) ||
			    (upd8087.last_ndp_cycles == 0)) {
				fprintf(stderr,
				        "selftest: 8087 production inventory failed at %02x:%02x "
				        "ip=%u/%u op=%03x/%03x ndp=%u cpuclk=%u rem=%d\n",
				        opcode, modrm, (unsigned)CPU_IP, expected_ip,
				        (unsigned)upd8087.opcode,
				        (unsigned)(((opcode & 7U) << 8) | modrm),
				        (unsigned)upd8087.last_ndp_cycles,
				        (unsigned)pccore_cpu_clock(), (int)CPU_REMCLOCK);
				upd9002_test_flat_memory_set(FALSE);
				np2cfg = saved_config;
				pccore_reset();
				return fail("8087 production inventory",
				            "documented slot did not traverse native CPU/8087 seam");
			}
		}
	}

	upd9002_test_flat_memory_set(FALSE);
	np2cfg = saved_config;
	pccore_reset();
	if (executed != 1597U) {
		return fail("8087 production inventory", "documented slot count changed");
	}
	fprintf(stderr, "selftest: 8087 production inventory ok (%u slots)\n", executed);
	return SUCCESS;
}

static int test_8087_production_path(void) {
	static const BYTE program[] = {
		0xd9, 0xe8,             /* FLD1 */
		0xd9, 0xe8,             /* FLD1 */
		0xd8, 0xc1,             /* FADD ST(0),ST(1) */
		0x2e, 0xd9, 0x1e, 0x00, 0x03 /* CS:FSTP dword ptr [0300h] */
	};
	NP2CFG saved_config;
	UINT32 ticks_at_5mhz;
	UINT32 ticks_at_10mhz;
	const unsigned program_steps = 4U;
	unsigned index;

	saved_config = np2cfg;
	upd9002_test_flat_memory_set(TRUE);
	np2cfg.upd8087_enable = 1;
	np2cfg.upd8087_clock_hz = 5000000U;
	pccore_reset();
	np2cfg.upd8087_clock_hz = 10000000U;
	if (upd8087.clock_hz != 5000000U) {
		upd9002_test_flat_memory_set(FALSE);
		np2cfg = saved_config;
		pccore_reset();
		return fail("8087 production path", "requested clock changed before reset");
	}
	pccore_reset();
	if (upd8087.clock_hz != 10000000U) {
		upd9002_test_flat_memory_set(FALSE);
		np2cfg = saved_config;
		pccore_reset();
		return fail("8087 production path", "reset did not apply requested clock");
	}
	np2cfg.upd8087_clock_hz = 5000000U;
	pccore_reset();
	CopyMemory(mem, program, sizeof(program));
	CPU_IP = 0;
	CPU_CS = 0;
	CPU_DS = 0;
	CS_BASE = 0;
	DS_BASE = 0;
	CPU_REMCLOCK = 1000000;
	for (index = 0; index < program_steps; index++) {
		upd9002_core_step();
	}
	if ((mem[0x300] != 0x00) || (mem[0x301] != 0x00) ||
	    (mem[0x302] != 0x00) || (mem[0x303] != 0x40)) {
		upd9002_test_flat_memory_set(FALSE);
		np2cfg = saved_config;
		pccore_reset();
		return fail("8087 production path", "native fetch did not store 2.0");
	}
	ticks_at_5mhz = upd8087.last_cpu_ticks;

	np2cfg.upd8087_clock_hz = 10000000U;
	pccore_reset();
	CopyMemory(mem, program, sizeof(program));
	CPU_IP = 0;
	CPU_CS = 0;
	CPU_DS = 0;
	CS_BASE = 0;
	DS_BASE = 0;
	CPU_REMCLOCK = 1000000;
	for (index = 0; index < program_steps; index++) {
		upd9002_core_step();
	}
	if ((mem[0x300] != 0x00) || (mem[0x301] != 0x00) ||
	    (mem[0x302] != 0x00) || (mem[0x303] != 0x40)) {
		upd9002_test_flat_memory_set(FALSE);
		np2cfg = saved_config;
		pccore_reset();
		return fail("8087 production path", "10 MHz native fetch did not store 2.0");
	}
	ticks_at_10mhz = upd8087.last_cpu_ticks;

	upd9002_test_flat_memory_set(FALSE);
	np2cfg = saved_config;
	pccore_reset();
	if ((ticks_at_5mhz <= ticks_at_10mhz) || (ticks_at_10mhz == 0)) {
		return fail("8087 production path", "configured frequency did not affect service time");
	}
	fprintf(stderr, "selftest: 8087 production path ok\n");
	return SUCCESS;
}

static int test_8087_cpu_decode_seams(void) {
	NP2CFG saved_config;
	UINT64 saved_remainder;
	UINT32 saved_cycles;
	int result = SUCCESS;

	saved_config = np2cfg;
	upd9002_test_flat_memory_set(TRUE);
	np2cfg.upd8087_enable = 0;
	np2cfg.upd8087_clock_hz = 10000000U;
	pccore_reset();
	CPU_CS = 0;
	CPU_DS = 0;
	CS_BASE = 0;
	DS_BASE = 0;
	CPU_IP = 0;
	CPU_REMCLOCK = 1000000;

	/* A disabled FPO1 register form must consume only its native bytes and
	 * must not call the NDP or read an operand bus. */
	mem[0] = 0xd9;
	mem[1] = 0xe8;             /* FLD1 */
	upd9002_test_flat_memory_reset_counters();
	upd8087.last_ndp_cycles = 0;
	upd8087.service_remainder = 0;
	upd9002_core_step();
	if (upd8087.enabled || (CPU_IP != 2) || upd8087.last_ndp_cycles != 0 ||
	    upd8087.service_remainder != 0 ||
	    upd9002_test_flat_memory_read_count() != 2 ||
	    upd9002_test_flat_memory_write_count() != 0) {
		result = fail("8087 CPU decode seams", "disabled native FPO1 form reached the NDP");
		goto cleanup;
	}

	/* A disabled memory form still advances through the native EA decoder, but
	 * it must not perform the NDP first-word latch or a device-bus read. */
	mem[0] = 0xd9;
	mem[1] = 0x06;             /* direct disp16 ModR/M */
	mem[2] = 0x34;
	mem[3] = 0x12;
	upd9002_test_flat_memory_reset_counters();
	CPU_IP = 0;
	upd9002_core_step();
	if ((CPU_IP != 4) || upd8087.last_ndp_cycles != 0 ||
	    upd9002_test_flat_memory_read_count() != 4 ||
	    upd9002_test_flat_memory_write_count() != 0) {
		result = fail("8087 CPU decode seams", "disabled memory FPO1 form performed an NDP bus access");
		goto cleanup;
	}

	/* 9Bh is the native CPU FWAIT/POLL seam.  With no device present, a
	 * POLL/FWAIT must consume its CPU instruction cost but must not wait on a
	 * nonexistent 8087 BUSY source. */
	mem[0] = 0x9b;
	upd9002_test_flat_memory_reset_counters();
	CPU_IP = 0;
	CPU_REMCLOCK = 1000;
	upd9002_core_step();
	if ((CPU_IP != 1) || (CPU_REMCLOCK <= 0) || upd8087_wait_blocked(&upd8087) ||
	    upd9002_test_flat_memory_read_count() != 1 ||
	    upd9002_test_flat_memory_write_count() != 0) {
		result = fail("8087 CPU decode seams", "absent-device POLL/FWAIT waited on a nonexistent BUSY source");
		goto cleanup;
	}

	/* 0x66 is a native reserved/FPO2 byte, not an ESC prefix.  It must remain
	 * outside the sole optional 8087 even when that device is enabled. */
	np2cfg.upd8087_enable = 1;
	pccore_reset();
	CPU_CS = 0;
	CPU_DS = 0;
	CS_BASE = 0;
	DS_BASE = 0;
	CPU_IP = 0;
	CPU_REMCLOCK = 1000000;
	mem[0] = 0x66;
	mem[1] = 0xc0;
	upd9002_test_flat_memory_reset_counters();
	upd8087.last_ndp_cycles = 0;
	upd8087.service_remainder = 0;
	upd9002_core_step();
	if (!upd8087.enabled || (CPU_IP != 1) || upd8087.last_ndp_cycles != 0 ||
	    upd8087.service_remainder != 0 ||
	    upd9002_test_flat_memory_read_count() != 1 ||
	    upd9002_test_flat_memory_write_count() != 0) {
		result = fail("8087 CPU decode seams", "native FPO2 byte reached the 8087");
		goto cleanup;
	}

	/* An unmasked ESC exception keeps the CPU-side POLL/FWAIT seam blocked
	 * until the guest recovery path clears the 8087 condition. */
	mem[0] = 0xd9;
	mem[1] = 0xc0;             /* FLD ST(0): unmasked empty-stack fault */
	CPU_IP = 0;
	CPU_REMCLOCK = 1000;
	upd8087_set_control(&upd8087,
	                   (uint16_t)(upd8087.control & (uint16_t)~UPD8087_STATUS_IE));
	upd9002_core_step();
	if (!upd8087_interrupt_pending(&upd8087) || !upd8087_wait_blocked(&upd8087)) {
		result = fail("8087 CPU decode seams", "unmasked ESC exception did not arm the POLL/FWAIT seam");
		goto cleanup;
	}
	mem[0] = 0x9b;             /* POLL/FWAIT */
	CPU_IP = 0;
	CPU_REMCLOCK = 1000;
	upd9002_core_step();
	if ((CPU_IP != 1) || (CPU_REMCLOCK != 0)) {
		result = fail("8087 CPU decode seams", "POLL/FWAIT did not yield to the pending 8087 condition");
		goto cleanup;
	}

	/* Enter the existing uPD70008 compatibility dispatch and execute an
	 * overlapping byte.  The compatibility path owns the step; no native ESC
	 * callback may be generated for it. */
	CPU_COMPAT_MODE = UPD9002_COMPAT_UPD70008;
	CPU_IP = 0;
	mem[0] = 0xd8;
	saved_remainder = upd8087.service_remainder;
	saved_cycles = upd8087.last_ndp_cycles;
	upd9002_test_flat_memory_reset_counters();
	upd9002_core_step();
	if (upd8087.service_remainder != saved_remainder ||
	    upd8087.last_ndp_cycles != saved_cycles) {
		result = fail("8087 CPU decode seams", "8080-compatible path reached the 8087");
		goto cleanup;
	}

	fprintf(stderr, "selftest: 8087 CPU decode seams ok\n");

cleanup:
	upd9002_test_flat_memory_set(FALSE);
	np2cfg = saved_config;
	pccore_reset();
	return result;
}

static int test_8087_guest_interrupt_route(void) {
	static const BYTE handler[] = {
		0xdb, 0xe2,             /* FCLEX */
		0xb0, 0x20,             /* MOV AL,20H */
		0xba, 0x84, 0x01,       /* MOV DX,0184H */
		0xee,                   /* OUT DX,AL: slave EOI */
		0xba, 0x88, 0x01,       /* MOV DX,0188H */
		0xee,                   /* OUT DX,AL: master EOI */
		0xcf                    /* IRET */
	};
	NP2CFG saved_config;
	unsigned index;
	int result = SUCCESS;

	saved_config = np2cfg;
	upd9002_test_flat_memory_set(TRUE);
	np2cfg.upd8087_enable = 1;
	np2cfg.upd8087_clock_hz = 10000000U;
	pccore_reset();
	CPU_IP = 0x0100;
	CPU_CS = 0;
	CPU_DS = 0;
	CPU_SS = 0;
	CPU_SP = 0x8000;
	CS_BASE = 0;
	DS_BASE = 0;
	SS_BASE = 0;
	CPU_FLAG |= I_FLAG;
	CPU_REMCLOCK = 1000000;
	upd8087_set_control(&upd8087,
	                   (uint16_t)(upd8087.control & (uint16_t)~UPD8087_STATUS_IE));
	if (((pic.pi[1].icw[1] & 0xf8) != 0x10) ||
	    ((pic.pi[1].icw[2] & 7) != 7) || !(pic.pi[0].icw[2] & PIC_SLAVE)) {
		result = fail("8087 guest interrupt route", "default VA PIC cascade is not PIC2 IR6 through PIC1 IR7");
		goto cleanup;
	}

	/* The default VA PIC maps slave IRQ6 (IRQ14) to vector 16H. */
	mem[0x0100] = 0xd9;
	mem[0x0101] = 0xc0;       /* FLD ST(0): masked-off empty-stack fault */
	mem[0x0016 * 4 + 0] = 0x00;
	mem[0x0016 * 4 + 1] = 0x02;
	mem[0x0016 * 4 + 2] = 0x00;
	mem[0x0016 * 4 + 3] = 0x00;
	CopyMemory(mem + 0x0200, handler, sizeof(handler));
	/* Expose the NDP line on the VA slave PIC while retaining other masks. */
	iocore_out8(0x0186, (REG8)(pic.pi[1].imr & (REG8)~PIC_NDP));

	upd9002_core_step();
	if (!upd8087_interrupt_pending(&upd8087) || !PICEXISTINTR ||
	    !(pic.pi[1].irr & PIC_NDP)) {
		result = fail("8087 guest interrupt route", "NDP exception did not reach the PIC");
		goto cleanup;
	}
	/* The controller request is level-pending, but CPU IF still masks its
	 * delivery.  pic_irq() must leave both the guest PC and the request
	 * untouched until the native CPU enables maskable interrupts. */
	CPU_FLAG &= (UINT16)~I_FLAG;
	pic_irq();
	if ((CPU_IP != 0x0102) || !PICEXISTINTR) {
		result = fail("8087 guest interrupt route", "CPU interrupt masking did not defer the NDP request");
		goto cleanup;
	}
	CPU_FLAG |= I_FLAG;
	pic_irq();
	if ((CPU_CS != 0) || (CPU_IP != 0x0200) || CPU_isEI) {
		result = fail("8087 guest interrupt route", "PIC did not vector the guest handler");
		goto cleanup;
	}
	if ((pic.pi[1].irr & PIC_NDP) || !(pic.pi[1].isr & PIC_NDP) ||
	    !(pic.pi[0].isr & PIC_SLAVE)) {
		result = fail("8087 guest interrupt route", "PIC acknowledgement did not separate IRR from ISR");
		goto cleanup;
	}
	/* FCLEX releases the source input, but it is not a PIC EOI. */
	upd9002_core_step();
	if (upd8087_interrupt_pending(&upd8087) || (pic.pi[1].irr & PIC_NDP) ||
	    !(pic.pi[1].isr & PIC_NDP) || !(pic.pi[0].isr & PIC_SLAVE)) {
		result = fail("8087 guest interrupt route", "FCLEX incorrectly acknowledged the PIC");
		goto cleanup;
	}
	/* Run the remaining handler instructions; the two EOI writes are part of
	 * the production guest path, not a direct test-side PIC mutation. */
	for (index = 1; index < 7; index++) {
		upd9002_core_step();
	}
	if ((CPU_CS != 0) || (CPU_IP != 0x0102) || !CPU_isEI ||
	    PICEXISTINTR || upd8087_interrupt_pending(&upd8087) ||
	    upd8087_wait_blocked(&upd8087)) {
		result = fail("8087 guest interrupt route", "guest handler did not clear and return from NDP IRQ");
		goto cleanup;
	}
	/* Edge-triggered PIC2 must not redeliver the held-high source after EOI. */
	CPU_FLAG |= I_FLAG;
	pic_irq();
	if ((CPU_IP != 0x0102) || (pic.pi[1].isr & PIC_NDP) ||
	    (pic.pi[0].isr & PIC_SLAVE)) {
		result = fail("8087 guest interrupt route", "held NDP INT generated a spurious edge redelivery");
		goto cleanup;
	}
	fprintf(stderr, "selftest: 8087 guest interrupt route ok\n");

cleanup:
	upd9002_test_flat_memory_set(FALSE);
	np2cfg = saved_config;
	pccore_reset();
	return result;
}

static int test_8087_pic_input_modes(void) {
	NP2CFG saved_config;
	int result = SUCCESS;

	saved_config = np2cfg;
	upd9002_test_flat_memory_set(TRUE);
	np2cfg.upd8087_enable = 1;
	np2cfg.upd8087_clock_hz = 10000000U;
	pccore_reset();

	/* Rebase PIC2 through the guest-visible ICW path.  A direct vector
	 * injection would incorrectly continue to use vector 16H; the cascade
	 * must instead select base 40H + slave IR6 = vector 46H. */
	CPU_CS = 0;
	CPU_DS = 0;
	CPU_SS = 0;
	CPU_SP = 0x8000;
	CS_BASE = 0;
	DS_BASE = 0;
	SS_BASE = 0;
	CPU_FLAG |= I_FLAG;
	CPU_REMCLOCK = 1000000;
	upd8087_set_control(&upd8087,
	                   (uint16_t)(upd8087.control & (uint16_t)~UPD8087_STATUS_IE));
	mem[0x0100] = 0xd9;
	mem[0x0101] = 0xc0;         /* FLD ST(0): unmasked empty-stack fault */
	mem[0x16 * 4 + 0] = 0x00;
	mem[0x16 * 4 + 1] = 0x04;  /* Distinguish an illegal direct 16H route. */
	mem[0x46 * 4 + 0] = 0x00;
	mem[0x46 * 4 + 1] = 0x03;
	CPU_IP = 0x0100;
	iocore_out8(0x0184, 0x11);
	iocore_out8(0x0186, 0x40);
	iocore_out8(0x0186, 0x07);
	iocore_out8(0x0186, 0x09);
	iocore_out8(0x0186, 0x00);
	upd9002_core_step();
	if (!upd8087_interrupt_pending(&upd8087) || !(pic.pi[1].irr & PIC_NDP)) {
		result = fail("8087 PIC input modes", "rebased NDP source did not reach PIC2");
		goto cleanup;
	}
	CPU_FLAG |= I_FLAG;
	pic_irq();
	if ((CPU_IP != 0x0300) ||
	    !(pic.pi[1].isr & PIC_NDP) || !(pic.pi[0].isr & PIC_SLAVE)) {
		result = fail("8087 PIC input modes", "PIC2 base 40H did not produce vector 46H");
		goto cleanup;
	}
	/* Source clear is separate from PIC acknowledgement, even after the
	 * vector has been rebased. */
	mem[0x0300] = 0xdb;
	mem[0x0301] = 0xe2;         /* FCLEX */
	upd9002_core_step();
	if (upd8087_interrupt_pending(&upd8087) || !(pic.pi[1].isr & PIC_NDP) ||
	    !(pic.pi[0].isr & PIC_SLAVE)) {
		result = fail("8087 PIC input modes", "FCLEX acknowledged the rebased PIC request");
		goto cleanup;
	}
	iocore_out8(0x0184, 0x20);
	iocore_out8(0x0188, 0x20);

	/* Reinitialize PIC2 in level-triggered mode and hold the logical NDP input
	 * asserted across both EOIs.  The source must be presented again only by
	 * PIC arbitration, not by a second direct CPU vector call. */
	pccore_reset();
	CPU_CS = 0;
	CPU_DS = 0;
	CPU_SS = 0;
	CPU_SP = 0x8000;
	CS_BASE = 0;
	DS_BASE = 0;
	SS_BASE = 0;
	CPU_FLAG |= I_FLAG;
	CPU_REMCLOCK = 1000000;
	mem[0x16 * 4 + 0] = 0x00;
	mem[0x16 * 4 + 1] = 0x03;
	CPU_IP = 0x0300;
	iocore_out8(0x0184, 0x19);   /* ICW1 + LTIM + ICW4 */
	iocore_out8(0x0186, 0x10);
	iocore_out8(0x0186, 0x07);
	iocore_out8(0x0186, 0x09);
	iocore_out8(0x0186, 0x00);
	pic_setirq_level(IRQ_NDP, TRUE);
	CPU_FLAG |= I_FLAG;
	pic_irq();
	if ((CPU_IP != 0x0300) || !(pic.pi[1].isr & PIC_NDP) ||
	    !(pic.pi[0].isr & PIC_SLAVE)) {
		result = fail("8087 PIC input modes", "level-triggered NDP input did not vector");
		goto cleanup;
	}
	iocore_out8(0x0184, 0x20);
	iocore_out8(0x0188, 0x20);
	if (!(pic.pi[1].irr & PIC_NDP) || !PICEXISTINTR) {
		result = fail("8087 PIC input modes", "held level did not reappear after PIC EOI");
		goto cleanup;
	}
	CPU_FLAG |= I_FLAG;
	CPU_IP = 0x0300;
	pic_irq();
	if ((CPU_IP != 0x0300) || !(pic.pi[1].isr & PIC_NDP) ||
	    !(pic.pi[0].isr & PIC_SLAVE)) {
		result = fail("8087 PIC input modes", "held level was not redelivered by PIC arbitration");
		goto cleanup;
	}
	pic_setirq_level(IRQ_NDP, FALSE);
	if (pic.pi[1].irr & PIC_NDP) {
		result = fail("8087 PIC input modes", "deasserted level remained in slave IRR");
		goto cleanup;
	}
	iocore_out8(0x0184, 0x20);
	iocore_out8(0x0188, 0x20);
	CPU_FLAG |= I_FLAG;
	pic_irq();
	if (PICEXISTINTR) {
		result = fail("8087 PIC input modes", "deasserted level remained deliverable");
		goto cleanup;
	}
	fprintf(stderr, "selftest: 8087 PIC input modes ok\n");

cleanup:
	upd9002_test_flat_memory_set(FALSE);
	np2cfg = saved_config;
	pccore_reset();
	return result;
}
#endif

static int test_statsave(void) {
	char path1[MAX_PATH];
	char path2[MAX_PATH];
	char pathbad[MAX_PATH];
	char err[256];
	BYTE *hostfat_image;
	UINT16 identity_ip;
	BYTE identity_memory;
	UINT8 saved_8087_enable;
	UINT32 saved_8087_config_clock;
	UINT32 expected_8087_clock;
	UINT64 expected_8087_remainder;
	int ret;
#if defined(VAEG_UPD780_INTEGRATION_TESTING)
	static const UINT8 f4_program[] = {0xaf, 0x3e, 0x5a, 0xd3, 0xf4, 0x00};
	VAEG_UPD780_INTEGRATION_TRACE_STATE upd780trace;
#endif

	SPRINTF(path1, "vaeg-selftest-%lu-1.sts", (unsigned long)getpid());
	SPRINTF(path2, "vaeg-selftest-%lu-2.sts", (unsigned long)getpid());
	SPRINTF(pathbad, "vaeg-selftest-%lu-bad.sts", (unsigned long)getpid());
	file_delete(path1);
	file_delete(path2);
	file_delete(pathbad);
	hostfat_image = NULL;
	saved_8087_enable = np2cfg.upd8087_enable;
	saved_8087_config_clock = np2cfg.upd8087_clock_hz;

	soundmng_initialize();
	commng_initialize();
	pccore_init();
	pccore_reset();
	/* Exercise the actual state-save section with a nonzero clock residue and
	 * a pending unmasked exception, rather than only testing the codec directly. */
	np2cfg.upd8087_enable = 1;
	np2cfg.upd8087_clock_hz = 8000000U;
	pccore_reset();
	if (!upd8087.enabled || (upd8087.clock_hz != 8000000U)) {
		pccore_term();
		soundmng_deinitialize();
		return fail("8087 statsave", "reset did not apply enabled 8 MHz configuration");
	}
	upd8087.service_remainder = 1234567;
	upd8087.status |= UPD8087_STATUS_IE;
	upd8087_set_control(&upd8087,
	                   (uint16_t)(upd8087.control & (uint16_t)~UPD8087_STATUS_IE));
	if (!upd8087.busy || !upd8087.pending_interrupt ||
	    (upd8087.status & (UPD8087_STATUS_IR | UPD8087_STATUS_B)) !=
	    (UPD8087_STATUS_IR | UPD8087_STATUS_B)) {
		pccore_term();
		soundmng_deinitialize();
		return fail("8087 statsave", "pending exception was not armed");
	}
	pic_setirq(IRQ_NDP);
	expected_8087_clock = upd8087.clock_hz;
	expected_8087_remainder = upd8087.service_remainder;
#if defined(VAEG_UPD9002_M46_TESTING)
	if (upd9002_dispatch_normalization_verify_live() != SUCCESS) {
		pccore_term();
		soundmng_deinitialize();
		return fail("dispatch normalization", "initialization/reset changed tables");
	}
#endif

#if defined(VAEG_UPD9002_M44_TESTING)
	if (upd9002_state_scenario_requested()) {
		ret = upd9002_state_scenario_run();
		pccore_term();
		soundmng_deinitialize();
		return (ret == SUCCESS) ? SUCCESS : fail("statsave-scenario", "scenario operation failed");
	}
#endif
	ret = STATFLAG_SUCCESS;
	if ((pccore.multiple != PCCORE_STANDARD_MULTIPLE) ||
	    (pccore.realclock != pccore.baseclock * PCCORE_STANDARD_MULTIPLE) ||
	    (pccore_cpu_multiple() != np2cfg.multiple)) {
		ret = STATFLAG_FAILURE;
	}
#if defined(VAEG_UPD9002_M42_TESTING)
	if ((ret == STATFLAG_SUCCESS) &&
	    (upd9002_harness_run_manifest(VAEG_UPD9002_HARNESS_MANIFEST_PATH) != SUCCESS)) {
		ret = STATFLAG_FAILURE;
	}
	if ((ret == STATFLAG_SUCCESS) &&
	    (upd9002_fixture_verify(VAEG_UPD9002_FIXTURE_PATH) != SUCCESS)) {
		ret = STATFLAG_FAILURE;
	}
#endif

#if defined(VAEG_UPD780_INTEGRATION_TESTING)
	if (ret == STATFLAG_SUCCESS) {
		subsystem_upd780_test_reset();
		subsystem_upd780_test_install(0, f4_program, sizeof(f4_program));
		subsystem_upd780_test_set_pc(0);
		subsystem_upd780_test_set_clock(22);
		subsystem_exec();
		subsystem_upd780_test_get_trace(&upd780trace);
		if ((upd780trace.f4_count != 1) || (upd780trace.f4_last_value != 0x5a)) {
			ret = STATFLAG_FAILURE;
		}
	}
#endif

	if (ret == STATFLAG_SUCCESS) {
		hostfat_image = (BYTE *)_MALLOC(HOSTFAT_IMAGE_SIZE, "hostfat-state-selftest");
		if (hostfat_image == NULL) {
			ret = STATFLAG_FAILURE;
		} else {
			ZeroMemory(hostfat_image, HOSTFAT_IMAGE_SIZE);
			hostfat_image[0] = 0xf0;
			ret = (hostfat_mount_image(hostfat_image, HOSTFAT_IMAGE_SIZE) == SUCCESS)
			          ? STATFLAG_SUCCESS
			          : STATFLAG_FAILURE;
		}
	}
	if (ret == STATFLAG_SUCCESS) {
		/* Match the GUI save boundary so host audio cannot advance YMFM state mid-save. */
		soundmng_stop();
		ret = statsave_save(path1);
	}
#if defined(VAEG_UPD9002_M44_TESTING)
	if ((ret == STATFLAG_SUCCESS) && (upd9002_statsave_boundary_verify(path1) != SUCCESS)) {
		ret = STATFLAG_FAILURE;
	}
#endif
	if (ret == STATFLAG_SUCCESS) {
		ZeroMemory(err, sizeof(err));
		ret = statsave_check(path1, err, sizeof(err));
	}
	if (ret == STATFLAG_SUCCESS) {
		hostfat_image[HOSTFAT_SECTOR_SIZE] ^= 0x5a;
		if (hostfat_mount_image(hostfat_image, HOSTFAT_IMAGE_SIZE) != SUCCESS) {
			ret = STATFLAG_FAILURE;
		} else {
			UINT32 mismatched_digest;

			identity_ip = CPU_IP;
			identity_memory = upd9002_memoryread(0x0400);
			mismatched_digest = hostfat_image_digest();
			ZeroMemory(err, sizeof(err));
			if ((statsave_check(path1, err, sizeof(err)) != STATFLAG_FAILURE) ||
			    (strstr(err, "HOSTFAT snapshot") == NULL) || (CPU_IP != identity_ip) ||
			    (upd9002_memoryread(0x0400) != identity_memory)) {
				ret = STATFLAG_FAILURE;
			}
			ZeroMemory(err, sizeof(err));
			if (ret == STATFLAG_SUCCESS) {
				if (statsave_check_hostfat_override(path1, err, sizeof(err)) != STATFLAG_SUCCESS) {
					ret = STATFLAG_FAILURE;
				} else {
					CPU_IP ^= 0x0100;
					upd9002_memorywrite(0x0400, identity_memory ^ 0xff);
					if ((statsave_load_hostfat_override(path1) != STATFLAG_SUCCESS) ||
					    (CPU_IP != identity_ip) ||
					    (upd9002_memoryread(0x0400) != identity_memory) ||
					    (hostfat_image_digest() != mismatched_digest)) {
						ret = STATFLAG_FAILURE;
					}
				}
			}
		}
		hostfat_image[HOSTFAT_SECTOR_SIZE] ^= 0x5a;
		if ((ret == STATFLAG_SUCCESS) &&
		    (hostfat_mount_image(hostfat_image, HOSTFAT_IMAGE_SIZE) != SUCCESS)) {
			ret = STATFLAG_FAILURE;
		}
	}
	if ((ret == STATFLAG_SUCCESS) &&
	    (make_statsave_with_retired_fmboard(path1, pathbad) != SUCCESS)) {
		ret = STATFLAG_FAILURE;
	}
	if (ret == STATFLAG_SUCCESS) {
		identity_ip = CPU_IP;
		identity_memory = upd9002_memoryread(0x0400);
		ZeroMemory(err, sizeof(err));
		if ((statsave_check(pathbad, err, sizeof(err)) != STATFLAG_FAILURE) ||
		    (strstr(err, "retired sound hardware") == NULL) || (CPU_IP != identity_ip) ||
		    (upd9002_memoryread(0x0400) != identity_memory) ||
		    (statsave_load(pathbad) != STATFLAG_FAILURE) || (CPU_IP != identity_ip) ||
		    (upd9002_memoryread(0x0400) != identity_memory)) {
			ret = STATFLAG_FAILURE;
		}
	}
	if ((ret == STATFLAG_SUCCESS) &&
	    (make_statsave_with_unsupported_subcpu(path1, pathbad) != SUCCESS)) {
		ret = STATFLAG_FAILURE;
	}
	if ((ret == STATFLAG_SUCCESS) && (statsave_load(pathbad) != STATFLAG_FAILURE)) {
		ret = STATFLAG_FAILURE;
	}
	if (ret == STATFLAG_SUCCESS) {
		/* Force the loader to initialize a different baseline before the saved
		 * UPD8087 section is applied. */
		np2cfg.upd8087_enable = 0;
		np2cfg.upd8087_clock_hz = 5000000U;
		ret = statsave_load(path1);
	}
	if ((ret == STATFLAG_SUCCESS) &&
	    (!upd8087.enabled || upd8087.clock_hz != expected_8087_clock ||
	     upd8087.service_remainder != expected_8087_remainder ||
	     !upd8087.busy || !upd8087.pending_interrupt ||
	     (upd8087.status & (UPD8087_STATUS_IR | UPD8087_STATUS_B)) !=
	     (UPD8087_STATUS_IR | UPD8087_STATUS_B) ||
	     !(pic.pi[1].irr & PIC_NDP))) {
		ret = STATFLAG_FAILURE;
	}
#if defined(VAEG_UPD9002_M46_TESTING)
	if ((ret == STATFLAG_SUCCESS) && (upd9002_dispatch_normalization_verify_live() != SUCCESS)) {
		ret = STATFLAG_FAILURE;
	}
#endif
#if defined(VAEG_UPD780_INTEGRATION_TESTING)
	if (ret == STATFLAG_SUCCESS) {
		subsystem_upd780_test_get_trace(&upd780trace);
		if ((upd780trace.f4_count != 1) || (upd780trace.f4_last_value != 0x5a)) {
			ret = STATFLAG_FAILURE;
		}
	}
#endif
	if (ret == STATFLAG_SUCCESS) {
		/* statsave_load() resumes audio after restoring the captured state. */
		soundmng_stop();
		ret = statsave_save(path2);
	}
#if defined(VAEG_UPD9002_SSTS_TESTING)
	/* Keep the legacy trace selftest's first eight CPU steps stable: the
	 * existing M42/M60a trace contract runs before these additional 8087
	 * production fixtures.  The fixtures still use the real machine path and
	 * run while the machine is live; by this point the bounded trace window has
	 * already completed. */
	if (ret == STATFLAG_SUCCESS && test_8087_production_path() != SUCCESS) {
		ret = STATFLAG_FAILURE;
	}
	if (ret == STATFLAG_SUCCESS && test_8087_production_inventory() != SUCCESS) {
		ret = STATFLAG_FAILURE;
	}
	if (ret == STATFLAG_SUCCESS && test_8087_cpu_decode_seams() != SUCCESS) {
		ret = STATFLAG_FAILURE;
	}
	if (ret == STATFLAG_SUCCESS && test_8087_guest_interrupt_route() != SUCCESS) {
		ret = STATFLAG_FAILURE;
	}
	if (ret == STATFLAG_SUCCESS && test_8087_pic_input_modes() != SUCCESS) {
		ret = STATFLAG_FAILURE;
	}
#endif
	np2cfg.upd8087_enable = saved_8087_enable;
	np2cfg.upd8087_clock_hz = saved_8087_config_clock;
	pccore_term();
	soundmng_deinitialize();
	hostfat_unmount();
	if (hostfat_image != NULL) {
		_MFREE(hostfat_image);
	}

	if (ret != STATFLAG_SUCCESS) {
		file_delete(path1);
		file_delete(path2);
		file_delete(pathbad);
		return (fail("statsave", "save/check/load returned failure"));
	}
	if (compare_statsave_stable_sections(path1, path2) != SUCCESS) {
		file_delete(path1);
		file_delete(path2);
		file_delete(pathbad);
		return (fail("statsave", "save/load/save bytes differ"));
	}

	file_delete(path1);
	file_delete(path2);
	file_delete(pathbad);
	fprintf(stderr, "selftest: statsave ok\n");
	return (SUCCESS);
}

typedef struct {
	char text[256];
} TOKENBUF;

static void tokenbuf_emit(const char *token, void *arg) {
	TOKENBUF *buf;

	buf = (TOKENBUF *)arg;
	if (buf->text[0] != '\0') {
		milstr_ncat(buf->text, ",", sizeof(buf->text));
	}
	milstr_ncat(buf->text, token, sizeof(buf->text));
}

static int test_romankana(void) {
	ROMANKANA_STATE state;
	TOKENBUF buf;

	romankana_reset(&state);
	ZeroMemory(&buf, sizeof(buf));
	romankana_feed(&state, "AkaShi", tokenbuf_emit, &buf);
	romankana_flush(&state, tokenbuf_emit, &buf);
	if (strcmp(buf.text, "a,ka,shi") != 0) {
		return (fail("romankana", "uppercase/basic syllables failed"));
	}

	romankana_reset(&state);
	ZeroMemory(&buf, sizeof(buf));
	romankana_feed(&state, "shi si tsu tu", tokenbuf_emit, &buf);
	romankana_flush(&state, tokenbuf_emit, &buf);
	if (strcmp(buf.text, "shi,?,shi,?,tsu,?,tsu") != 0) {
		return (fail("romankana", "shi/si and tsu/tu aliases failed"));
	}

	romankana_reset(&state);
	ZeroMemory(&buf, sizeof(buf));
	romankana_feed(&state, "nn n' nka kko", tokenbuf_emit, &buf);
	romankana_flush(&state, tokenbuf_emit, &buf);
	if (strcmp(buf.text, "nn,?,nn,?,nn,ka,?,xtsu,ko") != 0) {
		return (fail("romankana", "n and doubled-consonant handling failed"));
	}

	romankana_reset(&state);
	ZeroMemory(&buf, sizeof(buf));
	romankana_feed(&state, "gaza daba papa", tokenbuf_emit, &buf);
	romankana_flush(&state, tokenbuf_emit, &buf);
	if (strcmp(buf.text, "ga,za,?,da,ba,?,pa,pa") != 0) {
		return (fail("romankana", "voiced syllables failed"));
	}

	romankana_reset(&state);
	ZeroMemory(&buf, sizeof(buf));
	romankana_feed(&state, "nya nyu nyo kya sha syo chu tyo ryo gya ja pyo", tokenbuf_emit, &buf);
	romankana_flush(&state, tokenbuf_emit, &buf);
	if (strcmp(buf.text, "nya,?,nyu,?,nyo,?,kya,?,sha,?,sho,?,chu,?,cho,?,ryo,?,gya,?,ja,?,pyo") !=
	    0) {
		return (fail("romankana", "yoon syllables failed"));
	}

	romankana_reset(&state);
	ZeroMemory(&buf, sizeof(buf));
	romankana_feed(&state, "xya lyu xyo xa li xo xtu ltu", tokenbuf_emit, &buf);
	romankana_flush(&state, tokenbuf_emit, &buf);
	if (strcmp(buf.text, "xya,?,xyu,?,xyo,?,xa,?,xi,?,xo,?,xtsu,?,xtsu") != 0) {
		return (fail("romankana", "small kana aliases failed"));
	}

	romankana_reset(&state);
	ZeroMemory(&buf, sizeof(buf));
	romankana_feed(&state, "va vi vu ve vo", tokenbuf_emit, &buf);
	romankana_flush(&state, tokenbuf_emit, &buf);
	if (strcmp(buf.text, "va,?,vi,?,vu,?,ve,?,vo") != 0) {
		return (fail("romankana", "vu syllables failed"));
	}
	fprintf(stderr, "selftest: romankana ok\n");
	return (SUCCESS);
}

static int test_keyboard_mapping(void) {
	char printable[96];
	KBDPASTE_ACTION actions[96];
	size_t count;
	UINT skipped;
	UINT index;

	if (kbdmap_selftest() != SUCCESS) {
		return (fail("keyboard-map", "mapping lookup/persistence failed"));
	}
	if (test_romankana() != SUCCESS) {
		return (FAILURE);
	}
	for (index = 0; index < 95; index++) {
		printable[index] = (char)(0x20 + index);
	}
	printable[95] = '\0';
	count = kbdpaste_map_text(printable, actions, NELEMENTS(actions), &skipped);
	if ((count != 95) || (skipped != 0)) {
		return (fail("keyboard-paste", "printable ASCII coverage failed"));
	}
	count = kbdpaste_map_text("aA0@\"=_\\~", actions, NELEMENTS(actions), &skipped);
	if ((count != 9) || (skipped != 0) || (actions[0].guest_code != kbdmap_guest_code(KBDROLE_A)) ||
	    actions[0].shift || (actions[1].guest_code != kbdmap_guest_code(KBDROLE_A)) ||
	    !actions[1].shift || (actions[2].guest_code != kbdmap_guest_code(KBDROLE_0)) ||
	    actions[2].shift || (actions[3].guest_code != kbdmap_guest_code(KBDROLE_AT)) ||
	    actions[3].shift || (actions[4].guest_code != kbdmap_guest_code(KBDROLE_2)) ||
	    !actions[4].shift || (actions[5].guest_code != kbdmap_guest_code(KBDROLE_MINUS)) ||
	    !actions[5].shift || (actions[6].guest_code != kbdmap_guest_code(KBDROLE_UNDERSCORE)) ||
	    !actions[6].shift || (actions[7].guest_code != kbdmap_guest_code(KBDROLE_YEN)) ||
	    actions[7].shift || (actions[8].guest_code != kbdmap_guest_code(KBDROLE_AT)) ||
	    !actions[8].shift) {
		return (fail("keyboard-paste", "guest chord mapping failed"));
	}
	count = kbdpaste_map_text("a\rb\nc\r\nd", actions, NELEMENTS(actions), &skipped);
	if ((count != 7) || (skipped != 0) || (actions[0].guest_code != kbdmap_guest_code(KBDROLE_A)) ||
	    (actions[1].guest_code != kbdmap_guest_code(KBDROLE_RETURNL)) ||
	    (actions[3].guest_code != kbdmap_guest_code(KBDROLE_RETURNL)) ||
	    (actions[5].guest_code != kbdmap_guest_code(KBDROLE_RETURNL))) {
		return (fail("keyboard-paste", "CR/LF normalization failed"));
	}
	count = kbdpaste_map_text("a\t\xc3\xa9", actions, NELEMENTS(actions), &skipped);
	if ((count != 1) || (skipped != 2) || (actions[0].guest_code != kbdmap_guest_code(KBDROLE_A)) ||
	    (kbdpaste_interval_ms() != 20)) {
		return (fail("keyboard-paste", "UTF-8 skip or pacing failed"));
	}
	fprintf(stderr, "selftest: keyboard paste mapping ok\n");
	fprintf(stderr, "selftest: keyboard mapping ok\n");
	return (SUCCESS);
}

static int test_mouse_state(void) {
	VAEG_MOUSE_STATE state;
	SINT16 x;
	SINT16 y;
	BYTE buttons;
	BYTE saved_f12;

	vaeg_mouse_state_initialize(&state);
	buttons = vaeg_mouse_state_getstat(&state, &x, &y, 1);
	if ((x != 0) || (y != 0) || (buttons != VAEG_MOUSE_RELEASED)) {
		return (fail("mouse", "initial state is not released"));
	}
	vaeg_mouse_state_motion(&state, 20, -20);
	vaeg_mouse_state_button(&state, VAEG_MOUSE_BUTTON_LEFT, TRUE);
	buttons = vaeg_mouse_state_getstat(&state, &x, &y, 1);
	if ((x != 0) || (y != 0) || (buttons != VAEG_MOUSE_RELEASED)) {
		return (fail("mouse", "inactive input was accepted"));
	}

	vaeg_mouse_state_set_active(&state, TRUE);
	vaeg_mouse_state_motion(&state, 3, -5);
	buttons = vaeg_mouse_state_getstat(&state, &x, &y, 0);
	if ((x != 1) || (y != -2)) {
		return (fail("mouse", "relative movement scaling failed"));
	}
	vaeg_mouse_state_getstat(&state, &x, &y, 1);
	vaeg_mouse_state_motion(&state, 1, -1);
	vaeg_mouse_state_getstat(&state, &x, &y, 1);
	if ((x != 1) || (y != -1)) {
		return (fail("mouse", "relative movement remainder was lost"));
	}

	vaeg_mouse_state_button(&state, VAEG_MOUSE_BUTTON_LEFT, TRUE);
	buttons = vaeg_mouse_state_getstat(&state, &x, &y, 0);
	if (buttons != VAEG_MOUSE_RIGHTBIT) {
		return (fail("mouse", "left button is not active-low"));
	}
	vaeg_mouse_state_button(&state, VAEG_MOUSE_BUTTON_RIGHT, TRUE);
	buttons = vaeg_mouse_state_getstat(&state, &x, &y, 0);
	if (buttons != 0) {
		return (fail("mouse", "right button is not active-low"));
	}
	vaeg_mouse_state_button(&state, VAEG_MOUSE_BUTTON_LEFT, FALSE);
	vaeg_mouse_state_button(&state, VAEG_MOUSE_BUTTON_RIGHT, FALSE);
	buttons = vaeg_mouse_state_getstat(&state, &x, &y, 0);
	if (buttons != VAEG_MOUSE_RELEASED) {
		return (fail("mouse", "button release failed"));
	}

	state.x = ((SINT64)INT16_MAX + 1) * 2;
	vaeg_mouse_state_getstat(&state, &x, &y, 1);
	if (x != INT16_MAX) {
		return (fail("mouse", "large movement was not clamped"));
	}
	vaeg_mouse_state_getstat(&state, &x, &y, 1);
	if (x != 1) {
		return (fail("mouse", "clamped movement remainder was lost"));
	}
	state.x = INT64_MAX - 1;
	vaeg_mouse_state_motion(&state, INT32_MAX, 0);
	if (state.x != INT64_MAX) {
		return (fail("mouse", "movement accumulator overflowed"));
	}
	vaeg_mouse_state_reset(&state);
	if (!state.active || (state.x != 0) || (state.y != 0) ||
	    (state.buttons != VAEG_MOUSE_RELEASED)) {
		return (fail("mouse", "reset did not release active state"));
	}
	vaeg_mouse_state_set_active(&state, FALSE);
	if (state.active || (state.buttons != VAEG_MOUSE_RELEASED)) {
		return (fail("mouse", "capture disable did not release state"));
	}

	saved_f12 = np2oscfg.F12KEY;
	np2oscfg.F12KEY = 0;
	if (kbdmap_lookup(SDL_SCANCODE_F12) != KBDMAP_NC) {
		np2oscfg.F12KEY = saved_f12;
		return (fail("mouse", "F12 Mouse binding leaked a guest key"));
	}
	np2oscfg.F12KEY = 1;
	if (kbdmap_lookup(SDL_SCANCODE_F12) != 0x61) {
		np2oscfg.F12KEY = saved_f12;
		return (fail("mouse", "F12 COPY binding changed"));
	}
	np2oscfg.F12KEY = 2;
	if (kbdmap_lookup(SDL_SCANCODE_F12) != 0x60) {
		np2oscfg.F12KEY = saved_f12;
		return (fail("mouse", "F12 STOP binding changed"));
	}
	np2oscfg.F12KEY = 3;
	if (kbdmap_lookup(SDL_SCANCODE_F12) != 0x4d) {
		np2oscfg.F12KEY = saved_f12;
		return (fail("mouse", "F12 keypad-equal binding changed"));
	}
	np2oscfg.F12KEY = 4;
	if (kbdmap_lookup(SDL_SCANCODE_F12) != 0x4f) {
		np2oscfg.F12KEY = saved_f12;
		return (fail("mouse", "F12 keypad-comma binding changed"));
	}
	np2oscfg.F12KEY = 5;
	if (kbdmap_lookup(SDL_SCANCODE_F12) != 0x5a) {
		np2oscfg.F12KEY = saved_f12;
		return (fail("mouse", "F12 PC-key binding changed"));
	}
	np2oscfg.F12KEY = KBDMAP_F12_FULL_SPEED;
	if ((kbdmap_lookup(SDL_SCANCODE_F12) != KBDMAP_NC) ||
	    (kbdmap_special_action(SDL_SCANCODE_F12) != KBDMAP_SPECIAL_FAST_FORWARD)) {
		np2oscfg.F12KEY = saved_f12;
		return (fail("mouse", "F12 full-speed binding changed"));
	}
	np2oscfg.F12KEY = KBDMAP_F12_SCREENSHOT;
	if ((kbdmap_lookup(SDL_SCANCODE_F12) != KBDMAP_NC) ||
	    (kbdmap_special_action(SDL_SCANCODE_F12) != KBDMAP_SPECIAL_SCREENSHOT)) {
		np2oscfg.F12KEY = saved_f12;
		return (fail("mouse", "F12 screenshot binding changed"));
	}
	np2oscfg.F12KEY = saved_f12;
	fprintf(stderr, "selftest: mouse state ok\n");
	return (SUCCESS);
}

static void program_opn_test_voice(void) {
	static const REG8 slots[] = {0x00, 0x04, 0x08, 0x0c};
	UINT index;

	for (index = 0; index < NELEMENTS(slots); index++) {
		opngen_setreg(0, 0x30 + slots[index], 0x01);
		opngen_setreg(0, 0x40 + slots[index], 0x00);
		opngen_setreg(0, 0x50 + slots[index], 0x1f);
		opngen_setreg(0, 0x60 + slots[index], 0x00);
		opngen_setreg(0, 0x70 + slots[index], 0x00);
		opngen_setreg(0, 0x80 + slots[index], 0x0f);
		opngen_setreg(0, 0x90 + slots[index], 0x00);
	}
	opngen_setreg(0, 0xa0, 0x98);
	opngen_setreg(0, 0xa4, 0x22);
	opngen_setreg(0, 0xb0, 0x07);
	opngen_setreg(0, 0xb4, 0xc0);
	opngen_keyon(0, 0xf0);
}

static int test_sound_options(void) {
	if (!vaeg_sound_rate_valid(11025) || !vaeg_sound_rate_valid(22050) ||
	    !vaeg_sound_rate_valid(44100) || vaeg_sound_rate_valid(0) || vaeg_sound_rate_valid(48000)) {
		return (fail("sound-options", "sampling-rate validation failed"));
	}
	if (!vaeg_sound_buffer_valid(40) || !vaeg_sound_buffer_valid(1000) ||
	    vaeg_sound_buffer_valid(39) || vaeg_sound_buffer_valid(1001) ||
	    (vaeg_sound_buffer_clamp(39) != 40) || (vaeg_sound_buffer_clamp(1001) != 1000) ||
	    (vaeg_sound_buffer_clamp(200) != 200)) {
		return (fail("sound-options", "buffer validation failed"));
	}
	if ((vaeg_sound_buffer_samples(44100, 40, 2) != 1024) ||
	    (vaeg_sound_buffer_samples(22050, 500, 2) != 8192) ||
	    (vaeg_sound_buffer_samples(48000, 40, 2) != 0) ||
	    (vaeg_sound_buffer_samples(44100, 39, 2) != 0) ||
	    (vaeg_sound_buffer_samples(44100, 40, 0) != 0)) {
		return (fail("sound-options", "buffer sample calculation failed"));
	}
	fprintf(stderr, "selftest: sound options ok\n");
	return (SUCCESS);
}

static int opn_backend_produces_audio(UINT backend, REG8 channels, UINT flags) {
	SINT32 pcm[4096 * 2];
	UINT index;

	opngen_setbackend(backend);
	opngen_reset();
	opngen_setcfg(channels, flags);
	opngen_setvol(64);
	program_opn_test_voice();
	ZeroMemory(pcm, sizeof(pcm));
	opngen_getpcm(NULL, pcm, NELEMENTS(pcm) / 2);
	for (index = 0; index < NELEMENTS(pcm); index++) {
		if (pcm[index] != 0) {
			return (SUCCESS);
		}
	}
	return (FAILURE);
}

static int test_opn_backends(void) {
	BOOL saved_sound_enabled;

	if ((np2_default_sound_for_model(str_VA1) != FMBOARD_VA_OPN) ||
	    (np2_default_sound_for_model(str_VA2) != FMBOARD_VA_OPNA) ||
	    (np2_sound_for_model_selection(str_VA1, FMBOARD_VA_OPNA) != FMBOARD_VA_OPNA) ||
	    (np2_sound_for_model_selection(str_VA2, FMBOARD_VA_OPN) != FMBOARD_VA_OPNA) ||
	    (np2_sound_for_model_selection(str_VA1, FMBOARD_NONE) != FMBOARD_VA_OPN) ||
	    (np2_sound_hardware_valid(str_VA1, FMBOARD_NONE) != FALSE) ||
	    (np2_sound_hardware_valid(str_VA1, FMBOARD_VA_OPN) != TRUE) ||
	    (np2_sound_hardware_valid(str_VA1, FMBOARD_VA_OPNA) != TRUE) ||
	    (np2_sound_hardware_valid(str_VA2, FMBOARD_VA_OPN) != FALSE) ||
	    (np2_sound_hardware_valid(str_VA2, FMBOARD_VA_OPNA) != TRUE)) {
		return (fail("opn-backend", "VA sound hardware policy failed"));
	}
	saved_sound_enabled = soundmng_isenabled();
	soundmng_setenabled(FALSE);
	if (soundmng_isenabled()) {
		return (fail("opn-backend", "host audio disable was ignored"));
	}
	soundmng_setenabled(TRUE);
	if (!soundmng_isenabled()) {
		return (fail("opn-backend", "host audio enable was ignored"));
	}
	soundmng_setenabled(saved_sound_enabled);
	opngen_initialize(44100);
	if ((ymfm_opn_parsefidelity("minimum") != YMFMBRIDGE_FIDELITY_MINIMUM) ||
	    (ymfm_opn_parsefidelity("medium") != YMFMBRIDGE_FIDELITY_MEDIUM) ||
	    (ymfm_opn_parsefidelity("maximum") != YMFMBRIDGE_FIDELITY_MAXIMUM) ||
	    (ymfm_opn_parsefidelity("invalid") != YMFMBRIDGE_FIDELITY_DEFAULT)) {
		return (fail("opn-backend", "ymfm fidelity parsing failed"));
	}
	ymfm_opn_setfidelity(YMFMBRIDGE_FIDELITY_MEDIUM);
	if ((ymfm_opn_getfidelity() != YMFMBRIDGE_FIDELITY_MEDIUM) ||
	    strcmp(ymfm_opn_fidelityname(ymfm_opn_getfidelity()), "medium")) {
		return (fail("opn-backend", "ymfm fidelity selection failed"));
	}
	ymfm_opn_setfidelity(YMFMBRIDGE_FIDELITY_MAXIMUM);
	if ((ymfm_opn_getfidelity() != YMFMBRIDGE_FIDELITY_MAXIMUM) ||
	    strcmp(ymfm_opn_fidelityname(ymfm_opn_getfidelity()), "maximum")) {
		return (fail("opn-backend", "maximum ymfm fidelity selection failed"));
	}
	ymfm_opn_setfidelity(99);
	if (ymfm_opn_getfidelity() != YMFMBRIDGE_FIDELITY_DEFAULT) {
		return (fail("opn-backend", "invalid ymfm fidelity did not fallback"));
	}
	if (opngen_parsebackend("np2") != OPN_BACKEND_NP2 ||
	    opngen_parsebackend("ymfm") != OPN_BACKEND_YMFM ||
	    opngen_parsebackend("invalid") != OPN_BACKEND_YMFM ||
	    opngen_parsebackend("") != OPN_BACKEND_YMFM ||
	    opngen_parsebackend(NULL) != OPN_BACKEND_YMFM) {
		return (fail("opn-backend", "backend name parsing failed"));
	}
	if (opn_backend_produces_audio(OPN_BACKEND_NP2, 3, OPN_MONORAL | 0x007) != SUCCESS) {
		return (fail("opn-backend", "NP2 YM2203 path was silent"));
	}
	if (opn_backend_produces_audio(OPN_BACKEND_YMFM, 3, OPN_MONORAL | 0x007) != SUCCESS) {
		return (fail("opn-backend", "ymfm YM2203 path was silent"));
	}
	ymfm_opn_setfidelity(YMFMBRIDGE_FIDELITY_MAXIMUM);
	if (opn_backend_produces_audio(OPN_BACKEND_YMFM, 6, OPN_STEREO | 0x03f) != SUCCESS) {
		return (fail("opn-backend", "ymfm YM2608 path was silent"));
	}
	opngen_setbackend(OPN_BACKEND_YMFM);
	ymfm_opn_setfidelity(YMFMBRIDGE_FIDELITY_DEFAULT);
	opngen_reset();
	fprintf(stderr, "selftest: OPN backends ok\n");
	return (SUCCESS);
}

int vaeg_selftest_run(void) {
#if defined(VAEG_UPD9002_M44_TESTING)
	if (upd9002_state_scenario_requested()) {
		return (test_statsave() == SUCCESS ? SUCCESS : FAILURE);
	}
#endif

	if (sxsi_image_selftest() != SUCCESS) {
		return (fail("SCSI image backing", "creation or boundary tests failed"));
	}
	if (scsicmd_backend_selftest() != SUCCESS) {
		return (fail("SCSI backend", "compiled LUN/INQUIRY tests failed"));
	}
	if (scsiio_transfer_selftest() != SUCCESS) {
		return (fail("SCSI Transfer Info", "compiled controller-path tests failed"));
	}
	if (test_codecnv() != SUCCESS) {
		return (FAILURE);
	}
	if (test_romcheck() != SUCCESS) {
		return (FAILURE);
	}
	if (test_cli_boot_model() != SUCCESS) {
		return (FAILURE);
	}
	if (test_cli_options() != SUCCESS) {
		return (FAILURE);
	}
	if (test_screenshot_scheduler() != SUCCESS) {
		return (FAILURE);
	}
	if ((upd9002_debug_selftest() != SUCCESS) || (debug_harness_selftest() != SUCCESS)) {
		return (fail("debug harness", "counter, ordinal, or script test failed"));
	}
	fprintf(stderr, "selftest: debug harness ok\n");
	if (test_hostfat_snapshot() != SUCCESS) {
		return (FAILURE);
	}
	if (np2_cli_override_selftest() != TRUE) {
		return (fail("CLI options", "session override restoration failed"));
	}
	fprintf(stderr, "selftest: CLI override restoration ok\n");
	if (test_va_tvram_window() != SUCCESS) {
		return (FAILURE);
	}
	if (test_profile_ini() != SUCCESS) {
		return (FAILURE);
	}
	if (test_persistence_controls() != SUCCESS) {
		return (FAILURE);
	}
	if (test_main_ram_configuration() != SUCCESS) {
		return (FAILURE);
	}
	if (test_va_layer_display() != SUCCESS) {
		return (FAILURE);
	}
	if (test_framedisp() != SUCCESS) {
		return (FAILURE);
	}
	if (test_new_fdd_image() != SUCCESS) {
		return (FAILURE);
	}
	if (test_fdd_d88_production_path() != SUCCESS) {
		return (FAILURE);
	}
	if (test_sasi_image_validation() != SUCCESS) {
		return (FAILURE);
	}
	if (test_clockscale() != SUCCESS) {
		return (FAILURE);
	}
	if (test_sgp_speed() != SUCCESS) {
		return (FAILURE);
	}
	if (sgp_manual_commands_selftest() != SUCCESS) {
		return (fail("SGP manual commands", "descriptor, LINE, ROP, TP=2, or SCAN test failed"));
	}
	fprintf(stderr, "selftest: SGP manual commands ok\n");
	if (test_pacing() != SUCCESS) {
		return (FAILURE);
	}
	if (test_viewport() != SUCCESS) {
		return (FAILURE);
	}
	if (test_va_raster_guard() != SUCCESS) {
		return (FAILURE);
	}
	if (dropmedia_selftest() != SUCCESS) {
		return (fail("dropmedia", "extension, path, or sorting policy failed"));
	}
	fprintf(stderr, "selftest: dropmedia ok\n");
	if (test_mouse_state() != SUCCESS) {
		return (FAILURE);
	}
	if (test_sound_options() != SUCCESS) {
		return (FAILURE);
	}
	if (test_keyboard_mapping() != SUCCESS) {
		return (FAILURE);
	}
	if (test_opn_backends() != SUCCESS) {
		return (FAILURE);
	}
	if (test_statsave() != SUCCESS) {
		return (FAILURE);
	}
	if (test_va_bms_window() != SUCCESS) {
		return (FAILURE);
	}
	if (test_va_ems_board() != SUCCESS) {
		return (FAILURE);
	}
	if (test_hostfat_transport() != SUCCESS) {
		return (FAILURE);
	}
#if defined(VAEG_UPD780_INTEGRATION_TESTING)
	if (vaeg_upd780_subsystem_integration_test() != SUCCESS) {
		return (fail("uPD780 subsystem integration", "production seam test failed"));
	}
#endif
	fprintf(stderr, "selftest: all tests passed\n");
	return (SUCCESS);
}
