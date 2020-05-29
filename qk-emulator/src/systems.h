#pragma once

#include <memory>
#include "definitions.h"
#include "bus.h"
#include "cpu.h"
#include "memory.h"
#include "mem-mirror.h"
#include "nes-ppu.h"
#include "nes-cartridge.h"
#include "nes-controller.h"
#include "nes-apu.h"


namespace Qk {
	namespace NES
	{
		class NESConsole
		{
		protected:
			// Some of these objects would use a significant amount
			// of memory on stack, so dynamically allocate to put
			// them on heap instead
			Bus* m_bus = nullptr;
			MOS6502* m_cpu = nullptr;
			RAM* m_ram = nullptr;
			MemoryMirror* m_rmm = nullptr;
			RP2C02* m_ppu = nullptr;
			MemoryMirror* m_pmm = nullptr;
			APU* m_apu = nullptr;
			CartridgeSlot* m_cas = nullptr;
			ControllerInterface* m_ctr = nullptr;

			unsigned long m_systemClockCount = 0;
			FramebufferDescriptor* m_ppu_ps = nullptr;

		public:
			NESConsole();
			~NESConsole();

			void Clock();
			void Reset();
			void Reset(word programCounter);
			void InsertCartridge(const std::shared_ptr<Cartridge>& cart);

			// Video
			FramebufferDescriptor* GetVideoOutput();
			unsigned long GetPPUFrameCount() const;

			// Audio
			void FillAudioBuffer(audiosample* buffer, size_t numSamples);
			int GetAudioBufferSize() const;
			double GetAudioSampleRate() const;

			// Controller inputs
			void ControllerInput(Controller::Player pad, Controller::Button button, bool pressed);

#ifdef _DEBUG
			// DEBUG
			void PrintMemory(word addressStart, word addressEnd);
			void DumpCPUFrameLog(const std::string& path);
#endif
		};
	}
}