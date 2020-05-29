#include <sstream>
#include "nes-mapper.h"


using namespace Qk;
using namespace Qk::NES;


/**************************************************
	Qk::NES::Mapper (mapper base class)
***************************************************/

Mapper::Mapper(unsigned int PRGROMSize, unsigned int CHRROMSize, unsigned int PRGRAMSize)
{
	m_prgromSize = PRGROMSize;
	m_prgramSize = PRGRAMSize;
	m_chrromSize = CHRROMSize;
}

std::shared_ptr<Mapper> Mapper::GetMapper(unsigned int mapperId, unsigned int PRGROMSize, 
	unsigned int CHRROMSize, unsigned int PRGRAMSize)
{
	switch (mapperId)
	{
		case 0:
			return std::make_shared<Mapper000>(PRGROMSize, CHRROMSize, PRGRAMSize);
		default:
			throw QkError("No compatible mapper available for this cartridge", 511);
	}
}


/**************************************************
	Qk::NES::Mapper000 (NROM / iNES Mapper ID 0)
	https://wiki.nesdev.com/w/index.php/NROM
***************************************************/

Mapper000::Mapper000(unsigned int PRGROMSize, unsigned int CHRROMSize, unsigned int PRGRAMSize) 
	: Mapper(PRGROMSize, CHRROMSize, PRGRAMSize)
{

}

Mapper::MappedAddress Mapper000::MapBusAddress(word address, bool isWrite)
{
	/*
		PRG ROM size:				16 KiB for NROM-128, 32 KiB for NROM-256 (DIP-28 standard pinout)
		PRG ROM bank size:			Not bankswitched
		PRG RAM:					2 or 4 KiB, not bankswitched, only in Family Basic (but most emulators provide 8)
		CHR capacity:				8 KiB ROM (DIP-28 standard pinout) but most emulators support RAM
		CHR bank size:				Not bankswitched, see CNROM
		Nametable mirroring:		Solder pads select vertical or horizontal mirroring
		Subject to bus conflicts:	Yes, but irrelevant

		All Banks are fixed,
		CPU $6000-$7FFF: Family Basic only: PRG RAM, mirrored as necessary to fill entire 8 KiB window, write protectable with an external switch
		CPU $8000-$BFFF: First 16 KB of ROM.
		CPU $C000-$FFFF: Last 16 KB of ROM (NROM-256) or mirror of $8000-$BFFF (NROM-128).
	*/
	
	MappedAddress addr;

	if (m_prgramSize > 0 && address >= 0x6000 && address <= 0x7FFF)
	{
		addr.Target = Mapper::Memory::PRGRAM;
		addr.Offset = (address - 0x6000) & (m_prgramSize - 1); // Mask with max sensible address, which is size - 1 (vector is zero indexed)
	}
	else if (address >= 0x8000 && address <= 0xFFFF && isWrite == false) // Read only
	{
		addr.Target = Mapper::Memory::PRGROM;
		addr.Offset = (address - 0x8000) & (m_prgromSize - 1);
	}
	else
	{
		addr.Target = Mapper::Memory::NONE;
	}

	return addr;
}

Mapper::MappedAddress Mapper000::MapPPUAddress(word address, bool isWrite)
{
	MappedAddress addr;

	if (address >= 0x0000 && address <= 0x1FFF && isWrite == false) // Read only
	{
		addr.Target = Mapper::Memory::CHRROM;
		addr.Offset = address & (m_chrromSize - 1);
	}
	else
	{
		addr.Target = Mapper::Memory::NONE;
	}

	return addr;
}

NametableMirrorMode Mapper000::GetNametableMirrorMode(const NametableMirrorMode defaultMode) const
{
	return defaultMode;
}
