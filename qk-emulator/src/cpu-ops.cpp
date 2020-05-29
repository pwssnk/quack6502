#include "cpu.h"
#include "util.h"

using namespace Qk;

/*
	Constructors, destructor
*/

MOS6502::InstructionHandler::InstructionHandler(MOS6502& parent) : CPU(parent)
{

}

/*
	External interface methods
*/

void MOS6502::InstructionHandler::ExecuteNextInstruction()
{
	// Clear cache variables
	m_opcode = 0xEA; // Default to NOP
	m_cacheAbsoluteWorkingAddress = 0;
	m_cacheFetchedData = 0;
	m_doFetch = true;
	m_additionalCyclesNeeded = 0;

	m_didIRQ = false;
	m_didNMI = false;

	// Read next opcode
	m_opcode = CPU.Read(CPU.Registers.PC++);
	Instruction& op = OpcodeMap[m_opcode];

	// Resolve address
	CallFuncPtr(op.AdressingFunction);

	// Do operation (including fetch)
	CallFuncPtr(op.OperationFunction);
}

byte MOS6502::InstructionHandler::GetLastInstructionOpcode() const
{
	return m_opcode;
}

int MOS6502::InstructionHandler::GetLastInstructionCycles() const
{
	if (m_didIRQ)
		return 7;
	if (m_didNMI)
		return 8;

	int cycles = OpcodeMap[m_opcode].BaseCycles;
	return m_additionalCyclesNeeded >= 2 ? (cycles + m_additionalCyclesNeeded - 1) : cycles;
}

std::string MOS6502::InstructionHandler::GetLastInstructionMnemonic() const
{
	return std::string(OpcodeMap[m_opcode].Mnemonic);
}

std::string MOS6502::InstructionHandler::GetLastInstructionAddressingModeMnemonic() const
{
	/*
			void IMP(); // Implied
			void IMM(); // Immediate
			void ACC(); // Accumulator
			void ZP0(); // Zero page
			void ZPX(); // Zero page X
			void ZPY(); // Zero page Y
			void REL(); // Relative
			void ABS(); // Absolute
			void ABX(); // Absolute X
			void ABY(); // Absolute Y
			void IND(); // Indirect
			void IZX(); // Indexed indirect
			void IZY(); // Indirect indexed
	*/
	FuncPtr f = OpcodeMap[m_opcode].AdressingFunction;

	if (f == &InstructionHandler::IMP)
		return std::string("IMP");
	else if (f == &InstructionHandler::IMM)
		return std::string("IMM");
	else if (f == &InstructionHandler::ACC)
		return std::string("ACC");
	else if (f == &InstructionHandler::ZP0)
		return std::string("ZP0");
	else if (f == &InstructionHandler::ZPX)
		return std::string("ZPX");
	else if (f == &InstructionHandler::ZPY)
		return std::string("ZPY");
	else if (f == &InstructionHandler::REL)
		return std::string("REL");
	else if (f == &InstructionHandler::ABS)
		return std::string("ABS");
	else if (f == &InstructionHandler::ABX)
		return std::string("ABX");
	else if (f == &InstructionHandler::ABY)
		return std::string("ABY");
	else if (f == &InstructionHandler::IND)
		return std::string("IND");
	else if (f == &InstructionHandler::IZX)
		return std::string("IZX");
	else if (f == &InstructionHandler::IZY)
		return std::string("IZY");
	else
		return std::string("???");
}

word MOS6502::InstructionHandler::GetLastInstructionAddress() const
{
	return m_cacheAbsoluteWorkingAddress;
}

byte MOS6502::InstructionHandler::GetLastInstructionValue() const
{
	return m_cacheFetchedData;
}


/*
	Common functionality
*/

void MOS6502::InstructionHandler::CallFuncPtr(FuncPtr ptr)
{
	(this->*ptr)();  // C++ member function pointer syntax is truly awful
}

byte MOS6502::InstructionHandler::Fetch()
{
	if (m_doFetch)
	{
		m_cacheFetchedData = CPU.Read(m_cacheAbsoluteWorkingAddress);
	}

	return m_cacheFetchedData;
}

/*
	ADRESSING MODES
*/

// Implied
void MOS6502::InstructionHandler::IMP()
{
	m_cacheFetchedData = 0;
	m_doFetch = false;
}

// Immediate
void MOS6502::InstructionHandler::IMM()
{
	m_cacheAbsoluteWorkingAddress = CPU.Registers.PC;
	CPU.Registers.PC++;
}

// Accumulator
void MOS6502::InstructionHandler::ACC()
{
	m_cacheFetchedData = CPU.Registers.A;
	m_doFetch = false;
}

// Zero page
void MOS6502::InstructionHandler::ZP0()
{
	word address = CPU.Read(CPU.Registers.PC++);
	m_cacheAbsoluteWorkingAddress = address & 0x00FF;
}

