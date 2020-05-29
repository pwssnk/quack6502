#pragma once

#include <string>
#include "bus.h"

namespace Qk
{
	/*
		Generic Memory Components (RAM/ROM)
	*/
	class RAM : public Bus::Device
	{
	protected:
		byte* m_data;
		word m_size;

		word LocalizeAddress(word addressOnBus);
	public:
		RAM(Bus& bus, const AddressRange& addressableRange);
		~RAM();

		word GetSize() const;
		byte ReadFromDevice(word address, bool peek = false) override;
		void WriteToDevice(word address, byte data) override;
	};

	class ROM : public RAM
	{
	public:
		ROM(Bus& bus, const AddressRange& addressableRange);
		void WriteToDevice(word address, byte data) override;
		bool LoadROM(const std::string& path);
	};
}
