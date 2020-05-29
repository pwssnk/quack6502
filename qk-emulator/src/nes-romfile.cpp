#include "nes-romfile.h"


using namespace Qk;
using namespace Qk::NES;


ROMFile::ROMFile(const std::string& filepath)
{
	m_infile.open(filepath, std::ifstream::binary);

	// Check file available
	if (!m_infile)
		throw QkError("Cannot access ROM file", 550);
		
	// Check file size
	m_infile.seekg(0, std::ios_base::end);
	m_fileSize = (unsigned int)m_infile.tellg();
	m_infile.seekg(0, std::ios_base::beg);

	// Read 16 byte header information
	if (m_fileSize < 16)
	{
		m_meta.FileFormat = CartridgeMetadata::FileFormatType::INVALID;
		return;
	}

	m_infile.read((char*)m_header, 16);

	// Parse header
	m_meta.FileFormat = GetFileFormat();
	if (m_meta.FileFormat == CartridgeMetadata::FileFormatType::iNES)
	{
		ParseHeaderiNES();
	}
	else
	{
		// No support for parsing NES2.0 or archaic header formats yet
		// But, since NES2.0 is mostly backwards compatible, we can parse as
		// iNES for now

		ParseHeaderiNES();
	}
}

ROMFile::~ROMFile()
{
	m_infile.close();
}

CartridgeMetadata::FileFormatType ROMFile::GetFileFormat()
{
	// TODO: more heuristics for proper NES2.0 detection,
	// archaic iNES format detection

	// Check header constant in first four bytes ($4E $45 $53 $1A or "NES"+CTRL^Z)
	if (!(m_header[0] == 0x4E && m_header[1] == 0x45 && m_header[2] == 0x53 && m_header[3] == 0x1A))
	{
		return CartridgeMetadata::FileFormatType::INVALID;
	}

	// Check NES2.0 signature bits
	if ((m_header[7] & 0x0C) == 0x08)
	{
		// TODO: additional check: ROM size taking into account byte 9 does not exceed 
		// the actual size of the ROM image, then NES 2.0.

		return CartridgeMetadata::FileFormatType::NES2;
	}
	else
	{
		return CartridgeMetadata::FileFormatType::iNES;
	}
}

CartridgeMetadata ROMFile::GetMetadata() const
{
	return m_meta;
}

void ROMFile::LoadPRGROM(std::vector<byte>& buffer)
{
	if (m_meta.PRGROMSize == 0)
		return;

	// Go to PRGROM start offset
	unsigned int offset = 16; // 16-byte header

	if (m_meta.ContainsTrainer) // 512-byte trainer present?
		offset += 512;

	m_infile.seekg(offset, std::ios_base::beg);

	// Read PRGROMSize worth of bytes
	buffer.resize(m_meta.PRGROMSize);
	m_infile.read((char*)buffer.data(), buffer.size());
}

void ROMFile::LoadCHRROM(std::vector<byte>& buffer)
{
	// CHR ROM not necessarily present
	if (m_meta.CHRROMSize == 0)
		return;

	// Go to PRGROM start offset
	unsigned int offset = 16; // 16-byte header

	if (m_meta.ContainsTrainer) // 512-byte trainer present?
		offset += 512;

	offset += m_meta.PRGROMSize; // CHR ROM stored directly after PRG ROM

	m_infile.seekg(offset, std::ios_base::beg);

	// Read CHRROMSize worth of bytes
	buffer.resize(m_meta.CHRROMSize);
	m_infile.read((char*)buffer.data(), buffer.size());
}

void ROMFile::ParseHeaderiNES()
{
	// BYTE 4 and 5: PRG ROM and CHR ROM sizes (in 16kB and 8kB units respectively)
	m_meta.PRGROMSize = (unsigned int)m_header[4] * 16 * 1024;
	m_meta.CHRROMSize = (unsigned int)m_header[5] * 8 * 1024;

	// BYTE 6
	m_meta.DefaultMirrorMode = (m_header[6] & 0x01) == 1 ? NametableMirrorMode::Vertical : NametableMirrorMode::Horizontal;
	m_meta.ContainsPersistentMemory = (m_header[6] & 0x02) == 1 ? true : false;
	m_meta.ContainsTrainer = (m_header[6] & 0x04) == 1 ? true : false;

	if ((m_header[6] & 0x08) == 1)
		m_meta.DefaultMirrorMode = NametableMirrorMode::FourScreen;

	byte mapperNumberLowerNibble = (m_header[6] & 0xF0);

	// BYTE 7
	if ((m_header[7] & 0x01) == 1)
		m_meta.Console = CartridgeMetadata::ConsoleType::VsSystem;
	else if ((m_header[7] & 0x02) == 1)
		m_meta.Console = CartridgeMetadata::ConsoleType::PlayChoice10;

	byte mapperNumberUpperNibble = (m_header[7] & 0xF0);

	// BYTE 8: PRG RAM size
	if (m_header[8] == 0)
		m_meta.PRGRAMSize = 8 * 1024; // Infer 8kB for compatability
	else
		m_meta.PRGRAMSize = (unsigned int)m_header[8] * 8 * 1024;

	// BYTE 9
	m_meta.TVSystem = (m_header[9] & 0x01) == 1 ? CartridgeMetadata::TVSystemType::PAL : CartridgeMetadata::TVSystemType::NTSC;

	// BYTE 10 (unofficial, not part of spec)

	// BYTES 11-15 are unused in iNES format

	// Combine mapper number nibbles
	m_meta.MapperNumber = ((unsigned int)mapperNumberUpperNibble << 8) | (unsigned int)mapperNumberLowerNibble;
}

void ROMFile::ParseHeaderNES2()
{
	throw QkError("ROM file format not supported", 560);
}

void ROMFile::ParseHeaderArchaic()
{
	throw QkError("ROM file format not supported", 560);
}