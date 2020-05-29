#ifdef _DEBUG

#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>

#endif

#include "cpu.h"

//#define CPU_DEBUG


using namespace Qk;


/*
	Constructors, destructor
*/

MOS6502::MOS6502(Bus& bus) : Device(bus), m_instructionHandler(*this)
{
#ifdef CPU_DEBUG
	// DEBUG
	LOGGER = new CPUDebugLogger(*this);
#endif

	// Reset CPU, set PC to 0 for now, as we don't
	// want to load reset vector until the user tells
	// us it's safe to do so
	Reset(0x0000);
}

MOS6502::~MOS6502()
{
#ifdef CPU_DEBUG
	// DEBUG
	delete LOGGER;
#endif
}

/*
	CPU control
*/

void MOS6502::Reset()
{
	// Read reset vector at 0xFFFC and 0xFFFD
	word addr = ((word)Read(0xFFFD) << 8) | Read(0xFFFC);

	// Reset, pass reset vector value as PC initial value
	Reset(addr);
}

void MOS6502::Reset(word programCounter)
{
	// Set initial state for registers
	Registers.A = 0;
	Registers.X = 0;
	Registers.Y = 0;
	Registers.PC = programCounter;
	Registers.P = 0x24; // Set unused expansion bit, InterruptDisable
	Registers.S = 0xFD; // Stack range 0x0100 to 0x01FF (256 bytes)
						// but stack pointer only stores lower byte
}

void MOS6502::Cycle()
{
	if (m_halted)
		return;

	if (m_remainingCycles <= 0)
	{
#ifdef CPU_DEBUG
		// DEBUG
		// Start frame logging
		LOGGER->NewFrame();
		LOGGER->RecordPreOpCPUState();
#endif
		// First, deal with any interrupt requests
		if (m_nmiPending)
		{
			m_nmiPending = false;
			m_instructionHandler.NMI();
		}
		else if (m_irqPending)
		{
			m_irqPending = false;
			m_instructionHandler.IRQ();
		}
		else
		{
			// Execute next opcode
			m_instructionHandler.ExecuteNextInstruction();
		}

		// Add instruction duration to cycle wait counter
		m_remainingCycles += m_instructionHandler.GetLastInstructionCycles();

#ifdef CPU_DEBUG
		// DEBUG
		// Finish frame logging
		LOGGER->RecordPostOpCPUState();
#endif
	}

	m_remainingCycles--;
	m_cpuCycleCount++;
}

void MOS6502::SetDecimalModeAvailable(bool available)
{
	// Some variants of the 6502 chip -- notably the NES's -- do not
	// include the hardware for decimal mode. To accurately emulate those chips,
	// this functions can tell the CPU to block any decimal mode operations, 
	// regardless of the decimal mode flag being set in the CPU status register

	m_decimalModeAvailable = available;
}

/*
	Interrupt signal handling
*/

void MOS6502::GenerateInterrupt()
{
	// Set pending interrupt
	m_irqPending = true;
}

void MOS6502::GenerateNonMaskableInterrupt()
{
	m_nmiPending = true;
}

void MOS6502::OnBusSignal(int signalId)
{
	switch (signalId)
	{
		case SIGNAL_CPU_IRQ:
			GenerateInterrupt();
			break;
		case SIGNAL_CPU_NMI:
			GenerateNonMaskableInterrupt();
			break;
		case SIGNAL_CPU_HLT:
			m_halted = true;
			break;
		case SIGNAL_CPU_RSM:
			m_halted = false;
			break;
		default:
			break;
	}
}


/*
	Bus I/O
*/

byte MOS6502::Read(word address)
{
#ifdef CPU_DEBUG
	// DEBUG
	byte value = BUS.ReadFromBus(address);
	LOGGER->RecordIOEvent(address, value, false);
	return value;
#endif
#ifndef CPU_DEBUG
	return BUS.ReadFromBus(address);
#endif
}

void MOS6502::Write(word address, byte data)
{
#ifdef CPU_DEBUG
	// DEBUG
	LOGGER->RecordIOEvent(address, data, true);
#endif // DEBUG

	BUS.WriteToBus(address, data);
}


