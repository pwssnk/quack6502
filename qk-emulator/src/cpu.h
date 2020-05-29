#pragma once
#pragma warning (disable:26812)

#include <functional>
#include <string>
#include "definitions.h"
#include "bus.h"


namespace Qk
{
	class MOS6502 : public Bus::Device
	{
	public:
		struct
		{
			byte A;		// Accumulator
			byte X;		// Index X
			byte Y;		// Index Y
			word PC;	// Program Counter
			byte P;		// Process Status (flags)
			byte S;		// Stack Pointer
		} Registers;

		enum class Flag
		{
			Carry            = 0x01,
			Zero             = 0x02,
			InterruptDisable = 0x04,
			DecimalMode      = 0x08,
			Break            = 0x10,
			Expansion        = 0x20,
			Overflow         = 0x40,
			Negative         = 0x80
		};

	public:
		MOS6502(Bus& bus);
		~MOS6502();

		// CPU control
		void Reset();
		void Reset(word programCounter);
		void Cycle();
		void SetDecimalModeAvailable(bool available);

		// CPU status
		word GetStackPointerAddress() const;
		unsigned long GetCPUCycleCount() const;

		bool CheckFlag(Flag flag) const;
		void SetFlag(Flag flag);
		void SetFlag(Flag flag, bool state);
		void ClearFlag(Flag flag);
		void ClearFlags();

		// System interrupts
		void GenerateInterrupt();
		void GenerateNonMaskableInterrupt();

		// Bus signal listener
		void OnBusSignal(int signalId) override;

#ifdef _DEBUG
		// DEBUG
		void PrintDebugInfo() const;
		void SaveDebugInfo(const std::string& logfile) const;
#endif
	protected:
		class InstructionHandler
		{
		public:
			InstructionHandler(MOS6502& parent);

			void ExecuteNextInstruction();

			byte GetLastInstructionOpcode() const;
			int GetLastInstructionCycles() const;
			word GetLastInstructionAddress() const;
			byte GetLastInstructionValue() const;
			std::string GetLastInstructionMnemonic() const;
			std::string GetLastInstructionAddressingModeMnemonic() const;

			// System interrupt signals
			void IRQ();
			void NMI();

		protected:

#ifdef _DEBUG
			// DEBUG
			friend class CPUDebugLogger;
#endif
			// Reference to parent object
			MOS6502& CPU;

			// Shorthand for addressing/op methods function pointer
			typedef void(MOS6502::InstructionHandler::* FuncPtr)();

			// Instruction definition format
			struct Instruction
			{
				Instruction(const char* mnemonic, byte opcode, int baseCycles, FuncPtr addressingFunction, FuncPtr operationFunction)
					: Mnemonic(mnemonic), Opcode(opcode), BaseCycles(baseCycles), AdressingFunction(addressingFunction), OperationFunction(operationFunction) { }

				const char* Mnemonic;
				const byte Opcode;
				const int BaseCycles;
				const FuncPtr AdressingFunction;
				const FuncPtr OperationFunction;
			};

