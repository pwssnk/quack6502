#include "nes-ppu.h"
#include "util.h"
#include <iostream>
#include <iomanip>

using namespace Qk;
using namespace Qk::NES;

/*
	Constructors, destructor
*/

RP2C02::RP2C02(Bus& bus, CartridgeSlot& cartridge) 
	: Bus::Device(bus, true, AddressRange(0x2000, 0x2007)), m_cart(cartridge), m_fi(m_framebuffer, SCREEN_WIDTH, SCREEN_HEIGTH)
{
	Reset();
}

RP2C02::RP2C02(Bus& bus, const AddressRange& addressableRange, CartridgeSlot& cartridge) 
	: Bus::Device(bus, true, addressableRange), m_cart(cartridge), m_fi(m_framebuffer, SCREEN_WIDTH, SCREEN_HEIGTH)
{
	Reset();
}


/*
	API
*/

void RP2C02::Reset()
{
	Registers.PPUCtrl = 0;
	Registers.PPUMask = 0;
	Registers.PPUStatus = 0;
	Registers.OAMAddr = 0;
	Registers.OAMData = 0;
	Registers.PPUScroll = 0;
	Registers.PPUAddr = 0;
	Registers.PPUData = 0;

	m_doDMA = false;
	m_videoModeCheck = false;
}

void RP2C02::Cycle()
{
	if (!m_videoModeCheck)
	{
		if (m_cart.GetMetadata().TVSystem != CartridgeMetadata::TVSystemType::NTSC)
		{
			throw QkError("PPU error: unsupported video mode. Only NTSC ROMs are supported.", 630);
		}

		m_videoModeCheck = true;
	}

	// Direct Memory Access requests to OAM
	if (m_remainingOAMDMACycles > 0)
	{
		if (m_doDMA)
		{
			m_doDMA = false;
			BUS.EmitSignal(SIGNAL_CPU_HLT); // Suspend CPU during OAM DMA transer
			OAMDMA(); // Instanteneous DMA transfer -- not going to emulate individual read/write cycles for now
		}
		else if (m_remainingOAMDMACycles == 1)
		{
			BUS.EmitSignal(SIGNAL_CPU_RSM); // OAM DMA tranfser done; signal CPU to resume
		}

		m_remainingOAMDMACycles--;
	}

	// Rendering
	CycleRenderer();
}

FramebufferDescriptor* RP2C02::GetVideoOutput()
{
	return &m_fi;
}

unsigned long RP2C02::GetFrameCount() const
{
	return m_frameCounter;
}


/*
	Nametable access
*/

byte RP2C02::ReadNametable(word address)
{
	NametableMirrorMode mirrorMode = m_cart.GetNametableMirrorMode();

	/*
		https://wiki.nesdev.com/w/index.php/Mirroring#Nametable_Mirroring

			vertical		horizontal
			+---+---+		+---+---+
			| 0 | 1 |		| 0 | 0 |
			+---+---+		+---+---+
			| 0 | 1 |		| 1 | 1 |
			+---+---+		+---+---+
	*/

	if (mirrorMode == NametableMirrorMode::Vertical)
	{
		return VRAM.Nametable[(address >> 10) & 0x0001][address & 0x03FF];
	}
	else if (mirrorMode == NametableMirrorMode::Horizontal)
	{
		return VRAM.Nametable[(address >> 11) & 0x0001][address & 0x03FF];
	}
	else
	{
		throw QkError("Nametable mirroring mode not supported", 620);
	}

	return 0;
}

void RP2C02::WriteNametable(word address, byte data)
{
	NametableMirrorMode mirrorMode = m_cart.GetNametableMirrorMode();

	if (mirrorMode == NametableMirrorMode::Vertical)
	{
		VRAM.Nametable[(address >> 10) & 0x0001][address & 0x03FF] = data;
	}
	else if (mirrorMode == NametableMirrorMode::Horizontal)
	{
		VRAM.Nametable[(address >> 11) & 0x0001][address & 0x03FF] = data;
	}
	else
	{
		throw QkError("Nametable mirroring mode not supported", 620);
	}
}