/*
	CPU flag logic
*/

bool MOS6502::CheckFlag(Flag flag) const
{
	return (Registers.P & static_cast<int>(flag)) != 0;
}

void MOS6502::SetFlag(Flag flag)
{
	Registers.P |= static_cast<int>(flag);
}

void MOS6502::SetFlag(Flag flag, bool state)
{
	if (state)
		Registers.P |= static_cast<int>(flag);
	else
		Registers.P &= ~static_cast<int>(flag);
}

void MOS6502::ClearFlag(Flag flag)
{
	Registers.P &= ~static_cast<int>(flag);
}

void MOS6502::ClearFlags()
{
	Registers.P = 0x20;
}


/*
	Other convenience functions
*/

// Convert 8-bit stack pointer to usable 16-bit address
word MOS6502::GetStackPointerAddress() const
{
	// Stack range 0x0100 to 0x01FF (256 bytes)
	// but stack pointer only stores lower byte
	return 0x0100 | (word)Registers.S;
}

unsigned long MOS6502::GetCPUCycleCount() const
{
	return m_cpuCycleCount;
}


/*
	DEBUGGING STUFF
*/

#ifdef _DEBUG

// CPU

void MOS6502::PrintDebugInfo() const
{
	LOGGER->PrintFrameDebugInfo();
}

void MOS6502::SaveDebugInfo(const std::string& path) const
{
	LOGGER->SaveDebugInfo(path);
}


// Debug frame

CPUDebugFrame::CPUDebugFrame()
{

}

CPUDebugFrame::~CPUDebugFrame()
{
	for (auto ptr : IOEvents)
	{
		delete ptr;
	}
	IOEvents.clear();
}

std::string CPUDebugFrame::ToString() const
{
	return std::string("NOT IMPLEMENTED");
}

std::string CPUDebugFrame::ToJSONString() const
{
	using namespace std;

	stringstream s;

	s << "{";
	s << "\"Cycle\": " << Cycle << ",\n";
	s << "\"Opcode\": \"" << hex << setfill('0') << setw(2) << Opcode << "\",\n";
	s << "\"OpcodeMnemonic\": \"" << OpcodeMnemonic << "\",\n";
	s << "\"AddressingMode\": \"" << AddressingModeMnemonic<< "\",\n";

	s << "\"PreOpState\":  {\"A\": \"" << hex << setfill('0') << setw(2) << PreOpState.A << "\",";
	s << " \"X\": \"" << hex << setfill('0') << setw(2) << PreOpState.X << "\",";
	s << " \"Y\": \"" << hex << setfill('0') << setw(2) << PreOpState.Y << "\",";
	s << " \"P\": \"" << hex << setfill('0') << setw(2) << PreOpState.P << "\",";
	s << " \"S\": \"" << hex << setfill('0') << setw(2) << PreOpState.S << "\",";
	s << " \"PC\": \"" << hex << setfill('0') << setw(4) << PreOpState.PC << "\"},\n";

	s << "\"PostOpState\":  {\"A\": \"" << hex << setfill('0') << setw(2) << PostOpState.A << "\",";
	s << " \"X\": \"" << hex << setfill('0') << setw(2) << PostOpState.X << "\",";
	s << " \"Y\": \"" << hex << setfill('0') << setw(2) << PostOpState.Y << "\",";
	s << " \"P\": \"" << hex << setfill('0') << setw(2) << PostOpState.P << "\",";
	s << " \"S\": \"" << hex << setfill('0') << setw(2) << PostOpState.S << "\",";
	s << " \"PC\": \"" << hex << setfill('0') << setw(4) << PostOpState.PC << "\"},\n";

	s << "\"IOEvents\":\n";
	s << "\t[\n";

	for (unsigned int i = 0; i < IOEvents.size(); i++)
	{
		auto ev = IOEvents.at(i);

		// {"Type": "R", "Address": 0, "Value": 0},
		s << "\t\t{\"Type\": ";
		s << ((ev->IsWrite) ? "\"W\"," : "\"R\",");
		s << "\"Address\": \"" << hex << setfill('0') << setw(4) << ev->Address << "\", ";
		s << "\"Value\": \"" << hex << setfill('0') << setw(2) << ev->Value << "\"";

		// Valid JSON requires that last item in list does not end with comma
		if (i < IOEvents.size() - 1)
			s << "},\n";
		else
			s << "}\n";
	}

	s << "\t]\n}";

	return std::string(s.str());
}