// Zero page X
void MOS6502::InstructionHandler::ZPX()
{
	word address = CPU.Read(CPU.Registers.PC++);
	m_cacheAbsoluteWorkingAddress = (address + CPU.Registers.X) & 0x00FF;
}

// Zero page Y
void MOS6502::InstructionHandler::ZPY()
{
	word address = CPU.Read(CPU.Registers.PC++);
	m_cacheAbsoluteWorkingAddress = (address + CPU.Registers.Y) & 0x00FF;
}

// Relative
void MOS6502::InstructionHandler::REL()
{
	byte offset = CPU.Read(CPU.Registers.PC++);

	if (offset > 0x7F)
		m_cacheAbsoluteWorkingAddress = CPU.Registers.PC - (128 - (offset & 0x7F));
	else
		m_cacheAbsoluteWorkingAddress = CPU.Registers.PC + offset;

	// May require additional cycle
	m_additionalCyclesNeeded++;
}

// Absolute
void MOS6502::InstructionHandler::ABS()
{
	// Full 16-bit address is little-endian; read lsb first
	word lowerByte = CPU.Read(CPU.Registers.PC++);
	word upperByte = CPU.Read(CPU.Registers.PC++) << 8;
	m_cacheAbsoluteWorkingAddress = upperByte | lowerByte;
}

// Absolute X
void MOS6502::InstructionHandler::ABX()
{
	word lowerByte = CPU.Read(CPU.Registers.PC++);
	word upperByte = CPU.Read(CPU.Registers.PC++) << 8;

	m_cacheAbsoluteWorkingAddress = (upperByte | lowerByte) + CPU.Registers.X;

	// Check if we pass page boundary; extra cpu cycle may be required
	if ((m_cacheAbsoluteWorkingAddress & 0xFF00) != upperByte)
		m_additionalCyclesNeeded++;
}

// Absolute Y
void MOS6502::InstructionHandler::ABY()
{
	word lowerByte = CPU.Read(CPU.Registers.PC++);
	word upperByte = CPU.Read(CPU.Registers.PC++) << 8;

	m_cacheAbsoluteWorkingAddress = (upperByte | lowerByte) + CPU.Registers.Y;

	// Check if we pass page boundary; extra cpu cycle may be required
	if ((m_cacheAbsoluteWorkingAddress & 0xFF00) != upperByte)
		m_additionalCyclesNeeded++;
}

// Indirect
void MOS6502::InstructionHandler::IND()
{
	// Read indirect address
	word ptrLowerByte = CPU.Read(CPU.Registers.PC++);
	word ptrUpperByte = CPU.Read(CPU.Registers.PC++) << 8;
	word ptrAddress = (ptrUpperByte | ptrLowerByte);

	// An original 6502 does not correctly fetch the target address 
	// if the indirect vector falls on a page boundary (e.g. $xxFF 
	// where xx is any value from $00 to $FF). In this case fetches 
	// the LSB from $xxFF as expected but takes the MSB from $xx00. 
	// This is fixed in some later chips like the 65SC02 so for 
	// compatibility always ensure the indirect vector is not at 
	// the end of the page.
	// 
	// Use indirect address to read true address
	// Chip has a hardware bug where crossing page boundary
	// when reading second byte of 16-bit address causes wrap
	// around to lowest value in current page. Emulate here
	if (ptrLowerByte == 0x00FF) // Bug
		m_cacheAbsoluteWorkingAddress = (CPU.Read(ptrAddress & 0xFF00) << 8) | CPU.Read(ptrAddress);
	else // Ok!
		m_cacheAbsoluteWorkingAddress = (CPU.Read(ptrAddress + 1) << 8) | CPU.Read(ptrAddress);
}

// Indexed indirect
void MOS6502::InstructionHandler::IZX()
{
	word ptr = CPU.Read(CPU.Registers.PC++) + CPU.Registers.X;

	word lowerByte = CPU.Read(ptr & 0x00FF); // Only zero page addresses
	word upperByte = CPU.Read((ptr + 1) & 0x00FF) << 8;

	m_cacheAbsoluteWorkingAddress = upperByte | lowerByte;
}

// Indirect indexed
void MOS6502::InstructionHandler::IZY()
{
	word ptr = CPU.Read(CPU.Registers.PC++);

	word lowerByte = CPU.Read(ptr & 0x00FF); // Only zero page addresses
	word upperByte = CPU.Read((ptr + 1) & 0x00FF) << 8;

	m_cacheAbsoluteWorkingAddress = (upperByte | lowerByte) + CPU.Registers.Y;

	// Check if we pass page boundary; extra cpu cycle may be required
	if ((m_cacheAbsoluteWorkingAddress & 0xFF00) != upperByte)
		m_additionalCyclesNeeded++;
}

/*
	SYSTEM INTERRUPTS
*/