/*
	Object Attribute Memory access
*/

void RP2C02::OAMDMA()
{
	// Read high byte of starting address from 0x4014 on main bus
	// (on real NES, this is a PPU register, but our current bus
	// implementation does not allow for non-contiguous addresses,
	// so we use a "dummy" DMA forwarder device at 0x4014)
	word addr = MainBusRead(0x4014) << 8;

	// Read 256 bytes of data from main bus into OAM memory
	for (int lsb = 0; lsb < 256; lsb++)
	{
		VRAM.OAM[lsb] = MainBusRead(addr | lsb);
	}
}


/*
	Flags
*/

bool RP2C02::CheckFlag(MaskFlag flag) const
{
	return (Registers.PPUMask & static_cast<int>(flag)) != 0;
}

bool RP2C02::CheckFlag(StatusFlag flag) const
{
	return (Registers.PPUStatus & static_cast<int>(flag)) != 0;
}

bool RP2C02::CheckFlag(CtrlFlag flag) const
{
	return (Registers.PPUCtrl & static_cast<int>(flag)) != 0;
}

void RP2C02::SetFlag(MaskFlag flag, bool state)
{
	if (state)
		Registers.PPUMask |= static_cast<int>(flag);
	else
		Registers.PPUMask &= ~static_cast<int>(flag);
}

void RP2C02::SetFlag(StatusFlag flag, bool state)
{
	if (state)
		Registers.PPUStatus |= static_cast<int>(flag);
	else
		Registers.PPUStatus &= ~static_cast<int>(flag);
}

void RP2C02::SetFlag(CtrlFlag flag, bool state)
{
	if (state)
		Registers.PPUCtrl |= static_cast<int>(flag);
	else
		Registers.PPUCtrl &= ~static_cast<int>(flag);
}


/*
	System bus - outgoing I/O
*/

byte RP2C02::MainBusRead(word address)
{
	return BUS.ReadFromBus(address);
}

void RP2C02::MainBusWrite(word address, byte data)
{
	BUS.WriteToBus(address, data);
}


/*
	System bus - incoming I/O
*/

byte RP2C02::ReadFromDevice(word address, bool peek)
{
	byte tmp = 0;

	switch (address - m_addressableRange.Min)
	{
	case 0x00: // PPU CTRL
		// Can't read
		if (peek)
			return Registers.PPUCtrl;
		else
			return 0;

	case 0x01: // PPU MASK
		// Can't read
		if (peek)
			return Registers.PPUMask;
		else
			return 0;

	case 0x02: // PPU STATUS
		if (peek)
			return Registers.PPUStatus;

		// Read resets PPU write latch and VBlank flag on Status register
		// So we have to store the current value of PPU STATUS
		// before clearing the flag (otherwise it would always
		// read false!)
		Registers.PPUStatus = (Registers.PPUStatus & 0xE0) | (m_ppuRegWriteBuf & 0x1F);
		tmp = Registers.PPUStatus;
		SetFlag(StatusFlag::VBlank, false);
		m_state.WriteLatch = false;
		return tmp;

	case 0x03: // OAM ADDR
		// Can't read OAM ADDR
		if (peek)
			return Registers.OAMAddr;
		else
			return 0;

	case 0x04: // OAM DATA
		return VRAM.OAM[Registers.OAMAddr];

	case 0x05: // PPU SCROLL
		// Can't read scroll
		if (peek)
			return Registers.PPUScroll;
		else
			return 0;

	case 0x06: // PPU ADDR
		if (peek)
			return Registers.PPUAddr;
		// Can't read PPU ADDR
		return 0;

	case 0x07: // PPU DATA
		if (peek)
			return Registers.PPUData;
		
		// Output on PPUDATA is delayed one cycle,
		// except for read from pallete memory range
		// (0x3F00 - 0x3FFF)

		// Store previous
		tmp = Registers.PPUData;

		// Read new value
		Registers.PPUData = InternalBusRead(m_state.V.Address);

		if (m_state.V.Address >= 0x3F00 && m_state.V.Address <= 0x3FFF)
		{
			tmp = Registers.PPUData;
		}

		// Increment VRAM Address pointer with every read. Inc amount dictated by PPUCtrl flag
		m_state.V.Address += (CheckFlag(CtrlFlag::IncrementMode) ? 32 : 1);

		return tmp;

	default:
		return 0;
	}
}

