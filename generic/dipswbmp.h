
#ifdef __cplusplus
extern "C"{
#endif


// それぞれ 4bit BMPが返る (メモリ解放を行なうこと)

BYTE *dipswbmp_get9861(const BYTE *s, const BYTE *j);

BYTE *dipswbmp_getsnd26(BYTE cfg);
BYTE *dipswbmp_getsnd86(BYTE cfg);
BYTE *dipswbmp_getsndspb(BYTE cfg, BYTE vrc);
BYTE *dipswbmp_getmpu(BYTE cfg);

#ifdef __cplusplus
}
#endif