void MOS6502::InstructionHandler::IRQ()
{
	if (!CPU.CheckFlag(Flag::InterruptDisable))
	{
		// Push PC onto stack
		byte upperByte = (CPU.Registers.PC >> 8) & 0x00FF;
		byte lowerByte = CPU.Registers.PC & 0x00FF;

		CPU.Write(CPU.GetStackPointerAddress(), upperByte);
		CPU.Registers.S--;
		CPU.Write(CPU.GetStackPointerAddress(), lowerByte);
		CPU.Registers.S--;

		CPU.ClearFlag(Flag::Break);
		CPU.SetFlag(Flag::InterruptDisable);

		// Push P onto stack
		CPU.Write(CPU.GetStackPointerAddress(), CPU.Registers.P);
		CPU.Registers.S--;

		// Load interrupt vector into PC
		CPU.Registers.PC = ((word)CPU.Read(0xFFFF) << 8) | (word)CPU.Read(0xFFFE);

		// Interrupt takes 7 cycles; set m_didNMI tot true
		// to signal to cycle calculator 
		m_didNMI = true;
	}
}

void MOS6502::InstructionHandler::NMI()
{
	// Push PC onto stack
	byte upperByte = (CPU.Registers.PC >> 8) & 0x00FF;
	byte lowerByte = CPU.Registers.PC & 0x00FF;

	CPU.Write(CPU.GetStackPointerAddress(), upperByte);
	CPU.Registers.S--;
	CPU.Write(CPU.GetStackPointerAddress(), lowerByte);
	CPU.Registers.S--;

	CPU.ClearFlag(Flag::Break);
	CPU.SetFlag(Flag::InterruptDisable);

	// Push P onto stack
	CPU.Write(CPU.GetStackPointerAddress(), CPU.Registers.P);
	CPU.Registers.S--;

	// Load interrupt vector into PC
	CPU.Registers.PC = ((word)CPU.Read(0xFFFB) << 8) | (word)CPU.Read(0xFFFA);

	// Interrupt takes 8 cycles; set m_didNMI tot true
	// to signal to cycle calculator 
	m_didNMI = true;
}


/*
	INSTRUCTIONS
	
	See https://www.obelisk.me.uk/6502/reference.html for implementation details
*/

// Unkown or uninmplemented opcode
void MOS6502::InstructionHandler::XXX()
{
	return;
}

// No op
void MOS6502::InstructionHandler::NOP()
{
	// TODO: some unnoficial/undocumented instructions
	// are functionally NOPs, but some of them
	// are not 1 byte instructions. Should emulate
	// 2-byte unnofficial NOPs too at some point
	// as some programs do use them

	return;
}

// Force interrupt
void MOS6502::InstructionHandler::BRK()
{
	// Manually increment PC, as BRK opcode is always followed by
	// a padding  byte
	CPU.Registers.PC++;

	// Set interrupt flag
	CPU.SetFlag(Flag::Break);

	// Push PC onto stack
	byte upperByte = (CPU.Registers.PC >> 8) & 0xFF00;
	byte lowerByte = CPU.Registers.PC & 0x00FF;

	CPU.Write(CPU.GetStackPointerAddress(), upperByte);
	CPU.Registers.S--;
	CPU.Write(CPU.GetStackPointerAddress(), lowerByte);
	CPU.Registers.S--;

	// Set Break flag
	CPU.SetFlag(Flag::Break);

	// Push P onto stack
	CPU.Write(CPU.GetStackPointerAddress(), CPU.Registers.P);
	CPU.Registers.S--;

	// Clear Break flag
	CPU.ClearFlag(Flag::Break);

	// Load interrupt vector into PC
	CPU.Registers.PC = (word)CPU.Read(0xFFFE) | ((word)CPU.Read(0xFFFF) << 8);
}

// Return from interrupt
void MOS6502::InstructionHandler::RTI()
{
	// Pull processor status from stack
	CPU.Registers.S++;
	byte p = CPU.Read(CPU.GetStackPointerAddress());

	// Discard the Break flag:
	if (CPU.CheckFlag(Flag::Break))
		p |= static_cast<int>(Flag::Break);
	else
		p &= ~static_cast<int>(Flag::Break);

	CPU.Registers.P = p;

	// Expansion bit is always set
	CPU.SetFlag(Flag::Expansion);

	// Pull PC from stack
	CPU.Registers.S++;
	word lowerByte = CPU.Read(CPU.GetStackPointerAddress());
	CPU.Registers.S++;
	word upperByte = CPU.Read(CPU.GetStackPointerAddress());
	CPU.Registers.PC = (upperByte << 8) | lowerByte;
}

// Load A
void MOS6502::InstructionHandler::LDA()
{
	CPU.Registers.A = Fetch();
	CPU.SetFlag(Flag::Zero, CPU.Registers.A == 0);
	CPU.SetFlag(Flag::Negative, CPU.Registers.A & 0x80);
	m_additionalCyclesNeeded++;
}
// Load X
void MOS6502::InstructionHandler::LDX()
{
	CPU.Registers.X = Fetch();
	CPU.SetFlag(Flag::Zero, CPU.Registers.X == 0);
	CPU.SetFlag(Flag::Negative, CPU.Registers.X & 0x80);
	m_additionalCyclesNeeded++;
}