void RP2C02::WriteToDevice(word address, byte data)
{
	// Buffer written data -- needed later
	// for correctly emulating PPUSTATUS read
	m_ppuRegWriteBuf = data;

	switch (address - m_addressableRange.Min)
	{
	case 0x00: // PPU CTRL
		Registers.PPUCtrl = data;

		// Store nametable in T vram address
		m_state.T.NametableX = Registers.PPUCtrl & 0x01;
		m_state.T.NametableY = (Registers.PPUCtrl & 0x02) >> 1;
		break;

	case 0x01: // PPU MASK
		Registers.PPUMask = data;
		break;

	case 0x02: // PPU STATUS
		// PPUStatus not writeable
		break;

	case 0x03: // OAM ADDR
		Registers.OAMAddr = data;
		break;

	case 0x04: // OAM DATA
		// https://wiki.nesdev.com/w/index.php/PPU_registers#OAMDMA recommends ignoring
		// writes during PPU rendering for emulation purposes
		//if (m_state.ScanPos.Scanline >= 0 && m_state.ScanPos.Scanline <= 239)
		//	return;

		Registers.OAMData = data;
		VRAM.OAM[Registers.OAMAddr] = data;
		// Write to OAMDATA increments OAMADDR
		Registers.OAMAddr++;
		break;

	case 0x05: // PPU SCROLL
		Registers.PPUScroll = data;

		if (!m_state.WriteLatch)
		{
			m_state.FineX = data & 0x07;
			m_state.T.CoarseX = data >> 3;
			m_state.WriteLatch = true;
		}
		else
		{
			m_state.T.FineY = data & 0x07;
			m_state.T.CoarseY = data >> 3;
			m_state.WriteLatch = false;
		}

		break;

	case 0x06: // PPU ADDR
		Registers.PPUAddr = data;
		// It takes two 1 byte writes to construct
		// a 2 byte address, so alternate between
		// interpreting input as high byte
		// and low byte

		if (!m_state.WriteLatch)
		{
			m_state.T.Address = ((word)(data & 0x3F) << 8) | (m_state.T.Address & 0x00FF);
			m_state.WriteLatch = true;
		}
		else
		{
			m_state.T.Address = (m_state.T.Address & 0xFF00) | (word)data;
			m_state.WriteLatch = false;

			// Full address has been written to T address ptr; updata V address ptr with new value
			m_state.V.Address = m_state.T.Address;
		}

		break;

	case 0x07: // PPU DATA
		// Don't write to actual PPU DATA reg -- that's
		// used for output from PPU only
		// Write data to PPU bus directly using value
		// constructed from two writes to PPU ADDR
		InternalBusWrite(m_state.V.Address, data);

		// Increment VRAM Address pointer with every read. Inc amount dictated by PPUCtrl flag
		m_state.V.Address += (CheckFlag(CtrlFlag::IncrementMode) ? 32 : 1);
		break;

	default:
		break;
	}
}

void RP2C02::OnBusSignal(int signalId)
{
	// TODO: OAM load
	switch (signalId)
	{
	case SIGNAL_PPU_DMA:
		m_doDMA = true;
		// OAM transfer takes 513/514 cycles, depending on whether CPU is on even or odd cycle. 
		// For convenience, we'll assume 513 cycles is okay. Because PPU does 3 cycles for
		// every CPU cycle, we'll multiply by 3.
		m_remainingOAMDMACycles = 513 * 3;

		break;
	default:
		break;
	}
}


