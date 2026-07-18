
void v30cinit(void);

#if defined(VAEG_UPD9002_M46_TESTING)
int upd9002_dispatch_test_verify(void);
void upd9002_dispatch_test_require_immutable(void);
UINT upd9002_dispatch_test_construction_count(void);
UINT upd9002_dispatch_test_rejected_count(void);
#endif