// Load Y
void MOS6502::InstructionHandler::LDY()
{
	CPU.Registers.Y = Fetch();
	CPU.SetFlag(Flag::Zero, CPU.Registers.Y == 0);
	CPU.SetFlag(Flag::Negative, CPU.Registers.Y & 0x80);
	m_additionalCyclesNeeded++;
}

// STA - Store Accumulator
void MOS6502::InstructionHandler::STA()
{
	CPU.Write(m_cacheAbsoluteWorkingAddress, CPU.Registers.A);
}

// Store X Register
void MOS6502::InstructionHandler::STX()
{
	CPU.Write(m_cacheAbsoluteWorkingAddress, CPU.Registers.X);
}

// Store Y Register
void MOS6502::InstructionHandler::STY()
{
	CPU.Write(m_cacheAbsoluteWorkingAddress, CPU.Registers.Y);
}

// Transfer Accumulator to X
void MOS6502::InstructionHandler::TAX()
{
	CPU.Registers.X = CPU.Registers.A;
	CPU.SetFlag(Flag::Zero, CPU.Registers.X == 0);
	CPU.SetFlag(Flag::Negative, CPU.Registers.X & 0x80);
}

// Transfer Accumulator to Y
void MOS6502::InstructionHandler::TAY()
{
	CPU.Registers.Y = CPU.Registers.A;
	CPU.SetFlag(Flag::Zero, CPU.Registers.Y == 0);
	CPU.SetFlag(Flag::Negative, CPU.Registers.Y & 0x80);
}

// Transfer X to Accumulator
void MOS6502::InstructionHandler::TXA()
{
	CPU.Registers.A = CPU.Registers.X;
	CPU.SetFlag(Flag::Zero, CPU.Registers.A == 0);
	CPU.SetFlag(Flag::Negative, CPU.Registers.A & 0x80);
}

// Transfer Y to Accumulator
void MOS6502::InstructionHandler::TYA()
{
	CPU.Registers.A = CPU.Registers.Y;
	CPU.SetFlag(Flag::Zero, CPU.Registers.Y == 0);
	CPU.SetFlag(Flag::Negative, CPU.Registers.Y & 0x80);
}

// Transfer Stack Pointer to X
void MOS6502::InstructionHandler::TSX()
{
	CPU.Registers.X = CPU.Registers.S;
	CPU.SetFlag(Flag::Zero, CPU.Registers.X == 0);
	CPU.SetFlag(Flag::Negative, CPU.Registers.X & 0x80);
}

// Transfer X to Stack Pointer
void MOS6502::InstructionHandler::TXS()
{
	CPU.Registers.S = CPU.Registers.X;
}

// Push Accumulator onto stack
void MOS6502::InstructionHandler::PHA()
{
	CPU.Write(CPU.GetStackPointerAddress(), CPU.Registers.A);
	CPU.Registers.S--;
}

// Push Processor Status (flags) onto stack
void MOS6502::InstructionHandler::PHP()
{
	// Break and expansion flag set to 1 before push
	CPU.SetFlag(Flag::Break);
	CPU.Write(CPU.GetStackPointerAddress(), CPU.Registers.P);
	CPU.Registers.S--;
	CPU.ClearFlag(Flag::Break);
}

// Pull byte from stack into Accumulator
void MOS6502::InstructionHandler::PLA()
{
	CPU.Registers.S++;
	CPU.Registers.A = CPU.Read(CPU.GetStackPointerAddress());
	CPU.SetFlag(Flag::Zero, CPU.Registers.A == 0);
	CPU.SetFlag(Flag::Negative, CPU.Registers.A & 0x80);
}

// Pull byte from stack into Processor Status register
void MOS6502::InstructionHandler::PLP()
{
	CPU.Registers.S++;
	byte p = CPU.Read(CPU.GetStackPointerAddress());

	// Discard the Break flag:
	if (CPU.CheckFlag(Flag::Break))
		p |= static_cast<int>(Flag::Break);
	else
		p &= ~static_cast<int>(Flag::Break);

	CPU.Registers.P = p;

	// Expansion bit is always set
	CPU.SetFlag(Flag::Expansion);
}

// Logical AND on Accumulator
void MOS6502::InstructionHandler::AND()
{
	CPU.Registers.A &= Fetch();
	CPU.SetFlag(Flag::Zero, CPU.Registers.A == 0);
	CPU.SetFlag(Flag::Negative, CPU.Registers.A & 0x80);

	m_additionalCyclesNeeded++;
}

