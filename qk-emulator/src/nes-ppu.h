#pragma once

#include "definitions.h"
#include "bus.h"
#include "cpu.h"
#include "nes-cartridge.h"


namespace Qk { namespace NES
{
	constexpr int SCREEN_WIDTH = 256;
	constexpr int SCREEN_HEIGTH = 240;

	class RP2C02 : public Bus::Device
	{
	public:
		enum class CtrlFlag
		{
			IncrementMode			= 0x04,
			SpritePatternTable		= 0x08,
			BackgroundPatternTable	= 0x10,
			SpriteHeight			= 0x20,
			PPUMasterSlave			= 0x40,
			NMIEnable				= 0x80,
		};

		enum class MaskFlag
		{
			Greyscale				= 0x01,
			BackgroundLeftColEnable	= 0x02,
			SpriteLeftColEnable		= 0x04,
			BackgroundEnable		= 0x08,
			SpriteEnable			= 0x10,
			EmphasizeRed			= 0x20,
			EmphasizeGreen			= 0x40,
			EmphasizeBlue			= 0x80,
		};

		enum class StatusFlag
		{
			SpriteOverflow			= 0x20,
			SpriteZeroHit			= 0x40,
			VBlank					= 0x80,
		};

		struct 
		{
			byte PPUCtrl;
			byte PPUMask;
			byte PPUStatus;
			byte OAMAddr;
			byte OAMData;
			byte PPUScroll;
			byte PPUAddr;
			byte PPUData;
		} Registers;

		struct
		{
			byte Nametable[2][1024] = {};
			byte Palette[32] = { 0x0f };
			byte OAM[256] = {};
		} VRAM;

	public:
		RP2C02(Bus& bus, CartridgeSlot& cartridge);
		RP2C02(Bus& bus, const AddressRange& addressableRange, CartridgeSlot& cartridge);
			   		
		void Cycle();
		void Reset();

		// Incoming I/O from main bus
		byte ReadFromDevice(word address, bool peek = false) override;
		void WriteToDevice(word address, byte data) override;
		void OnBusSignal(int signalId) override;

		// Flags
		bool CheckFlag(MaskFlag flag) const;
		bool CheckFlag(StatusFlag flag) const;
		bool CheckFlag(CtrlFlag flag) const;
		void SetFlag(MaskFlag flag, bool state);
		void SetFlag(StatusFlag flag, bool state);
		void SetFlag(CtrlFlag flag, bool state);

		// Output handles
		FramebufferDescriptor* GetVideoOutput();
		unsigned long GetFrameCount() const;

	protected:
		// Background fetches
		void FetchNextBgAddress();
		void FetchNextBgAttribute();
		void FetchNextBgLSB();
		void FetchNextBgMSB();
		void PushBgBufferToShiftRegisters();

		// Sprite fetches
		void EvaluateSprites();
		word FetchSpriteRow(byte tileIndex, byte attribute, int row);

		// Nametable convenience functions
		byte ReadNametable(word address);
		void WriteNametable(word address, byte data);

		// OAM convenience functions
		void OAMDMA();
		
		// Outgoing I/O to main bus and ppu bus
		byte MainBusRead(word address);
		void MainBusWrite(word address, byte data);
		byte InternalBusRead(word address);
		void InternalBusWrite(word address, byte data);

		// Rendering
		void CycleRenderer();
		Pixel& Muxer();
		Pixel& GetRGBColorFromPalette(byte palette, byte pixelValue);
		void DrawPixel(Pixel& pixel, int x, int y);

	protected:
		class RenderState
		{
		public:
			// VRAM address notation described here: 
			// https://wiki.nesdev.com/w/index.php/PPU_scrolling#PPU_internal_registers
			union VRAMAddress
			{
				struct
				{
					word CoarseX : 5;
					word CoarseY : 5;
					word NametableX : 1;
					word NametableY : 1;
					word FineY : 3;
					word Unused : 1;
				};
				word Address = 0;
			} V, T;