/*
	PPU internal bus I/O
*/

byte RP2C02::InternalBusRead(word address)
{
	if (address >= 0x0000 && address <= 0x1FFF)
	{
		// Access memory on cartridge
		return m_cart.PPUReadFromDevice(address);
	}
	else if (address >= 0x2000 && address <= 0x3EFF)
	{
		// Access internal nametable VRAM
		return ReadNametable(address);
	}
	else if (address >= 0x3F00 && address <= 0x3FFF)
	{
		// Access internal palette VRAM (32 bytes, mirrored)
		// The palette for the background runs from VRAM $3F00 
		// to $3F0F; the palette for the sprites runs from $3F10 
		// to $3F1F. Each color takes up one byte. 
		// Addresses $3F10/$3F14/$3F18/$3F1C are mirrors of 
		// $3F00/$3F04/$3F08/$3F0C. Note that this goes for writing 
		// as well as reading.

		// First, deal with mirrored addresses
		if (address == 0x3F10)
			address = 0x3F00;
		else if (address == 0x3F14)
			address = 0x3F04;
		else if (address == 0x3F18)
			address = 0x3F08;
		else if (address == 0x3F1C)
			address = 0x3F0C;

		// Mask to mirror addresses outside 32 bytes of actual available RAM
		address &= 0x001F;

		/*
			Bit 0 of PPUMASK controls a greyscale mode, which causes the palette to use only the colors
			from the grey column: $00, $10, $20, $30. This is implemented as a bitwise AND with $30 on 
			any value read from PPU $3F00-$3FFF, both on the display and through PPUDATA. Writes to 
			the palette through PPUDATA are not affected. Also note that black colours like $0F will 
			be replaced by a non-black grey $00.
		*/
		if (CheckFlag(MaskFlag::Greyscale))
			return VRAM.Palette[address] & 0x30;
		else
			return VRAM.Palette[address];
	}
	else
	{
		return 0;
	}
}

void RP2C02::InternalBusWrite(word address, byte data)
{
	if (address >= 0x0000 && address <= 0x1FFF)
	{
		// Access memory on cartridge
		m_cart.PPUWriteToDevice(address, data);
	}
	else if (address >= 0x2000 && address <= 0x3EFF)
	{
		// Access internal nametable VRAM
		WriteNametable(address, data);
	}
	else if (address >= 0x3F00 && address <= 0x3FFF)
	{
		// Access internal palette VRAM (32 bytes, mirrored)
		if (address == 0x3F10)
			address = 0x3F00;
		else if (address == 0x3F14)
			address = 0x3F04;
		else if (address == 0x3F18)
			address = 0x3F08;
		else if (address == 0x3F1C)
			address = 0x3F0C;

		address &= 0x001F;

		VRAM.Palette[address] = data;
	}
}


/*
	PPU rendering
*/