			// Opcode map
			using o = InstructionHandler;
			using i = Instruction;
			Instruction OpcodeMap[256] = {
				i("BRK", 0x00, 7, &o::IMM, &o::BRK), i("ORA", 0x01, 6, &o::IZX, &o::ORA), i("XXX", 0x02, 2, &o::IMP, &o::XXX), i("XXX", 0x03, 8, &o::IMP, &o::XXX),
				i("XXX", 0x04, 3, &o::IMP, &o::NOP), i("ORA", 0x05, 3, &o::ZP0, &o::ORA), i("ASL", 0x06, 5, &o::ZP0, &o::ASL), i("XXX", 0x07, 5, &o::IMP, &o::XXX),
				i("PHP", 0x08, 3, &o::IMP, &o::PHP), i("ORA", 0x09, 2, &o::IMM, &o::ORA), i("ASL", 0x0a, 2, &o::ACC, &o::ASL), i("XXX", 0x0b, 2, &o::IMP, &o::XXX),
				i("XXX", 0x0c, 4, &o::IMP, &o::NOP), i("ORA", 0x0d, 4, &o::ABS, &o::ORA), i("ASL", 0x0e, 6, &o::ABS, &o::ASL), i("XXX", 0x0f, 6, &o::IMP, &o::XXX),
				i("BPL", 0x10, 2, &o::REL, &o::BPL), i("ORA", 0x11, 5, &o::IZY, &o::ORA), i("XXX", 0x12, 2, &o::IMP, &o::XXX), i("XXX", 0x13, 8, &o::IMP, &o::XXX),
				i("XXX", 0x14, 4, &o::IMP, &o::NOP), i("ORA", 0x15, 4, &o::ZPX, &o::ORA), i("ASL", 0x16, 6, &o::ZPX, &o::ASL), i("XXX", 0x17, 6, &o::IMP, &o::XXX),
				i("CLC", 0x18, 2, &o::IMP, &o::CLC), i("ORA", 0x19, 4, &o::ABY, &o::ORA), i("XXX", 0x1a, 2, &o::IMP, &o::NOP), i("XXX", 0x1b, 7, &o::IMP, &o::XXX),
				i("XXX", 0x1c, 4, &o::IMP, &o::NOP), i("ORA", 0x1d, 4, &o::ABX, &o::ORA), i("ASL", 0x1e, 7, &o::ABX, &o::ASL), i("XXX", 0x1f, 7, &o::IMP, &o::XXX),
				i("JSR", 0x20, 6, &o::ABS, &o::JSR), i("AND", 0x21, 6, &o::IZX, &o::AND), i("XXX", 0x22, 2, &o::IMP, &o::XXX), i("XXX", 0x23, 8, &o::IMP, &o::XXX),
				i("BIT", 0x24, 3, &o::ZP0, &o::BIT), i("AND", 0x25, 3, &o::ZP0, &o::AND), i("ROL", 0x26, 5, &o::ZP0, &o::ROL), i("XXX", 0x27, 5, &o::IMP, &o::XXX),
				i("PLP", 0x28, 4, &o::IMP, &o::PLP), i("AND", 0x29, 2, &o::IMM, &o::AND), i("ROL", 0x2a, 2, &o::ACC, &o::ROL), i("XXX", 0x2b, 2, &o::IMP, &o::XXX),
				i("BIT", 0x2c, 4, &o::ABS, &o::BIT), i("AND", 0x2d, 4, &o::ABS, &o::AND), i("ROL", 0x2e, 6, &o::ABS, &o::ROL), i("XXX", 0x2f, 6, &o::IMP, &o::XXX),
				i("BMI", 0x30, 2, &o::REL, &o::BMI), i("AND", 0x31, 5, &o::IZY, &o::AND), i("XXX", 0x32, 2, &o::IMP, &o::XXX), i("XXX", 0x33, 8, &o::IMP, &o::XXX),
				i("XXX", 0x34, 4, &o::IMP, &o::NOP), i("AND", 0x35, 4, &o::ZPX, &o::AND), i("ROL", 0x36, 6, &o::ZPX, &o::ROL), i("XXX", 0x37, 6, &o::IMP, &o::XXX),
				i("SEC", 0x38, 2, &o::IMP, &o::SEC), i("AND", 0x39, 4, &o::ABY, &o::AND), i("XXX", 0x3a, 2, &o::IMP, &o::NOP), i("XXX", 0x3b, 7, &o::IMP, &o::XXX),
				i("XXX", 0x3c, 4, &o::IMP, &o::NOP), i("AND", 0x3d, 4, &o::ABX, &o::AND), i("ROL", 0x3e, 7, &o::ABX, &o::ROL), i("XXX", 0x3f, 7, &o::IMP, &o::XXX),
				i("RTI", 0x40, 6, &o::IMP, &o::RTI), i("EOR", 0x41, 6, &o::IZX, &o::EOR), i("XXX", 0x42, 2, &o::IMP, &o::XXX), i("XXX", 0x43, 8, &o::IMP, &o::XXX),
				i("XXX", 0x44, 3, &o::IMP, &o::NOP), i("EOR", 0x45, 3, &o::ZP0, &o::EOR), i("LSR", 0x46, 5, &o::ZP0, &o::LSR), i("XXX", 0x47, 5, &o::IMP, &o::XXX),
				i("PHA", 0x48, 3, &o::IMP, &o::PHA), i("EOR", 0x49, 2, &o::IMM, &o::EOR), i("LSR", 0x4a, 2, &o::ACC, &o::LSR), i("XXX", 0x4b, 2, &o::IMP, &o::XXX),
				i("JMP", 0x4c, 3, &o::ABS, &o::JMP), i("EOR", 0x4d, 4, &o::ABS, &o::EOR), i("LSR", 0x4e, 6, &o::ABS, &o::LSR), i("XXX", 0x4f, 6, &o::IMP, &o::XXX),
				i("BVC", 0x50, 2, &o::REL, &o::BVC), i("EOR", 0x51, 5, &o::IZY, &o::EOR), i("XXX", 0x52, 2, &o::IMP, &o::XXX), i("XXX", 0x53, 8, &o::IMP, &o::XXX),
				i("XXX", 0x54, 4, &o::IMP, &o::NOP), i("EOR", 0x55, 4, &o::ZPX, &o::EOR), i("LSR", 0x56, 6, &o::ZPX, &o::LSR), i("XXX", 0x57, 6, &o::IMP, &o::XXX),
				i("CLI", 0x58, 2, &o::IMP, &o::CLI), i("EOR", 0x59, 4, &o::ABY, &o::EOR), i("XXX", 0x5a, 2, &o::IMP, &o::NOP), i("XXX", 0x5b, 7, &o::IMP, &o::XXX),
				i("XXX", 0x5c, 4, &o::IMP, &o::NOP), i("EOR", 0x5d, 4, &o::ABX, &o::EOR), i("LSR", 0x5e, 7, &o::ABX, &o::LSR), i("XXX", 0x5f, 7, &o::IMP, &o::XXX),
				i("RTS", 0x60, 6, &o::IMP, &o::RTS), i("ADC", 0x61, 6, &o::IZX, &o::ADC), i("XXX", 0x62, 2, &o::IMP, &o::XXX), i("XXX", 0x63, 8, &o::IMP, &o::XXX),
				i("XXX", 0x64, 3, &o::IMP, &o::NOP), i("ADC", 0x65, 3, &o::ZP0, &o::ADC), i("ROR", 0x66, 5, &o::ZP0, &o::ROR), i("XXX", 0x67, 5, &o::IMP, &o::XXX),
				i("PLA", 0x68, 4, &o::IMP, &o::PLA), i("ADC", 0x69, 2, &o::IMM, &o::ADC), i("ROR", 0x6a, 2, &o::ACC, &o::ROR), i("XXX", 0x6b, 2, &o::IMP, &o::XXX),
				i("JMP", 0x6c, 5, &o::IND, &o::JMP), i("ADC", 0x6d, 4, &o::ABS, &o::ADC), i("ROR", 0x6e, 6, &o::ABS, &o::ROR), i("XXX", 0x6f, 6, &o::IMP, &o::XXX),
				i("BVS", 0x70, 2, &o::REL, &o::BVS), i("ADC", 0x71, 5, &o::IZY, &o::ADC), i("XXX", 0x72, 2, &o::IMP, &o::XXX), i("XXX", 0x73, 8, &o::IMP, &o::XXX),
				i("XXX", 0x74, 4, &o::IMP, &o::NOP), i("ADC", 0x75, 4, &o::ZPX, &o::ADC), i("ROR", 0x76, 6, &o::ZPX, &o::ROR), i("XXX", 0x77, 6, &o::IMP, &o::XXX),
				i("SEI", 0x78, 2, &o::IMP, &o::SEI), i("ADC", 0x79, 4, &o::ABY, &o::ADC), i("XXX", 0x7a, 2, &o::IMP, &o::NOP), i("XXX", 0x7b, 7, &o::IMP, &o::XXX),
				i("XXX", 0x7c, 4, &o::IMP, &o::NOP), i("ADC", 0x7d, 4, &o::ABX, &o::ADC), i("ROR", 0x7e, 7, &o::ABX, &o::ROR), i("XXX", 0x7f, 7, &o::IMP, &o::XXX),
				i("XXX", 0x80, 2, &o::IMP, &o::NOP), i("STA", 0x81, 6, &o::IZX, &o::STA), i("XXX", 0x82, 2, &o::IMP, &o::NOP), i("XXX", 0x83, 6, &o::IMP, &o::XXX),
				i("STY", 0x84, 3, &o::ZP0, &o::STY), i("STA", 0x85, 3, &o::ZP0, &o::STA), i("STX", 0x86, 3, &o::ZP0, &o::STX), i("XXX", 0x87, 3, &o::IMP, &o::XXX),
				i("DEY", 0x88, 2, &o::IMP, &o::DEY), i("XXX", 0x89, 2, &o::IMP, &o::NOP), i("TXA", 0x8a, 2, &o::IMP, &o::TXA), i("XXX", 0x8b, 2, &o::IMP, &o::XXX),
				i("STY", 0x8c, 4, &o::ABS, &o::STY), i("STA", 0x8d, 4, &o::ABS, &o::STA), i("STX", 0x8e, 4, &o::ABS, &o::STX), i("XXX", 0x8f, 4, &o::IMP, &o::XXX),
				i("BCC", 0x90, 2, &o::REL, &o::BCC), i("STA", 0x91, 6, &o::IZY, &o::STA), i("XXX", 0x92, 2, &o::IMP, &o::XXX), i("XXX", 0x93, 6, &o::IMP, &o::XXX),
				i("STY", 0x94, 4, &o::ZPX, &o::STY), i("STA", 0x95, 4, &o::ZPX, &o::STA), i("STX", 0x96, 4, &o::ZPY, &o::STX), i("XXX", 0x97, 4, &o::IMP, &o::XXX),
				i("TYA", 0x98, 2, &o::IMP, &o::TYA), i("STA", 0x99, 5, &o::ABY, &o::STA), i("TXS", 0x9a, 2, &o::IMP, &o::TXS), i("XXX", 0x9b, 5, &o::IMP, &o::XXX),
				i("XXX", 0x9c, 5, &o::IMP, &o::NOP), i("STA", 0x9d, 5, &o::ABX, &o::STA), i("XXX", 0x9e, 5, &o::IMP, &o::XXX), i("XXX", 0x9f, 5, &o::IMP, &o::XXX),
				i("LDY", 0xa0, 2, &o::IMM, &o::LDY), i("LDA", 0xa1, 6, &o::IZX, &o::LDA), i("LDX", 0xa2, 2, &o::IMM, &o::LDX), i("XXX", 0xa3, 6, &o::IMP, &o::XXX),
				i("LDY", 0xa4, 3, &o::ZP0, &o::LDY), i("LDA", 0xa5, 3, &o::ZP0, &o::LDA), i("LDX", 0xa6, 3, &o::ZP0, &o::LDX), i("XXX", 0xa7, 3, &o::IMP, &o::XXX),
				i("TAY", 0xa8, 2, &o::IMP, &o::TAY), i("LDA", 0xa9, 2, &o::IMM, &o::LDA), i("TAX", 0xaa, 2, &o::IMP, &o::TAX), i("XXX", 0xab, 2, &o::IMP, &o::XXX),
				i("LDY", 0xac, 4, &o::ABS, &o::LDY), i("LDA", 0xad, 4, &o::ABS, &o::LDA), i("LDX", 0xae, 4, &o::ABS, &o::LDX), i("XXX", 0xaf, 4, &o::IMP, &o::XXX),
				i("BCS", 0xb0, 2, &o::REL, &o::BCS), i("LDA", 0xb1, 5, &o::IZY, &o::LDA), i("XXX", 0xb2, 2, &o::IMP, &o::XXX), i("XXX", 0xb3, 5, &o::IMP, &o::XXX),
				i("LDY", 0xb4, 4, &o::ZPX, &o::LDY), i("LDA", 0xb5, 4, &o::ZPX, &o::LDA), i("LDX", 0xb6, 4, &o::ZPY, &o::LDX), i("XXX", 0xb7, 4, &o::IMP, &o::XXX),
				i("CLV", 0xb8, 2, &o::IMP, &o::CLV), i("LDA", 0xb9, 4, &o::ABY, &o::LDA), i("TSX", 0xba, 2, &o::IMP, &o::TSX), i("XXX", 0xbb, 4, &o::IMP, &o::XXX),
				i("LDY", 0xbc, 4, &o::ABX, &o::LDY), i("LDA", 0xbd, 4, &o::ABX, &o::LDA), i("LDX", 0xbe, 4, &o::ABY, &o::LDX), i("XXX", 0xbf, 4, &o::IMP, &o::XXX),
				i("CPY", 0xc0, 2, &o::IMM, &o::CPY), i("CMP", 0xc1, 6, &o::IZX, &o::CMP), i("XXX", 0xc2, 2, &o::IMP, &o::NOP), i("XXX", 0xc3, 8, &o::IMP, &o::XXX),
				i("CPY", 0xc4, 3, &o::ZP0, &o::CPY), i("CMP", 0xc5, 3, &o::ZP0, &o::CMP), i("DEC", 0xc6, 5, &o::ZP0, &o::DEC), i("XXX", 0xc7, 5, &o::IMP, &o::XXX),
				i("INY", 0xc8, 2, &o::IMP, &o::INY), i("CMP", 0xc9, 2, &o::IMM, &o::CMP), i("DEX", 0xca, 2, &o::IMP, &o::DEX), i("XXX", 0xcb, 2, &o::IMP, &o::XXX),
				i("CPY", 0xcc, 4, &o::ABS, &o::CPY), i("CMP", 0xcd, 4, &o::ABS, &o::CMP), i("DEC", 0xce, 6, &o::ABS, &o::DEC), i("XXX", 0xcf, 6, &o::IMP, &o::XXX),
				i("BNE", 0xd0, 2, &o::REL, &o::BNE), i("CMP", 0xd1, 5, &o::IZY, &o::CMP), i("XXX", 0xd2, 2, &o::IMP, &o::XXX), i("XXX", 0xd3, 8, &o::IMP, &o::XXX),
				i("XXX", 0xd4, 4, &o::IMP, &o::NOP), i("CMP", 0xd5, 4, &o::ZPX, &o::CMP), i("DEC", 0xd6, 6, &o::ZPX, &o::DEC), i("XXX", 0xd7, 6, &o::IMP, &o::XXX),
				i("CLD", 0xd8, 2, &o::IMP, &o::CLD), i("CMP", 0xd9, 4, &o::ABY, &o::CMP), i("NOP", 0xda, 2, &o::IMP, &o::NOP), i("XXX", 0xdb, 7, &o::IMP, &o::XXX),
				i("XXX", 0xdc, 4, &o::IMP, &o::NOP), i("CMP", 0xdd, 4, &o::ABX, &o::CMP), i("DEC", 0xde, 7, &o::ABX, &o::DEC), i("XXX", 0xdf, 7, &o::IMP, &o::XXX),
				i("CPX", 0xe0, 2, &o::IMM, &o::CPX), i("SBC", 0xe1, 6, &o::IZX, &o::SBC), i("XXX", 0xe2, 2, &o::IMP, &o::NOP), i("XXX", 0xe3, 8, &o::IMP, &o::XXX),
				i("CPX", 0xe4, 3, &o::ZP0, &o::CPX), i("SBC", 0xe5, 3, &o::ZP0, &o::SBC), i("INC", 0xe6, 5, &o::ZP0, &o::INC), i("XXX", 0xe7, 5, &o::IMP, &o::XXX),
				i("INX", 0xe8, 2, &o::IMP, &o::INX), i("SBC", 0xe9, 2, &o::IMM, &o::SBC), i("NOP", 0xea, 2, &o::IMP, &o::NOP), i("XXX", 0xeb, 2, &o::IMP, &o::SBC),
				i("CPX", 0xec, 4, &o::ABS, &o::CPX), i("SBC", 0xed, 4, &o::ABS, &o::SBC), i("INC", 0xee, 6, &o::ABS, &o::INC), i("XXX", 0xef, 6, &o::IMP, &o::XXX),
				i("BEQ", 0xf0, 2, &o::REL, &o::BEQ), i("SBC", 0xf1, 5, &o::IZY, &o::SBC), i("XXX", 0xf2, 2, &o::IMP, &o::XXX), i("XXX", 0xf3, 8, &o::IMP, &o::XXX),
				i("XXX", 0xf4, 4, &o::IMP, &o::NOP), i("SBC", 0xf5, 4, &o::ZPX, &o::SBC), i("INC", 0xf6, 6, &o::ZPX, &o::INC), i("XXX", 0xf7, 6, &o::IMP, &o::XXX),
				i("SED", 0xf8, 2, &o::IMP, &o::SED), i("SBC", 0xf9, 4, &o::ABY, &o::SBC), i("NOP", 0xfa, 2, &o::IMP, &o::NOP), i("XXX", 0xfb, 7, &o::IMP, &o::XXX),
				i("XXX", 0xfc, 4, &o::IMP, &o::NOP), i("SBC", 0xfd, 4, &o::ABX, &o::SBC), i("INC", 0xfe, 7, &o::ABX, &o::INC), i("XXX", 0xff, 7, &o::IMP, &o::XXX)
			};

