#pragma once

#include "bus.h"

namespace Qk
{
	class MemoryMirror : public Bus::Device
	{
	protected:
		Bus::Device& m_mirroredDevice;
		word m_mirroredDeviceBaseAddress = 0;
		unsigned int m_mirrorLength = 0;

		word GetMirroredAddress(word address);

	public:
		MemoryMirror(Bus& bus, Bus::Device& mirroredDevice, const AddressRange& mirrorRange);

		byte ReadFromDevice(word address, bool peek = false) override;
		void WriteToDevice(word address, byte data) override;
	};
}