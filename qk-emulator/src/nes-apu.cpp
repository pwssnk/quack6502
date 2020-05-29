#include "nes-apu.h"
#include <iostream>


using namespace Qk;
using namespace Qk::NES;


/*
	Constructors, intitializtion
*/

APU::APU(Bus& bus)
	: Bus::Device(bus, true, AddressRange(0x4000, 0x4015)),
	  ChPulse1(*this, 1), ChPulse2(*this, 2), ChTriangle(*this),
	  ChNoise(*this), m_audiobuffer(APU_SAMPLE_BUFFER_SIZE)
{
	Initialize();
}

APU::APU(Bus& bus, const AddressRange& addressableRange)
	: Bus::Device(bus, true, AddressRange(0x4000, 0x4015)),
	  ChPulse1(*this, 1), ChPulse2(*this, 2), ChTriangle(*this),
	  ChNoise(*this), m_audiobuffer(APU_SAMPLE_BUFFER_SIZE)
{
	Initialize();
}

void APU::Initialize()
{
	PopulateMixerLookupTables();
	Reset();
}

void APU::Reset()
{
	Status.EnableDMC = false;
	Status.EnableNoise = false;
	Status.EnableTriangle = false;
	Status.EnablePulse1 = false;
	Status.EnablePulse2 = false;

	Status.DMCInterrupt = false;
}

/*
	APU Output
*/

int APU::GetAudioBufferSize() const
{
	return APU_SAMPLE_BUFFER_SIZE;
}

double APU::GetAudioSampleRate() const
{
	return APU_SAMPLERATE_HZ;
}

void APU::FillAudioBuffer(audiosample* buffer, size_t numSamples)
{

	// Buffer may be accessed from different thread
	// that is feeding the audio device, so acquire
	// a lock before checking/changing the buffer's state
	std::lock_guard<std::mutex> lock(m_buffermutex);

	if (numSamples > m_audiobuffer.GetSize())
	{
		// Will likely be thrown in a thread other than main thread,
		// which is inconvenient -- renderer implementation will have 
		// to deal with that (or better yet: just not cause silly errors)
		throw QkError("APU error: incompatible buffer size", 710);
	}

	m_audiobuffer.Copy(buffer, numSamples);
}


/*
	APU cycle
*/

void APU::Cycle()
{
#ifndef NES_AUDIO_ENABLED	// APU code does not work as of now, so audio is disabled
	return;
#endif

	// CHANNEL TIMERS
	if (m_updateLengths)
	{
		m_updateLengths = false;

		ChPulse1.UpdateTimer();
		ChPulse2.UpdateTimer();
		ChTriangle.UpdateTimer();
		ChNoise.UpdateTimer();
	}
	else
	{
		m_updateLengths = true;

		ChTriangle.UpdateTimer();
	}

	// FRAME COUNTER
	if (FrameCounterClock())
	{
		UpdateFrameCounter();
	}

	// GENERATE SAMPLE
	if (SampleClock())
	{
		audiosample sample = MixSample();

		// Buffer may be accessed from different thread
		// that is feeding the audio device, so acquire
		// a lock before changing the buffer's state
		std::unique_lock<std::mutex> lock(m_buffermutex);
		m_audiobuffer.Push(sample);
		lock.unlock();
	}
}

bool APU::FrameCounterClock()
{
	if (m_fcUpdateCounter == 0)
	{
		m_fcUpdateCounter = m_fcUpdateInterval;
		return true;
	}
	else
	{
		m_fcUpdateCounter--;
		return false;
	}
}

