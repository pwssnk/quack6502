#include <fstream>
#include "nes-cartridge.h"
#include "nes-mapper.h"


using namespace Qk;
using namespace Qk::NES;


/**************************************************
	Qk::NES::CartridgeSlot
***************************************************/

CartridgeSlot::CartridgeSlot(Bus& bus, const AddressRange& addressableRange)
	: Bus::Device(bus, true, addressableRange)
{

}

void CartridgeSlot::InsertCartridge(const std::shared_ptr<Cartridge>& cartridge)
{
	m_cart = cartridge;
}

CartridgeMetadata& CartridgeSlot::GetMetadata() const
{
	return m_cart->Metadata;
}

NametableMirrorMode CartridgeSlot::GetNametableMirrorMode() const
{
	return m_cart->GetNametableMirrorMode();
}

byte CartridgeSlot::ReadFromDevice(word address, bool peek)
{
	if (m_cart)
		return m_cart->MainBusRead(address);
	else
		return 0;
}

void CartridgeSlot::WriteToDevice(word address, byte data)
{
	if (m_cart)
		m_cart->MainBusWrite(address, data);
}

byte CartridgeSlot::PPUReadFromDevice(word address, bool peek)
{
	if (m_cart)
		return m_cart->PPUBusRead(address);
	else
		return 0;
}

void CartridgeSlot::PPUWriteToDevice(word address, byte data)
{
	if (m_cart)
		m_cart->PPUBusWrite(address, data);
}


/**************************************************
	Qk::NES::Cartridge
***************************************************/

/*
	Constructor
*/

Cartridge::Cartridge(const std::string& filepath)
{
	ROMFile rom(filepath);
	Metadata = rom.GetMetadata();

	if (Metadata.FileFormat == CartridgeMetadata::FileFormatType::INVALID)
		throw QkError("Invalid ROM dump file", 510);

	// Set up PRG and CHR ROM
	rom.LoadPRGROM(m_PRGROM);
	rom.LoadCHRROM(m_CHRROM);

	// Set up cartridge PRG RAM
	if (Metadata.PRGRAMSize > 0)
		m_PRGRAM.resize(Metadata.PRGRAMSize);

	// Configure mapper
	m_mapper = Mapper::GetMapper(
		Metadata.MapperNumber,
		Metadata.PRGROMSize,
		Metadata.CHRROMSize,
		Metadata.PRGRAMSize
	);
}

/*
	Internal functionality
*/

byte Cartridge::ReadInternal(Mapper::MappedAddress address)
{
	switch (address.Target)
	{
		case Mapper::Memory::PRGROM:
			return m_PRGROM[address.Offset];
		case Mapper::Memory::CHRROM:
			return m_CHRROM[address.Offset];
		case Mapper::Memory::PRGRAM:
			return m_PRGRAM[address.Offset];
		default:
			return 0;
	}
}

void Cartridge::WriteInternal(Mapper::MappedAddress address, byte data)
{
	switch (address.Target)
	{
	case Mapper::Memory::PRGRAM:
		m_PRGRAM[address.Offset] = data;
		break;
	case Mapper::Memory::PRGROM:
		m_PRGROM[address.Offset] = data; // Let mapper decide if we can write to ROM
	case Mapper::Memory::CHRROM:
		m_CHRROM[address.Offset] = data; // Let mapper decide if we can write to ROM
		break;
	default:
		break;
	}
}

/*
	Public interface methods
*/

byte Cartridge::MainBusRead(word address)
{
	return ReadInternal(m_mapper->MapBusAddress(address, false));
}

void Cartridge::MainBusWrite(word address, byte data)
{
	WriteInternal(m_mapper->MapBusAddress(address, true), data);
}

byte Cartridge::PPUBusRead(word address)
{
	return ReadInternal(m_mapper->MapPPUAddress(address, false));
}

void Cartridge::PPUBusWrite(word address, byte data)
{
	WriteInternal(m_mapper->MapPPUAddress(address, true), data);
}

NametableMirrorMode Cartridge::GetNametableMirrorMode() const
{
	return m_mapper->GetNametableMirrorMode(Metadata.DefaultMirrorMode);
}
