#include "dosio.h"

/* M142: incompatible new container; old host-dependent saves are rejected. */
#define VAEG_STATE_FILE_VERSION UINT32_C(0x261031)
enum {
	STATSAVE_DISK_PATH_BYTES = 4096,
	STATSAVE_DISK_RECORD_BYTES = 4160,
	STATSAVE_DISK_COUNT = 13 /* four FDDs, two SASI, all seven SCSI IDs */
};

enum {
	STATFLAG_SUCCESS = 0,
	STATFLAG_DISKCHG = 0x0001,
	STATFLAG_VERCHG = 0x0002,
	STATFLAG_WARNING = 0x0080,
	STATFLAG_VERSION = 0x0100,
	STATFLAG_FAILURE = -1
};

typedef struct {
	char index[10];
	UINT16 ver;
	UINT32 size;
} STFLAGHDR;

typedef struct {
	STFLAGHDR hdr;
	UINT pos;
	char *err;
	int errlen;
} _STFLAGH, *STFLAGH;

typedef struct {
	char index[10];
	UINT16 ver;
	UINT16 type;
	void *arg1;
	UINT arg2;
} SFENTRY;

#ifdef __cplusplus
extern "C" {
#endif

int statflag_read(STFLAGH sfh, void *ptr, UINT size);
int statflag_write(STFLAGH sfh, const void *ptr, UINT size);
void statflag_seterr(STFLAGH sfh, const char *str);

int statsave_save(const char *filename);
int statsave_check(const char *filename, char *buf, int size);
/* Explicit UI-confirmed override; all non-HOSTFAT checks remain active. */
int statsave_check_hostfat_override(const char *filename, char *buf, int size);
/* Optional frontend remapping, shared by preflight and actual load. The
 * callback returns TRUE only for an existing, unambiguous local file. */
typedef BOOL (*STATSAVE_DISK_RESOLVER)(const char *saved, char *local, UINT size);
STATSAVE_DISK_RESOLVER statsave_set_disk_resolver(STATSAVE_DISK_RESOLVER resolver);
int statsave_load(const char *filename);
int statsave_load_checked(const char *filename, char *error, int error_size);
/* Retains the current HOSTFAT mount state and read-only snapshot. */
int statsave_load_hostfat_override(const char *filename);
int statsave_load_hostfat_override_checked(const char *filename, char *error, int error_size);

#if defined(SUPPORT_OPRECORD)
typedef void (*STATSAVE_LOAD_DISK_HOOK)(int drv, char *path, UINT size, int *readonly);

void statsave_set_load_disk_hook(STATSAVE_LOAD_DISK_HOOK hook);

#endif

#ifdef __cplusplus
}
#endif