void RP2C02::CycleRenderer()
{
	// SCANLINE POSITION SHORTHANDS
	int s = m_state.ScanPos.Scanline;
	int d = m_state.ScanPos.Dots;
	bool preLine = (s == 261);
	bool postLine = (s == 240);
	bool visibleLine = (s >= 0 && s <= 239);
	bool visibleDot = (d >= 1 && d <= 256);
	bool visibleFrame = (visibleLine && visibleDot);
	bool vblankStart = (s == 241 && d == 1);
	bool vblankEnd = (preLine && d == 1);
	bool fetch = (((d >= 1 && d <= 256) || (d >= 321 && d <= 336)) && (visibleLine || preLine));
	bool renderEnable = (CheckFlag(MaskFlag::BackgroundEnable) || CheckFlag(MaskFlag::SpriteEnable));

	// PPU FLAGS
	if (vblankStart)
	{
		SetFlag(StatusFlag::VBlank, true);

		if (CheckFlag(CtrlFlag::NMIEnable))
			BUS.EmitSignal(SIGNAL_CPU_NMI);

		m_frameCounter++;
	}
	else if (vblankEnd)
	{
		SetFlag(StatusFlag::VBlank, false);
		SetFlag(StatusFlag::SpriteOverflow, false);
		SetFlag(StatusFlag::SpriteZeroHit, false);
	}

	// BACKGROUND DATA FETCHES
	if (fetch)
	{
		m_state.ShiftBgRegisters();

		switch (d % 8)
		{
		case 1:
			PushBgBufferToShiftRegisters();
			FetchNextBgAddress();
			break;
		case 3:
			FetchNextBgAttribute();
			break;
		case 5:
			FetchNextBgLSB();
			break;
		case 7:
			FetchNextBgMSB();
			break;
		default:
			break;
		}
	}

	// BACKGROUND V/T UPDATES
	if (renderEnable)
	{
		if (preLine)
		{
			// inc hori(v) on every 8th dot between d=[0, 256], and on d=328, and d=336
			if ((visibleDot && ((d % 8) == 0)) || d == 328 || d == 336)
			{
				m_state.IncrementHorizontalPos();
			}

			if (d >= 280 && d <= 304) // d=[280, 304] do vert(v) = vert(t). ONLY APPLIES TO PRE-RENDER SCANLINE
			{
				m_state.CopyVerticalPos();
			}
		}
		else if (visibleLine)
		{
			// inc hori(v) on every 8th dot between d=[0, 256], and on d=328, and d=336
			if ((visibleDot && ((d % 8) == 0)) || d == 328 || d == 336)
			{
				m_state.IncrementHorizontalPos();
			}

			if (d == 256) // inc vert(v) on d=256
			{
				m_state.IncrementVerticalPos();
			}
			else if (d == 257) // hori(v) = hori(t) on d=257
			{
				m_state.CopyHorizontalPos();
			}
		}
	}

	// SPRITES
	if (renderEnable)
	{
		if ((visibleLine || preLine) && d == 257)
			EvaluateSprites();
	}

	// RENDER PIXEL
	if (visibleFrame)
	{
		DrawPixel(Muxer(), d - 1, s);
	}

	// SCANLINE/DOT POSITION UPDATE
	m_state.IncrementScanPos();

	// Cycle skip on odd frames
	if ((m_frameCounter % 2) != 0 && s == 261 && d == 339)
	{
		m_state.IncrementScanPos();
	}
}

