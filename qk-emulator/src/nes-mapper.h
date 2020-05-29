#pragma once

#include <memory>
#include "definitions.h"
#include "nes-definitions.h"


namespace Qk { namespace NES
{
	class Mapper
	{
	protected:
		unsigned int m_prgromSize;
		unsigned int m_prgramSize;
		unsigned int m_chrromSize;
	public:
		enum Memory
		{
			NONE = 0,
			PRGROM = 1,
			CHRROM = 2,
			PRGRAM = 3
		};

		struct MappedAddress
		{
			Memory Target = Memory::NONE;
			dword Offset = 0;
		};

		Mapper(unsigned int PRGROMSize, unsigned int CHRROMSize, unsigned int PRGRAMSize);

		virtual MappedAddress MapBusAddress(word address, bool isWrite) = 0;
		virtual MappedAddress MapPPUAddress(word address, bool isWrite) = 0;
		virtual NametableMirrorMode GetNametableMirrorMode(const NametableMirrorMode defaultMode) const = 0;

		static std::shared_ptr<Mapper> GetMapper(unsigned int mapperId, 
			unsigned int PRGROMSize, unsigned int CHRROMSize, unsigned int PRGRAMSize);
	};

	class Mapper000 : public Mapper
	{
	public:
		Mapper000(unsigned int PRGROMSize, unsigned int CHRROMSize, unsigned int PRGRAMSize);
		MappedAddress MapBusAddress(word address, bool isWrite) override;
		MappedAddress MapPPUAddress(word address, bool isWrite) override;
		NametableMirrorMode GetNametableMirrorMode(const NametableMirrorMode defaultMode) const override;
	};
}}