// Logger

CPUDebugLogger::CPUDebugLogger(MOS6502& cpu) : m_cpu(cpu)
{
	CurrentFrame = nullptr;
}

CPUDebugLogger::~CPUDebugLogger()
{
	for (auto ptr : Frames)
	{
		delete ptr;
	}
	Frames.clear();
}

void CPUDebugLogger::NewFrame()
{
	CurrentFrame = new CPUDebugFrame();
	Frames.push_back(CurrentFrame);
}

void CPUDebugLogger::RecordIOEvent(word address, byte value, bool isWrite)
{
	if (CurrentFrame == nullptr)
		return;

	CPUDebugFrame::ReadWriteEvent* event = new CPUDebugFrame::ReadWriteEvent;

	event->IsWrite = isWrite;
	event->Address = address;
	event->Value = value;

	CurrentFrame->IOEvents.push_back(event);
}

void CPUDebugLogger::RecordPreOpCPUState()
{
	// Record cpu state
	CurrentFrame->PreOpState.A = m_cpu.Registers.A;
	CurrentFrame->PreOpState.X = m_cpu.Registers.X;
	CurrentFrame->PreOpState.Y = m_cpu.Registers.Y;
	CurrentFrame->PreOpState.P = m_cpu.Registers.P;
	CurrentFrame->PreOpState.S = m_cpu.Registers.S;
	CurrentFrame->PreOpState.PC = m_cpu.Registers.PC;
}

void CPUDebugLogger::RecordPostOpCPUState()
{
	CurrentFrame->Opcode = m_cpu.m_instructionHandler.GetLastInstructionOpcode();
	CurrentFrame->OpcodeMnemonic = m_cpu.m_instructionHandler.GetLastInstructionMnemonic();
	CurrentFrame->AddressingModeMnemonic = m_cpu.m_instructionHandler.GetLastInstructionAddressingModeMnemonic();
	
	// Record cpu state
	CurrentFrame->PostOpState.A = m_cpu.Registers.A;
	CurrentFrame->PostOpState.X = m_cpu.Registers.X;
	CurrentFrame->PostOpState.Y = m_cpu.Registers.Y;
	CurrentFrame->PostOpState.P = m_cpu.Registers.P;
	CurrentFrame->PostOpState.S = m_cpu.Registers.S;
	CurrentFrame->PostOpState.PC = m_cpu.Registers.PC;
	CurrentFrame->Cycle = m_cpu.GetCPUCycleCount();
	
	CurrentFrame->PostOpState.AddressedLocation = m_cpu.m_instructionHandler.m_cacheAbsoluteWorkingAddress;
	CurrentFrame->PostOpState.FetchedData = m_cpu.m_instructionHandler.m_cacheFetchedData;
}

void CPUDebugLogger::PrintFrameDebugInfo() const
{
	std::cout << CurrentFrame->ToString() << std::endl;
}

void CPUDebugLogger::SaveDebugInfo(const std::string& filepath) const
{
	std::ofstream out(filepath);

	if (out)
	{
		out << "[\n";
		
		for (unsigned int i = 0; i < Frames.size(); i++)
		{
			auto fr = Frames.at(i);

			out << fr->ToJSONString();

			// Valid JSON requires that last item in list does not end with comma
			if (i < Frames.size() - 1)
				out << ",\n\n";
			else
				out << "\n\n";
		}

		out << "]";

		out.close();
		std::cout << "Saved CPU debug info to: " << filepath << std::endl;
	}
	else
	{
		std::cout << "Could not open file: " << filepath << std::endl;
	}
}

#endif