Pixel& RP2C02::Muxer()
{
	byte bgpix = 0;
	byte bgpal = 0;
	byte sppix = 0;
	byte sppal = 0;
	byte sppriority = 0;
	bool spriteZero = false;

	// Get background pixel & palette
	if (CheckFlag(MaskFlag::BackgroundEnable))
	{
		word bitmux = 0x8000 >> m_state.FineX;

		byte pixbit0 = (m_state.VCache.ShiftBg.TileLSB & bitmux) != 0 ? 0x01 : 0x00;
		byte pixbit1 = (m_state.VCache.ShiftBg.TileMSB & bitmux) != 0 ? 0x01 : 0x00;

		byte palbit0 = (m_state.VCache.ShiftBg.ColorLSB & bitmux) != 0 ? 0x01 : 0x00;
		byte palbit1 = (m_state.VCache.ShiftBg.ColorMSB & bitmux) != 0 ? 0x01 : 0x00;

		bgpix = (pixbit1 << 1) | pixbit0;
		bgpal = (palbit1 << 1) | palbit0;
	}

	// Get foreground pixel & palette
	if (CheckFlag(MaskFlag::SpriteEnable))
	{
		for (int i = 0; i < m_state.VCache.SpriteCount; i++)
		{
			int diff = (m_state.ScanPos.Dots - 1) - (int)m_state.VCache.BufferSpr[i].PositionX;
			if (diff >= 0  && diff < 8)
			{
				byte pixbit2 = ((m_state.VCache.BufferSpr[i].RowLSB << diff) & 0x80) >> 7;
				byte pixbit3 = ((m_state.VCache.BufferSpr[i].RowMSB << diff) & 0x80) >> 7;

				sppix = (pixbit3 << 1) | pixbit2;
				sppal = m_state.VCache.BufferSpr[i].Palette;
				sppriority = m_state.VCache.BufferSpr[i].Priority;
				spriteZero = m_state.VCache.BufferSpr[i].IsSpriteZero;

				if (sppix != 0)
					break;
			}
		}
	}

	// Left column enable flags
	if (m_state.ScanPos.Dots < 8)
	{
		if (!CheckFlag(MaskFlag::BackgroundLeftColEnable))
		{
			bgpix = 0;
			bgpal = 0;
		}
		
		if (!CheckFlag(MaskFlag::SpriteLeftColEnable))
		{
			sppal = 0;
			sppix = 0;
		}
	}

	  
	/*
		Determine pixel priority
			https://wiki.nesdev.com/w/index.php/PPU_rendering#Preface

			      Priority multiplexer decision table 
			==================================================
			BG pixel 	Sprite pixel 	Priority 	Output
			0 			0 				X 			0 (BG $3F00)
			0 			1-3 			X 			Sprite
			1-3 		0		 		X			BG
			1-3 		1-3 			0 			Sprite
			1-3 		1-3 			1 			BG 
	*/

	byte outpix = 0;
	byte outpal = 0;

	if (bgpix == 0 && sppix == 0)
	{
		outpix = 0;
		outpal = 0;
	}
	else if (bgpix == 0 && sppix > 0)
	{
		outpix = sppix;
		outpal = sppal;
	}
	else if (bgpix > 0 && sppix == 0)
	{
		outpix = bgpix;
		outpal = bgpal;
	}
	else if (bgpix > 0 && sppix > 0)
	{
		if (sppriority == 0) // 0 indicates sprite precedence over bg
		{
			outpix = sppix;
			outpal = sppal;
		}
		else
		{
			outpix = bgpix;
			outpal = bgpal;
		}

		// Bg and sprite pixels are both non-transparent -- sprite zero hit?
		if (spriteZero)
		{ 
			SetFlag(StatusFlag::SpriteZeroHit, true);
		}
	}

	return GetRGBColorFromPalette(outpal, outpix);
}

Pixel& RP2C02::GetRGBColorFromPalette(byte paletteIndex, byte twoBitPixelValue)
{
	// 1. Fetch 1 byte "NES-format" color data from palette memory
	// 2. Use lookup table to determine corresponding 3-byte RGB color value

	// Palette memory starts at 0x3F00 on PPU internal bus
	// Each palette takes up 4 bytes of color information, so multiply palette index by 4
	// Then, add the twoBitPixelValue extracted from CHR ROM to get
	// the proper "color byte" from one of the four colors in the palette
	word addr = 0x3F00 + paletteIndex * 4 + twoBitPixelValue;
	byte nesColor = InternalBusRead(addr);

	// Lookup and return
	return m_paletteRGB[nesColor & 0x3F];
}

void RP2C02::DrawPixel(Pixel& pixel, int x, int y)
{
	int offset = y * SCREEN_WIDTH + x;

#ifdef _DEBUG
	// Bounds checking
	if (offset > (SCREEN_WIDTH * SCREEN_HEIGTH - 1))
		throw QkError("PPU screen array index out of bounds", 666);
	else
		m_framebuffer[offset] = pixel;
#else
	m_framebuffer[offset] = pixel;
#endif	
}


/*
	Screen scanning position
*/

void RP2C02::RenderState::IncrementScanPos()
{
	ScanPos.Dots++;
	if (ScanPos.Dots > 340)
	{
		ScanPos.Dots = 0;
		ScanPos.Scanline++;
		
		if (ScanPos.Scanline > 261)
		{
			ScanPos.Scanline = 0;
		}
	}
}


/*
	Background fetch and decode
*/