void APU::UpdateFrameCounter()
{
	// Increment and reset
	if (FrameCounter.Count < FrameCounter.Period)
		FrameCounter.Count++;
	else
		FrameCounter.Count = 0;

	auto envelopes = [&]() 
	{
		ChPulse1.UpdateEnvelope();
		ChPulse2.UpdateEnvelope();
		ChTriangle.UpdateLinearCounter();
		ChNoise.UpdateEnvelope();
	};

	auto lengths = [&]()
	{
		ChPulse1.UpdateLength();
		ChPulse2.UpdateLength();
		ChTriangle.UpdateLength();
		ChNoise.UpdateLength();
	};

	auto sweeps = [&]()
	{
		ChPulse1.UpdateSweep();
		ChPulse2.UpdateSweep();
	};

	auto do_irq = [&]()
	{
		if (!FrameCounter.IRQInhibit)
			BUS.EmitSignal(SIGNAL_CPU_IRQ);
	};

	/*
		mode 0:    mode 1:       function
		---------  -----------  -----------------------------
		- - - f    - - - - -    IRQ (if bit 6 is clear)
		- l - l    - l - - l    Length counter and sweep
		e e e e    e e e - e    Envelope and linear counter
	*/

	if (FrameCounter.Period == 4)
	{
		switch (FrameCounter.Count)
		{
			case 0:
				envelopes();
				break;
			case 1:
				lengths();
				sweeps();
				envelopes();
				break;
			case 2:
				envelopes();
				break;
			case 3:
				lengths();
				sweeps();
				envelopes();
				do_irq();
				break;
		}
	}
	else // Period == 5
	{
		switch (FrameCounter.Count)
		{
		case 0:
			envelopes();
			break;
		case 1:
			lengths();
			sweeps();
			envelopes();
			break;
		case 2:
			envelopes();
			break;
		case 3:
			break;
		case 4:
			lengths();
			sweeps();
			envelopes();
			break;
		}
	}
}

bool APU::SampleClock()
{
	if (m_sampleIntervalCounter == 0)
	{
		// Switch internval between 40 and 41 cycles to better
		// approximate the ~40.6 cycles it should be on average.
		m_sampleInterval ^= 0x01;
		m_sampleIntervalCounter = m_sampleInterval;
		return true;
	}
	else
	{
		m_sampleIntervalCounter--;
		return false;
	}
}

/*
	Main bus connectivity
*/

byte APU::ReadFromDevice(word address, bool peek)
{
	byte data = 0;

	switch (address - m_addressableRange.Min)
	{
		case 0x14: // OAM DMA register
			return m_addressDMA;

		case 0x15: // APU Status
			data |= Status.DMCInterrupt ? 0x80 : 0x00;
			//data |= FRAMEINTERRUPT ? 0x40 : 0x00;
			//data |= DMCACTIVE ? 0x10 : 0x00;
			data |= ChNoise.LengthCounter > 0 ? 0x08 : 0x00;
			data |= ChTriangle.LengthCounter > 0 ? 0x04 : 0x00;
			data |= ChPulse2.LengthCounter > 0 ? 0x02 : 0x00;
			data |= ChPulse1.LengthCounter > 0 ? 0x01 : 0x00;
			return data;

		default:
			return 0;
	}
}

