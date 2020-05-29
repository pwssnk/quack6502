#include "mem-mirror.h"

using namespace Qk;

/*
	Constructor
*/

MemoryMirror::MemoryMirror(Bus& bus, Bus::Device& mirroredDevice, const AddressRange& mirrorRange)
	: Bus::Device(bus, true, mirrorRange), m_mirroredDevice(mirroredDevice)
{
	AddressRange r = m_mirroredDevice.GetAddressableRange();
	m_mirrorLength = r.Max - r.Min;
	m_mirroredDeviceBaseAddress = r.Min;
}

/*
	Internal methods
*/

word MemoryMirror::GetMirroredAddress(word address)
{
	// Convert absolute address to relative offset
	word relative = address - m_addressableRange.Min;

	// Deal with wrap around -- only keep bits
	// that fit in max sensible address range,
	// which is the length of the mirrored device
	// address range
	relative &= m_mirrorLength;

	// Construct absolute address to mirrored device
	return m_mirroredDeviceBaseAddress + relative;
}

/* 
	Bus I/O forwarding
*/

byte MemoryMirror::ReadFromDevice(word address, bool peek)
{
	return m_mirroredDevice.ReadFromDevice(GetMirroredAddress(address));
}

void MemoryMirror::WriteToDevice(word address, byte data)
{
	m_mirroredDevice.WriteToDevice(GetMirroredAddress(address), data);
}