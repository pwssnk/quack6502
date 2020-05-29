#pragma warning (disable:26812)

#include "systems.h"
#include <iostream>
#include <iomanip>

using namespace Qk;
using namespace Qk::NES;

/*
	Constructor, destructor

		NES CPU memory map
			https://wiki.nesdev.com/w/index.php/CPU_memory_map

				Address			Size	Description
				-----------     -----   -----------------------------------------------------------------------
			X	$0000-$07FF 	$0800 	2KB internal RAM
				-----------     -----   -----------------------------------------------------------------------
			X	$0800-$0FFF 	$0800 	Mirrors of $0000-$07FF
			X	$1000-$17FF 	$0800
			X	$1800-$1FFF 	$0800
				-----------     -----   -----------------------------------------------------------------------
			X	$2000-$2007 	$0008 	NES PPU registers
				-----------     -----   -----------------------------------------------------------------------
			X	$2008-$3FFF 	$1FF8 	Mirrors of $2000-2007 (repeats every 8 bytes)
				-----------     -----   -----------------------------------------------------------------------
			V	$4000-$4017 	$0018 	NES APU and I/O registers
				-----------     -----   -----------------------------------------------------------------------
			V	$4018-$401F 	$0008 	APU and I/O functionality that is normally disabled. See CPU Test Mode.
				-----------     -----   -----------------------------------------------------------------------
			X	$4020-$FFFF 	$BFE0 	Cartridge space: PRG ROM, PRG RAM, and mapper registers (See Note)
				-----------     -----   -----------------------------------------------------------------------
*/

NESConsole::NESConsole()
{
	// Dynamically allocate console component objects to heap, 
	// so we don't take too big a bite out of stack memory
	m_bus = new Bus();
	m_cpu = new MOS6502(*m_bus);
	m_ram = new RAM(*m_bus, AddressRange(0x000, 0x07FF));
	m_rmm = new MemoryMirror(*m_bus, *m_ram, AddressRange(0x0800, 0x1FFF));
	m_cas = new CartridgeSlot(*m_bus, AddressRange(0x4020, 0xFFFF));
	m_ppu = new RP2C02(*m_bus, AddressRange(0x2000, 0x2007), *m_cas);
	m_pmm = new MemoryMirror(*m_bus, *m_ppu, AddressRange(0x2008, 0x3FFF));
	
	// Because OAM DMA address $4014 inconveniently falls within what is otherwise APU address range ($4000-$4015), 
	// and our bus code assumes that devices occupy a contiguous address range, we'll handle DMA forwards in the APU
	// class, even though it's not really related. Kinda janky, but avoids code rewrites and additional complexity.
	// On a real NES, addresses $4000-$4017 all connect to the custom 6502 CPU chip, which includes the APU
	// and some I/O registers
	m_apu = new APU(*m_bus, AddressRange(0x4000, 0x4015));

	// Likewise, because NES controllers and APU frame counter share $4017,
	// forward frame counter updates to APU in the ControllerInterface class.
	// Yuck.
	m_ctr = new ControllerInterface(*m_bus, AddressRange(0x4016, 0x4017));

	// Initialize components
	m_cpu->Reset();
	m_ppu->Reset();

	// The NES's 6502 chip does not include hardware support for
	// decimal mode, so disable it
	m_cpu->SetDecimalModeAvailable(false);

	// Keep track of PPU frame rendering status
	m_ppu_ps = m_ppu->GetVideoOutput();
}

NESConsole::~NESConsole()
{
	delete m_bus;
	delete m_cpu;
	delete m_ram;
	delete m_rmm;
	delete m_cas;
	delete m_ppu;
	delete m_pmm;
	delete m_apu;
	delete m_ctr;
}

/*
	API
*/

void NESConsole::Clock()
{
	// Cycle components at proper relative frequencies
	// PPU does 3 cycles for every 1 CPU cycle

	m_ppu->Cycle();

	if (m_systemClockCount % 3 == 0)
	{
		m_cpu->Cycle();
		m_apu->Cycle();
	}

	m_systemClockCount++;
}

void NESConsole::Reset()
{
	// Resetting NES only affects CPU; RAM and PPU unaffected
	m_cpu->Reset();
}

void NESConsole::Reset(word programCounter)
{
	m_cpu->Reset(programCounter);
}

void NESConsole::InsertCartridge(const std::shared_ptr<Cartridge>& cartridge)
{
	m_cas->InsertCartridge(cartridge);
}

FramebufferDescriptor* NESConsole::GetVideoOutput()
{
	return m_ppu_ps;
}

unsigned long NESConsole::GetPPUFrameCount() const
{
	return m_ppu->GetFrameCount();
}

void NESConsole::FillAudioBuffer(audiosample* buffer, size_t numSamples)
{
	m_apu->FillAudioBuffer(buffer, numSamples);
}

int NESConsole::GetAudioBufferSize() const
{
	return m_apu->GetAudioBufferSize();
}

double NESConsole::GetAudioSampleRate() const
{
	return m_apu->GetAudioSampleRate();
}

void NESConsole::ControllerInput(Controller::Player pad, Controller::Button button, bool pressed)
{
	if (pressed)
		m_ctr->PressButton(pad, button);
	else
		m_ctr->ReleaseButton(pad, button);
}


/*
	DEBUG
*/

#ifdef _DEBUG

void NESConsole::PrintMemory(word addressStart, word addressEnd)
{
	using namespace std;

	if (addressStart > addressEnd)
	{
		return;
	}

	for (addressStart; addressStart <= addressEnd; addressStart++)
	{
		cout << "$" << hex << setfill('0') << setw(4) << addressStart << "  #"
			<< hex << setfill('0') << setw(2) << (int)m_bus->Peek(addressStart) << endl;
	}
}

void NESConsole::DumpCPUFrameLog(const std::string& path)
{
	m_cpu->SaveDebugInfo(path);
}

#endif