// Exclusive OR on Accumulator
void MOS6502::InstructionHandler::EOR()
{
	CPU.Registers.A ^= Fetch();
	CPU.SetFlag(Flag::Zero, CPU.Registers.A == 0);
	CPU.SetFlag(Flag::Negative, CPU.Registers.A & 0x80);

	m_additionalCyclesNeeded++;
}

// Logical Inclusive OR on Accumulator
void MOS6502::InstructionHandler::ORA()
{
	CPU.Registers.A |= Fetch();
	CPU.SetFlag(Flag::Zero, CPU.Registers.A == 0);
	CPU.SetFlag(Flag::Negative, CPU.Registers.A & 0x80);

	m_additionalCyclesNeeded++;
}

// Bit Test memory with contents of Accumulator as bitmask
void MOS6502::InstructionHandler::BIT()
{
	byte data = Fetch();

	// Set zero flag if the result if the AND is zero
	CPU.SetFlag(Flag::Zero, (data & CPU.Registers.A) == 0);
	// Set overflow flag to bit 6 of the memory value
	CPU.SetFlag(Flag::Overflow, data & 0x40);
	// Set negative flag to bit 7 of the memory value
	CPU.SetFlag(Flag::Negative, data & 0x80);
}

// Add with Carry (add memory value with carry bit to Acc)
void MOS6502::InstructionHandler::ADC()
{
	// This instruction is affected by Decimal mode
	// See http://www.6502.org/tutorials/decimal_mode.html#3.2

	if (!CPU.CheckFlag(Flag::DecimalMode) || !CPU.m_decimalModeAvailable) // Binary Mode
	{
		// Store A value register var larger data type
		word aval = CPU.Registers.A;
		// Get value in memory
		word value = Fetch();

		// Add both values and the carry bit together and store in temporary var
		word temp = aval + value + (CPU.CheckFlag(Flag::Carry) ? 0x01 : 0x00);

		// Check for overflow into upper byte, set carry flag
		CPU.SetFlag(Flag::Carry, (temp & 0xFF00) != 0);
		// Set zero flag
		CPU.SetFlag(Flag::Zero, (temp & 0x00FF) == 0);
		// Set overflow flag
		CPU.SetFlag(Flag::Overflow, (~(aval ^ value) & (aval ^ temp)) & 0x0080);
		// Set Negative flag
		CPU.SetFlag(Flag::Negative, temp & 0x0080);

		// Save to actual Acc
		CPU.Registers.A = temp & 0x00FF;
	}
	else // Decimal mode
	{
		// This is my half-assed attempt at implementing BCD addition/subtraction
		// based on http://www.6502.org/tutorials/decimal_mode.html
		// No idea if this works as it should
		
		byte valueBCD = Fetch();
		byte avalBCD = CPU.Registers.A;

		// Lazy mode -- just convert to regular binary and add, then convert back to BCD
		byte temp = Util::BCDtoBIN(avalBCD) + Util::BCDtoBIN(valueBCD) + (CPU.CheckFlag(Flag::Carry) ? 0x01 : 0x00);

		byte resultBCD;
		// valid range 0-99, wrap around in case of overflow
		if (temp > 99)
			resultBCD = Util::BINtoBCD((temp - 100) & 0x00FF);
		else	
			resultBCD = Util::BINtoBCD(temp & 0x00FF);
		
		CPU.SetFlag(Flag::Carry, temp > 99);
		CPU.SetFlag(Flag::Zero, resultBCD == 0);
		CPU.SetFlag(Flag::Negative, resultBCD & 0x80);
		// Overflow flag does not have well defined meaning in Decimal Mode
		// So just clear it, w/e
		CPU.ClearFlag(Flag::Overflow); 

		CPU.Registers.A = resultBCD;
	}

	// Extra cycle if this instruction is used in conjunction
	// with certain addressing modes and page boundary is crossed
	m_additionalCyclesNeeded++;
}

