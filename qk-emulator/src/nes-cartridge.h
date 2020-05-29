#pragma once

#include <string>
#include <vector>
#include <memory>
#include "definitions.h"
#include "nes-definitions.h"
#include "bus.h"
#include "nes-romfile.h"
#include "nes-mapper.h"


namespace Qk { namespace NES 
{
	class Cartridge
	{
	public:		
		Cartridge(const std::string& filepath);

		byte MainBusRead(word address);
		void MainBusWrite(word address, byte data);
		
		byte PPUBusRead(word address);
		void PPUBusWrite(word address, byte data);

		NametableMirrorMode GetNametableMirrorMode() const;

		CartridgeMetadata Metadata;
	protected:
		std::vector<byte> m_PRGROM;
		std::vector<byte> m_CHRROM;
		std::vector<byte> m_PRGRAM;
		std::shared_ptr<Mapper> m_mapper;

		byte ReadInternal(Mapper::MappedAddress addr);
		void WriteInternal(Mapper::MappedAddress addr, byte data);
	};

	class CartridgeSlot : public Bus::Device
	{
	public:
		CartridgeSlot(Bus& bus, const AddressRange& addressableRange);

		void InsertCartridge(const std::shared_ptr<Cartridge>& cartridge);

		CartridgeMetadata& GetMetadata() const;
		NametableMirrorMode GetNametableMirrorMode() const;

		// Main bus connectivity
		byte ReadFromDevice(word address, bool peek = false) override;
		void WriteToDevice(word address, byte data) override;

		// PPU "bus" connectivity
		byte PPUReadFromDevice(word address, bool peek = false);
		void PPUWriteToDevice(word address, byte data);
	protected:
		std::shared_ptr<Cartridge> m_cart;
	};

}}