			// Data cache for use accross "micro-ops" within a single instruction
			byte m_opcode = 0xEA; // Default to NOP
			word m_cacheAbsoluteWorkingAddress = 0;
			byte m_cacheFetchedData = 0;
			bool m_doFetch = true;
			int m_additionalCyclesNeeded = 0;

			bool m_didIRQ = false;
			bool m_didNMI = false;

			// Common functionality
			void CallFuncPtr(FuncPtr ptr);
			byte Fetch();

			// Addressing Modes
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

			// Operations
			void XXX(); // Unused or unimplemented opcode

			void NOP(); // No op
			void BRK(); // Force interrupt
			void RTI(); // Return form interrupt

			void LDA(); // Load A
			void LDX(); // Load X
			void LDY(); // Load Y
			void STA(); // Store A
			void STX(); // Store X
			void STY(); // Store Y

			void TAX(); // Transfer A -> X
			void TAY(); // Transfer A -> Y
			void TXA(); // Transfer X -> A
			void TYA(); // Transfer Y -> A

			void TSX(); // Transfer SP -> X
			void TXS(); // Transfer X -> SP
			void PHA(); // Push A on stack
			void PHP(); // Push P on stack
			void PLA(); // Pull A from stack
			void PLP(); // Pull P from stack

			void AND(); // Logical AND
			void EOR(); // Exclusive OR
			void ORA(); // Logical Inclusive OR
			void BIT(); // Bit test