// Subtract with Carry
void MOS6502::InstructionHandler::SBC()
{
	// This instruction is affected by Decimal mode
	// See http://www.6502.org/tutorials/decimal_mode.html#3.2

	if (!CPU.CheckFlag(Flag::DecimalMode) || !CPU.m_decimalModeAvailable) // Binary Mode
	{
		// Store A value register var larger data type
		word aval = CPU.Registers.A;

		// Get value in memory
		word value = Fetch();
		// Invert bottom byte of value
		value ^= 0x00FF;

		// Add both values and the carry bit together and store in temporary var
		word temp = aval + value + (CPU.CheckFlag(Flag::Carry) ? 0x01 : 0x00);

		// Check for overflow into upper byte, set carry flag
		CPU.SetFlag(Flag::Carry, (temp & 0xFF00) != 0);
		// Set zero flag
		CPU.SetFlag(Flag::Zero, (temp & 0x00FF) == 0);
		// Set overflow flag
		CPU.SetFlag(Flag::Overflow, (~(aval ^ value) & (aval ^ temp)) & 0x0080);
		// Set Negative flag
		CPU.SetFlag(Flag::Negative, temp & 0x0080);

		// Save to actual Acc
		CPU.Registers.A = temp & 0x00FF;
	}
	else // Decimal mode
	{
		// This is my half-assed attempt at implementing BCD addition/subtraction
		// based on http://www.6502.org/tutorials/decimal_mode.html
		// No idea if this works as it should

		byte valueBCD = Fetch();
		byte avalBCD = CPU.Registers.A;

		int temp = Util::BCDtoBIN(avalBCD) - Util::BCDtoBIN(valueBCD) - (CPU.CheckFlag(Flag::Carry) ? 0x00 : 0x01);

		byte resultBCD;

		// valid range 0-99, wrap around in case of overflow
		if (temp < 0)
			resultBCD = Util::BINtoBCD((99 + temp) & 0xFF);
		else
			resultBCD = Util::BINtoBCD(temp & 0xFF);

		CPU.SetFlag(Flag::Carry, temp < 0);
		CPU.SetFlag(Flag::Zero, resultBCD == 0);
		CPU.SetFlag(Flag::Negative, resultBCD & 0x80);
		CPU.ClearFlag(Flag::Overflow);

		CPU.Registers.A = resultBCD;
	}

	m_additionalCyclesNeeded++;
}

// Compare Accumulator with memory value
void MOS6502::InstructionHandler::CMP()
{
	byte data = Fetch();

	// Set carry flag if A >= data
	CPU.SetFlag(Flag::Carry, CPU.Registers.A >= data);
	// Set zero flag if A == data
	CPU.SetFlag(Flag::Zero, CPU.Registers.A == data);
	// Set negative flag to bit 7 of the sum
	CPU.SetFlag(Flag::Negative, (CPU.Registers.A - data) & 0x80);

	m_additionalCyclesNeeded++;
}

// Compare X with memory value
void MOS6502::InstructionHandler::CPX()
{
	byte data = Fetch();

	// Set carry flag if X >= data
	CPU.SetFlag(Flag::Carry, CPU.Registers.X >= data);
	// Set zero flag if X == data 
	CPU.SetFlag(Flag::Zero, CPU.Registers.X == data);
	// Set negative flag to bit 7 of the sum
	CPU.SetFlag(Flag::Negative, (CPU.Registers.X - data) & 0x80);
}

// Compare Y with memory value
void MOS6502::InstructionHandler::CPY()
{
	byte data = Fetch();

	// Set carry flag if Y >= data
	CPU.SetFlag(Flag::Carry, CPU.Registers.Y >= data);
	// Set zero flag if Y == data 
	CPU.SetFlag(Flag::Zero, CPU.Registers.Y == data);
	// Set negative flag to bit 7 of the sum
	CPU.SetFlag(Flag::Negative, (CPU.Registers.Y - data) & 0x80);
}

// Increment Memory
void MOS6502::InstructionHandler::INC()
{
	byte data = Fetch();
	data++;
	CPU.SetFlag(Flag::Zero, data == 0);
	CPU.SetFlag(Flag::Negative, data & 0x80);
	CPU.Write(m_cacheAbsoluteWorkingAddress, data);
}

// Increment X Register
void MOS6502::InstructionHandler::INX()
{
	CPU.Registers.X++;
	CPU.SetFlag(Flag::Zero, CPU.Registers.X == 0);
	CPU.SetFlag(Flag::Negative, CPU.Registers.X & 0x80);
}

// Increment Y Register
void MOS6502::InstructionHandler::INY()
{
	CPU.Registers.Y++;
	CPU.SetFlag(Flag::Zero, CPU.Registers.Y == 0);
	CPU.SetFlag(Flag::Negative, CPU.Registers.Y & 0x80);
}

// Decrement Memory
void MOS6502::InstructionHandler::DEC()
{
	byte data = Fetch();
	data--;
	CPU.SetFlag(Flag::Zero, data == 0);
	CPU.SetFlag(Flag::Negative, data & 0x80);
	CPU.Write(m_cacheAbsoluteWorkingAddress, data);
}

// Decrement X Register
void MOS6502::InstructionHandler::DEX()
{
	CPU.Registers.X--;
	CPU.SetFlag(Flag::Zero, CPU.Registers.X == 0);
	CPU.SetFlag(Flag::Negative, CPU.Registers.X & 0x80);
}

// Decrement Y Register
void MOS6502::InstructionHandler::DEY()
{
	CPU.Registers.Y--;
	CPU.SetFlag(Flag::Zero, CPU.Registers.Y == 0);
	CPU.SetFlag(Flag::Negative, CPU.Registers.Y & 0x80);
}

