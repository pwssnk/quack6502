#pragma once
#pragma warning (disable:4244)

#include <mutex>
#include "definitions.h"
#include "nes-definitions.h"
#include "bus.h"
#include "util.h"


namespace Qk {namespace NES {

	// APU SAMPLES
	static constexpr int APU_SAMPLERATE_HZ = 44100;
	static constexpr int APU_SAMPLE_BUFFER_SIZE = 2048;
	static constexpr int APU_SAMPLE_INTERVAL_CYCLES = (int)NES_CPU_CLOCK_FREQ / (double)APU_SAMPLERATE_HZ;

	class APU : public Bus::Device
	{
	public:
		struct
		{
			bool EnableDMC = false;
			bool EnableNoise = false;
			bool EnableTriangle = false;
			bool EnablePulse1 = false;
			bool EnablePulse2 = false;
			bool DMCInterrupt = false;
		} Status;

		struct
		{
			byte Period = 4; 
			bool IRQInhibit = false;
			int Count = 0;
		} FrameCounter;

	public:
		APU(Bus& bus);
		APU(Bus& bus, const AddressRange& addressableRange);

		void Reset();
		void Cycle();
		 
		int GetAudioBufferSize() const;
		double GetAudioSampleRate() const;
		void FillAudioBuffer(audiosample* buffer, size_t numSamples);

		byte ReadFromDevice(word address, bool peek = false) override;
		void WriteToDevice(word address, byte data) override;
		void OnBusSignal(int signalId) override;

	protected:
		void Initialize();
		void PopulateMixerLookupTables();
		
		audiosample MixSample();
		bool SampleClock();
		void UpdateFrameCounter();
		bool FrameCounterClock();

	protected:
		// APU Channels
		class PulseChannel
		{
		public:
			// Waveform generator data
			byte DutyCycle = 0;
			byte SequencerStep = 0;

			byte LengthCounter = 0;
			bool LengthCounterHalt = false;

			bool MaintainConstantVolume = false;
			byte ConstantVolumeLevel = 0;
	
			bool EnvelopeStart = true;
			byte EnvelopePeriod = 0;
			byte EnvelopeCounter = 0;
			byte EnvelopeDecay = 0;

			bool SweepEnabled = false;
			bool SweepStart = true;
			bool SweepNegate = false;
			byte SweepPeriod = 0;
			byte SweepCounter = 0;
			byte SweepShift = 0;

			word TimerCounter = 0;
			word TimerPeriod = 0;
		public:
			PulseChannel(APU& apu, int channelId) : APU(apu), m_pulseChId(channelId) {};

			byte Output();
			void UpdateTimer();
			void UpdateEnvelope();
			void UpdateLength();
			void UpdateSweep();

			void WriteRegisterControl(byte data);
			void WriteRegisterSweep(byte data);
			void WriteRegisterTimerLow(byte data);
			void WriteRegisterTimerHigh(byte data);

		protected:
			APU& APU;
			int m_pulseChId;
		} ChPulse1, ChPulse2;

		class TriangleChannel
		{
		public:
			byte SequencerStep = 0;

			byte LinearCounter = 0;
			byte LinearCounterPeriod = 0;
			bool LinearCounterStart = false;

			byte LengthCounter = 0;
			bool LengthCounterHalt = false;

			word TimerCounter = 0;
			word TimerPeriod = 0;

		public:
			TriangleChannel(APU& parent) : APU(parent) {};

			byte Output();
			void UpdateTimer();
			void UpdateLinearCounter();
			void UpdateLength();

			void WriteRegisterControl(byte data);
			void WriteRegisterTimerLow(byte data);
			void WriteRegisterTimerHigh(byte data);

		protected:
			APU& APU;
		} ChTriangle;

		class NoiseChannel
		{
		public:
			bool Mode = false;
			
			word ShiftRegister = 1;

			bool EnvelopeStart = true;
			byte EnvelopePeriod = 0;
			byte EnvelopeCounter = 0;
			byte EnvelopeDecay = 0;

			bool MaintainConstantVolume = false;
			byte ConstantVolumeLevel = 0;

			byte LengthCounter = 0;
			bool LengthCounterHalt = false;

			word TimerCounter = 0;
			word TimerPeriod = 0;

		public:
			NoiseChannel(APU& parent) : APU(parent) {};

			void WriteRegisterControl(byte data);
			void WriteRegisterPeriod(byte data);
			void WriteRegisterLength(byte data);

			byte Output();
			void UpdateTimer();
			void UpdateEnvelope();
			void UpdateLength();

		protected:
			APU& APU;
		} ChNoise;

		// APU lookup tables
		struct
		{
			word Pulse[31] = { 0 };
			word TND[203] = { 0 };
		} m_mixtables;

		const byte m_tableLength[32] = {
			10,  254, 20,  2,   40,  4,   80,  6,
			160, 8,   60,  10,  14,  12,  26,  14,
			12,  16,  24,  18, 	48,  20,  96,  22,
			192, 24,  72,  26,  16,  28,  32,  30
		};

		const byte m_tableDuty[4][8] = {
			{0, 1, 0, 0, 0, 0, 0, 0},
			{0, 1, 1, 0, 0, 0, 0, 0},
			{0, 1, 1, 1, 1, 0, 0, 0},
			{1, 0, 0, 1, 1, 1, 1, 1}
		};

		const byte m_tableTriangle[64] = {
			15, 14, 13, 12, 11, 10, 9,  8,
			7,  6,  5,  4,  3,  2,  1,  0,
			0,  1,  2,  3,  4,  5,  6,  7,
			8,  9,  10, 11, 12, 13, 14, 15
		};

		const byte m_tableDMC[16] = {
			214, 190, 170, 160, 143, 127, 113, 107,
			95,  80,  71,  64,  53,  42,  36,  27
		};

		const word m_tableNoise[16] = {
			4,   8,   16,  32,  64,  96,   128,  160,
			202, 254, 380, 508, 762, 1016, 2034, 4068
		};

		// Frame counter updates
		int m_fcUpdateInterval = (int)(NES_CPU_CLOCK_FREQ / 240.0);
		int m_fcUpdateCounter = 0;
		bool m_updateLengths = true;


		// APU audio sample buffer
		Util::CircularBuffer<audiosample> m_audiobuffer;
		std::mutex m_buffermutex;
		int m_sampleInterval = APU_SAMPLE_INTERVAL_CYCLES;
		int m_sampleIntervalCounter = 0;
		
		// PPU OAM DMA forward -- not APU related
		// but APU memory range occupies DMA register,
		// so we handle it here for convenience
		byte m_addressDMA = 0;
	};
}}