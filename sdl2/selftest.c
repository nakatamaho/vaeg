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
#include	"compiler.h"
#include	"selftest.h"
#include	"codecnv.h"
#include	"commng.h"
#include	"clockscale.h"
#include	"bmsio.h"
#include	"cliopts.h"
#include	"dosio.h"
#include	"dropmedia.h"
#include	"fddfile.h"
#include	"fdd_d88.h"
#include	"fdd_xdf.h"
#include	"framedisp.h"
#include	"hostfat.h"
#include	"hostfat_snapshot.h"
#include	"ini.h"
#include	"pccore.h"
#include	"cpucore.h"
#include	"iocore.h"
#include	"iocoreva.h"
#include	"kbdmap.h"
#include	"kbdpaste.h"
#include	"memoryva.h"
#include	"mousestate.h"
#include	"newdisk.h"
#include	"np2.h"
#include	"pacing.h"
#include	"sound.h"
#include	"opngen.h"
#include	"profile.h"
#include	"romcheck.h"
#include	"romankana.h"
#include	"s98.h"
#include	"sgp.h"
#include	"sxsi.h"
#include	"scrndraw.h"
#include	"scrnmng.h"
#include	"scrndrawva.h"
#include	"sdrawva.h"
#include	"soundmng.h"
#include	"soundopts.h"
#include	"strres.h"
#include	"viewport.h"
#include	"ymfmbridge.h"
#if defined(VAEG_UPD9002_M42_TESTING)
#include	"tests/upd9002/direct_harness.h"
#include	"tests/upd9002/fixtures.h"
#endif
#if defined(VAEG_UPD9002_M44_TESTING)
#include	"tests/upd9002/state_scenario.h"
#include	"tests/upd9002/statsave_boundary.h"
#endif
#if defined(VAEG_UPD9002_M46_TESTING)
#include	"tests/upd9002/dispatch_normalization.h"
#endif
#if defined(VAEG_Z80_INTEGRATION_TESTING)
#include	"iova/subsystem.h"
#include	"tests/z80/subsystem_integration.h"
#endif

static int fail(const char *name, const char *detail) {

	fprintf(stderr, "selftest: %s failed: %s\n", name, detail);
	return(FAILURE);
}

static int test_codecnv(void) {

	const char	sjis_wave[] = {(char)0x81, (char)0x60, '\0'};
	UINT16		utf[4];
	char		euc[8];
	char		sjis[8];

	ZeroMemory(utf, sizeof(utf));
	codecnv_sjis2utf(utf, NELEMENTS(utf), sjis_wave, sizeof(sjis_wave));
	if ((utf[0] != 0xff5e) || (utf[1] != 0)) {
		return(fail("codecnv", "CP932 wave-dash maps incorrectly"));
	}

	ZeroMemory(euc, sizeof(euc));
	ZeroMemory(sjis, sizeof(sjis));
	codecnv_sjis2euc(euc, sizeof(euc), sjis_wave, sizeof(sjis_wave));
	codecnv_euc2sjis(sjis, sizeof(sjis), euc, sizeof(euc));
	if ((memcmp(sjis, sjis_wave, 2) != 0) || (sjis[2] != '\0')) {
		return(fail("codecnv", "SJIS/EUC round-trip changed bytes"));
	}
	fprintf(stderr, "selftest: codecnv ok\n");
	return(SUCCESS);
}

static int test_romcheck(void) {

	static const char vector[] = "123456789";
	ROMCHECKSUM result;
	char sha1[41];

	romcheck_buffer(vector, sizeof(vector) - 1, &result);
	romcheck_sha1_string(result.sha1, sha1);
	if ((result.size != 9) || (result.crc32 != 0xcbf43926) ||
		strcmp(sha1, "f7c3bc1d808e04732adf679965ccc34ca7ae3441")) {
		return(fail("romcheck", "CRC32/SHA-1 test vector failed"));
	}
	fprintf(stderr, "selftest: romcheck ok\n");
	return(SUCCESS);
}

static int test_cli_boot_model(void) {

	if ((np2_cli_boot_model("va") != str_VA1) ||
		(np2_cli_boot_model("VA") != str_VA1) ||
		(np2_cli_boot_model("va2") != str_VA2) ||
		(np2_cli_boot_model("VA2") != str_VA2) ||
		(np2_cli_boot_model("va1") != NULL) ||
		(np2_cli_boot_model("va3") != NULL) ||
		(np2_cli_boot_model("") != NULL) ||
		(np2_cli_boot_model(NULL) != NULL)) {
		return(fail("CLI boot model", "model name parsing failed"));
	}
	fprintf(stderr, "selftest: CLI boot model ok\n");
	return(SUCCESS);
}

static int test_cli_options(void) {

	char *valid[] = {
		"vaeg", "--model", "VA", "--fmbackend", "NP2",
		"--fmsound", "OPNA", "--ymfm-fidelity", "MAXIMUM",
		"--samplerate", "44100", "--soundbuffer", "40", "--mute",
		"--fdd1", "boot.d88", "--fdd2", "none",
		"--sasi1", "disk.hdi", "--sasi2", "NONE",
		"--hostfat-dir", "host-root",
		"--cpumult", "32", "--sgp", "16", "--nowait",
		"--frameskip", "4", "--fullscreen", "--effect", "crt-lite",
		"--scaling", "fit-8dot", "--controller", "mouse",
		"--keyboard-layout", "custom", "--debug", "--fdctrace",
		"--pacelog", "--trace-cpu", "17", "--smoke"
	};
	char *positional[] = {"vaeg", "boot.d88"};
	char *invalid_model[] = {"vaeg", "--model", "va3"};
	char *invalid_backend[] = {"vaeg", "--fmbackend", "mame"};
	char *invalid_rate[] = {"vaeg", "--samplerate", "48000"};
	char *invalid_sgp[] = {"vaeg", "--sgp", "17"};
	char *invalid_scaling[] = {"vaeg", "--scaling", "nearest"};
	char *missing_value[] = {"vaeg", "--fdd1"};
	char *hostfat_disabled[] = {"vaeg", "--smoke"};
	VAEG_CLI_OPTIONS options;
	char error[256];

	if (vaeg_cli_parse((int)NELEMENTS(valid), valid, &options, error,
											sizeof(error)) != SUCCESS) {
		return(fail("CLI options", error));
	}
	if ((options.model != VAEG_CLI_MODEL_VA) ||
		(options.fm_backend != VAEG_CLI_FM_BACKEND_NP2) ||
		(options.fm_sound != VAEG_CLI_FM_SOUND_OPNA) ||
		(options.ymfm_fidelity != VAEG_CLI_FIDELITY_MAXIMUM) ||
		(options.sample_rate != 44100) || (options.sound_buffer != 40) ||
		!options.mute ||
		(options.fdd_mode[0] != VAEG_CLI_MEDIA_PATH) ||
		strcmp(options.fdd_path[0], "boot.d88") ||
		(options.fdd_mode[1] != VAEG_CLI_MEDIA_NONE) ||
		(options.sasi_mode[0] != VAEG_CLI_MEDIA_PATH) ||
		strcmp(options.sasi_path[0], "disk.hdi") ||
		(options.sasi_mode[1] != VAEG_CLI_MEDIA_NONE) ||
		(options.hostfat_path == NULL) ||
		strcmp(options.hostfat_path, "host-root") ||
		(options.cpu_multiplier != 32) ||
		(options.sgp_mode != VAEG_CLI_SGP_CUSTOM) ||
		(options.sgp_multiplier != 16) || !options.nowait ||
		(options.frame_skip != VAEG_CLI_FRAMESKIP_QUARTER) ||
		(options.display_mode != VAEG_CLI_DISPLAY_FULLSCREEN) ||
		(options.effect != VAEG_CLI_EFFECT_CRT_LITE) ||
		(options.scaling != VAEG_CLI_SCALING_FIT_8DOT) ||
		(options.controller != VAEG_CLI_CONTROLLER_MOUSE) ||
		(options.keyboard_layout != VAEG_CLI_KEYBOARD_CUSTOM) ||
		(options.trace_cpu != 17) ||
		!options.debug || !options.fdctrace || !options.pacelog ||
		!options.smoke) {
		return(fail("CLI options", "accepted values were parsed incorrectly"));
	}
	if ((vaeg_cli_parse((int)NELEMENTS(hostfat_disabled), hostfat_disabled,
			&options, error, sizeof(error)) != SUCCESS) ||
		(options.hostfat_path != NULL)) {
		return(fail("CLI options", "HOSTFAT was not disabled by default"));
	}
	if ((vaeg_cli_parse((int)NELEMENTS(positional), positional, &options,
			error, sizeof(error)) == SUCCESS) ||
		(strstr(error, "positional FDD arguments were removed") == NULL) ||
		(vaeg_cli_parse((int)NELEMENTS(invalid_model), invalid_model, &options,
			error, sizeof(error)) == SUCCESS) ||
		(vaeg_cli_parse((int)NELEMENTS(invalid_backend), invalid_backend,
			&options, error, sizeof(error)) == SUCCESS) ||
		(vaeg_cli_parse((int)NELEMENTS(invalid_rate), invalid_rate, &options,
			error, sizeof(error)) == SUCCESS) ||
		(vaeg_cli_parse((int)NELEMENTS(invalid_sgp), invalid_sgp, &options,
			error, sizeof(error)) == SUCCESS) ||
		(vaeg_cli_parse((int)NELEMENTS(invalid_scaling), invalid_scaling,
			&options, error, sizeof(error)) == SUCCESS) ||
		(vaeg_cli_parse((int)NELEMENTS(missing_value), missing_value, &options,
			error, sizeof(error)) == SUCCESS)) {
		return(fail("CLI options", "invalid input was accepted"));
	}
	fprintf(stderr, "selftest: CLI options ok\n");
	return(SUCCESS);
}

static void hostfat_send_string(const char *value) {

	while (*value != '\0') {
		iocoreva_out8(0x07ef, (REG8)*value++);
	}
}