void RP2C02::RenderState::IncrementVerticalPos()
{
	/*
		https://wiki.nesdev.com/w/index.php/PPU_scrolling#Y_increment

		if ((v & 0x7000) != 0x7000)        // if fine Y < 7
		  v += 0x1000                      // increment fine Y
		else
		  v &= ~0x7000                     // fine Y = 0
		  int y = (v & 0x03E0) >> 5        // let y = coarse Y
		  if (y == 29)
			y = 0                          // coarse Y = 0
			v ^= 0x0800                    // switch vertical nametable
		  else if (y == 31)
			y = 0                          // coarse Y = 0, nametable not switched
		  else
			y += 1                         // increment coarse Y
		  v = (v & ~0x03E0) | (y << 5)     // put coarse Y back into v
	*/

	// Union + bitfield struct magic makes this easier
	if (V.FineY < 7)
	{
		V.FineY++;
	}
	else
	{
		V.FineY = 0;
		
		if (V.CoarseY == 29)
		{
			V.CoarseY = 0;
			V.NametableY = ~V.NametableY;
		}
		else if (V.CoarseY == 31)
		{
			V.CoarseY = 0;
		}
		else
		{
			V.CoarseY++;
		}
	}
}

void RP2C02::RenderState::IncrementHorizontalPos()
{
	/*
		https://wiki.nesdev.com/w/index.php/PPU_scrolling#Coarse_X_increment

			if ((v & 0x001F) == 31) // if coarse X == 31
			  v &= ~0x001F          // coarse X = 0
			  v ^= 0x0400           // switch horizontal nametable
			else
			  v += 1                // increment coarse X
	*/

	if (V.CoarseX == 31)
	{
		V.CoarseX = 0;
		V.NametableX = ~V.NametableX;
	}
	else
	{
		V.CoarseX++;
	}
}

void RP2C02::RenderState::CopyHorizontalPos()
{
	V.CoarseX = T.CoarseX;
	V.NametableX = T.NametableX;
}

void RP2C02::RenderState::CopyVerticalPos()
{
	V.FineY = T.FineY;
	V.CoarseY = T.CoarseY;
	V.NametableY = T.NametableY;
}

void RP2C02::FetchNextBgAddress()
{
	// tile address      = 0x2000 | (v & 0x0FFF)
	m_state.VCache.BufferBg.TileIndex = InternalBusRead(0x2000 | (m_state.V.Address & 0x0FFF));
}

void RP2C02::FetchNextBgAttribute()
{
	// attribute address = 0x23C0 | (v & 0x0C00) | ((v >> 4) & 0x38) | ((v >> 2) & 0x07)

	// Fetch attribute data
	byte data = InternalBusRead(
		0x23C0
		| (m_state.V.Address & 0x0C00)
		| ((m_state.V.Address >> 4) & 0x38)
		| ((m_state.V.Address >> 2) & 0x07)
	);

	// Extract pallete information for current cell
	if (m_state.V.CoarseY & 0x02)
		data >>= 4;

	if (m_state.V.CoarseX & 0x02)
		data >>= 2;

	m_state.VCache.BufferBg.Attribute = data;
}

void RP2C02::FetchNextBgLSB()
{
	byte data = InternalBusRead(
		(CheckFlag(CtrlFlag::BackgroundPatternTable) ? 0x1000 : 0x000)
		+ m_state.VCache.BufferBg.TileIndex * 16
		+ m_state.V.FineY
	);

	m_state.VCache.BufferBg.LSB = data;
}

void RP2C02::FetchNextBgMSB()
{
	byte data = InternalBusRead(
		(CheckFlag(CtrlFlag::BackgroundPatternTable) ? 0x1000 : 0x000)
		+ m_state.VCache.BufferBg.TileIndex * 16
		+ m_state.V.FineY
		+ 0x0008
	);

	m_state.VCache.BufferBg.MSB = data;
}