//Arithmetic Shift Left
void MOS6502::InstructionHandler::ASL()
{
	byte data = Fetch();

	// Old bit 7 should be shifted into carry flag
	CPU.SetFlag(Flag::Carry, data & 0x80);

	data = data << 1; // Shift left

	// Set zero flag if result is zero
	CPU.SetFlag(Flag::Zero, data == 0x00);
	// Set negative flag if bit 7 is set
	CPU.SetFlag(Flag::Negative, data & 0x80);

	// If m_doFetch is false, we can safely
	// assume ACC addressing and target
	// the accumulator register
	if (!m_doFetch)
		CPU.Registers.A = data;
	else // Otherwise, target memory address on bus
		CPU.Write(m_cacheAbsoluteWorkingAddress, data);
}

// Logical Shift Right
void MOS6502::InstructionHandler::LSR()
{
	byte data = Fetch();

	// Rightmost bit should be shifted into carry flag
	CPU.SetFlag(Flag::Carry, data & 0x01);

	data = data >> 1; // Shift right

	// Set zero flag if result is zero
	CPU.SetFlag(Flag::Zero, data == 0x00);
	// Set negative flag if bit 7 is set
	CPU.SetFlag(Flag::Negative, data & 0x80);
	
	// If m_doFetch is false, we can safely
	// assume ACC addressing and target
	// the accumulator register
	if (!m_doFetch)
		CPU.Registers.A = data;
	else // Otherwise, target memory address on bus
		CPU.Write(m_cacheAbsoluteWorkingAddress, data);
}

// Rotate Left
void MOS6502::InstructionHandler::ROL()
{
	byte data = Fetch();

	// Store current carry flag
	bool oldCarryFlag = CPU.CheckFlag(Flag::Carry);

	// Bit 7 should be moved into carry flag
	CPU.SetFlag(Flag::Carry, data & 0x80);

	data = data << 1; // Shift left
	data |= (oldCarryFlag ? 0x01 : 0x00); // Put old carry flag in bit 0

	// Set zero flag if result is zero
	CPU.SetFlag(Flag::Zero, data == 0x00);
	// Set negative flag if bit 7 is set
	CPU.SetFlag(Flag::Negative, data & 0x80);

	// If m_doFetch is false, we can safely
	// assume ACC addressing and target
	// the accumulator register
	if (!m_doFetch)
		CPU.Registers.A = data;
	else // Otherwise, target memory address on bus
		CPU.Write(m_cacheAbsoluteWorkingAddress, data);
}

// Rotate Right
void MOS6502::InstructionHandler::ROR()
{
	byte data = Fetch();

	// Store current carry flag
	bool oldCarryFlag = CPU.CheckFlag(Flag::Carry);

	// Bit 0 should be moved into carry flag
	CPU.SetFlag(Flag::Carry, data & 0x01);

	data = data >>  1; // Shift right
	data |= (oldCarryFlag ? 0x80 : 0x00); // Put old carry flag in bit 7

	// Set zero flag if result is zero
	CPU.SetFlag(Flag::Zero, data == 0x00);
	// Set negative flag if bit 7 is set
	CPU.SetFlag(Flag::Negative, data & 0x80);

	// If m_doFetch is false, we can safely
	// assume ACC addressing and target
	// the accumulator register
	if (!m_doFetch)
		CPU.Registers.A = data;
	else // Otherwise, target memory address on bus
		CPU.Write(m_cacheAbsoluteWorkingAddress, data);
}

// Jump (set PC to address specified)
void MOS6502::InstructionHandler::JMP()
{
	CPU.Registers.PC = m_cacheAbsoluteWorkingAddress;
}

// Jump to Subroutine
void MOS6502::InstructionHandler::JSR()
{
	// Push current PC - 1 to stack
	CPU.Registers.PC--;
	byte pcUpperByte = (CPU.Registers.PC >> 8) & 0x00FF;
	byte pcLowerByte = CPU.Registers.PC & 0x00FF;

	CPU.Write(CPU.GetStackPointerAddress(), pcUpperByte);
	CPU.Registers.S--;
	CPU.Write(CPU.GetStackPointerAddress(), pcLowerByte);
	CPU.Registers.S--;

	CPU.Registers.PC = m_cacheAbsoluteWorkingAddress;
}

// Return from Subroutine
void MOS6502::InstructionHandler::RTS()
{
	// Pull current PC + 1 from stack
	CPU.Registers.S++;
	word pc = CPU.Read(CPU.GetStackPointerAddress());
	CPU.Registers.S++;
	pc |= (word)CPU.Read(CPU.GetStackPointerAddress()) << 8;

	CPU.Registers.PC = pc + 1;
}

// Branch if Carry Clear
void MOS6502::InstructionHandler::BCC()
{
	if (!CPU.CheckFlag(Flag::Carry))
	{
		// Additional cycle if branch succeeds
		m_additionalCyclesNeeded++;

		// Additional cycle in case of page boundary pass
		if ((CPU.Registers.PC & 0xFF00) != (m_cacheAbsoluteWorkingAddress & 0xFF00))
			m_additionalCyclesNeeded++;

		CPU.Registers.PC = m_cacheAbsoluteWorkingAddress;
	}
}