static void hostfat_send_string_generic(const char *value) {

	while (*value != '\0') {
		iocore_out8(0x07ef, (REG8)*value++);
	}
}

static void hostfat_send_far_pointer(UINT32 value) {

	int index;

	for (index=0; index<4; index++) {
		iocoreva_out8(0x07ed, (REG8)value);
		value >>= 8;
	}
}

static int test_hostfat_transport(void) {

	BYTE *image;
	BYTE packet[18];
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
		return(fail("HOSTFAT transport", "image allocation failed"));
	}
	ZeroMemory(image, HOSTFAT_IMAGE_SIZE);
	for (index=0; index<HOSTFAT_SECTOR_SIZE; index++) {
		image[(3 * HOSTFAT_SECTOR_SIZE) + index] = (BYTE)(index ^ 0x5a);
	}
	if (hostfat_mount_image(image, HOSTFAT_IMAGE_SIZE) != SUCCESS) {
		_MFREE(image);
		return(fail("HOSTFAT transport", "image mount failed"));
	}
	_MFREE(image);

	packet_address = 0x01000;
	destination_address = 0x02000;
	request_pointer = 0x01000000;
	MEML_READ(packet_address, saved_packet, sizeof(saved_packet));
	MEML_READ(destination_address, saved_destination,
									sizeof(saved_destination));
	ZeroMemory(packet, sizeof(packet));
	packet[0] = sizeof(packet);
	packet[2] = 4;
	STOREINTELWORD(packet + 10, 0);
	STOREINTELWORD(packet + 12, 0x0200);
	STOREINTELWORD(packet + 14, 1);
	STOREINTELWORD(packet + 16, 3);
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
	iocoreva_bind();
	hostfat_send_string_generic("check_hostfat");
	if ((iocore_inp8(0x07ef) != 'H') ||
		(iocore_inp8(0x07ef) != '1')) {
		goto transport_cleanup;
	}
	np2sysp_reset();
	hostfat_send_string("check_hostfat");
	if ((iocoreva_inp8(0x07ef) != 'H') ||
		(iocoreva_inp8(0x07ef) != '1')) {
		goto transport_cleanup;
	}
	hostfat_send_far_pointer(request_pointer);
	hostfat_send_string("read_hostfat1");
	result = iocoreva_inp8(0x07ed);
	if (result != HOSTFAT_RESULT_OK) {
		goto transport_cleanup;
	}
	for (index=0; index<HOSTFAT_SECTOR_SIZE; index++) {
		if (i286_memoryread(destination_address + index) !=
				(BYTE)(index ^ 0x5a)) {
			goto transport_cleanup;
		}
	}
	STOREINTELWORD(packet + 16, HOSTFAT_TOTAL_SECTORS);
	MEML_WRITE(packet_address, packet, sizeof(packet));
	MEML_WRITE(destination_address, unchanged, sizeof(unchanged));
	hostfat_send_far_pointer(request_pointer);
	hostfat_send_string("read_hostfat1");
	result = iocoreva_inp8(0x07ed);
	if (result != HOSTFAT_RESULT_RANGE) {
		goto transport_cleanup;
	}
	for (index=0; index<HOSTFAT_SECTOR_SIZE; index++) {
		if (i286_memoryread(destination_address + index) != 0xa5) {
			goto transport_cleanup;
		}
	}
	packet[0] = sizeof(packet) - 1;
	STOREINTELWORD(packet + 16, 3);
	MEML_WRITE(packet_address, packet, sizeof(packet));
	hostfat_send_far_pointer(request_pointer);
	hostfat_send_string("read_hostfat1");
	if (iocoreva_inp8(0x07ed) != HOSTFAT_RESULT_BAD_REQUEST) {
		goto transport_cleanup;
	}
	packet[0] = sizeof(packet);
	STOREINTELWORD(packet + 14, 129);
	MEML_WRITE(packet_address, packet, sizeof(packet));
	hostfat_send_far_pointer(request_pointer);
	hostfat_send_string("read_hostfat1");
	if (iocoreva_inp8(0x07ed) != HOSTFAT_RESULT_RANGE) {
		goto transport_cleanup;
	}
	STOREINTELWORD(packet + 14, 1);
	STOREINTELWORD(packet + 10, 0);
	STOREINTELWORD(packet + 12, 0x9ff0);
	MEML_WRITE(packet_address, packet, sizeof(packet));
	hostfat_send_far_pointer(request_pointer);
	hostfat_send_string("read_hostfat1");
	if (iocoreva_inp8(0x07ed) != HOSTFAT_RESULT_RANGE) {
		goto transport_cleanup;
	}
	for (index=0; index<HOSTFAT_SECTOR_SIZE; index++) {
		if (i286_memoryread(destination_address + index) != 0xa5) {
			goto transport_cleanup;
		}
	}
	np2sysp_reset();
	if (!hostfat_is_mounted()) {
		goto transport_cleanup;
	}
	hostfat_unmount();
	hostfat_send_string("check_hostfat");
	if (iocoreva_inp8(0x07ef) != 0) {
		goto transport_cleanup;
	}
	hostfat_send_far_pointer(request_pointer);
	hostfat_send_string("read_hostfat1");
	if (iocoreva_inp8(0x07ed) != HOSTFAT_RESULT_NOT_MOUNTED) {
		goto transport_cleanup;
	}
	status = SUCCESS;

transport_cleanup:
	iocore_destroy();
	CPU_REMCLOCK = saved_remclock;
	hostfat_unmount();
	MEML_WRITE(packet_address, saved_packet, sizeof(saved_packet));
	MEML_WRITE(destination_address, saved_destination,
									sizeof(saved_destination));
	if (status != SUCCESS) {
		return(fail("HOSTFAT transport",
				"VA 07EDH/07EFH read or atomic range rejection failed"));
	}
	fprintf(stderr, "selftest: HOSTFAT transport ok\n");
	return(SUCCESS);
}

static int test_hostfat_snapshot(void) {

	if (hostfat_snapshot_selftest() != SUCCESS) {
		return(fail("HOSTFAT snapshot",
				"FAT generation, determinism, or rejection policy failed"));
	}
	fprintf(stderr, "selftest: HOSTFAT snapshot ok\n");
	return(SUCCESS);
}

