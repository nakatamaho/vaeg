
enum {
	ROMVA_N80_NAMES = 4
};

#ifdef __cplusplus
extern "C" {
#endif

extern const char *const romva_n80_names[ROMVA_N80_NAMES];

/* M113: called at the end of romva_initialize() to replace VA ROM images
 * with a PC-8801 machine's ROMs (set by the frontend; NULL for none). */
extern void (*romva_machine_hook)(void);

void romva_initialize(void);
void romva_load_n80_rom(void);
const char *romva_default_font_filename(void);
BOOL romva_load_default_font(void);
BOOL romva_load_pc98_font(const char *filename);

#ifdef __cplusplus
}
#endif
