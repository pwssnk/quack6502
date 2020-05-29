#pragma once

namespace Qk { namespace NES
{
	// NES system information
	static constexpr double NES_CPU_CLOCK_FREQ = 1789773.0;

	// NES-specific bus signals
	static constexpr int SIGNAL_PPU_DMA = 1100; // PPU OAM DMA transfer

	static constexpr int SIGNAL_APU_FRC_NONE = 1200; // APU frame counter updates
	static constexpr int SIGNAL_APU_FRC_I = 1201;  
	static constexpr int SIGNAL_APU_FRC_M = 1202;
	static constexpr int SIGNAL_APU_FRC_MI = 1203;

	enum class NametableMirrorMode
	{
		Horizontal = 0,
		Vertical = 1,
		FourScreen = 2,
	};

	struct CartridgeMetadata
	{
		enum class FileFormatType
		{
			iNES    = 0,
			NES2    = 1,
			Archaic = 2,
			INVALID = 99
		} FileFormat = FileFormatType::INVALID;

		enum class TVSystemType
		{
			NTSC              = 0,
			PAL               = 1,
			DualCompatability = 3
		} TVSystem = TVSystemType::NTSC;

		enum class ConsoleType
		{
			NES	         = 0,
			VsSystem     = 1,
			PlayChoice10 = 2,
			Other        = 3
		} Console = ConsoleType::NES;

		NametableMirrorMode DefaultMirrorMode = NametableMirrorMode::Horizontal;

		unsigned int PRGROMSize = 0;
		unsigned int CHRROMSize = 0;
		unsigned int PRGRAMSize = 0;

		unsigned int MapperNumber = 0;

		bool ContainsTrainer = false;
		bool ContainsPersistentMemory = false;
	};

	struct Controller
	{
		enum class Button
		{
			Right   = 0x01,
			Left    = 0x02,
			Down    = 0x04,
			Up      = 0x08,
			Start   = 0x10,
			Select  = 0x20,
			B       = 0x40,
			A       = 0x80
		};

		enum class Player
		{
			One = 1,
			Two = 2
		};
	};
}}