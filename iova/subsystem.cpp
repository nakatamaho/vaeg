/*
 * subsystem.cpp: PC-88VA FD Sub System
 *
 */

#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"

#include	<cstdint>

#include	"cpucva/z80_disasm.h"
#if defined(VAEG_Z80_CORE_SUZUKIPLAN)
#include	"cpucva/z80_core.h"
#else
#include	"cpucva/z80c.h"
#endif
#include	"i8255.h"
#include	"subsystemif.h"
#include	"fdc.h"

#include	"subsystem.h"


// TRACEOUTを有効にする場合は、以下の1を0にする
#if 1
#undef TRACEOUT
#define TRACEOUT(arg)
#endif

#if defined(VAEG_Z80_INTEGRATION_TRACE)
#define Z80TRACE(arg)	fdc_trace_text arg
#else
#define Z80TRACE(arg)
#endif

#if defined(VAEG_Z80_CORE_SUZUKIPLAN)
#define Z80CORENAME	"suzukiplan"
#else
#define Z80CORENAME	"legacy"
#endif

#define SLEEP_HACK		// メインからのコマンド待ち時にCPUを停止する機能を有効にする 

enum {
	Z80_STATUS_WAIT_OFFSET = 57,
	Z80_STATUS_WAIT_EXTERNAL = 0x02
};


static	_I8255CFG i8255cfg;
		_SUBSYSTEM subsystem;

#if defined(VAEG_Z80_INTEGRATION_TESTING)
static VAEG_Z80_INTEGRATION_TRACE_STATE z80testtrace;
#endif


// ---- Clock

class Clock : public IClock
{
	#if defined(VAEG_Z80_INTEGRATION_TESTING)
public:
	Clock() : testoverride(false), testnow(0) {}
	void SetTestNow(std::uint32_t now) { testoverride = true; testnow = now; }
	#endif

	std::uint32_t IFCALL now() {
	#if defined(VAEG_Z80_INTEGRATION_TESTING)
		if (testoverride) return testnow;
	#endif
		return CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK;
	}

	#if defined(VAEG_Z80_INTEGRATION_TESTING)
private:
	bool testoverride;
	std::uint32_t testnow;
	#endif
};


// ---- ClockCounter

class ClockCounter : public IClockCounter
{
public:
	void IFCALL past(std::int32_t clock);
	std::int32_t IFCALL GetRemainclock();
	void IFCALL SetRemainclock(std::int32_t clock);
	void IFCALL SetMultiple(int multiple);
private:
	SINT32 remainclock;
	int mul;
};

// ---- ClockCounter implementation

void ClockCounter::SetMultiple(int _mul) {
	mul = _mul;
}

void IFCALL ClockCounter::past(std::int32_t clock) {
	remainclock -= clock * mul;
}
std::int32_t IFCALL ClockCounter::GetRemainclock() {
	return remainclock;
}
void IFCALL ClockCounter::SetRemainclock(std::int32_t clock) {
	remainclock = clock;
}


// ---- Subsystem

class Subsystem : public IMemoryAccess, public IIOAccess
{
public:
	Subsystem();
	~Subsystem();

	void Initialize();
	void Reset();
	void IRQ(BOOL irq);
	void Exec();
	void SetWait(bool wait, const char *source);
	bool SaveStatus(UINT8 *buffer);
	bool LoadStatus(const UINT8 *buffer);

	std::uint32_t IFCALL Read8(std::uint32_t addr);
	void IFCALL Write8(std::uint32_t addr, std::uint32_t data);

	std::uint32_t IFCALL In(std::uint32_t port);
	void IFCALL Out(std::uint32_t port, std::uint32_t data);



	enum SpecialPort2
	{
		pres2 = 0x100,	// たぶんつかわない
		pirq2,			// たぶんつかわない
		piac2,
		pfdstat,		// たぶんつかわない
						// FD の動作状況 (b0-1 = LAMP, b2-3 = MODE, b4=SEEK)
		portend2
	};

public:
	Z80C *z80;

private:
	ClockCounter *clockcounter;
	Clock *clock;
	bool waitactive;