void RP2C02::PushBgBufferToShiftRegisters()
{
	m_state.VCache.ShiftBg.TileLSB = (m_state.VCache.ShiftBg.TileLSB & 0xFF00) | m_state.VCache.BufferBg.LSB;
	m_state.VCache.ShiftBg.TileMSB = (m_state.VCache.ShiftBg.TileMSB & 0xFF00) | m_state.VCache.BufferBg.MSB;

	m_state.VCache.ShiftBg.ColorLSB = (m_state.VCache.ShiftBg.ColorLSB & 0xFF00) | ((m_state.VCache.BufferBg.Attribute & 0b01) ? 0xFF : 0x00);
	m_state.VCache.ShiftBg.ColorMSB = (m_state.VCache.ShiftBg.ColorMSB & 0xFF00) | ((m_state.VCache.BufferBg.Attribute & 0b10) ? 0xFF : 0x00);
}

void RP2C02::RenderState::ShiftBgRegisters()
{
	VCache.ShiftBg.TileLSB <<= 1;
	VCache.ShiftBg.TileMSB <<= 1;
	VCache.ShiftBg.ColorLSB <<= 1;
	VCache.ShiftBg.ColorMSB <<= 1;
}


/*
	Sprite fetch and decode
*/

void RP2C02::EvaluateSprites()
{
	int scanl = m_state.ScanPos.Scanline;

	if (scanl == 261)
		scanl = 0;
	else if (scanl <= 238)
		scanl++;
	else
		return;

	int& sprCount = m_state.VCache.SpriteCount;
	sprCount = 0;
	int h = CheckFlag(CtrlFlag::SpriteHeight) ? 16 : 8;

	for (int i = 0; i < 64; i++)
	{
		byte y = VRAM.OAM[4 * i];
		int row = (int)y - scanl + 8;

		if (row >= 0 && row < h)
		{
			if (sprCount < 8)
			{
				byte t = VRAM.OAM[4 * i + 1];
				byte a = VRAM.OAM[4 * i + 2];
				byte x = VRAM.OAM[4 * i + 3];

				m_state.VCache.BufferSpr[sprCount].PositionX = x;
				m_state.VCache.BufferSpr[sprCount].Palette = (a & 3) + 4;
				m_state.VCache.BufferSpr[sprCount].Priority = (a & 0x20) != 0 ? 0x01 : 0x00;

				word pattern = FetchSpriteRow(t, a, row);

				m_state.VCache.BufferSpr[sprCount].RowLSB = pattern & 0x00FF;
				m_state.VCache.BufferSpr[sprCount].RowMSB = (pattern >> 8) & 0x00FF;

				if (i == 0)
					m_state.VCache.BufferSpr[sprCount].IsSpriteZero = true;
				else
					m_state.VCache.BufferSpr[sprCount].IsSpriteZero = false;

				sprCount++;
			}
			else
			{
				SetFlag(StatusFlag::SpriteOverflow, true);
				break;
			}
		}
	}
}

word RP2C02::FetchSpriteRow(byte tileIndex, byte attribute, int row)
{
	word tile = tileIndex;
	word attr = attribute;
	word addr = 0;

	if (!CheckFlag(CtrlFlag::SpriteHeight))
	{
		if ((attr & 0x80) == 0)
			row = 7 - row;

		word table = (CheckFlag(CtrlFlag::SpritePatternTable) ? 0x1000 : 0x0000);
		addr = table + (tile * 16) + row;
	}
	else
	{
		if ((attr & 0x80) == 0)
			row = 15 - row;

		word table = ((tile & 1) != 0) ? 0x1000 : 0x000;
		tile &= 0xFE;

		if (row > 7)
		{
			tile += 1;
			row -= 8;
		}

		addr = table + (tile * 16) + row;
	}

	byte lsb = (word)InternalBusRead(addr);
	byte msb = (word)InternalBusRead(addr + 8);

	if ((attr & 0x40) != 0)
	{
		lsb = Util::ReverseBits(lsb);
		msb = Util::ReverseBits(msb);
	}

	return ((word)msb << 8) | (word)lsb;
}