			byte FineX = 0;
			bool WriteLatch = false;

			struct
			{
				int Dots = 0;
				int Scanline = 261;
				bool VisibleFrameDone = false;
			} ScanPos;

			struct
			{
				struct BgRow
				{
					byte TileIndex = 0;
					byte LSB = 0;
					byte MSB = 0;
					byte Attribute = 0;
				} BufferBg;

				struct
				{
					word TileLSB = 0;
					word TileMSB = 0;
					word ColorLSB = 0;
					word ColorMSB = 0;
				} ShiftBg;

				struct SpriteRow
				{
					byte PositionX = 0;
					byte Palette = 0;
					byte Priority = 0;
					byte RowLSB = 0;
					byte RowMSB = 0;
					bool IsSpriteZero = false;
				} BufferSpr[8];

				int SpriteCount = 0;

			} VCache;

		public:
			void IncrementScanPos();

			void IncrementVerticalPos();
			void IncrementHorizontalPos();
			void CopyHorizontalPos();
			void CopyVerticalPos();

			void ShiftBgRegisters();
		} m_state;

		CartridgeSlot& m_cart;
		bool m_videoModeCheck = false;

		byte m_ppuRegWriteBuf = 0;
		bool m_doDMA = false;
		int m_remainingOAMDMACycles = 0;

		unsigned long m_frameCounter = 0;
		Pixel m_framebuffer[SCREEN_WIDTH * SCREEN_HEIGTH];
		FramebufferDescriptor m_fi;

		// NES palette RGB values sourced from: https://wiki.nesdev.com/w/index.php/PPU_palettes#2C02
		Pixel m_paletteRGB[64] = {
			Pixel(84, 84, 84),		Pixel(0, 30, 116),		Pixel(8, 16, 144),		Pixel(48, 0, 136),
			Pixel(68, 0, 100),		Pixel(92, 0, 48),		Pixel(84, 4, 0),		Pixel(60, 24, 0),
			Pixel(32, 42, 0),		Pixel(8, 58, 0),		Pixel(0, 64, 0),		Pixel(0, 60, 0),
			Pixel(0, 50, 60),		Pixel(0, 0, 0),			Pixel(0, 0, 0),			Pixel(0, 0, 0),
			Pixel(152, 150, 152),	Pixel(8, 76, 196),		Pixel(48, 50, 236),		Pixel(92, 30, 228),
			Pixel(136, 20, 176),	Pixel(160, 20, 100),	Pixel(152, 34, 32),		Pixel(120, 60, 0),
			Pixel(84, 90, 0),		Pixel(40, 114, 0),		Pixel(8, 124, 0),		Pixel(0, 118, 40),
			Pixel(0, 102, 120),		Pixel(0, 0, 0),			Pixel(0, 0, 0),			Pixel(0, 0, 0),
			Pixel(236, 238, 236),	Pixel(76, 154, 236),	Pixel(120, 124, 236),	Pixel(176, 98, 236),
			Pixel(228, 84, 236),	Pixel(236, 88, 180),	Pixel(236, 106, 100),	Pixel(212, 136, 32),
			Pixel(160, 170, 0),		Pixel(116, 196, 0),		Pixel(76, 208, 32),		Pixel(56, 204, 108),
			Pixel(56, 180, 204),	Pixel(60, 60, 60),		Pixel(0, 0, 0),			Pixel(0, 0, 0),
			Pixel(236, 238, 236),	Pixel(168, 204, 236),	Pixel(188, 188, 236),	Pixel(212, 178, 236),
			Pixel(236, 174, 236),	Pixel(236, 174, 212),	Pixel(236, 180, 176),	Pixel(228, 196, 144),
			Pixel(204, 210, 120),	Pixel(180, 222, 120),	Pixel(168, 226, 144),	Pixel(152, 226, 180),
			Pixel(160, 214, 228),	Pixel(160, 162, 160),	Pixel(0, 0, 0),			Pixel(0, 0, 0)
		};
	};
}}