	std::uint32_t _ram_rd(std::uint32_t addr);
	std::uint32_t ram_rd(std::uint32_t addr);
	std::uint32_t rom_rd(std::uint32_t addr);
	std::uint32_t nonmem_rd(std::uint32_t addr);
	void ram_wt(std::uint32_t addr, std::uint32_t data);

#if defined(VAEG_Z80_INTEGRATION_TESTING)
public:
	void TestReset();
	void TestInstall(WORD address, const UINT8 *data, UINT size);
	void TestSetPC(WORD pc);
	void TestSetClock(UINT32 now);
	BOOL TestGetState(VAEG_Z80_INTEGRATION_CPU_STATE *state);
#endif

#if defined(SLEEP_HACK)
	bool SleepCheck_VA(std::uint32_t portc);
	bool SleepCheck_Sorcerian(std::uint32_t portc);
#endif
};




// ---- Sybsystem implementation

Subsystem::Subsystem() {
	z80 = new Z80C();
	clockcounter = new ClockCounter();
	clock = new Clock();
	waitactive = false;
}

Subsystem::~Subsystem() {
	if (z80) delete z80;
	if (clockcounter) delete clockcounter;
	if (clock) delete clock;
}

//BYTE *Subsystem::GetRomBuf() {
//	return rom;
//}

void Subsystem::Initialize() {
	z80->Init(this, this, clock, clockcounter, piac2);
	//rom[0] = 0xf3;
	//rom[1] = 0x76;
	i8255_init(&i8255cfg, &subsystem.i8255);
	i8255cfg.busoutportb = subsystemif_businporta;
	i8255cfg.busoutportc = subsystemif_businportc;
}

void Subsystem::Reset() {
	clockcounter->SetMultiple(pccore.multiple);
	z80->Reset();
	SetWait(false, "reset");
	i8255_reset(&i8255cfg);
	subsystem.intopcode = 0x7f;
}

void Subsystem::IRQ(BOOL irq) {
	Z80TRACE(("z80trace core=%s event=irq level=%u live=%04x public=%04x",
			Z80CORENAME, (unsigned)!!irq, (unsigned)z80->GetPC(),
			(unsigned)z80->GetReg()->pc));
	z80->IRQ(0,irq);
}

void Subsystem::Exec() {
	Z80TRACE(("z80trace core=%s event=exec-enter live=%04x public=%04x wait=%u remain=%d now=%u",
			Z80CORENAME, (unsigned)z80->GetPC(),
			(unsigned)z80->GetReg()->pc, (unsigned)waitactive,
			(int)clockcounter->GetRemainclock(), (unsigned)clock->now()));
	z80->Exec();
	Z80TRACE(("z80trace core=%s event=exec-exit live=%04x public=%04x wait=%u remain=%d now=%u",
			Z80CORENAME, (unsigned)z80->GetPC(),
			(unsigned)z80->GetReg()->pc, (unsigned)waitactive,
			(int)clockcounter->GetRemainclock(), (unsigned)clock->now()));
}

void Subsystem::SetWait(bool wait, const char *source) {
	z80->Wait(wait);
	waitactive = wait;
	Z80TRACE(("z80trace core=%s event=wait level=%u source=%s live=%04x public=%04x remain=%d now=%u",
			Z80CORENAME, (unsigned)wait, source,
			(unsigned)z80->GetPC(), (unsigned)z80->GetReg()->pc,
			(int)clockcounter->GetRemainclock(), (unsigned)clock->now()));
#if defined(VAEG_Z80_INTEGRATION_TESTING)
	z80testtrace.wait_active = wait ? TRUE : FALSE;
	if (wait) {
		z80testtrace.wait_assert_count++;
	}
	else {
		z80testtrace.wait_release_count++;
	}
#endif
}

bool Subsystem::SaveStatus(UINT8 *buffer) {
	return z80->SaveStatus(buffer);
}

bool Subsystem::LoadStatus(const UINT8 *buffer) {
	if (!z80->LoadStatus(buffer)) {
		return false;
	}
	waitactive = (buffer[Z80_STATUS_WAIT_OFFSET] &
						Z80_STATUS_WAIT_EXTERNAL) != 0;
#if defined(VAEG_Z80_INTEGRATION_TESTING)
	z80testtrace.wait_active = waitactive ? TRUE : FALSE;
#endif
	return true;
}

