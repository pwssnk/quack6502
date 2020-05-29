#pragma once

#include <string>
#include <fstream>
#include <vector>

#include "definitions.h"
#include "nes-definitions.h"

namespace Qk { namespace NES 
{
	class ROMFile
	{
	public:
		ROMFile(const std::string& filepath);
		~ROMFile();

		CartridgeMetadata GetMetadata() const;
		void LoadPRGROM(std::vector<byte>& buffer);
		void LoadCHRROM(std::vector<byte>& buffer);
	protected:
		CartridgeMetadata m_meta;
		byte m_header[16] = { 0 };
		std::ifstream m_infile;
		unsigned int m_fileSize = 0;

		CartridgeMetadata::FileFormatType GetFileFormat();
		void ParseHeaderiNES();
		void ParseHeaderNES2();
		void ParseHeaderArchaic();
	};


}}