// Branch if Carry Set
void MOS6502::InstructionHandler::BCS()
{
	if (CPU.CheckFlag(Flag::Carry))
	{
		// Additional cycle if branch succeeds
		m_additionalCyclesNeeded++;

		// Additional cycle in case of page boundary pass
		if ((CPU.Registers.PC & 0xFF00) != (m_cacheAbsoluteWorkingAddress & 0xFF00))
			m_additionalCyclesNeeded++;

		CPU.Registers.PC = m_cacheAbsoluteWorkingAddress;
	}
}

// Branch if Equal (Zero flag set)
void MOS6502::InstructionHandler::BEQ()
{
	if (CPU.CheckFlag(Flag::Zero))
	{
		// Additional cycle if branch succeeds
		m_additionalCyclesNeeded++;

		// Additional cycle in case of page boundary pass
		if ((CPU.Registers.PC & 0xFF00) != (m_cacheAbsoluteWorkingAddress & 0xFF00))
			m_additionalCyclesNeeded++;

		CPU.Registers.PC = m_cacheAbsoluteWorkingAddress;
	}
}

// Branch if Minus (Negative flag set)
void MOS6502::InstructionHandler::BMI()
{
	if (CPU.CheckFlag(Flag::Negative))
	{
		// Additional cycle if branch succeeds
		m_additionalCyclesNeeded++;

		// Additional cycle in case of page boundary pass
		if ((CPU.Registers.PC & 0xFF00) != (m_cacheAbsoluteWorkingAddress & 0xFF00))
			m_additionalCyclesNeeded++;

		CPU.Registers.PC = m_cacheAbsoluteWorkingAddress;
	}
}

// Branch if Not Equal (Zero flag is clear)
void MOS6502::InstructionHandler::BNE()
{
	if (!CPU.CheckFlag(Flag::Zero))
	{
		// Additional cycle if branch succeeds
		m_additionalCyclesNeeded++;

		// Additional cycle in case of page boundary pass
		if ((CPU.Registers.PC & 0xFF00) != (m_cacheAbsoluteWorkingAddress & 0xFF00))
			m_additionalCyclesNeeded++;

		CPU.Registers.PC = m_cacheAbsoluteWorkingAddress;
	}
}

// Branch if Positive (Negative flag is clear)
void MOS6502::InstructionHandler::BPL()
{
	if (!CPU.CheckFlag(Flag::Negative))
	{
		// Additional cycle if branch succeeds
		m_additionalCyclesNeeded++;

		// Additional cycle in case of page boundary pass
		if ((CPU.Registers.PC & 0xFF00) != (m_cacheAbsoluteWorkingAddress & 0xFF00))
			m_additionalCyclesNeeded++;

		CPU.Registers.PC = m_cacheAbsoluteWorkingAddress;
	}
}

// Branch if Overflow Clear
void MOS6502::InstructionHandler::BVC()
{
	if (!CPU.CheckFlag(Flag::Overflow))
	{
		// Additional cycle if branch succeeds
		m_additionalCyclesNeeded++;

		// Additional cycle in case of page boundary pass
		if ((CPU.Registers.PC & 0xFF00) != (m_cacheAbsoluteWorkingAddress & 0xFF00))
			m_additionalCyclesNeeded++;

		CPU.Registers.PC = m_cacheAbsoluteWorkingAddress;
	}
}

// Branch if Overflow Set
void MOS6502::InstructionHandler::BVS()
{
	if (CPU.CheckFlag(Flag::Overflow))
	{
		// Additional cycle if branch succeeds
		m_additionalCyclesNeeded++;

		// Additional cycle in case of page boundary pass
		if ((CPU.Registers.PC & 0xFF00) != (m_cacheAbsoluteWorkingAddress & 0xFF00))
			m_additionalCyclesNeeded++;

		CPU.Registers.PC = m_cacheAbsoluteWorkingAddress;
	}
}

// Clear Carry Flag
void MOS6502::InstructionHandler::CLC()
{
	CPU.ClearFlag(Flag::Carry);
}

// Clear Decimal Mode
void MOS6502::InstructionHandler::CLD()
{
	CPU.ClearFlag(Flag::DecimalMode);
}

// Clear Interrupt Disable
void MOS6502::InstructionHandler::CLI()
{
	CPU.ClearFlag(Flag::InterruptDisable);
}

// Clear Overflow Flag
void MOS6502::InstructionHandler::CLV()
{
	CPU.ClearFlag(Flag::Overflow);
}

// Set Carry Flag
void MOS6502::InstructionHandler::SEC()
{
	CPU.SetFlag(Flag::Carry);
}

// Set Decimal Mode
void MOS6502::InstructionHandler::SED()
{
	CPU.SetFlag(Flag::DecimalMode);
}

// Set Interrupt Disable
void MOS6502::InstructionHandler::SEI()
{
	CPU.SetFlag(Flag::InterruptDisable);
}