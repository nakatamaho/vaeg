
#ifdef __cplusplus
extern "C" {
#endif

extern BYTE textraster[];
extern BYTE textcolorraster[];

void maketextva_initialize(void);
void maketextva(void);

void maketextva_begin(BOOL *scrn200);
void maketextva_raster(void);
void maketextva_blankraster(void);
UINT32 maketextva_bytelocal(UINT32 local);
BOOL maketextva_bytelocal_usable(UINT32 local);

#ifdef __cplusplus
}
#endif