#if defined(VAEG_Z80_INTEGRATION_TESTING)
void Subsystem::TestReset() {
	clock->SetTestNow(0);
	clockcounter->SetMultiple(1);
	ZeroMemory(subsystem.rom, sizeof(subsystem.rom));
	ZeroMemory(subsystem.ram, sizeof(subsystem.ram));
	z80->Reset();
	waitactive = false;
	i8255_reset(&i8255cfg);
	subsystem.intopcode = 0x7f;
	ZeroMemory(&z80testtrace, sizeof(z80testtrace));
}

void Subsystem::TestInstall(WORD address, const UINT8 *data, UINT size) {
	UINT i;

	if (data == NULL) return;
	for (i=0; i<size; i++) {
		const UINT current = (address + i) & 0xffff;
		if (current < 0x2000) {
			subsystem.rom[current] = data[i];
		}
		else if ((current >= 0x4000) && (current < 0x8000)) {
			subsystem.ram[current - 0x4000] = data[i];
		}
	}
}

void Subsystem::TestSetPC(WORD pc) {
	z80->SetPC(pc);
}

void Subsystem::TestSetClock(UINT32 now) {
	clock->SetTestNow(now);
}

BOOL Subsystem::TestGetState(VAEG_Z80_INTEGRATION_CPU_STATE *state) {
	UINT8 status[68];
	const Z80Reg *reg;

	if ((state == NULL) || (z80->GetStatusSize() != sizeof(status)) ||
			!z80->SaveStatus(status)) {
		return FALSE;
	}
	reg = z80->GetReg();
	state->af = LOADINTELWORD(status + 0);
	state->hl = LOADINTELWORD(status + 4);
	state->de = LOADINTELWORD(status + 8);
	state->bc = LOADINTELWORD(status + 12);
	state->sp = LOADINTELWORD(status + 24);
	state->live_pc = (UINT16)z80->GetPC();
	state->public_pc = reg->pc;
	state->irq = status[56];
	state->wait_flags = status[57];
	state->remainclock = clockcounter->GetRemainclock();
	state->lastclock = (SINT32)LOADINTELDWORD(status + 64);
	return TRUE;
}
#endif

std::uint32_t Subsystem::_ram_rd(std::uint32_t addr) {
	return subsystem.ram[addr-0x4000];
}

std::uint32_t Subsystem::ram_rd(std::uint32_t addr) {
	return _ram_rd(addr);
}

void Subsystem::ram_wt(std::uint32_t addr, std::uint32_t data) {
	subsystem.ram[addr-0x4000] = data;
}

std::uint32_t Subsystem::rom_rd(std::uint32_t addr) {
	return subsystem.rom[addr];
}

std::uint32_t Subsystem::nonmem_rd(std::uint32_t addr) {
	return 0xff;
}


std::uint32_t IFCALL Subsystem::Read8(std::uint32_t addr) {
	switch(addr >> 13) {
	case 0: return rom_rd(addr);
	case 2: return ram_rd(addr);
	case 3: return ram_rd(addr);
	default: return nonmem_rd(addr);
	}
}

void IFCALL Subsystem::Write8(std::uint32_t addr, std::uint32_t data) {
	switch(addr >> 13) {
	case 2: ram_wt(addr, data);
	case 3: ram_wt(addr, data);
	}
}

#if defined(SLEEP_HACK)
// VA/2 の ROMの処理の場合
bool Subsystem::SleepCheck_VA(std::uint32_t portc) {
	return !(portc & 0x08) && _ram_rd(0x7f67)==0xff && z80->GetReg()->pc == 0x1732;
	/*
	  7f67 は サブシステムスリープ時 ffh, アクティブ時 00h
      アクティブ時は適当な時間をまってFDDモータをOFFする処理が動いているので、
	  ATN=0の条件だけでZ80を停止してはならない。そのため、条件にスリープ時であること
	  を追加している。
      1732 のコードでポートfehからinしている
	*/
}

// ソーサリアンの場合
bool Subsystem::SleepCheck_Sorcerian(std::uint32_t portc) {
	return !(portc & 0x08) && z80->GetReg()->pc == 0x700e;
	/*
	  700e のコードでポートfehからinしている
	*/
}
#endif