			void ADC(); // Add with carry
			void SBC(); // Subtract with carry
			void CMP(); // Compare A
			void CPX(); // Compare X
			void CPY(); // Compare Y

			void INC(); // Increment memory location
			void INX(); // Increment X
			void INY(); // Increment Y
			void DEC(); // Decrement memory location
			void DEX(); // Decrement X
			void DEY(); // Decrement Y

			void ASL(); // Arithmetic shift left
			void LSR(); // Logical shift right
			void ROL(); // Rotate left
			void ROR(); // Rotate right

			void JMP(); // Jump
			void JSR(); // Jump to subroutine
			void RTS(); // Return from subroutine

			void BCC(); // Branch if carry flag clear
			void BCS(); // Branch if carry flag set
			void BEQ(); // Branch if zero flag set
			void BMI(); // Branch if negative flag set
			void BNE(); // Branch if zero flag clear
			void BPL(); // Branch if negative flag clear
			void BVC(); // Branch if overflow flag clear
			void BVS(); // Branch if overflow flag set

			void CLC(); // Clear carry flag
			void CLD(); // Clear decimal mode flag
			void CLI(); // Clear interrupt disable flag
			void CLV(); // Clear overflow flag
			void SEC(); // Set carry flag
			void SED(); // Set decimal mode flag
			void SEI(); // Set interrupt disable flag
		};
		friend class InstructionHandler;
	protected:
#ifdef _DEBUG
		// DEBUG
		friend class CPUDebugLogger;
		CPUDebugLogger* LOGGER;
#endif