void APU::WriteToDevice(word address, byte data)
{
	switch (address - m_addressableRange.Min)
	{
		/* PULSE CHANNEL 1 REGISTERS */
		case 0x00: // Pulse1 Control
			ChPulse1.WriteRegisterControl(data);
			break;
		case 0x01: // Pulse1 Sweep
			ChPulse1.WriteRegisterSweep(data);
			break;
		case 0x02: // Pulse1 Timer low byte
			ChPulse1.WriteRegisterTimerLow(data);
			break;
		case 0x03: // Pulse1 Timer high byte
			ChPulse1.WriteRegisterTimerHigh(data);
			break;

		/* PULSE CHANNEL 2 REGISTERS */
		case 0x04: // Pulse2 Control
			ChPulse2.WriteRegisterControl(data);
			break;
		case 0x05: // Pulse2 Sweep
			ChPulse2.WriteRegisterSweep(data);
			break;
		case 0x06: // Pulse2 Timer low byte
			ChPulse2.WriteRegisterTimerLow(data);
			break;
		case 0x07: // Pulse2 Timer high byte
			ChPulse2.WriteRegisterTimerHigh(data);
			break;

		/* TRIANGLE CHANNEL REGISTERS */
		case 0x08: // Triangle Control
			ChTriangle.WriteRegisterControl(data);
			break;
		case 0x0A: // Triangle Timer low byte
			ChTriangle.WriteRegisterTimerLow(data);
			break;
		case 0x0B: // Triangle Timer high byte
			ChTriangle.WriteRegisterTimerHigh(data);
			break;

		/* NOISE CHANNEL REGISTERS */
		case 0x0C: // Noise Control
			ChNoise.WriteRegisterControl(data);
			break;
		case 0x0E: // Noise Period
			ChNoise.WriteRegisterPeriod(data);
			break;
		case 0x0F: // Noise Length
			ChNoise.WriteRegisterLength(data);
			break;

		case 0x14: // OAM DMA register
			m_addressDMA = data;            // Store address for OAM DMA; PPU reads it from bus next cycle
			BUS.EmitSignal(SIGNAL_PPU_DMA); // Forward DMA request to PPU using bus signal
			break;

		case 0x15: // APU Status
			Status.EnableDMC = (data & 0x10) != 0 ? true : false;
			Status.EnableNoise = (data & 0x08) != 0 ? true : false;
			Status.EnableTriangle = (data & 0x04) != 0 ? true : false;
			Status.EnablePulse2 = (data & 0x02) != 0 ? true : false;
			Status.EnablePulse1 = (data & 0x01) != 0 ? true : false;
			Status.DMCInterrupt = false;
			break;

		// 0x17 Frame Counter -- handled by ControllerInterface, see OnBusSignal

		default:
			break;
	}
}

void APU::OnBusSignal(int signalId)
{
	// $4017 frame counter flags -- forwarded by ControllerInterface
	// which occupies $4017 on the bus
	if (signalId >= SIGNAL_APU_FRC_NONE && signalId <= SIGNAL_APU_FRC_MI)
	{
		FrameCounter.IRQInhibit = (signalId & 0x01) != 0 ? true : false;
		FrameCounter.Period = (signalId & 0x02) != 0 ? 5 : 4;
	}
}


/*
	Audio synthesis
*/

void APU::PopulateMixerLookupTables()
{
	// See http://wiki.nesdev.com/w/index.php/APU_Mixer#Lookup_Table

	// Pulse channels
	for (int n = 0; n < 31; n++)
	{
		m_mixtables.Pulse[n] = (word)((95.52 / (8128.0 / (double)n + 100)) * UINT8_MAX);
	}

	// Triangle, noise and DMC channels
	for (int n = 0; n < 203; n++)
	{
		m_mixtables.TND[n] = (word)((163.67 / (24329.0 / (double)n + 100)) * UINT8_MAX);
	}
}

audiosample APU::MixSample()
{
	// See http://wiki.nesdev.com/w/index.php/APU_Mixer#Lookup_Table
	
	// TODO
	byte pulse1 = ChPulse1.Output();
	byte pulse2 = ChPulse2.Output();
	byte triangle = ChTriangle.Output();
	byte noise = ChNoise.Output();
	byte dmc = 0;

	audiosample pulseOut = m_mixtables.Pulse[pulse1 + pulse2];
	audiosample tndOut = m_mixtables.TND[3 * triangle + 2 * noise + dmc];

	return pulseOut + tndOut;
}


// PULSE CHANNEL (SQUARE WAVE)

byte APU::PulseChannel::Output()
{
	if ((m_pulseChId == 1 && APU.Status.EnablePulse1) || (m_pulseChId == 2 && APU.Status.EnablePulse2))
	{
		// Channel enabled -- calculate output value
		if (LengthCounter == 0 
			|| TimerPeriod < 8 
			|| TimerPeriod > 0x7FF 
			|| APU.m_tableDuty[DutyCycle][SequencerStep] == 0)
		{
			return 0;
		}
		else
		{
			if (MaintainConstantVolume)
				return ConstantVolumeLevel;
			else
				return EnvelopeDecay;
		}
	}
	else
	{
		// Channel disabled
		return 0;
	}
}