std::uint32_t IFCALL Subsystem::In(std::uint32_t port) {
	std::uint32_t ret = 0xff;

	switch(port) {
	case 0xf8:
		fdcsubsys_o_tc();
		ret = 0xff;
		break;
	case 0xfa:
		ret = fdcsubsys_i_fdc0();
		break;
	case 0xfb:
		ret = fdcsubsys_i_fdc1();
		break;
	case 0xfc:
		ret = i8255_inporta(&i8255cfg);
		break;
	case 0xfd:
		ret = i8255_inportb(&i8255cfg);
		break;
	case 0xfe: {
		ret = i8255_inportc(&i8255cfg);
#if defined(SLEEP_HACK)
		const bool sleepva = SleepCheck_VA(ret);
		const bool sleepsorcerian = !sleepva && SleepCheck_Sorcerian(ret);
		if (sleepva || sleepsorcerian) {
#if defined(VAEG_Z80_INTEGRATION_TESTING)
			z80testtrace.sleep_live_pc = (WORD)z80->GetPC();
			z80testtrace.sleep_public_pc = z80->GetReg()->pc;
			z80testtrace.sleep_path = sleepva ? 1 : 2;
			z80testtrace.sleep_port_value = (BYTE)ret;
			z80testtrace.sleep_memory_value =
						sleepva ? (BYTE)_ram_rd(0x7f67) : 0;
#endif
			Z80TRACE(("z80trace core=%s event=sleep-assert path=%s port=fe value=%02x live=%04x public=%04x memory=%02x",
					Z80CORENAME, sleepva ? "va" : "sorcerian",
					(unsigned)(ret & 0xff), (unsigned)z80->GetPC(),
					(unsigned)z80->GetReg()->pc,
					(unsigned)(sleepva ? _ram_rd(0x7f67) : 0)));
			// サブシステムZ80スリープ
			SetWait(true, sleepva ? "sleep-va" : "sleep-sorcerian");
		}
#endif
		break;
	}

	// 仮想ポート
	case piac2:
		// 割り込み受理
		ret = subsystem.intopcode;
#if defined(VAEG_Z80_INTEGRATION_TESTING)
		z80testtrace.acknowledge_count++;
#endif
		Z80TRACE(("z80trace core=%s event=ack port=%03x value=%02x live=%04x public=%04x",
				Z80CORENAME, piac2, (unsigned)(ret & 0xff),
				(unsigned)z80->GetPC(), (unsigned)z80->GetReg()->pc));
		//z80->IRQ(0,0);
		break;
	}
#if defined(VAEG_Z80_INTEGRATION_TESTING)
	if (port == 0xfe) {
		z80testtrace.fe_read_count++;
	}
#endif
	Z80TRACE(("z80trace core=%s event=in port=%03x value=%02x live=%04x public=%04x",
			Z80CORENAME, (unsigned)port, (unsigned)(ret & 0xff),
			(unsigned)z80->GetPC(), (unsigned)z80->GetReg()->pc));
	if (port != 0xfe) {
		TRACEOUT(("subsys: in : %02x -> %02x  [%04x]", port, ret, z80->GetReg()->pc));
	}
	return ret;
}

void IFCALL Subsystem::Out(std::uint32_t port, std::uint32_t dat) {
	TRACEOUT(("subsys: out: %02x <- %02x  [%04x]", port, dat, z80->GetReg()->pc));
	Z80TRACE(("z80trace core=%s event=out port=%02x value=%02x live=%04x public=%04x",
			Z80CORENAME, (unsigned)(port & 0xff),
			(unsigned)(dat & 0xff), (unsigned)z80->GetPC(),
			(unsigned)z80->GetReg()->pc));

	switch(port) {
	case 0xf0:
		subsystem.intopcode = dat;
		break;
	case 0xf4:
#if defined(VAEG_Z80_INTEGRATION_TESTING)
		z80testtrace.f4_count++;
		z80testtrace.f4_last_value = (BYTE)dat;
#endif
		fdcsubsys_o_dskctl(dat);
		break;
	case 0xf8:
		fdcsubsys_o_mtrctl(dat);
		break;
	case 0xfb:
		fdcsubsys_o_fdc1(dat);
		break;
	case 0xfc:
		i8255_outporta(&i8255cfg, dat);
		break;
	case 0xfd:
		i8255_outportb(&i8255cfg, dat);
		break;
	case 0xfe:
		i8255_outportc(&i8255cfg, dat);
		break;
	case 0xff:
		i8255_outctrl(&i8255cfg, dat);
		break;
	default:	// unknown
		{
			int a;
			a=0;
		}
		break;
	}
}