		// Instruction handler object
		InstructionHandler m_instructionHandler;

		// Remaing cycles for current op
		int m_remainingCycles = 0;

		// Decimal mode hardware available?
		bool m_decimalModeAvailable = true;

		// CPU cycle tracker
		unsigned long m_cpuCycleCount = 0;

		// Status and pending interrupt requests
		// Deal with at next CPU clock cycle
		bool m_irqPending = false;
		bool m_nmiPending = false;
		bool m_halted = false;

		// Bus I/O
		byte Read(word address);
		void Write(word address, byte data);
	};

	/*
		DEBUGGING

			100% "Dirtiest logging solution you've ever seen" guarantee
	*/

#ifdef _DEBUG

	class CPUDebugFrame
	{
	public:
		struct CPUState {
			int A = 0;
			int X = 0;
			int Y = 0;
			int P = 0;
			int S = 0;
			int PC = 0;
			int AddressedLocation = 0;
			int FetchedData = 0;
		} PreOpState, PostOpState;

		struct ReadWriteEvent
		{
			bool IsWrite = false;
			int Address = 0;
			int Value = 0;
		};

		unsigned long Cycle = 0;
		int Opcode = 0;
		std::string OpcodeMnemonic;
		std::string AddressingModeMnemonic;
		std::vector<ReadWriteEvent*> IOEvents;

	public:
		CPUDebugFrame();
		~CPUDebugFrame();

		std::string ToString() const;
		std::string ToJSONString() const;
	};

	class CPUDebugLogger
	{
	public:
		std::vector<CPUDebugFrame*> Frames;
		CPUDebugFrame* CurrentFrame;

	public:
		CPUDebugLogger(MOS6502& cpu);
		~CPUDebugLogger();

		void NewFrame();
		void RecordIOEvent(word address, byte value, bool isWrite);
		void RecordPreOpCPUState();
		void RecordPostOpCPUState();

		void PrintFrameDebugInfo() const;
		void SaveDebugInfo(const std::string& filepath) const;

	protected:
		MOS6502& m_cpu;
	};

#endif
}