void APU::PulseChannel::UpdateTimer()
{
	if (TimerCounter == 0)
	{
		TimerCounter = TimerPeriod;
		SequencerStep = (SequencerStep + 1) % 8;
	}
	else
	{
		TimerCounter--;
	}
}

void APU::PulseChannel::UpdateEnvelope()
{
	if (EnvelopeStart)
	{
		EnvelopeStart = false;
		EnvelopeCounter = EnvelopePeriod;
		EnvelopeDecay = 15;
	}
	else if (EnvelopeCounter == 0)
	{
		if (EnvelopeDecay > 0)
			EnvelopeDecay--;
		else if (LengthCounterHalt) // LengthCounterHalt also indicates Envelope loop
			EnvelopeDecay = 15;

		EnvelopeCounter = EnvelopePeriod;
	}
	else
	{
		EnvelopeCounter--;
	}
}

void APU::PulseChannel::UpdateLength()
{
	if (!LengthCounterHalt && LengthCounter > 0)
		LengthCounter--;
}

void APU::PulseChannel::UpdateSweep()
{
	bool doSweep = false;

	// Update counter
	if (SweepStart)
	{
		SweepStart = false;

		if (SweepEnabled && SweepCounter == 0)
			doSweep = true;

		SweepCounter = SweepPeriod;
	}
	else if (SweepCounter == 0) 
	{
		if (SweepEnabled)
			doSweep = true;

		SweepCounter = SweepPeriod;
	}
	else
	{
		SweepCounter--;
	}

	// Do sweep
	if (doSweep)
	{
		byte diff = TimerPeriod >> SweepShift;

		if (SweepNegate)
		{
			TimerPeriod = TimerPeriod - diff - (m_pulseChId == 1 ? 1 : 0);
		}
		else 
		{
			TimerPeriod += diff;
		}
	}
}

void APU::PulseChannel::WriteRegisterControl(byte data)
{
	DutyCycle = (data >> 6) & 0x03;
	LengthCounterHalt = (data & 0x20) != 0 ? true : false;
	MaintainConstantVolume = (data & 0x10) != 0 ? true : false;
	EnvelopePeriod = data & 0x0F;
	ConstantVolumeLevel = EnvelopePeriod;

	// Restart envelope on write
	EnvelopeStart = true;
}

void APU::PulseChannel::WriteRegisterSweep(byte data)
{
	SweepEnabled = (data & 0x80) != 0 ? true : false;
	SweepPeriod = ((data >> 4) & 0x07) + 1;
	SweepNegate = (data & 0x08) != 0 ? true : false;
	SweepShift = data & 0x07;

	// Restart sweep on write
	SweepStart = true;
}

void APU::PulseChannel::WriteRegisterTimerLow(byte data)
{
	TimerPeriod = (TimerPeriod & 0xFF00) | (word)data;
}

void APU::PulseChannel::WriteRegisterTimerHigh(byte data)
{
	TimerPeriod = (((word)data & 0xF8) << 8) | (TimerPeriod & 0x00FF);
	TimerCounter = TimerPeriod;

	if ((m_pulseChId == 1 && APU.Status.EnablePulse1) || (m_pulseChId == 2 && APU.Status.EnablePulse2))
	{
		LengthCounter = APU.m_tableLength[data >> 3];
	}
}


// TRIANGLE CHANNEL

byte APU::TriangleChannel::Output()
{
	if (APU.Status.EnableTriangle && LengthCounter > 0 && LinearCounter > 0)
	{
		return APU.m_tableTriangle[SequencerStep];
	}
	else
	{
		return 0;
	}
}

