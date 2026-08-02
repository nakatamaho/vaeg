
typedef struct {
	UINT	port;
	UINT	phase;
	BYTE	reg[0x30];
	UINT8	auxstatus;
	UINT8	scsistatus;
	UINT8	membank;
	UINT8	memwnd;
	UINT8	resent;
	UINT8	datmap;
	UINT	cmdpos;
	UINT	wrdatpos;
	UINT	rddatpos;
	BYTE	cmd[12];
	BYTE	data[0x10000];
	/* Keep the historical serialized image size without owning board ROM. */
	BYTE	reserved[2][0x2000];

} _SCSIIO, *SCSIIO;


#ifdef __cplusplus
extern "C" {
#endif

extern	_SCSIIO		scsiio;

void scsiioint(NEVENTITEM item);
void scsiio_watchdog_event(NEVENTITEM item);

void scsiio_reset(void);
void scsiio_bind(void);
void scsiio_trace_enable(BOOL enabled);
void scsiio_trace_compact(BOOL compact);
void scsiio_trace_limit(UINT limit);
void scsiio_trace_jitter(BOOL enabled, UINT seed, UINT span);
BOOL scsiio_trace_stop_requested(void);
void scsiio_trace_pic_irq(REG8 irq, BOOL asserted);

#ifdef __cplusplus
}
#endif