static int test_va_tvram_window(void) {

	_MEMORYVA	saved_memoryva;
	UINT8		saved_model_va;
	BYTE		saved_bytes[4];
	BOOL		saved_dirty;
	int		result;

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

	i286_memorywrite_va_w(0x0afffe, 0x1234);
	if (i286_memoryread_va_w(0x0afffe) != 0x1234) {
		result = fail("VA TVRAM", "valid word access failed");
	}
	i286_memorywrite_va_w(0x0affff, 0x5678);
	if ((i286_memoryread_va_w(0x0affff) != 0xff78) ||
		(textmem[0x10000] != 0x5a)) {
		result = fail("VA TVRAM", "AFFFF boundary crossed into unused memory");
	}
	i286_memorywrite_va(0x0b0000, 0x6c);
	if ((i286_memoryread_va(0x0b0000) != 0xff) ||
		(textmem[0x10000] != 0x5a)) {
		result = fail("VA TVRAM", "unused byte access did not use open bus");
	}
	i286_memorywrite_va_w(0x0bfff0, 0xabcd);
	if ((i286_memoryread_va_w(0x0bfff0) != 0xffff) ||
		(textmem[0x1fff0] != 0xa5)) {
		result = fail("VA TVRAM", "B0000-DFFFF is not open bus");
	}

	pccore.model_va = PCMODEL_VA2;
	i286_memorywrite_va(0x0b0000, 0x6c);
	if ((i286_memoryread_va(0x0b0000) != 0x6c) ||
		(textmem[0x10000] != 0x6c)) {
		result = fail("VA2 TVRAM", "legacy B0000 byte access was blocked");
	}
	i286_memorywrite_va_w(0x0bfff0, 0xabcd);
	if ((i286_memoryread_va_w(0x0bfff0) != 0xabcd) ||
		(textmem[0x1fff0] != 0xcd)) {
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
	return(result);
}

static int test_va_bms_window(void) {

	_BMSIOCFG	saved_bmsiocfg;
	_BMSIO		saved_bmsio;
	_BMSIOWORK	saved_bmsiowork;
	BYTE		*retained_mem;
	int		result;

	saved_bmsiocfg = bmsiocfg;
	saved_bmsio = bmsio;
	saved_bmsiowork = bmsiowork;
	ZeroMemory(&bmsio, sizeof(bmsio));
	ZeroMemory(&bmsiowork, sizeof(bmsiowork));
	result = SUCCESS;

	bmsiocfg.enabled = FALSE;
	if ((BMSIO_PORT_DEFAULT != 0x01d0) || (BMSIO_PORT_COMPAT != 0x00ec)) {
		result = fail("VA BMS", "unexpected default or compatibility port");
		goto bms_test_cleanup;
	}
	bmsiocfg.port = BMSIO_PORT_DEFAULT;
	bmsiocfg.portmask = BMSIO_PORT_MASK;
	bmsiocfg.numbanks = BMSIO_DEFAULT_BANKS;
	bmsio_set();
	bmsio_reset();
	i286_memorywrite_va(0x080000, 0x12);
	i286_memorywrite_va_w(0x09fffe, 0x3456);
	if ((i286_memoryread_va(0x080000) != 0x12) ||
		(i286_memoryread_va_w(0x09fffe) != 0x3456) ||
		(bmsiowork.bmsmem != NULL) || (bmsiowork.bmsmemsize != 0) ||
		(bmsio.cfg.port != BMSIO_PORT_DEFAULT) || (bmsio.nomem != 0)) {
		result = fail("VA BMS", "bank zero did not pass through main RAM");
	}

	bmsiocfg.enabled = TRUE;
	bmsiocfg.port = BMSIO_PORT_COMPAT;
	bmsiocfg.numbanks = 2;
	bmsio_set();
	bmsio_reset();
	if ((bmsiowork.bmsmem == NULL) ||
		(bmsiowork.bmsmemsize != 0x40000) ||
		(bmsio.cfg.enabled == FALSE) ||
		(bmsio.cfg.port != BMSIO_PORT_COMPAT) ||
		(bmsio.cfg.numbanks != 2) || (bmsio.bank != 0) ||
		(bmsio.nomem != 0)) {
		result = fail("VA BMS", "enabled configuration was not applied");
		goto bms_test_cleanup;
	}
	ZeroMemory(bmsiowork.bmsmem, bmsiowork.bmsmemsize);
	if ((i286_memoryread_va(0x080000) != 0x12) ||
		(i286_memoryread_va_w(0x09fffe) != 0x3456)) {
		result = fail("VA BMS", "enabled bank zero did not preserve main RAM");
	}

	retained_mem = bmsiowork.bmsmem;
	bmsio_set();
	bmsio_reset();
	if ((bmsiowork.bmsmem != retained_mem) ||
		(i286_memoryread_va(0x080000) != 0x12) ||
		(i286_memoryread_va_w(0x09fffe) != 0x3456)) {
		result = fail("VA BMS", "ordinary reset did not retain BMS contents");
	}

	bmsio.bank = 1;
	bmsio.nomem = 0;
	i286_memorywrite_va(0x080000, 0x78);
	i286_memorywrite_va_w(0x09fffe, 0x9abc);
	if ((i286_memoryread_va(0x080000) != 0x78) ||
		(i286_memoryread_va_w(0x09fffe) != 0x9abc)) {
		result = fail("VA BMS", "bank 1 access failed");
	}
	bmsio.bank = 0;
	if ((i286_memoryread_va(0x080000) != 0x12) ||
		(i286_memoryread_va_w(0x09fffe) != 0x3456)) {
		result = fail("VA BMS", "bank switch did not preserve bank 0");
	}

	bmsiocfg.enabled = FALSE;
	bmsio_set();
	bmsio_reset();
	if ((bmsiowork.bmsmem != NULL) || (bmsiowork.bmsmemsize != 0) ||
		(bmsio.nomem != 0) ||
		(i286_memoryread_va(0x080000) != 0x12)) {
		result = fail("VA BMS", "disable did not restore main RAM");
	}

bms_test_cleanup:
	if (bmsiowork.bmsmem != NULL) {
		_MFREE(bmsiowork.bmsmem);
	}
	bmsiocfg = saved_bmsiocfg;
	bmsio = saved_bmsio;
	bmsiowork = saved_bmsiowork;
	if (result == SUCCESS) {
		fprintf(stderr, "selftest: VA BMS config/window lifecycle ok\n");
	}
	return(result);
}

typedef struct {
	UINT	format;
	UINT8	d88_type;
	UINT8	cylinders;
	UINT8	heads;
	UINT8	sectors;
	UINT8	sector_n;
	UINT16	sector_size;
	UINT8	sectors_per_cluster;
	UINT8	media;
	UINT16	root_entries;
	UINT16	fat_sectors;
} SELFTESTFDDGEOMETRY;

static const SELFTESTFDDGEOMETRY selftest_fdd_geometry[] = {
	{NEWDISK_FDD_MSDOS_2HD, 0x20, 77, 2, 8, 3, 1024, 1, 0xfe, 192, 2},
	{NEWDISK_FDD_MSDOS_2DD, 0x10, 80, 2, 8, 2, 512, 2, 0xfb, 112, 2}
};

static int test_new_fdd_image(void) {

	const SELFTESTFDDGEOMETRY	*geometry;
	_D88HEAD					header;
	_D88SEC					sector_header;
	_FDDFILE					parsed;
	BYTE						sector[1024];
	char						path[MAX_PATH];
	FILEH						fh;
	UINT32						data_offset;
	UINT32						expected_size;
	UINT32						fat_sector;
	UINT32						total_sectors;
	UINT						index;
	UINT						tracks;
	int						result;

	result = SUCCESS;
	for (index=0; index<NELEMENTS(selftest_fdd_geometry); index++) {
		geometry = selftest_fdd_geometry + index;
		SPRINTF(path, "vaeg-selftest-%lu-fdd-%u.d88",
				(unsigned long)getpid(), geometry->format);
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
		total_sectors = geometry->cylinders * geometry->heads *
											geometry->sectors;
		tracks = geometry->cylinders * geometry->heads;
		expected_size = sizeof(header) + total_sectors *
							(sizeof(sector_header) + geometry->sector_size);
		if ((file_getsize(fh) != expected_size) ||
			(file_read(fh, &header, sizeof(header)) != sizeof(header)) ||
			(header.fd_type != geometry->d88_type) ||
			(LOADINTELDWORD(header.fd_size) != expected_size) ||
			(LOADINTELDWORD(header.trackp[0]) != sizeof(header)) ||
			(LOADINTELDWORD(header.trackp[tracks - 1]) !=
			 sizeof(header) + (tracks - 1) * geometry->sectors *
						(sizeof(sector_header) + geometry->sector_size)) ||
			((tracks < NELEMENTS(header.trackp)) &&
			 (LOADINTELDWORD(header.trackp[tracks]) != 0))) {
			result = fail("new-fdd", "D88 header or track table is invalid");
			file_close(fh);
			file_delete(path);
			break;
		}
		if ((file_read(fh, &sector_header, sizeof(sector_header)) !=
			 sizeof(sector_header)) ||
			(file_read(fh, sector, geometry->sector_size) !=
			 geometry->sector_size) ||
			(sector_header.c != 0) || (sector_header.h != 0) ||
			(sector_header.r != 1) ||
			(sector_header.n != geometry->sector_n) ||
			(LOADINTELWORD(sector_header.sectors) != geometry->sectors) ||
			(LOADINTELWORD(sector_header.size) != geometry->sector_size) ||
			(LOADINTELWORD(sector + 11) != geometry->sector_size) ||
			(sector[13] != geometry->sectors_per_cluster) ||
			(LOADINTELWORD(sector + 14) != 1) || (sector[16] != 2) ||
			(LOADINTELWORD(sector + 17) != geometry->root_entries) ||
			(LOADINTELWORD(sector + 19) != total_sectors) ||
			(sector[21] != geometry->media) ||
			(LOADINTELWORD(sector + 22) != geometry->fat_sectors) ||
			(LOADINTELWORD(sector + 24) != geometry->sectors) ||
			(LOADINTELWORD(sector + 26) != geometry->heads) ||
			(sector[510] != 0x55) || (sector[511] != 0xaa)) {
			result = fail("new-fdd", "D88 sector or FAT12 BPB is invalid");
			file_close(fh);
			file_delete(path);
			break;
		}
		fat_sector = 1 + geometry->fat_sectors;
		data_offset = sizeof(header) + fat_sector *
							(sizeof(sector_header) + geometry->sector_size);
		if ((file_seek(fh, data_offset + sizeof(sector_header),
										SEEK_SET) != (long)(data_offset +
										sizeof(sector_header))) ||
			(file_read(fh, sector, 3) != 3) ||
			(sector[0] != geometry->media) ||
			(sector[1] != 0xff) || (sector[2] != 0xff)) {
			result = fail("new-fdd", "second FAT was not initialized");
			file_close(fh);
			file_delete(path);
			break;
		}
		file_close(fh);
		file_delete(path);

		SPRINTF(path, "vaeg-selftest-%lu-fdd-%u.img",
				(unsigned long)getpid(), geometry->format);
		file_delete(path);
		if (newdisk_fdd_msdos_ex(path, geometry->format,
							NEWDISK_FDD_CONTAINER_RAW) != SUCCESS) {
			result = fail("new-fdd", "formatted raw creation failed");
			break;
		}
		if (newdisk_fdd_msdos_ex(path, geometry->format,
							NEWDISK_FDD_CONTAINER_RAW) != FAILURE) {
			result = fail("new-fdd", "existing raw image was overwritten");
			file_delete(path);
			break;
		}
		ZeroMemory(&parsed, sizeof(parsed));
		if ((fddxdf_set(&parsed, path, 0) != SUCCESS) ||
			(parsed.inf.xdf.headersize != 0) ||
			(parsed.inf.xdf.tracks != tracks) ||
			(parsed.inf.xdf.sectors != geometry->sectors) ||
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
			(file_read(fh, sector, geometry->sector_size) !=
			 geometry->sector_size) ||
			(LOADINTELWORD(sector + 11) != geometry->sector_size) ||
			(sector[13] != geometry->sectors_per_cluster) ||
			(LOADINTELWORD(sector + 17) != geometry->root_entries) ||
			(LOADINTELWORD(sector + 19) != total_sectors) ||
			(sector[21] != geometry->media) ||
			(LOADINTELWORD(sector + 22) != geometry->fat_sectors) ||
			(LOADINTELWORD(sector + 24) != geometry->sectors) ||
			(LOADINTELWORD(sector + 26) != geometry->heads) ||
			(sector[510] != 0x55) || (sector[511] != 0xaa)) {
			result = fail("new-fdd", "raw FAT12 BPB is invalid");
			file_close(fh);
			file_delete(path);
			break;
		}
		data_offset = fat_sector * geometry->sector_size;
		if ((file_seek(fh, data_offset, SEEK_SET) != (long)data_offset) ||
			(file_read(fh, sector, 3) != 3) ||
			(sector[0] != geometry->media) ||
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
	return(result);
}

static int test_sasi_image_validation(void) {

	char valid_path[MAX_PATH];
	char invalid_path[MAX_PATH];
	FILEH fh;
	HDIHDR valid_header;
	int result;

	SPRINTF(valid_path, "vaeg-selftest-%lu-sasi.hdi", (unsigned long)getpid());
	SPRINTF(invalid_path, "vaeg-selftest-%lu-invalid.hdi",
												(unsigned long)getpid());
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
		(file_read(fh, &valid_header, sizeof(valid_header)) !=
											sizeof(valid_header))) {
		if (fh != FILEH_INVALID) {
			file_close(fh);
		}
		result = fail("SASI image", "valid HDI header could not be read");
		goto done;
	}
	file_close(fh);
	fh = file_create(invalid_path);
	if ((fh == FILEH_INVALID) ||
		(file_write(fh, &valid_header, sizeof(valid_header)) !=
											sizeof(valid_header))) {
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

done:
	file_delete(valid_path);
	file_delete(invalid_path);
	if (result == SUCCESS) {
		fprintf(stderr, "selftest: SASI image validation ok\n");
	}
	return(result);
}

static int test_profile_ini(void) {

	char	path[MAX_PATH];
	char	name[32];
	char	read_name[32];
	UINT8	flag;
	UINT8	read_flag;
	UINT16	count;
	UINT16	read_count;
	UINT8	bytes[3];
	UINT8	read_bytes[3];
	UINT8	effect;
	UINT8	read_effect;
	UINT16	window_width;
	UINT16	read_window_width;
	UINT16	pacing_ms;
	UINT16	read_pacing_ms;
	_BMSIOCFG	write_bms;
	_BMSIOCFG	read_bms;
	PFTBL	write_tbl[] = {
		{"name", PFTYPE_STR, name, sizeof(name)},
		{"flag", PFTYPE_BOOL, &flag, 0},
		{"count", PFTYPE_UINT16, &count, 0},
		{"bytes", PFTYPE_BIN, bytes, sizeof(bytes)},
		{"effect", PFTYPE_UINT8, &effect, 0},
		{"win_width", PFTYPE_UINT16, &window_width, 0}
	};
	PFTBL	read_tbl[] = {
		{"name", PFTYPE_STR, read_name, sizeof(read_name)},
		{"flag", PFTYPE_BOOL, &read_flag, 0},
		{"count", PFTYPE_UINT16, &read_count, 0},
		{"bytes", PFTYPE_BIN, read_bytes, sizeof(read_bytes)},
		{"effect", PFTYPE_UINT8, &read_effect, 0},
		{"win_width", PFTYPE_UINT16, &read_window_width, 0}
	};
	INITBL	write_bms_tbl[] = {
		{"Use_BMS_", INITYPE_BOOL, &write_bms.enabled, 0},
		{"BMS_Port", INITYPE_HEX16, &write_bms.port, 0},
		{"BMS_Size", INITYPE_UINT8, &write_bms.numbanks, 0},
		{"PacingMs", INITYPE_UINT16, &pacing_ms, 0}
	};
	INITBL	read_bms_tbl[] = {
		{"Use_BMS_", INITYPE_BOOL, &read_bms.enabled, 0},
		{"BMS_Port", INITYPE_HEX16, &read_bms.port, 0},
		{"BMS_Size", INITYPE_UINT8, &read_bms.numbanks, 0},
		{"PacingMs", INITYPE_UINT16, &read_pacing_ms, 0}
	};

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
	read_flag = 0;
	read_count = 0;
	ZeroMemory(read_bytes, sizeof(read_bytes));
	read_effect = 0;
	read_window_width = 0;
	read_pacing_ms = 0;
	write_bms.enabled = TRUE;
	write_bms.port = BMSIO_PORT_COMPAT;
	write_bms.portmask = BMSIO_PORT_MASK;
	write_bms.numbanks = 32;
	ZeroMemory(&read_bms, sizeof(read_bms));

	profile_iniwrite(path, "selftest", write_tbl, NELEMENTS(write_tbl),
																	NULL);
	profile_iniread(path, "selftest", read_tbl, NELEMENTS(read_tbl),
																	NULL);
	ini_write(path, "selftest-bms", write_bms_tbl,
										NELEMENTS(write_bms_tbl));
	ini_read(path, "selftest-bms", read_bms_tbl,
										NELEMENTS(read_bms_tbl));
	file_delete(path);

	if (strcmp(read_name, name) != 0) {
		return(fail("ini", "string value did not round-trip"));
	}
	if ((read_flag != flag) || (read_count != count) ||
		(memcmp(read_bytes, bytes, sizeof(bytes)) != 0) ||
		(read_effect != effect) || (read_window_width != window_width)) {
		return(fail("ini", "typed values did not round-trip"));
	}
	if ((read_bms.enabled != TRUE) ||
		(read_bms.port != BMSIO_PORT_COMPAT) ||
		(read_bms.numbanks != 32) || (read_pacing_ms != pacing_ms)) {
		return(fail("ini", "BMS settings did not round-trip"));
	}
	fprintf(stderr, "selftest: ini ok\n");
	return(SUCCESS);
}

static int test_framedisp(void) {

	VAEG_FRAMEDISP state;

	vaeg_framedisp_reset(&state, 1000, 100);
	if (vaeg_framedisp_update(&state, 2999, 220) != FALSE) {
		return(fail("framedisp", "measurement renewed before two seconds"));
	}
	if ((vaeg_framedisp_update(&state, 3000, 220) != TRUE) ||
		(state.fps_tenths != 600)) {
		return(fail("framedisp", "60.0 FPS measurement is incorrect"));
	}
	if (vaeg_framedisp_update(&state, 4000, 280) != FALSE) {
		return(fail("framedisp", "measurement interval was not restarted"));
	}
	if ((vaeg_framedisp_update(&state, 5000, 280) != TRUE) ||
		(state.fps_tenths != 300)) {
		return(fail("framedisp", "30.0 FPS measurement is incorrect"));
	}
	vaeg_framedisp_reset(&state, 0, 0);
	if ((vaeg_framedisp_update(&state, 2000, 0) != TRUE) ||
		(state.fps_tenths != 0)) {
		return(fail("framedisp", "zero-draw interval is incorrect"));
	}
	vaeg_framedisp_reset(&state, 0xffffffffU - 1000U,
									0xffffffffU - 3U);
	if ((vaeg_framedisp_update(&state, 999, 116) != TRUE) ||
		(state.fps_tenths != 600)) {
		return(fail("framedisp", "counter wrap handling is incorrect"));
	}
	if (vaeg_framedisp_update(NULL, 0, 0) != FALSE) {
		return(fail("framedisp", "NULL state was accepted"));
	}
	vaeg_framedisp_reset(NULL, 0, 0);
	fprintf(stderr, "selftest: frame display ok\n");
	return(SUCCESS);
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
		return(fail("clockscale", "invalid ratio was accepted"));
	}
	if (clockscale_configure(&scale, 2, 3) != SUCCESS) {
		return(fail("clockscale", "x3 CPU ratio was rejected"));
	}
	total = 0;
	for (index=0; index<30000; index++) {
		total += clockscale_apply(&scale, 1);
	}
	if ((total != 20000) || (scale.remainder != 0)) {
		return(fail("clockscale", "fractional carry drifted"));
	}
	clockscale_configure(&scale, 2, 32);
	if ((clockscale_apply(&scale, 15) != 0) ||
		(clockscale_apply(&scale, 1) != 1)) {
		return(fail("clockscale", "small CPU slices lost their remainder"));
	}
	clockscale_configure(&scale, 32, 1);
	if (clockscale_apply(&scale, 0xffffffffU) !=
											(UINT64)0xffffffffU * 32) {
		return(fail("clockscale", "wide multiplication overflowed"));
	}
	clockscale_configure(&scale, 2, 3);
	(void)clockscale_apply(&scale, 1);
	clockscale_reset(&scale);
	if (scale.remainder != 0) {
		return(fail("clockscale", "reset retained a remainder"));
	}
	if (!pccore_cpu_multiple_valid(1) ||
		!pccore_cpu_multiple_valid(2) ||
		!pccore_cpu_multiple_valid(3) ||
		!pccore_cpu_multiple_valid(4) ||
		!pccore_cpu_multiple_valid(8) ||
		!pccore_cpu_multiple_valid(32) ||
		pccore_cpu_multiple_valid(0) || pccore_cpu_multiple_valid(33)) {
		return(fail("clockscale", "CPU multiplier validation failed"));
	}
	saved_config_multiple = np2cfg.multiple;
	saved_baseclock = pccore.baseclock;
	pccore.baseclock = PCBASECLOCK40;
	for (index=0; index<NELEMENTS(cpu_multipliers); index++) {
		np2cfg.multiple = cpu_multipliers[index];
		pccore_clockrestore();
		if ((pccore.multiple != PCCORE_STANDARD_MULTIPLE) ||
			(pccore.realclock != PCBASECLOCK40 * PCCORE_STANDARD_MULTIPLE) ||
			(pccore_cpu_multiple() != cpu_multipliers[index]) ||
			(pccore_cpu_clock() != PCBASECLOCK40 * cpu_multipliers[index])) {
			np2cfg.multiple = saved_config_multiple;
			pccore.baseclock = saved_baseclock;
			pccore_clockrestore();
			return(fail("clockscale", "CPU scaling changed machine time"));
		}
	}
	np2cfg.multiple = saved_config_multiple;
	pccore.baseclock = saved_baseclock;
	pccore_clockrestore();
	fprintf(stderr, "selftest: clockscale ok\n");
	return(SUCCESS);
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

	if (!sgp_speed_mode_valid(SGP_SPEED_MODEL_DEFAULT) ||
		!sgp_speed_mode_valid(SGP_SPEED_FOLLOW_CPU) ||
		!sgp_speed_mode_valid(SGP_SPEED_CUSTOM) ||
		sgp_speed_mode_valid(SGP_SPEED_MODE_COUNT) ||
		!sgp_speed_multiplier_valid(1) ||
		!sgp_speed_multiplier_valid(2) ||
		!sgp_speed_multiplier_valid(3) ||
		!sgp_speed_multiplier_valid(16) ||
		sgp_speed_multiplier_valid(0) || sgp_speed_multiplier_valid(17)) {
		return(fail("sgp-speed", "SGP setting validation failed"));
	}
	if ((sgp_speed_ratio(SGP_SPEED_MODEL_DEFAULT, 1, 32,
							&numerator, &denominator) != SUCCESS) ||
		(numerator != 1) || (denominator != 1)) {
		return(fail("sgp-speed", "Model default ratio changed"));
	}
	if ((sgp_speed_ratio(SGP_SPEED_FOLLOW_CPU, 1, 1,
							&numerator, &denominator) != SUCCESS) ||
		(numerator != 1) || (denominator != 2)) {
		return(fail("sgp-speed", "Follow CPU x1 ratio failed"));
	}
	if ((sgp_speed_ratio(SGP_SPEED_FOLLOW_CPU, 1, 4,
							&numerator, &denominator) != SUCCESS) ||
		(numerator != 4) || (denominator != 2)) {
		return(fail("sgp-speed", "Follow CPU x4 ratio failed"));
	}
	if ((sgp_speed_ratio(SGP_SPEED_CUSTOM, 3, 2,
							&numerator, &denominator) != SUCCESS) ||
		(numerator != 3) || (denominator != 1) ||
		(sgp_speed_ratio(SGP_SPEED_CUSTOM, 0, 2,
							&numerator, &denominator) != FAILURE)) {
		return(fail("sgp-speed", "Custom ratio failed"));
	}
	if ((sgp_model_clock(PCMODEL_VA1) != PCBASECLOCK40) ||
		(sgp_model_clock(PCMODEL_VA2) != (PCBASECLOCK40 * 2))) {
		return(fail("sgp-speed", "Model clock selection failed"));
	}

	saved_model_va = pccore.model_va;
	saved_sgp_speed_mode = np2cfg.sgp_speed_mode;
	saved_sgp_multiplier = np2cfg.sgp_multiplier;
	saved_config_multiple = np2cfg.multiple;
	pccore.model_va = PCMODEL_VA1;
	np2cfg.sgp_speed_mode = SGP_SPEED_MODEL_DEFAULT;
	np2cfg.sgp_multiplier = 1;
	np2cfg.multiple = PCCORE_STANDARD_MULTIPLE;
	pccore_clockrestore();
	sgp_configure_speed();
	if (sgp_scale_elapsed(20000) != 20000) {
		return(fail("sgp-speed", "VA Model default timing changed"));
	}
	pccore.model_va = PCMODEL_VA2;
	sgp_configure_speed();
	if (sgp_scale_elapsed(20000) != 40000) {
		return(fail("sgp-speed", "VA2 Model default timing failed"));
	}

	np2cfg.sgp_speed_mode = SGP_SPEED_FOLLOW_CPU;
	np2cfg.sgp_multiplier = 1;
	np2cfg.multiple = 3;
	pccore_clockrestore();
	sgp_configure_speed();
	total = 0;
	for (index=0; index<20000; index++) {
		total += sgp_scale_elapsed(1);
	}
	if (total != 60000) {
		return(fail("sgp-speed", "Follow CPU fractional timing drifted"));
	}
	pccore.model_va = saved_model_va;
	np2cfg.sgp_speed_mode = saved_sgp_speed_mode;
	np2cfg.sgp_multiplier = saved_sgp_multiplier;
	np2cfg.multiple = saved_config_multiple;
	pccore_clockrestore();
	sgp_configure_speed();
	fprintf(stderr, "selftest: SGP speed ok\n");
	return(SUCCESS);
}

static int test_pacing(void) {

	VAEG_PACING_STATE state;
	BOOL configured_nowait;
	UINT configured_drawskip;

	configured_nowait = FALSE;
	configured_drawskip = 3;
	vaeg_pacing_reset(&state);
	if (state.fast_forward_held ||
		vaeg_pacing_effective_nowait(&state, configured_nowait) ||
		(vaeg_pacing_effective_drawskip(&state, configured_drawskip) != 3)) {
		return(fail("pacing", "reset state changed configured pacing"));
	}
	if (!vaeg_pacing_key(&state, SDL_SCANCODE_F11, TRUE, FALSE) ||
		!state.fast_forward_held ||
		!vaeg_pacing_effective_nowait(&state, configured_nowait) ||
		(vaeg_pacing_effective_drawskip(&state, configured_drawskip) != 16)) {
		return(fail("pacing", "F11 keydown did not enable fast-forward"));
	}
	if (!vaeg_pacing_key(&state, SDL_SCANCODE_F11, TRUE, TRUE) ||
		!state.fast_forward_held) {
		return(fail("pacing", "F11 repeat damaged held state"));
	}
	if (!vaeg_pacing_key(&state, SDL_SCANCODE_F11, FALSE, FALSE) ||
		state.fast_forward_held) {
		return(fail("pacing", "F11 keyup did not disable fast-forward"));
	}
	if (vaeg_pacing_key(&state, SDL_SCANCODE_RALT, TRUE, FALSE)) {
		return(fail("pacing", "Right Alt was consumed as a shortcut"));
	}
	(void)vaeg_pacing_key(&state, SDL_SCANCODE_F11, TRUE, FALSE);
	vaeg_pacing_reset(&state);
	if (state.fast_forward_held || configured_nowait ||
		(configured_drawskip != 3)) {
		return(fail("pacing", "focus/reset cleanup changed saved settings"));
	}
	fprintf(stderr, "selftest: pacing ok\n");
	return(SUCCESS);
}

static int expect_viewport(int drawable_width, int drawable_height,
						int menu_inset, int scaling, BOOL aspect,
						int x, int y, int width, int height) {

	VAEG_VIEWPORT_INPUT input;
	VAEG_VIEWPORT viewport;

	input.guest_width = 640;
	input.guest_height = 400;
	input.drawable_width = drawable_width;
	input.drawable_height = drawable_height;
	input.menu_inset = menu_inset;
	input.scaling = scaling;
	input.aspect = aspect;
	if ((vaeg_viewport_calculate(&input, &viewport) != SUCCESS) ||
		(viewport.x != x) || (viewport.y != y) ||
		(viewport.width != width) || (viewport.height != height)) {
		fprintf(stderr,
			"selftest: viewport expected=%d,%d %dx%d actual=%d,%d %dx%d "
			"drawable=%dx%d menu=%d mode=%d aspect=%d\n",
			x, y, width, height, viewport.x, viewport.y,
			viewport.width, viewport.height, drawable_width, drawable_height,
			menu_inset, scaling, aspect);
		return(FAILURE);
	}
	return(SUCCESS);
}

static int test_viewport(void) {

	VAEG_VIEWPORT_INPUT input;
	VAEG_VIEWPORT viewport;
	BOOL masked;
	int guest_x;
	int guest_y;
	int width;
	int height;
	UINT value;

	if ((expect_viewport(640, 400, 0, VAEG_SCALING_FIT, FALSE,
								0, 0, 640, 400) != SUCCESS) ||
		(expect_viewport(800, 600, 0, VAEG_SCALING_FIT, FALSE,
								0, 50, 800, 500) != SUCCESS) ||
		(expect_viewport(1024, 768, 0, VAEG_SCALING_FIT, FALSE,
								0, 64, 1024, 640) != SUCCESS) ||
		(expect_viewport(1280, 720, 0, VAEG_SCALING_FIT, FALSE,
								64, 0, 1152, 720) != SUCCESS) ||
		(expect_viewport(1920, 1080, 0, VAEG_SCALING_FIT, FALSE,
								96, 0, 1728, 1080) != SUCCESS)) {
		return(fail("viewport", "Fit geometry failed"));
	}
	if ((expect_viewport(800, 600, 0, VAEG_SCALING_NATIVE, FALSE,
								80, 100, 640, 400) != SUCCESS) ||
		(expect_viewport(1280, 900, 0, VAEG_SCALING_INTEGER, FALSE,
								0, 50, 1280, 800) != SUCCESS) ||
		(expect_viewport(1003, 700, 0, VAEG_SCALING_FIT_8DOT, FALSE,
								1, 37, 1000, 625) != SUCCESS) ||
		(expect_viewport(800, 600, 0, VAEG_SCALING_STRETCH, FALSE,
								0, 0, 800, 600) != SUCCESS) ||
		(expect_viewport(640, 422, 22, VAEG_SCALING_FIT, FALSE,
								0, 22, 640, 400) != SUCCESS) ||
		(expect_viewport(1280, 844, 44, VAEG_SCALING_FIT, FALSE,
								0, 44, 1280, 800) != SUCCESS)) {
		return(fail("viewport", "mode or inset geometry failed"));
	}
	input.guest_width = 640;
	input.guest_height = 400;
	input.drawable_width = 800;
	input.drawable_height = 600;
	input.menu_inset = 0;
	input.scaling = VAEG_SCALING_FIT;
	input.aspect = FALSE;
	if ((vaeg_viewport_calculate(&input, &viewport) != SUCCESS) ||
		(vaeg_viewport_map_point(&viewport, 640, 400, 400, 300,
										&guest_x, &guest_y) != SUCCESS) ||
		(guest_x != 320) || (guest_y != 200) ||
		(vaeg_viewport_map_point(&viewport, 640, 400, 400, 25,
										&guest_x, &guest_y) != FAILURE)) {
		return(fail("viewport", "inverse coordinate transform failed"));
	}
	input.drawable_width = 0;
	if (vaeg_viewport_calculate(&input, &viewport) != FAILURE) {
		return(fail("viewport", "zero drawable size was accepted"));
	}
	for (value=0; value<=7; value++) {
		if ((vaeg_fscrnmod_sanitize(value, &masked) != value) || masked) {
			return(fail("viewport", "valid fscrnmod changed"));
		}
	}
	if ((vaeg_fscrnmod_sanitize(0x87, &masked) != 7) || !masked) {
		return(fail("viewport", "fscrnmod upper bits were not masked"));
	}
	vaeg_fullscreen_size(0, 0, 0, 1920, 1080, &width, &height);
	if ((width != 640) || (height != 400)) {
		return(fail("viewport", "legacy fullscreen fallback failed"));
	}
	vaeg_fullscreen_size(1280, 720, 4, 1920, 1080, &width, &height);
	if ((width != 1920) || (height != 1080)) {
		return(fail("viewport", "current display fallback failed"));
	}
	scrnmng_initialize();
	for (value=0; value<VAEG_EFFECT_COUNT; value++) {
		scrnmng_set_effect((int)value);
		if (scrnmng_get_effect() != (int)value) {
			return(fail("viewport", "effect selection failed"));
		}
	}
	scrnmng_set_effect(VAEG_EFFECT_COUNT);
	if (scrnmng_get_effect() != VAEG_EFFECT_UNFILTERED) {
		return(fail("viewport", "invalid effect did not fall back"));
	}
	for (value=0; value<VAEG_SCALING_COUNT; value++) {
		scrnmng_set_scaling((int)value);
		if (scrnmng_get_scaling() != (int)value) {
			return(fail("viewport", "scaling selection failed"));
		}
	}
	scrnmng_set_scaling(VAEG_SCALING_COUNT);
	if (scrnmng_get_scaling() != VAEG_SCALING_FIT) {
		return(fail("viewport", "invalid scaling did not fall back"));
	}
	fprintf(stderr, "selftest: viewport ok\n");
	return(SUCCESS);
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
		return(fail("VA raster", "16bpp converter is unavailable"));
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
		(LOADINTELWORD(output +
			(SCRNMNG_SURFACE_GUARD_LEFT * 2)) != 0x1234) ||
		(LOADINTELWORD(output +
			((SCRNMNG_SURFACE_GUARD_LEFT + SURFACE_WIDTH - 1) * 2)) !=
				0xabcd)) {
		return(fail("VA raster", "guard or visible edge pixels are wrong"));
	}
	fprintf(stderr, "selftest: VA raster guard ok\n");
	return(SUCCESS);
}

static int read_whole_file(const char *path, BYTE **out, UINT *out_size) {

	FILEH	fh;
	UINT	size;
	BYTE	*buf;
	int		ret;

	*out = NULL;
	*out_size = 0;
	fh = file_open_rb(path);
	if (fh == FILEH_INVALID) {
		return(FAILURE);
	}
	size = file_getsize(fh);
	buf = (BYTE *)_MALLOC(size, "selftest-file");
	if (buf == NULL) {
		file_close(fh);
		return(FAILURE);
	}
	ret = SUCCESS;
	if (file_read(fh, buf, size) != size) {
		ret = FAILURE;
	}
	file_close(fh);
	if (ret != SUCCESS) {
		_MFREE(buf);
		return(FAILURE);
	}
	*out = buf;
	*out_size = size;
	return(SUCCESS);
}

static BOOL statsave_section_body_stable(const BYTE *hdr) {

	if (!memcmp(hdr, "CGWINDOW", 8) || !memcmp(hdr, "TEXTRAM", 7)) {
		return(FALSE);
	}
	return(TRUE);
}

static int make_statsave_with_unsupported_subcpu(const char *source,
											const char *destination) {

	BYTE	*data;
	UINT	size;
	UINT	pos;
	BOOL	found;
	FILEH	fh;
	int		ret;

	data = NULL;
	if (read_whole_file(source, &data, &size) != SUCCESS) {
		return(FAILURE);
	}
	found = FALSE;
	pos = 0x30;
	while((pos + 16) <= size) {
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
	if (found && (fh != FILEH_INVALID) &&
		(file_write(fh, data, size) == size)) {
		ret = SUCCESS;
	}
	if (fh != FILEH_INVALID) {
		file_close(fh);
	}
	_MFREE(data);
	return(ret);
}

static int compare_statsave_stable_sections(const char *left,
													const char *right) {

	BYTE	*ldata;
	BYTE	*rdata;
	UINT	lsize;
	UINT	rsize;
	UINT	pos;
	int		ret;

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
		return(FAILURE);
	}
	ret = FAILURE;
	if ((lsize != rsize) || (lsize < 0x30) || memcmp(ldata, rdata, 0x30)) {
		goto cmp_done;
	}
	pos = 0x30;
	while(pos < lsize) {
		UINT	size;
		UINT	padded;
		UINT	body;

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

			for (diff=0; diff<padded; diff++) {
				if (ldata[body + diff] != rdata[body + diff]) {
					fprintf(stderr,
						"selftest: statsave section %.10s differs at %u: %02x/%02x\n",
						ldata + pos, diff, ldata[body + diff],
						rdata[body + diff]);
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
	return(ret);
}

static int test_statsave(void) {

	char	path1[MAX_PATH];
	char	path2[MAX_PATH];
	char	pathbad[MAX_PATH];
	char	err[256];
	int		ret;
#if defined(VAEG_Z80_INTEGRATION_TESTING)
	static const UINT8 f4_program[] = {0xaf, 0x3e, 0x5a, 0xd3, 0xf4, 0x00};
	VAEG_Z80_INTEGRATION_TRACE_STATE z80trace;
#endif

	SPRINTF(path1, "vaeg-selftest-%lu-1.sts", (unsigned long)getpid());
	SPRINTF(path2, "vaeg-selftest-%lu-2.sts", (unsigned long)getpid());
	SPRINTF(pathbad, "vaeg-selftest-%lu-bad.sts", (unsigned long)getpid());
	file_delete(path1);
	file_delete(path2);
	file_delete(pathbad);

	soundmng_initialize();
	commng_initialize();
	pccore_init();
	S98_init();
	pccore_reset();
#if defined(VAEG_UPD9002_M46_TESTING)
	if (upd9002_dispatch_normalization_verify_live() != SUCCESS) {
		S98_trash();
		pccore_term();
		soundmng_deinitialize();
		return fail("dispatch normalization", "initialization/reset changed tables");
	}
#endif

#if defined(VAEG_UPD9002_M44_TESTING)
	if (upd9002_state_scenario_requested()) {
		ret = upd9002_state_scenario_run();
		S98_trash();
		pccore_term();
		soundmng_deinitialize();
		return (ret == SUCCESS) ? SUCCESS :
			fail("statsave-scenario", "scenario operation failed");
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
		(upd9002_harness_run_manifest(VAEG_UPD9002_HARNESS_MANIFEST_PATH)
												!= SUCCESS)) {
		ret = STATFLAG_FAILURE;
	}
	if ((ret == STATFLAG_SUCCESS) &&
		(upd9002_fixture_verify(VAEG_UPD9002_FIXTURE_PATH) != SUCCESS)) {
		ret = STATFLAG_FAILURE;
	}
#endif

#if defined(VAEG_Z80_INTEGRATION_TESTING)
	if (ret == STATFLAG_SUCCESS) {
		subsystem_z80_test_reset();
		subsystem_z80_test_install(0, f4_program, sizeof(f4_program));
		subsystem_z80_test_set_pc(0);
		subsystem_z80_test_set_clock(22);
		subsystem_exec();
		subsystem_z80_test_get_trace(&z80trace);
		if ((z80trace.f4_count != 1) || (z80trace.f4_last_value != 0x5a)) {
			ret = STATFLAG_FAILURE;
		}
	}
#endif

	if (ret == STATFLAG_SUCCESS) {
		ret = statsave_save(path1);
	}
#if defined(VAEG_UPD9002_M44_TESTING)
	if ((ret == STATFLAG_SUCCESS) &&
		(upd9002_statsave_boundary_verify(path1) != SUCCESS)) {
		ret = STATFLAG_FAILURE;
	}
#endif
	if (ret == STATFLAG_SUCCESS) {
		ZeroMemory(err, sizeof(err));
		ret = statsave_check(path1, err, sizeof(err));
	}
	if ((ret == STATFLAG_SUCCESS) &&
		(make_statsave_with_unsupported_subcpu(path1, pathbad) != SUCCESS)) {
		ret = STATFLAG_FAILURE;
	}
	if ((ret == STATFLAG_SUCCESS) &&
		(statsave_load(pathbad) != STATFLAG_FAILURE)) {
		ret = STATFLAG_FAILURE;
	}
	if (ret == STATFLAG_SUCCESS) {
		ret = statsave_load(path1);
	}
#if defined(VAEG_UPD9002_M46_TESTING)
	if ((ret == STATFLAG_SUCCESS) &&
		(upd9002_dispatch_normalization_verify_live() != SUCCESS)) {
		ret = STATFLAG_FAILURE;
	}
#endif
#if defined(VAEG_Z80_INTEGRATION_TESTING)
	if (ret == STATFLAG_SUCCESS) {
		subsystem_z80_test_get_trace(&z80trace);
		if ((z80trace.f4_count != 1) || (z80trace.f4_last_value != 0x5a)) {
			ret = STATFLAG_FAILURE;
		}
	}
#endif
	if (ret == STATFLAG_SUCCESS) {
		ret = statsave_save(path2);
	}
	S98_trash();
	pccore_term();
	soundmng_deinitialize();

	if (ret != STATFLAG_SUCCESS) {
		file_delete(path1);
		file_delete(path2);
		file_delete(pathbad);
		return(fail("statsave", "save/check/load returned failure"));
	}
	if (compare_statsave_stable_sections(path1, path2) != SUCCESS) {
		file_delete(path1);
		file_delete(path2);
		file_delete(pathbad);
		return(fail("statsave", "save/load/save bytes differ"));
	}

	file_delete(path1);
	file_delete(path2);
	file_delete(pathbad);
	fprintf(stderr, "selftest: statsave ok\n");
	return(SUCCESS);
}

typedef struct {
	char	text[256];
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

	ROMANKANA_STATE	state;
	TOKENBUF		buf;

	romankana_reset(&state);
	ZeroMemory(&buf, sizeof(buf));
	romankana_feed(&state, "AkaShi", tokenbuf_emit, &buf);
	romankana_flush(&state, tokenbuf_emit, &buf);
	if (strcmp(buf.text, "a,ka,shi") != 0) {
		return(fail("romankana", "uppercase/basic syllables failed"));
	}

	romankana_reset(&state);
	ZeroMemory(&buf, sizeof(buf));
	romankana_feed(&state, "shi si tsu tu", tokenbuf_emit, &buf);
	romankana_flush(&state, tokenbuf_emit, &buf);
	if (strcmp(buf.text, "shi,?,shi,?,tsu,?,tsu") != 0) {
		return(fail("romankana", "shi/si and tsu/tu aliases failed"));
	}

	romankana_reset(&state);
	ZeroMemory(&buf, sizeof(buf));
	romankana_feed(&state, "nn n' nka kko", tokenbuf_emit, &buf);
	romankana_flush(&state, tokenbuf_emit, &buf);
	if (strcmp(buf.text, "nn,?,nn,?,nn,ka,?,xtsu,ko") != 0) {
		return(fail("romankana", "n and doubled-consonant handling failed"));
	}

	romankana_reset(&state);
	ZeroMemory(&buf, sizeof(buf));
	romankana_feed(&state, "gaza daba papa", tokenbuf_emit, &buf);
	romankana_flush(&state, tokenbuf_emit, &buf);
	if (strcmp(buf.text, "ga,za,?,da,ba,?,pa,pa") != 0) {
		return(fail("romankana", "voiced syllables failed"));
	}

	romankana_reset(&state);
	ZeroMemory(&buf, sizeof(buf));
	romankana_feed(&state, "nya nyu nyo kya sha syo chu tyo ryo gya ja pyo",
				   tokenbuf_emit, &buf);
	romankana_flush(&state, tokenbuf_emit, &buf);
	if (strcmp(buf.text,
			   "nya,?,nyu,?,nyo,?,kya,?,sha,?,sho,?,chu,?,cho,?,ryo,?,gya,?,ja,?,pyo") != 0) {
		return(fail("romankana", "yoon syllables failed"));
	}

	romankana_reset(&state);
	ZeroMemory(&buf, sizeof(buf));
	romankana_feed(&state, "xya lyu xyo xa li xo xtu ltu",
				   tokenbuf_emit, &buf);
	romankana_flush(&state, tokenbuf_emit, &buf);
	if (strcmp(buf.text, "xya,?,xyu,?,xyo,?,xa,?,xi,?,xo,?,xtsu,?,xtsu") != 0) {
		return(fail("romankana", "small kana aliases failed"));
	}

	romankana_reset(&state);
	ZeroMemory(&buf, sizeof(buf));
	romankana_feed(&state, "va vi vu ve vo", tokenbuf_emit, &buf);
	romankana_flush(&state, tokenbuf_emit, &buf);
	if (strcmp(buf.text, "va,?,vi,?,vu,?,ve,?,vo") != 0) {
		return(fail("romankana", "vu syllables failed"));
	}
	fprintf(stderr, "selftest: romankana ok\n");
	return(SUCCESS);
}

static int test_keyboard_mapping(void) {

	char printable[96];
	KBDPASTE_ACTION actions[96];
	size_t count;
	UINT skipped;
	UINT index;

	if (kbdmap_selftest() != SUCCESS) {
		return(fail("keyboard-map", "mapping lookup/persistence failed"));
	}
	if (test_romankana() != SUCCESS) {
		return(FAILURE);
	}
	for (index = 0; index < 95; index++) {
		printable[index] = (char)(0x20 + index);
	}
	printable[95] = '\0';
	count = kbdpaste_map_text(printable, actions, NELEMENTS(actions),
							  &skipped);
	if ((count != 95) || (skipped != 0)) {
		return(fail("keyboard-paste", "printable ASCII coverage failed"));
	}
	count = kbdpaste_map_text("aA0@\"=_\\~", actions,
							  NELEMENTS(actions), &skipped);
	if ((count != 9) || (skipped != 0) ||
		(actions[0].guest_code != kbdmap_guest_code(KBDROLE_A)) ||
		actions[0].shift ||
		(actions[1].guest_code != kbdmap_guest_code(KBDROLE_A)) ||
		!actions[1].shift ||
		(actions[2].guest_code != kbdmap_guest_code(KBDROLE_0)) ||
		actions[2].shift ||
		(actions[3].guest_code != kbdmap_guest_code(KBDROLE_AT)) ||
		actions[3].shift ||
		(actions[4].guest_code != kbdmap_guest_code(KBDROLE_2)) ||
		!actions[4].shift ||
		(actions[5].guest_code != kbdmap_guest_code(KBDROLE_MINUS)) ||
		!actions[5].shift ||
		(actions[6].guest_code != kbdmap_guest_code(KBDROLE_UNDERSCORE)) ||
		!actions[6].shift ||
		(actions[7].guest_code != kbdmap_guest_code(KBDROLE_YEN)) ||
		actions[7].shift ||
		(actions[8].guest_code != kbdmap_guest_code(KBDROLE_AT)) ||
		!actions[8].shift) {
		return(fail("keyboard-paste", "guest chord mapping failed"));
	}
	count = kbdpaste_map_text("a\rb\nc\r\nd", actions,
							  NELEMENTS(actions), &skipped);
	if ((count != 7) || (skipped != 0) ||
		(actions[0].guest_code != kbdmap_guest_code(KBDROLE_A)) ||
		(actions[1].guest_code != kbdmap_guest_code(KBDROLE_RETURNL)) ||
		(actions[3].guest_code != kbdmap_guest_code(KBDROLE_RETURNL)) ||
		(actions[5].guest_code != kbdmap_guest_code(KBDROLE_RETURNL))) {
		return(fail("keyboard-paste", "CR/LF normalization failed"));
	}
	count = kbdpaste_map_text("a\t\xc3\xa9", actions,
							  NELEMENTS(actions), &skipped);
	if ((count != 1) || (skipped != 2) ||
		(actions[0].guest_code != kbdmap_guest_code(KBDROLE_A)) ||
		(kbdpaste_interval_ms() != 20)) {
		return(fail("keyboard-paste", "UTF-8 skip or pacing failed"));
	}
	fprintf(stderr, "selftest: keyboard paste mapping ok\n");
	fprintf(stderr, "selftest: keyboard mapping ok\n");
	return(SUCCESS);
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
		return(fail("mouse", "initial state is not released"));
	}
	vaeg_mouse_state_motion(&state, 20, -20);
	vaeg_mouse_state_button(&state, VAEG_MOUSE_BUTTON_LEFT, TRUE);
	buttons = vaeg_mouse_state_getstat(&state, &x, &y, 1);
	if ((x != 0) || (y != 0) || (buttons != VAEG_MOUSE_RELEASED)) {
		return(fail("mouse", "inactive input was accepted"));
	}

	vaeg_mouse_state_set_active(&state, TRUE);
	vaeg_mouse_state_motion(&state, 3, -5);
	buttons = vaeg_mouse_state_getstat(&state, &x, &y, 0);
#if defined(VAEG_FIX)
	if ((x != 1) || (y != -2)) {
#else
	if ((x != 3) || (y != -5)) {
#endif
		return(fail("mouse", "relative movement scaling failed"));
	}
	vaeg_mouse_state_getstat(&state, &x, &y, 1);
	vaeg_mouse_state_motion(&state, 1, -1);
	vaeg_mouse_state_getstat(&state, &x, &y, 1);
	if ((x != 1) || (y != -1)) {
		return(fail("mouse", "relative movement remainder was lost"));
	}

	vaeg_mouse_state_button(&state, VAEG_MOUSE_BUTTON_LEFT, TRUE);
	buttons = vaeg_mouse_state_getstat(&state, &x, &y, 0);
	if (buttons != VAEG_MOUSE_RIGHTBIT) {
		return(fail("mouse", "left button is not active-low"));
	}
	vaeg_mouse_state_button(&state, VAEG_MOUSE_BUTTON_RIGHT, TRUE);
	buttons = vaeg_mouse_state_getstat(&state, &x, &y, 0);
	if (buttons != 0) {
		return(fail("mouse", "right button is not active-low"));
	}
	vaeg_mouse_state_button(&state, VAEG_MOUSE_BUTTON_LEFT, FALSE);
	vaeg_mouse_state_button(&state, VAEG_MOUSE_BUTTON_RIGHT, FALSE);
	buttons = vaeg_mouse_state_getstat(&state, &x, &y, 0);
	if (buttons != VAEG_MOUSE_RELEASED) {
		return(fail("mouse", "button release failed"));
	}

	state.x = ((SINT64)INT16_MAX + 1) * 2;
	vaeg_mouse_state_getstat(&state, &x, &y, 1);
	if (x != INT16_MAX) {
		return(fail("mouse", "large movement was not clamped"));
	}
	vaeg_mouse_state_getstat(&state, &x, &y, 1);
#if defined(VAEG_FIX)
	if (x != 1) {
#else
	if (x != INT16_MAX) {
#endif
		return(fail("mouse", "clamped movement remainder was lost"));
	}
	state.x = INT64_MAX - 1;
	vaeg_mouse_state_motion(&state, INT32_MAX, 0);
	if (state.x != INT64_MAX) {
		return(fail("mouse", "movement accumulator overflowed"));
	}
	vaeg_mouse_state_reset(&state);
	if (!state.active || (state.x != 0) || (state.y != 0) ||
		(state.buttons != VAEG_MOUSE_RELEASED)) {
		return(fail("mouse", "reset did not release active state"));
	}
	vaeg_mouse_state_set_active(&state, FALSE);
	if (state.active || (state.buttons != VAEG_MOUSE_RELEASED)) {
		return(fail("mouse", "capture disable did not release state"));
	}

	saved_f12 = np2oscfg.F12KEY;
	np2oscfg.F12KEY = 0;
	if (kbdmap_lookup(SDL_SCANCODE_F12) != KBDMAP_NC) {
		np2oscfg.F12KEY = saved_f12;
		return(fail("mouse", "F12 Mouse binding leaked a guest key"));
	}
	np2oscfg.F12KEY = 1;
	if (kbdmap_lookup(SDL_SCANCODE_F12) != 0x61) {
		np2oscfg.F12KEY = saved_f12;
		return(fail("mouse", "F12 COPY binding changed"));
	}
	np2oscfg.F12KEY = 2;
	if (kbdmap_lookup(SDL_SCANCODE_F12) != 0x60) {
		np2oscfg.F12KEY = saved_f12;
		return(fail("mouse", "F12 STOP binding changed"));
	}
	np2oscfg.F12KEY = 3;
	if (kbdmap_lookup(SDL_SCANCODE_F12) != 0x4d) {
		np2oscfg.F12KEY = saved_f12;
		return(fail("mouse", "F12 keypad-equal binding changed"));
	}
	np2oscfg.F12KEY = 4;
	if (kbdmap_lookup(SDL_SCANCODE_F12) != 0x4f) {
		np2oscfg.F12KEY = saved_f12;
		return(fail("mouse", "F12 keypad-comma binding changed"));
	}
	np2oscfg.F12KEY = saved_f12;
	fprintf(stderr, "selftest: mouse state ok\n");
	return(SUCCESS);
}

static void program_opn_test_voice(void) {

	static const REG8 slots[] = {0x00, 0x04, 0x08, 0x0c};
	UINT index;

	for (index=0; index<NELEMENTS(slots); index++) {
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

	if (!vaeg_sound_rate_valid(11025) ||
		!vaeg_sound_rate_valid(22050) ||
		!vaeg_sound_rate_valid(44100) ||
		vaeg_sound_rate_valid(0) || vaeg_sound_rate_valid(48000)) {
		return(fail("sound-options", "sampling-rate validation failed"));
	}
	if (!vaeg_sound_buffer_valid(40) ||
		!vaeg_sound_buffer_valid(1000) ||
		vaeg_sound_buffer_valid(39) || vaeg_sound_buffer_valid(1001) ||
		(vaeg_sound_buffer_clamp(39) != 40) ||
		(vaeg_sound_buffer_clamp(1001) != 1000) ||
		(vaeg_sound_buffer_clamp(200) != 200)) {
		return(fail("sound-options", "buffer validation failed"));
	}
	if ((vaeg_sound_buffer_samples(44100, 40, 2) != 1024) ||
		(vaeg_sound_buffer_samples(22050, 500, 2) != 8192) ||
		(vaeg_sound_buffer_samples(48000, 40, 2) != 0) ||
		(vaeg_sound_buffer_samples(44100, 39, 2) != 0) ||
		(vaeg_sound_buffer_samples(44100, 40, 0) != 0)) {
		return(fail("sound-options", "buffer sample calculation failed"));
	}
	fprintf(stderr, "selftest: sound options ok\n");
	return(SUCCESS);
}

static int opn_backend_produces_audio(UINT backend, REG8 channels,
												UINT flags) {

	SINT32 pcm[4096 * 2];
	UINT index;

	opngen_setbackend(backend);
	opngen_reset();
	opngen_setcfg(channels, flags);
	opngen_setvol(64);
	program_opn_test_voice();
	ZeroMemory(pcm, sizeof(pcm));
	opngen_getpcm(NULL, pcm, NELEMENTS(pcm) / 2);
	for (index=0; index<NELEMENTS(pcm); index++) {
		if (pcm[index] != 0) {
			return(SUCCESS);
		}
	}
	return(FAILURE);
}

static int test_opn_backends(void) {

	BOOL saved_sound_enabled;

	if ((np2_default_sound_for_model(str_VA1) != FMBOARD_VA_OPN) ||
		(np2_default_sound_for_model(str_VA2) != FMBOARD_VA_OPNA) ||
		(np2_sound_hardware_valid(str_VA1, FMBOARD_NONE) != FALSE) ||
		(np2_sound_hardware_valid(str_VA1, FMBOARD_VA_OPN) != TRUE) ||
		(np2_sound_hardware_valid(str_VA1, FMBOARD_VA_OPNA) != TRUE) ||
		(np2_sound_hardware_valid(str_VA2, FMBOARD_VA_OPN) != FALSE) ||
		(np2_sound_hardware_valid(str_VA2, FMBOARD_VA_OPNA) != TRUE)) {
		return(fail("opn-backend", "VA sound hardware policy failed"));
	}
	saved_sound_enabled = soundmng_isenabled();
	soundmng_setenabled(FALSE);
	if (soundmng_isenabled()) {
		return(fail("opn-backend", "host audio disable was ignored"));
	}
	soundmng_setenabled(TRUE);
	if (!soundmng_isenabled()) {
		return(fail("opn-backend", "host audio enable was ignored"));
	}
	soundmng_setenabled(saved_sound_enabled);
	opngen_initialize(44100);
	if ((ymfm_opn_parsefidelity("minimum") !=
							YMFMBRIDGE_FIDELITY_MINIMUM) ||
		(ymfm_opn_parsefidelity("medium") !=
							YMFMBRIDGE_FIDELITY_MEDIUM) ||
		(ymfm_opn_parsefidelity("maximum") !=
							YMFMBRIDGE_FIDELITY_MAXIMUM) ||
		(ymfm_opn_parsefidelity("invalid") !=
							YMFMBRIDGE_FIDELITY_DEFAULT)) {
		return(fail("opn-backend", "ymfm fidelity parsing failed"));
	}
	ymfm_opn_setfidelity(YMFMBRIDGE_FIDELITY_MEDIUM);
	if ((ymfm_opn_getfidelity() != YMFMBRIDGE_FIDELITY_MEDIUM) ||
		strcmp(ymfm_opn_fidelityname(ymfm_opn_getfidelity()), "medium")) {
		return(fail("opn-backend", "ymfm fidelity selection failed"));
	}
	ymfm_opn_setfidelity(YMFMBRIDGE_FIDELITY_MAXIMUM);
	if ((ymfm_opn_getfidelity() != YMFMBRIDGE_FIDELITY_MAXIMUM) ||
		strcmp(ymfm_opn_fidelityname(ymfm_opn_getfidelity()), "maximum")) {
		return(fail("opn-backend", "maximum ymfm fidelity selection failed"));
	}
	ymfm_opn_setfidelity(99);
	if (ymfm_opn_getfidelity() != YMFMBRIDGE_FIDELITY_DEFAULT) {
		return(fail("opn-backend", "invalid ymfm fidelity did not fallback"));
	}
	if (opngen_parsebackend("np2") != OPN_BACKEND_NP2 ||
		opngen_parsebackend("ymfm") != OPN_BACKEND_YMFM ||
		opngen_parsebackend("invalid") != OPN_BACKEND_YMFM ||
		opngen_parsebackend("") != OPN_BACKEND_YMFM ||
		opngen_parsebackend(NULL) != OPN_BACKEND_YMFM) {
		return(fail("opn-backend", "backend name parsing failed"));
	}
	if (opn_backend_produces_audio(OPN_BACKEND_NP2, 3,
										OPN_MONORAL | 0x007) != SUCCESS) {
		return(fail("opn-backend", "NP2 YM2203 path was silent"));
	}
	if (opn_backend_produces_audio(OPN_BACKEND_YMFM, 3,
									OPN_MONORAL | 0x007) != SUCCESS) {
		return(fail("opn-backend", "ymfm YM2203 path was silent"));
	}
	ymfm_opn_setfidelity(YMFMBRIDGE_FIDELITY_MAXIMUM);
	if (opn_backend_produces_audio(OPN_BACKEND_YMFM, 6,
										OPN_STEREO | 0x03f) != SUCCESS) {
		return(fail("opn-backend", "ymfm YM2608 path was silent"));
	}
	opngen_setbackend(OPN_BACKEND_YMFM);
	ymfm_opn_setfidelity(YMFMBRIDGE_FIDELITY_DEFAULT);
	opngen_reset();
	fprintf(stderr, "selftest: OPN backends ok\n");
	return(SUCCESS);
}

int vaeg_selftest_run(void) {
#if defined(VAEG_UPD9002_M44_TESTING)
	if (upd9002_state_scenario_requested()) {
		return(test_statsave() == SUCCESS ? SUCCESS : FAILURE);
	}
#endif

	if (test_codecnv() != SUCCESS) {
		return(FAILURE);
	}
	if (test_romcheck() != SUCCESS) {
		return(FAILURE);
	}
	if (test_cli_boot_model() != SUCCESS) {
		return(FAILURE);
	}
	if (test_cli_options() != SUCCESS) {
		return(FAILURE);
	}
	if (test_hostfat_snapshot() != SUCCESS) {
		return(FAILURE);
	}
	if (np2_cli_override_selftest() != TRUE) {
		return(fail("CLI options", "session override restoration failed"));
	}
	fprintf(stderr, "selftest: CLI override restoration ok\n");
	if (test_va_tvram_window() != SUCCESS) {
		return(FAILURE);
	}
	if (test_profile_ini() != SUCCESS) {
		return(FAILURE);
	}
	if (test_framedisp() != SUCCESS) {
		return(FAILURE);
	}
	if (test_new_fdd_image() != SUCCESS) {
		return(FAILURE);
	}
	if (test_sasi_image_validation() != SUCCESS) {
		return(FAILURE);
	}
	if (test_clockscale() != SUCCESS) {
		return(FAILURE);
	}
	if (test_sgp_speed() != SUCCESS) {
		return(FAILURE);
	}
	if (test_pacing() != SUCCESS) {
		return(FAILURE);
	}
	if (test_viewport() != SUCCESS) {
		return(FAILURE);
	}
	if (test_va_raster_guard() != SUCCESS) {
		return(FAILURE);
	}
	if (dropmedia_selftest() != SUCCESS) {
		return(fail("dropmedia", "extension, path, or sorting policy failed"));
	}
	fprintf(stderr, "selftest: dropmedia ok\n");
	if (test_mouse_state() != SUCCESS) {
		return(FAILURE);
	}
	if (test_sound_options() != SUCCESS) {
		return(FAILURE);
	}
	if (test_keyboard_mapping() != SUCCESS) {
		return(FAILURE);
	}
	if (test_opn_backends() != SUCCESS) {
		return(FAILURE);
	}
	if (test_statsave() != SUCCESS) {
		return(FAILURE);
	}
	if (test_va_bms_window() != SUCCESS) {
		return(FAILURE);
	}
	if (test_hostfat_transport() != SUCCESS) {
		return(FAILURE);
	}
#if defined(VAEG_Z80_INTEGRATION_TESTING)
	if (vaeg_z80_subsystem_integration_test() != SUCCESS) {
		return(fail("Z80 subsystem integration", "production seam test failed"));
	}
#endif
	fprintf(stderr, "selftest: all tests passed\n");
	return(SUCCESS);
}