void APU::TriangleChannel::UpdateTimer()
{
	if (TimerCounter == 0)
	{
		TimerCounter = TimerPeriod;
		if (LengthCounter > 0 && LinearCounter > 0)
		{
			SequencerStep = (SequencerStep + 1) % 32;
		}
	}
	else
	{
		TimerCounter--;
	}
}

void APU::TriangleChannel::UpdateLinearCounter()
{
	if (LinearCounterStart)
	{
		LinearCounter = LinearCounterPeriod;
	}
	else if (LinearCounter > 0) {
		LinearCounter--;
	}

	if (!LengthCounterHalt)
	{
		LinearCounterStart = false;
	}
}

void APU::TriangleChannel::UpdateLength()
{
	if (!LengthCounterHalt && LengthCounter > 0)
	{
		LengthCounter--;
	}
}

void APU::TriangleChannel::WriteRegisterControl(byte data)
{
	LengthCounterHalt = (data & 0x80) != 0 ? true : false;
	LinearCounterPeriod = data & 0x7F;
}

void APU::TriangleChannel::WriteRegisterTimerLow(byte data)
{
	TimerPeriod = (TimerPeriod & 0xFF00) | (word)data;
}

void APU::TriangleChannel::WriteRegisterTimerHigh(byte data)
{
	TimerPeriod = (((word)data & 0xF8) << 8) | (TimerPeriod & 0x00FF);
	LengthCounter = APU.m_tableLength[data >> 3];
	LinearCounterStart = true;
}

// NOISE CHANNEL

byte APU::NoiseChannel::Output()
{
	if (!APU.Status.EnableNoise || LengthCounter == 0 || (ShiftRegister & 0x0001) != 0)
	{
		return 0;
	}
	else
	{
		if (MaintainConstantVolume)
			return ConstantVolumeLevel;
		else
			return EnvelopeDecay;
	}
}

void APU::NoiseChannel::UpdateTimer()
{
	if (TimerCounter == 0)
	{
		TimerCounter = TimerPeriod;

		byte a = ShiftRegister & 0x01;
		byte b = (ShiftRegister >> (Mode ? 6 : 1)) & 0x01;

		ShiftRegister >>= 1;
		ShiftRegister = (a ^ b) << 14;
	}
	else
	{
		TimerCounter--;

	}
}

void APU::NoiseChannel::UpdateEnvelope()
{
	if (EnvelopeStart)
	{
		EnvelopeStart = false;
		EnvelopeDecay = 15;
		EnvelopeCounter = EnvelopePeriod;
	}
	else if (EnvelopeCounter > 0)
	{
		EnvelopeCounter--;
	}
	else
	{
		EnvelopeCounter = EnvelopePeriod;

		if (EnvelopeDecay > 0)
			EnvelopeDecay--;
		else if (LengthCounterHalt)
			EnvelopeDecay = 15;
	}
}

void APU::NoiseChannel::UpdateLength()
{
	if (!LengthCounterHalt && LengthCounter > 0)
		LengthCounter--;
}


void APU::NoiseChannel::WriteRegisterControl(byte data)
{
	// 	--LC VVVV 	Envelope loop / length counter halt (L), constant volume (C), volume/envelope (V) 
	LengthCounterHalt = (data & 0x20) != 0 ? true : false;
	MaintainConstantVolume = (data & 0x10) != 0 ? true : false;
	EnvelopePeriod = data & 0x0F;
	ConstantVolumeLevel = EnvelopePeriod;

	EnvelopeStart = true;
}

void APU::NoiseChannel::WriteRegisterPeriod(byte data)
{
	Mode = (data & 0x80) != 0 ? true : false;
	TimerPeriod = APU.m_tableNoise[data & 0x0F];
}

void APU::NoiseChannel::WriteRegisterLength(byte data)
{
	LengthCounter = APU.m_tableLength[data >> 3];
	EnvelopeStart = true;
}