// ----

	Subsystem subsystemobj;

// ---- Main → Subsystem connection

void subsystem_businporta(BYTE dat) {
	i8255_businporta(&i8255cfg, dat);
}

void subsystem_businportc(BYTE dat) {
	i8255_businportc(&i8255cfg, (BYTE)(dat >> 4));
#if defined(SLEEP_HACK)
	if (dat & 0x80) {
		// ATN = 1
		subsystemobj.SetWait(false, "atn");
	}
#endif
}

// ---- C - C++ bridge for Subsystem

void subsystem_initialize(void) {
	subsystemobj.Initialize();	
}

void subsystem_reset(void) {
	subsystemobj.Reset();
}

void subsystem_irq(BOOL irq) {
	subsystemobj.IRQ(irq);
}

void subsystem_exec(void) {
	subsystemobj.Exec();
}

BYTE subsystem_readmem(WORD addr) {
	return subsystemobj.Read8(addr);
}

const struct Z80Reg *subsystem_getcpureg(void) {
	return subsystemobj.z80->GetReg();
}

static std::uint8_t subsystem_disasm_read(void *opaque,
		std::uint16_t address) {
	Subsystem *target = static_cast<Subsystem *>(opaque);
	return static_cast<std::uint8_t>(target->Read8(address));
}

WORD subsystem_disassemble_bounded(WORD pc, char *str, UINT capacity) {
	const VaegZ80DisasmResult result = VaegZ80Disassemble(
		static_cast<std::uint16_t>(pc), str,
		static_cast<std::uint32_t>(capacity), subsystem_disasm_read,
		&subsystemobj);
	return static_cast<WORD>(result.next_pc);
}

WORD subsystem_disassemble(WORD pc, char *str) {
	return subsystem_disassemble_bounded(
		pc, str, SUBSYSTEM_DISASSEMBLY_CAPACITY);
}

UINT subsystem_getcpustatussize(void) {
	return subsystemobj.z80->GetStatusSize();
}

BOOL subsystem_savecpustatus(UINT8 *buf) {
	const BOOL result = subsystemobj.SaveStatus(buf) ? TRUE : FALSE;
	Z80TRACE(("z80trace core=%s event=state-save result=%u live=%04x public=%04x",
			Z80CORENAME, (unsigned)result,
			(unsigned)subsystemobj.z80->GetPC(),
			(unsigned)subsystemobj.z80->GetReg()->pc));
	return result;
}

BOOL subsystem_loadcpustatus(const UINT8 *buf) {
	const BOOL result = subsystemobj.LoadStatus(buf) ? TRUE : FALSE;
	Z80TRACE(("z80trace core=%s event=state-load result=%u live=%04x public=%04x",
			Z80CORENAME, (unsigned)result,
			(unsigned)subsystemobj.z80->GetPC(),
			(unsigned)subsystemobj.z80->GetReg()->pc));
	return result;
}

#if defined(VAEG_Z80_INTEGRATION_TESTING)
void subsystem_z80_test_reset(void) {
	subsystemobj.TestReset();
}

void subsystem_z80_test_install(WORD address, const UINT8 *data, UINT size) {
	subsystemobj.TestInstall(address, data, size);
}

void subsystem_z80_test_set_pc(WORD pc) {
	subsystemobj.TestSetPC(pc);
}

void subsystem_z80_test_set_clock(UINT32 now) {
	subsystemobj.TestSetClock(now);
}

void subsystem_z80_test_set_wait(BOOL wait) {
	subsystemobj.SetWait(wait != FALSE, "test");
}

void subsystem_z80_test_reset_trace(void) {
	ZeroMemory(&z80testtrace, sizeof(z80testtrace));
}

void subsystem_z80_test_get_trace(VAEG_Z80_INTEGRATION_TRACE_STATE *trace) {
	if (trace != NULL) {
		*trace = z80testtrace;
	}
}

BOOL subsystem_z80_test_get_state(VAEG_Z80_INTEGRATION_CPU_STATE *state) {
	return subsystemobj.TestGetState(state);
}
#endif
